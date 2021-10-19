/*

eSwitch is a multi-purpose electronic switch replacement.
On power up it connects to Wifi and sends it's local IP address to a private server (no security concern since it only knows it's local IP address).
This allows the user to login to the switch by simply scanning a Qrcode of the Mac address.

When the unit is new or following a factory reset, the switch sets up a WiFi access point called "eSwitch".
To allow the switch to connect to your WiFi, connect to the Access Point and browse to 192.168.4.1.
You can then set the switch to connect to your WiFi.
On reboot the switch will announce itself to mDNS as "eSwitch".
You can then configure the switch name (this will be announced to mDNS after re-boot for easy access).
You can also configure a SinricPro account setup and switch ID to facilitate Alexa control.
Go to www.SinricPro.com to setup a free user account for this.
Finally you can configure the switch to automatically operate a slave eSwitch, either in direct or inverted mode.
This can theoretically allow an unlimited number of slaved eSwitches to operate on a single free SinricPro account

*/

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266HTTPUpdateServer.h>
#include <EEPROM.h>
#include <LittleFS.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"

#include "RootPage.h"
#include "SetupPage.h"

#define OutputPin 2
#define InputPin 0

volatile bool InterruptFlag = false;
long LastIntMillis;
bool SwitchState;

String SwitchStateString;
char InvertSlaveString[18];

const char* update_path = "/update";

const char *ap_ssid = "eSWITCH";
const char* web_username = "admin";
const char* web_password = "admin";
const char* update_username = "admin";
const char* update_password = "admin";

boolean WlanConnected = false;

String WIFI_SSID;
String WIFI_PASS;
String DeviceName;
String AppKeyString;
String AppSecretString;
String SwitchIDString;
String SlaveName;
String SlaveIP;
bool InvertSlave;
int NumSlaves;
String IP_Host = "http://smithers.family";
String MAC_String = WiFi.macAddress();
int ServerPort = 80;


ESP8266WebServer server(ServerPort);
ESP8266HTTPUpdateServer httpUpdater;

void ICACHE_RAM_ATTR InterruptHandler()
{
  InterruptFlag=true;
}


void setup() 
{
  int IPStart;
  int IPEnd;
//  delay(10000);
//  Serial.begin(115200);
//  Serial.println("Running...");
  pinMode(OutputPin,OUTPUT);
  digitalWrite(OutputPin,HIGH); 
  pinMode(InputPin,INPUT_PULLUP);
  httpUpdater.setup(&server, update_path, update_username, update_password);
  attachInterrupt(digitalPinToInterrupt(InputPin),InterruptHandler,FALLING);

  EEPROM.begin(1024);
  delay(10);

  WIFI_SSID   = GetSSIDFromEEPROM();
  WIFI_PASS   = GetPasswordFromEEPROM();
  DeviceName  = GetDeviceNameFromEEPROM();
  AppKeyString = GetAppKeyFromEEPROM();
  AppSecretString = GetAppSecretFromEEPROM();
  SwitchIDString = GetSwitchIDFromEEPROM();
  SlaveName=GetSlaveNameFromEEPROM();
  if (SlaveName.length() == 0)
    SlaveName = "None";
  if (SlaveName != "None")
      {
      IPStart=SlaveName.indexOf("(")+1;
      IPEnd=SlaveName.indexOf(")");
      SlaveIP=SlaveName.substring(IPStart,IPEnd);  
      }

  LittleFS.begin();

  if (SwitchIDString.length()>0)
    {
    SinricProSwitch& Switch = SinricPro[SwitchIDString.c_str()];
    Switch.onPowerState(onPowerState);
    }

  // setup SinricPro
  if ((AppKeyString.length()>0) && (AppSecretString.length()>0))
    SinricPro.begin(AppKeyString.c_str(), AppSecretString.c_str());

  if (WIFI_SSID.length() > 0)
    {
    if (ConnectToWLAN(WIFI_SSID.c_str(), WIFI_PASS.c_str()))
      {
      WiFi.mode(WIFI_STA);
      if (DeviceName.length() > 0)
        MDNS.begin(DeviceName.c_str(),WiFi.localIP());
      else  
        MDNS.begin("eSwitch",WiFi.localIP());
      // Add service to MDNS-SD
      MDNS.addService(ap_ssid, "tcp",ServerPort );
      PostData();
      }
    else
      {
      WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
      WiFi.mode(WIFI_AP_STA);
      WiFi.softAP(ap_ssid);    
      }
    }  
  else
  {
    WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ap_ssid);        
  }
  ServerSetup();
  IPAddress apIP = WiFi.softAPIP();
  IPAddress myIP = WiFi.localIP(); 
}

