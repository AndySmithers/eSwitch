const char ROOT_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head>
    <meta charset='utf-8'>
      <meta name='viewport' content='width=device-width, initial-scale=1, shrink-to-fit=no'><link href='All.css' rel='stylesheet'>
      <title>AMSONOFF eSwitch</title>
  </head>
  <body onload='LoadData()' style='color:black;background-color:white;'>
    <div class='text-center'><img src='amsonoff.jpg' class='img-fluid' alt='Switch Image'></div>
    <div class='container' role='main'>
      <div class='card text-white bg-dark mb-3'>
        <div class='card-header' style='background-color: #00AA9E;'>Version 1.11<a href='/setup'><img src='gear.svg' style='float:right' width='32' height='32'></a></div>
        <br><br>
        <table class='table' style='background-color:white'>
          <tbody>
            <tr>
              <td>WiFi:</td>
              <td><span id='WifiID'>Syncing with switch...</span></td>
            </tr>
            <tr>  
              <td>DeviceName:</td>
              <td><span id='DeviceID'>Syncing with switch...</span></td>
            </tr>
            <tr>  
            <tr>
              <td><span id='SlavePolarity'>Slave:</span></td>
              <td><span id='SlaveID'>Syncing with switch...</span>
              <img id='SlaveOKImage' src='blank.png' style='float:right' width='32' height='32'></td>
            </tr>  
          </tbody>
        </table>
      </div>
      <form action='/switch' class='form-horizontal' style='color:black;' id='SwitchForm' role='form' method='POST'>
        <div class='card text-white bg-dark mb-3'>
          <div class='card-header' style='background-color: #00AA9E;'>Switch</div>
          <div class='card-body'>
            <div class='onoffswitch'>
              <input type='checkbox' name='onoffswitch' value='1' class='onoffswitch-checkbox' id='onoffswitch' tabindex='0' onchange='SendSwitch();'>  
                <label class='onoffswitch-label' for='onoffswitch'><span class='onoffswitch-inner'></span><span class='onoffswitch-switch'></span></label>
              </input>
            </div>
            <input type='hidden' value='0' name='onoffswitch'></input>
          </div>
        </div>
      </form>
    </div>

    <script>
      function SendSwitch(SwitchState)
      {
        var OldSwitch = document.getElementById('onoffswitch').checked;
        var xhttp =  new XMLHttpRequest();
        xhttp.onreadystatechange = function()
        {
          if (this.readyState == 4 && this.status == 200)
            {
              var ReturnArray = this.responseText.split('|');
              if (ReturnArray[0] === 'true')
                {
                  document.getElementById('onoffswitch').checked = true;
                }
              else
                {
                  document.getElementById('onoffswitch').checked = false; 
                }
              if (ReturnArray[1] === 'false')
                {
                alert("Slave did not respond!");  
                }
            }
        };
        xhttp.open('GET','switch?SwitchState='+document.getElementById('onoffswitch').checked,true);
        xhttp.timeout = 2000;
        xhttp.ontimeout = function () {alert("No response from Switch"); document.getElementById('onoffswitch').checked = !OldSwitch;}
        xhttp.send();   
      }

      function LoadData()
      {
        var xhttp =  new XMLHttpRequest();
        xhttp.onreadystatechange = function()
        {
          if (this.readyState == 4 && this.status == 200)
            {
              var ReturnArray = this.responseText.split('|');
              document.getElementById('WifiID').innerHTML = ReturnArray[0];
              document.getElementById('DeviceID').innerHTML = ReturnArray[1];
              document.getElementById('SlaveID').innerHTML = ReturnArray[2];
              document.getElementById('SlavePolarity').innerHTML = ReturnArray[3];
              if (ReturnArray[4] == 'true')
                {
                document.getElementById('SlaveOKImage').src = 'check.png';
                }
              else if (ReturnArray[4] == 'false') 
                {
                document.getElementById('SlaveOKImage').src = 'cross.png';
                }
              else  
                {
                document.getElementById('SlaveOKImage').src = 'blank.png';
                }
              if (ReturnArray[5] === 'true')
                {
                  document.getElementById('onoffswitch').checked = true;
                }
              else
                {
                  document.getElementById('onoffswitch').checked = false; 
                }
              
            }
        };   
        xhttp.open('GET','getinitialdata',true);
        xhttp.send();
       }
    </script>
  </body>
</html>

)=====";
