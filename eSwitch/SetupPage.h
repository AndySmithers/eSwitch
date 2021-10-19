const char SETUP_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <meta charset='utf-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'><link href='All.css' rel='stylesheet'>
      <title>AMSONOFF eSwitch</title>
  </head>
  <body onload='LoadCurrentData()' style='color:black;background-color:white;'>
    <div class='text-center'><img src='amsonoff.jpg' class='img-fluid' alt='Switch Image'></div>
    <div class='container' role='main'>
      <div class='card text-white bg-dark mb-3'>
        <div class='card-header' style='background-color: #00AA9E;'>Setup<a href='/'><img src='house.svg' style='float:right' width='32' height='32'></a></div>
        <div class='card-body'>
          <div class='card text-white bg-dark mb-3'>
            <div class='card-header' style='background-color: #00AA9E;'>WiFi</div>
              <div class='input-group'>
                <div class='input-group-prepend'><label class='input-group-text' style='color:black;' for='SSID'>SSID</label></div>
                <select class='custom-select' name='SSID' id='SSID'>
                  <option value='None'>Syncing with switch...</option>
                </select>
              </div>
              <div class='input-group'>
                <div class='input-group-prepend'>
                  <label class='input-group-text' style='color:black;' for='Password'>Password</label>
                </div>
                <input type='password' class='form-control' name='Password' id='Password' value=''></input>
              </div>
              <div class='form-group'>
                <div class='col-xs-offset-3 col-xs-9'>
                  <input type='button' class='btn btn-primary' value='Update WiFi' onclick="if(confirm('Confirm WiFi change?')){ChangeWifi();}else{return false;};"></input>
                </div>
              </div>
      <div class='card text-white bg-dark mb-3'><div class='card-header' style='background-color: #00AA9E;'>Device</div>
      <div class='input-group'>
        <div class='input-group-prepend'>
          <label class='input-group-text' style='color:black;' for='DeviceName'>Device Name</label>
        </div>
        <input type='text' class='form-control' name='DeviceName' id='DeviceName' value='Syncing with switch...'></input>
      </div>
      <div class='form-group'>
        <div class='col-xs-offset-3 col-xs-9'>
          <input type=button class='btn btn-primary' value='Update Device'  onclick="if(confirm('Confirm Device Name change?')){ChangeDevice();}else{return false;};"></input>
        </div>
      </div>
      <div class='card text-white bg-dark mb-3'><div class='card-header' style='background-color: #00AA9E;'>Sinric Settings</div>
      <div class='input-group'>
        <div class='input-group-prepend'>
          <label class='input-group-text' style='color:black;' for='AppKey'>App Key</label>
        </div>
        <input type='text' class='form-control' name='AppKey' id='AppKey' value='Syncing with switch...'></input>
      </div>
      <div class='input-group'>
        <div class='input-group-prepend'>
          <label class='input-group-text' style='color:black;' for='AppSecret'>App Secret</label>
        </div>
        <input type='text' class='form-control' name='AppSecret' id='AppSecret' value='Syncing with switch...'></input>
      </div>
      <div class='input-group'>
        <div class='input-group-prepend'>
          <label class='input-group-text' style='color:black;' for='SwitchID'>Switch ID</label>
        </div>
        <input type='text' class='form-control' name='SwitchID' id='SwitchID' value='Syncing with switch...'></input>
      </div>
      <div class='form-group'>
        <div class='col-xs-offset-3 col-xs-9'>
          <input type='button' class='btn btn-primary' value='Update Sinric Settings'  onclick="if(confirm('Confirm Sinric Settings change?')){ChangeSinric();}else{return false;};"></input>
        </div>
      </div>
    <div class='card text-white bg-dark mb-3'>
      <div class='card-header' style='background-color: #00AA9E;'>Slave Settings</div>
      <div class='input-group'>
        <div class='input-group-prepend'>
          <label class='input-group-text' style='color:black;' for='Slave'>Slave</label>
        </div>
        <select class='custom-select' name='Slave' id='Slave'>
          <option value='None'>Syncing with switch...</option>
        </select>
      </div>
      <div class='form-group'>
        <div class='col-xs-offset-3 col-xs-9'>
          <div class='form-check'>
            <input class='form-check-input form-check-inline' type='checkbox' value='1' name='InvertSlave' id='InvertSlave'></input>
            <label class='form-check-label' style='color:white;' for='InvertSlave'>Invert Slave?</label>
          </div>
        </div>
        <input type='button' class='btn btn-primary' value='Update Slave' onclick="if(confirm('Confirm Slave(s) change?')){ChangeSlave();}else{return false;};"></input>
      </div>
    </div>
    <a class='btn btn-primary btn-lg btn-block' href='/resetwifi' onclick="return confirm('Are you sure you want to Reboot Switch?')">Reboot Switch</a><br><br>
    <a class='btn btn-primary btn-lg btn-block' href='/reset' onclick="return confirm('This will delete all settings and return to factory defaults - Are you sure?')">Factory Reset</a><br><br>
    <a class='btn btn-primary btn-lg btn-block' href='/update' onclick="return confirm('WARNING - Incorrect usage can permanetly damage this switch! - Are you sure you know what you are doing?')">Update Firmware</a>
  </div>