void loop() 
{
  MDNS.update();
  if ((InterruptFlag) && (millis() > (LastIntMillis + 1000)))                 // Switch 1s debounce
    {
    LastIntMillis = millis();

    // Start of Long Switch Press factory reset sequence
    for (int SwitchCheckLoop = 0; SwitchCheckLoop < 5000; SwitchCheckLoop++)
      {
        if (digitalRead(InputPin))
          break;
        else if (SwitchCheckLoop < 4999)
          delay(1);
        else
          {
            ResetEEPROM();
            WiFi.disconnect(true);
            ESP.restart();
          }     
      }

      
    InterruptFlag = false;                                                  // Reset switch interrupt
    SwitchOnOff(!SwitchState);                                              // Toggle output
    SinricProSwitch& Switch = SinricPro[SwitchIDString.c_str()];            // Get Switch device
    Switch.sendPowerStateEvent(SwitchState);                                // Tell Sinric that output has switched
    }
    else InterruptFlag = false;
  SinricPro.handle();
  server.handleClient();
}


boolean ConnectToWLAN(const char* ssid, const char* password)
{
  int Retries = 0;
  if (password && strlen(password) > 0 ) {
    WiFi.begin(ssid, password);
  } else {
    WiFi.begin(ssid);
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Retries++;
    if (Retries > 20) {
      return false;
    }
  }
  return true;
}

void ServerSetup() {

    server.on("/", [](){
      return RootPageHandler();
    });
    
    server.on("/setup", []() {
     if (!server.authenticate(web_username, web_password)) {
      return server.requestAuthentication();
      }
    return SetupPageHandler();
    });
    
    server.on("/switch", [](){
      return SwitchPageHandler();
    });
    
    server.on("/wifi", [](){
    if (!server.authenticate(web_username, web_password)) {
      return server.requestAuthentication();
    }
      return WifiPageHandler();
    });
    
    server.on("/device", [](){
    if (!server.authenticate(web_username, web_password)) {
      return server.requestAuthentication();
    }
      return DevicePageHandler();
    });
    
    server.on("/sinric", [](){
    if (!server.authenticate(web_username, web_password)) {
      return server.requestAuthentication();
    }
      return SinricPageHandler();
    });
    
    server.on("/slave", [](){
    if (!server.authenticate(web_username, web_password)) {
      return server.requestAuthentication();
    }
      return SlavePageHandler();
    });

    server.on("/reset", []() {
     if (!server.authenticate(web_username, web_password)) {
      return server.requestAuthentication();
      }
    return ResetPageHandler();
    });
    
    server.on("/resetwifi", []() {
     if (!server.authenticate(web_username, web_password)) {
      return server.requestAuthentication();
      }
    return ResetWiFiPageHandler();
    });

    server.on("/getinitialdata", []() {
      return GetInitialDataPageHandler();
    });

    server.on("/getwifilist", []() {
      return GetWifiListHandler();
    });

    server.on("/getdevicename", []() {
      return GetDeviceNameHandler();
    });

    server.on("/getsinric", []() {
      return GetSinricHandler();
    });

    server.on("/getslave", []() {
      return GetSlaveHandler();
    });

    server.onNotFound([](){
      if (!handleFileRead(server.uri()))
        server.send(404, "text/plain", "404: Not Found");  
    });

    // Start the server
    httpUpdater.setup(&server, update_path, web_username, web_password);
    server.begin();
}

void PostData()
{
  MAC_String[2]='-';
  MAC_String[5]='-';
  MAC_String[8]='-';
  MAC_String[11]='-';
  MAC_String[14]='-';
  WiFiClient WifiClient;
  HTTPClient SwitchID;
  String IP_String = WiFi.localIP().toString();
  String UrlGet;
  UrlGet = IP_Host + "/eswitch/eswitch.php?IP=" + IP_String + "&MAC=" + MAC_String;

  SwitchID.begin(WifiClient,UrlGet.c_str());
  int GetResponse= SwitchID.GET();
  SwitchID.end();
}

bool PostDataToSlave(String SlaveToCall, bool TestOnly, bool OnOff)
{
  WiFiClient SlaveWifi;
  HTTPClient SlaveID;
  String UrlGet="http://"+SlaveToCall+"/switch";
  if (!TestOnly)
    {
    UrlGet += "?SwitchState=";
      if (OnOff) UrlGet += "true";
      else UrlGet += "false";
    }
  SlaveID.setTimeout(1000);     
  SlaveID.begin(SlaveWifi,UrlGet.c_str());
  int ret = SlaveID.GET();
  return (ret==200);
  SlaveID.end();
}
 

bool SwitchOnOff(bool OnorOff)
{
bool ReturnValue;  
if (OnorOff)
  {
  digitalWrite(OutputPin,LOW);
  SwitchState=true; 
  if (SlaveName!="None")
      if (PostDataToSlave(SlaveIP,false,!InvertSlave))
        ReturnValue=true;
      else
        ReturnValue=false;
   else
    ReturnValue=true;             
  }
else
  {
  digitalWrite(OutputPin,HIGH);
  SwitchState=false;      
  if (SlaveName!="None")
      if (PostDataToSlave(SlaveIP,false,InvertSlave))      
        ReturnValue=true;
      else
        ReturnValue=false;        
   else
    ReturnValue=true;             
  }
return ReturnValue;  
}

String GetSSIDFromEEPROM() {
  String esid = "";
  for (int i = 0; i < 32; i++)
  {
    byte readByte = EEPROM.read(i);
    if (readByte == 0) {
      break;
    } else if ((readByte < 32) || (readByte == 0xFF)) {
      continue;
    }
    esid += char(readByte);
  }
  return esid;
}

String GetPasswordFromEEPROM() {
  String epass = "";
  for (int i = 32; i < 64; i++)
  {
    byte readByte = EEPROM.read(i);
    if (readByte == 0) {
      break;
    } else if ((readByte < 32) || (readByte == 0xFF)) {
      continue;
    }
    epass += char(EEPROM.read(i));
  }
  return epass;
}

String GetDeviceNameFromEEPROM() {
  String device = "";
  for (int i = 64; i < 96; i++)
  {
    byte readByte = EEPROM.read(i);
    if (readByte == 0) {
      break;
    } else if ((readByte < 32) || (readByte == 0xFF)) {
      continue;
    }
    device += char(EEPROM.read(i));
  }
  return device;
}

String GetAppKeyFromEEPROM() {
  String key = "";
  for (int i = 96; i < 224; i++)
  {
    byte readByte = EEPROM.read(i);
    if (readByte == 0) {
      break;
    } else if ((readByte < 32) || (readByte == 0xFF)) {
      continue;
    }
    key += char(EEPROM.read(i));
  }
  return key;
}

String GetAppSecretFromEEPROM() {
  String key = "";
  for (int i = 224; i < 352; i++)
  {
    byte readByte = EEPROM.read(i);
    if (readByte == 0) {
      break;
    } else if ((readByte < 32) || (readByte == 0xFF)) {
      continue;
    }
    key += char(EEPROM.read(i));
  }
  return key;
}

String GetSwitchIDFromEEPROM() {
  String id = "";
  for (int i = 352; i < 480; i++)
  {
    byte readByte = EEPROM.read(i);
    if (readByte == 0) {
      break;
    } else if ((readByte < 32) || (readByte == 0xFF)) {
      continue;
    }
    id += char(EEPROM.read(i));
  }
  return id;
}