</div>
</div>
</div>  
</div>

<script>
  
function LoadCurrentData()
      {
        LoadWifiList();
        LoadDevice();
        LoadSinric();
        LoadSlave();
        
      }
      
function LoadWifiList()
      {
        var xhttp = new XMLHttpRequest();  
        xhttp.onreadystatechange = function()
        {
          if (this.readyState == 4 && this.status == 200)
            {
              var ReturnArray = this.responseText.split('|');
              if (ReturnArray.length > 0)
                {
                  var OptionList = document.getElementById('SSID');
                  OptionList.remove(0);
                  for (var i = 1; i < ReturnArray.length; i++)
                  {
                    OptionList.options[OptionList.options.length] = new Option(ReturnArray[i],ReturnArray[i]);
                  }
                  OptionList.value = ReturnArray[0];
                }
            }
        };   
        xhttp.open('GET','getwifilist',true);
        xhttp.send();   
      }

function LoadDevice()
      {
        var xhttp = new XMLHttpRequest();  
        xhttp.onreadystatechange = function()
        {
          if (this.readyState == 4 && this.status == 200)
            {
              document.getElementById("DeviceName").value = this.responseText;
            }
        };    
        xhttp.open('GET','getdevicename',true);
        xhttp.send();          
      }

function LoadSinric()
      {
        var xhttp = new XMLHttpRequest();  
        xhttp.onreadystatechange = function()
        {
          if (this.readyState == 4 && this.status == 200)
            {
              var ReturnArray = this.responseText.split('|');
              document.getElementById("AppKey").value = ReturnArray[0];
              document.getElementById("AppSecret").value =  ReturnArray[1];
              document.getElementById("SwitchID").value =  ReturnArray[2];
            }
        };    
        xhttp.open('GET','getsinric',true);
        xhttp.send();          
      }

function LoadSlave()
      {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function()
        {
          if (this.readyState == 4 && this.status == 200)
            {
              var ReturnArray = this.responseText.split('|');
              if (ReturnArray.length > 0)
                {
                  var OptionList = document.getElementById('Slave');
                  OptionList.remove(0);
                  for (var i = 1; i < ReturnArray.length; i++)
                  {
                    OptionList.options[OptionList.options.length] = new Option(ReturnArray[i],ReturnArray[i]);
                  }
                if (ReturnArray[0] == "true")  
                  document.getElementById('InvertSlave').checked=true;
                else
                  document.getElementById('InvertSlave').checked=false;                      
                }
            }
        };
        xhttp.open('GET','getslave',true);
        xhttp.send();          
      }

function ChangeWifi()
      {
        var xhttp = new XMLHttpRequest();  
        var WifiList = document.getElementById("SSID");
        var Password = document.getElementById("Password")  ;
        var SelectedWifi = WifiList.selectedOptions;
        xhttp.open('GET','wifi?Wifi='+SelectedWifi[0].text+'&Password='+Password.value,true);
        xhttp.send();
      }

function ChangeDevice()
      {
        var xhttp = new XMLHttpRequest();  
        var DeviceElement = document.getElementById("DeviceName");
        xhttp.open('GET','device?DeviceName='+DeviceElement.value,true);
        xhttp.send();
      }

function ChangeSinric()
      {
        var xhttp = new XMLHttpRequest();  
        var AppKeyElement = document.getElementById("AppKey");
        var AppSecretElement = document.getElementById("AppSecret");
        var SwitchIDElement = document.getElementById("SwitchID");
        xhttp.open('GET','sinric?AppKey='+AppKeyElement.value+'&AppSecret='+AppSecretElement.value+'&SwitchID='+SwitchIDElement.value,true);
        xhttp.send();
      }

function ChangeSlave()
      {
        var xhttp = new XMLHttpRequest();  
        var DeviceElement = document.getElementById("Slave");
        xhttp.open('GET','slave?Slave='+DeviceElement.value+'&InvertSlave='+document.getElementById('InvertSlave').checked,true);
        xhttp.send();
      }
      
    </script> 
  </body>
</html>

)=====";