String GetSlaveNameFromEEPROM()
  {
  String Slave="";
  for (int i = 0; i < 40; i++)
    {
    int Location = i+481;  
    byte ReadByte = EEPROM.read(Location);
    if (ReadByte == 0)
      break;
    else if ((ReadByte < 32) || (ReadByte == 0xFF)) 
      continue;
    Slave += char(EEPROM.read(Location));        
    }
    if (EEPROM.read(521)==1)
      InvertSlave=true;
    else
      InvertSlave=false;  
  return Slave;  
  }

void WriteEEPROMWiFi(String qsid, String qpass) 
{
  for (int i = 0; i < 32; i++)
  {
    if (i < qsid.length()) {
      EEPROM.write(i, qsid[i]);
    } else {
      EEPROM.write(i, 0);
    }
  }
  for (int i = 0; i < 32; i++)
  {
    if ( i < qpass.length()) {
      EEPROM.write(i+32, qpass[i]);
    } else {
      EEPROM.write(i+32, 0);
    }
  }

  EEPROM.commit();
}

void WriteEEPROMDevice(String Device) 
{
  for (int i = 0; i < 32; i++)
  {
    if ( i < Device.length()) {
      EEPROM.write(i+64, Device[i]);
    } else {
      EEPROM.write(i+64, 0);
    }
  }
  EEPROM.commit();
}

void WriteEEPROMSinric(String AppKey, String AppSecret, String SwitchID)
{
  for (int i = 0; i < 128; i++)
  {
    if ( i < AppKey.length()) {
      EEPROM.write(i+96, AppKey[i]);
    } else {
      EEPROM.write(i+96, 0);
    }
  }

  for (int i = 0; i < 128; i++)
  {
    if ( i < AppSecret.length()) {
      EEPROM.write(i+224, AppSecret[i]);
    } else {
      EEPROM.write(i+224, 0);
    }
  }

  for (int i = 0; i < 128; i++)
  {
    if ( i < SwitchID.length()) {
      EEPROM.write(i+352, SwitchID[i]);
    } else {
      EEPROM.write(i+352, 0);
    }
  }

  EEPROM.commit();
}

void WriteEEPROMSlave() 
{
    for (int i = 0; i < 40; i++)
    {
      if (i < SlaveName.length()) {
        EEPROM.write(i+481, SlaveName[i]);
      } else {
        EEPROM.write(i+481, 0);
      }
    }
  if (InvertSlave)
    EEPROM.write(521,1);
  else
    EEPROM.write(521,0);  
  EEPROM.commit();
}



void ResetEEPROM() {
  for (int i = 0; i < 1024; i++) {EEPROM.write(i, 0);}
  EEPROM.commit();
}


bool onPowerState(const String &deviceId, bool &state) {
  SwitchState = state;
  SwitchOnOff(SwitchState);
  return true; // request handled properly
}


void SwitchPageHandler()
{
  if (server.hasArg("SwitchState"))
    {
    if ((SwitchState) && (server.arg("SwitchState")=="false"))
      {
      SwitchState=false;
      SwitchStateString = "false"; 
      }
    else if ((!SwitchState) && (server.arg("SwitchState")=="true"))
      {
      SwitchState=true;
      SwitchStateString = "true";
      }
    if(SwitchOnOff(SwitchState))
      SwitchStateString += "|true";                                     //No slave or slave responded
    else  
      SwitchStateString += "|false";                                    //Slave didn't respond
    SinricProSwitch& Switch = SinricPro[SwitchIDString.c_str()];
    Switch.sendPowerStateEvent(SwitchState);
    }
  server.send(200, "text/plain", SwitchStateString);
}

void SinricPageHandler()
{
  if (server.hasArg("AppKey"))
    AppKeyString = server.arg("AppKey");  
  if (server.hasArg("AppSecret"))
    AppSecretString = server.arg("AppSecret");  
  if (server.hasArg("SwitchID"))
    SwitchIDString = server.arg("SwitchID");  
  WriteEEPROMSinric(AppKeyString,AppSecretString,SwitchIDString);      
  server.send(200, "text/plain", SwitchIDString);
}

void SlavePageHandler()
{ 
  if (server.hasArg("InvertSlave"))
   {
   if (server.arg("InvertSlave")=="true")
    InvertSlave=true;
   else   
    InvertSlave=false;   
   }
   if (server.hasArg("Slave"))
    {
      SlaveName = server.arg("Slave");   
      int IPStart=SlaveName.indexOf("(")+1;
      int IPEnd=SlaveName.indexOf(")");
      SlaveIP=SlaveName.substring(IPStart,IPEnd);
      WriteEEPROMSlave(); 
    }
      
  server.send(200, "text/plain", SlaveName);
}


void DevicePageHandler()
{
  if (server.hasArg("DeviceName"))
    {
    MDNS.removeService(DeviceName.c_str(),"http", "tcp");
    MDNS.end();
    DeviceName = server.arg("DeviceName");
    MDNS.begin(DeviceName.c_str(),WiFi.localIP());
    MDNS.addService("eSWITCH", "tcp", ServerPort);
    WriteEEPROMDevice(DeviceName);
    }
  server.send(200, "text/plain", DeviceName);
}



void ResetPageHandler() {
  String PageData = WriteHTMLHead();
  PageData += "<div class=\"container\" role=\"main\"><h3 style=\"color:white;\" class=\"sub-header\">Factory Reset</h3>";
  PageData += "<div class=\"alert alert-success fade in\"><strong>Success!</strong> Reset done.</div>";
  PageData += "<div class=\"alert alert-success fade in\">Attempting reboot, but power cycle if needed. ";
  PageData += "You will then need to connect to the <b>"+String(ap_ssid)+"</b> SSID and open ";
  PageData += "<b>http://192.168.4.1</b> in a web browser to reconfigure.</div></div>";
  ResetEEPROM();
  server.send(200, "text/html", PageData);
  delay(1000);    // wait to deliver response
  WiFi.disconnect(true);
  ESP.restart();  // should reboot
}

void ResetWiFiPageHandler() {
  String PageData = WriteHTMLHead();
  PageData += "<div class=\"alert alert-success fade in\"><strong>Attempting a restart.</strong>";
  PageData += "<script> var timer = setTimeout(function() {window.location='/'}, 10000);</script>";  
  server.send(200, "text/html", PageData);
  delay(1000);    // wait to deliver response
  ESP.restart();  // should reboot
  delay(10000);
  PageData = "<!DOCTYPE html>\n"
                    "<html>\n"
                    "<head>\n";
  PageData += "<meta http-equiv=\"refresh\" content=\"1; url=/\"\n";
  PageData += "</head>\n";
  PageData += "<body>\n";
  PageData += "</body>\n";
  PageData += "</html>\n";               
  server.send(200, "text/html", PageData);
}

void GetInitialDataPageHandler()
{
  if (SlaveName != "None")
    {
    if (PostDataToSlave(SlaveIP,true,false))                       //Call slave IP in 'test' mode to see if alive
      SwitchStateString = "true";                                  //Root page will show a check against slave
    else
      SwitchStateString = "false";                                 //Root page will show a cross against slave
    }
  else  
    SwitchStateString = "N/A";                                     //Root page will show nothing against 'None' slave name

  if (SwitchState==true)                                           //Switch state string response to show on root page
      SwitchStateString += "|true";
  else
      SwitchStateString += "|false";

  if (InvertSlave==true)                                           //To show 'Slave' or 'Slave (Inverted)' on root page
      strcpy(InvertSlaveString,"Slave (Inverted):");
  else
      strcpy(InvertSlaveString,"Slave:");

      
  server.send(200, "text/plain", WIFI_SSID + '|' + DeviceName + '|' + SlaveName + '|' + InvertSlaveString + '|' + SwitchStateString); 
}

void GetWifiListHandler()
{
  String SSID_Array;
  int ap_count = WiFi.scanNetworks();
  if (ap_count != 0)
  {
    if (WiFi.status() == WL_CONNECTED) 
      {
        SSID_Array =  WIFI_SSID;
        SSID_Array += '|';
      }  
    else
      SSID_Array =  "Not Connected";     
    for (uint8_t ap_idx = 0; ap_idx < ap_count; ap_idx++)
    {
      SSID_Array += String(WiFi.SSID(ap_idx));
      if ((ap_idx+1) < ap_count)
        SSID_Array += '|';
    }
  }
  
  server.send(200, "text/plain", SSID_Array);  
}

void GetDeviceNameHandler()
{
  server.send(200, "text/plain", DeviceName);  
}

void GetSinricHandler()
{
  server.send(200, "text/plain", AppKeyString+'|'+AppSecretString+'|'+SwitchIDString);  
}

void GetSlaveHandler()
{
  NumSlaves=MDNS.queryService("eSWITCH","tcp");
  String SlaveData;
  if (InvertSlave)
    SlaveData += "true|";
  else
    SlaveData += "false|";  
  SlaveData += SlaveName;
  for (uint8_t slave_idx = 0; slave_idx < NumSlaves; slave_idx++)
    {
      String SlaveStr=MDNS.hostname(slave_idx);
      int LocalPos=SlaveStr.indexOf("local");
      String SlaveNameStr=SlaveStr.substring(0,LocalPos-1);  
      String SlaveIP = MDNS.IP(slave_idx).toString();
      String SlaveId = SlaveNameStr+" ("+SlaveIP+")";
      if ((SlaveNameStr != DeviceName) && (SlaveId != SlaveName))
        {
        SlaveData += "|";
        SlaveData += SlaveId;        
        }
    }
  if (SlaveName != "None")
    SlaveData += "|None";  
  server.send(200, "text/plain", SlaveData);  
}


void WifiPageHandler()
{
  int DelayLoop=0;
  if ((server.hasArg("Wifi")) && ((server.arg("Wifi") != WIFI_SSID) || (server.arg("Password") != WIFI_PASS)))
  {
    if (server.hasArg("Password"))
    {
      WIFI_SSID = server.arg("Wifi");
      WIFI_PASS = server.arg("Password");
      WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }
    else
    {
      WIFI_SSID = server.arg("Wifi");
      WiFi.begin(WIFI_SSID.c_str());
    }
    while ((WiFi.status() != WL_CONNECTED) && (DelayLoop < 10))
    {
      DelayLoop++;
      delay(500);
    }

    WiFi.mode(WIFI_STA);
    WriteEEPROMWiFi(server.arg("Wifi"), server.arg("Password"));

  }
  server.send(200, "text/plain", WIFI_SSID);
  ESP.restart();  // should reboot 
}


String WriteHTMLHead()
{
  String HTML = "<!DOCTYPE html>\n"
                    "<html>\n"
                    "<head>\n";
  HTML += "<meta charset=\"utf-8\">\n";
  HTML += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=no\">\n";
  HTML += "<link href=\"All.css\" rel=\"stylesheet\">\n";
  HTML += "<title>AMSONOFF eSwitch</title></head>\n";
  HTML += "<body style=\"color:black;background-color:white;\">\n";
  HTML += " <div class=\"text-center\">\n";
  HTML += "   <img src=\"amsonoff.jpg\" class=\"img-fluid\" alt=\"Switch Image\">\n";
  HTML += " </div>\n"; 
  return(HTML);
}

void RootPageHandler()
{ 
  server.send_P(200, "text/html", ROOT_page);
}

void SetupPageHandler()
{   
  server.send_P(200, "text/html", SETUP_page);
}


String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".jpg")) return "image/jpeg"; 
  else if(filename.endsWith(".svg")) return "image/svg+xml";
  return "text/plain";
}


bool handleFileRead(String path) { // send the right file to the client (if it exists)
  String contentType = getContentType(path);            // Get the MIME type
  if (LittleFS.exists(path)) {                            // If the file exists
    File file = LittleFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  return false;                                         // If the file doesn't exist, return false
}
