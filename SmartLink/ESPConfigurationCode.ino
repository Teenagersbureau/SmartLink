//Web Server specific header files
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//Header Files and Defination for Blynk remote controlling
#define BLYNK_TEMPLATE_ID "<YOUR-BLYNK-TEMPLAT-ID>"
#define BLYNK_DEVICE_NAME "<YOUR-BLYNK-DEVICE-NAME>"
#define BLYNK_AUTH_TOKEN "<YOUR-AUTH-TOKEN-HERE>"
#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>

//Header files for controlling device through whatsapp (twilio integration)
#include <ThingESP.h>
#include <BlynkSimpleEsp8266.h>

//Variable Declarations
static const uint8_t D0   = 16;
static const uint8_t WhatsAppControlledESP8266PinD0 = D0;

//Setting output pin=16 for web server requirements in the code further 
const int output=16;
const int buzzerPin=5;

//Useful params and instance of Thing
ThingESP8266 thing("<YOUR-ID-HERE>", "<YOUR-PROJECT-NAME-HERE>", "<YOUR-PASSWORD-HERE>");
char auth[] = "<YOUR-AUTH-TOKEN-HERE>";
char ssid[] = "<YOUR-SSID-HERE>";
char pass[] = "<YOUR-WIFI-PASSWORD-HERE>";

//Username and password for web server login
const char http_username[] = "<WEB-SERVER-USERNAME-DECLARATION>";
const char http_password[] = "<WEB-SERVER-PASSWORD-DECLARATION>";

const char PARAM_INPUT_1[] = "state";

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

//HTML FOR THE HOME PAGE 
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>IoT Switch Web Server</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      html 
      {
       font-family: Arial; 
       display: inline-block; 
       text-align: center;
      }
      h2 
      {
       font-size: 2.6rem;
      }
      body 
      {
       max-width: 600px; 
       margin:0px auto; 
       padding-bottom: 10px;
      }
      .switch 
      {
        position: relative;
        display: inline-block;
        width: 120px;
        height: 68px
      } 
      .switch input 
      {
        display: none
      }
      .slider 
      {
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        bottom: 0;
        background-color: #ccc;
        border-radius: 34px
      }
      .slider:before 
      {
        position: absolute;
        content: "";
        height: 52px;
        width: 52px;
        left: 8px;
        bottom: 8px;
        background-color: #fff;
        -webkit-transition: .4s;
        transition: .4s;
        border-radius: 68px
      }
      input:checked+.slider 
      {
        background-color: #2196F3;
      }
      input:checked+.slider:before 
      {
        -webkit-transform: translateX(52px);
        -ms-transform: translateX(52px);
        transform: translateX(52px)
      }
    </style>
  </head>
  <body>
    <h2>WiFi Switch Web Server</h2>
    <button onclick="logoutButton()">Logout</button>
    <p>WiFi-Switch is <span id="state">%STATE%</span></p>
    %BUTTONPLACEHOLDER%
    <script>
      function toggleCheckbox(element) 
      {
        var xhr = new XMLHttpRequest();
        if(element.checked)
        { 
          xhr.open("GET", "/update?state=0", true); 
          document.getElementById("state").innerHTML = "ON";  
        }
        else 
        { 
          xhr.open("GET", "/update?state=1", true); 
          document.getElementById("state").innerHTML = "OFF";      
        }
        xhr.send();
      }
      function logoutButton() 
      {
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "/logout", true);
        xhr.send();
        setTimeout(function(){ window.open("/logged-out","_self"); }, 1000);
      }
    </script>
  </body>
</html>
)rawliteral";

//HTML FOR LOGOUT PAGE
const char logout_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
  </head>
  <body>
    <p>You have been successfully logged out from the web server ! <a href="/">Return to homepage</a>.</p>
  </body>
</html>
)rawliteral";

// Replaces placeholder with button section in your web page
String processor(const String& var)
{
  //Serial.println(var);
  if(var == "BUTTONPLACEHOLDER")
  {
    String buttons ="";
    String outputStateValue = outputState();
    buttons+= "<p><label class=\"switch\"><input type=\"checkbox\" onchange=\"toggleCheckbox(this)\" id=\"output\" " + outputStateValue + "><span class=\"slider\"></span></label></p>";
    return buttons;
  }
  if (var == "STATE")
  {
    if(!digitalRead(output))
    {
      return "ON";
    }
    else 
    {
      return "OFF";
    }
  }
  return String();
}

//Function to give the output state of the device
String outputState()
{
  if(digitalRead(output))
  {
    return "checked";
  }
  else 
  {
    return "";
  }
  return "";
}

//blynk.cc specific stuff
BLYNK_WRITE(V0) {
  digitalWrite(D0, param.asInt());
}

//Setup function runs once in the entire program lifetime
void setup() {
  pinMode(D0, OUTPUT);
  Blynk.begin(auth, ssid, pass, "blynk.cloud", 80);

  //Baud Rate
  Serial.begin(115200);
  
  thing.SetWiFi("<YOUR-SSID-HERE>", "<YOUR-WIFI-PASSWORD-HERE>");
  thing.initDevice();

  //Print the ip address once the device has been connected to wifi for interacting through web server  
  Serial.println(WiFi.localIP());
  
  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(http_username, http_password))
    {
        return request->requestAuthentication();
    }
    request->send_P(200, "text/html", index_html, processor);
  });
    
  server.on("/logout", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send(401);
  });

  server.on("/logged-out", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    request->send_P(200, "text/html", logout_html, processor);
  });

  // Send a GET request to <ESP_IP>/update?state=<inputMessage>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) 
  {
    if(!request->authenticate(http_username, http_password))
    {
      return request->requestAuthentication();
    }
    
    String inputMessage;
    String inputParam;
    
    // GET input1 value on <ESP_IP>/update?state=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) 
    {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      digitalWrite(output, inputMessage.toInt());
    }
    else 
    {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    Serial.println(inputMessage);
    request->send(200, "text/plain", "OK");
  });
  
  // Start server
  server.begin();
}

void loop() {
  Blynk.run();
  thing.Handle();
}


//Twilio Specific Strings 
String HandleResponse(String query)
{
  // 1-->off 0-->on
  
  if (query == "Switch1 on" || query == "switch1 on") 
  {

    //if returns 0, means switch is already on, so negate the returned value
    if(!digitalRead(WhatsAppControlledESP8266PinD0))
    {
      return "*Wifi Switch One is already Turned ON*";
    }
    digitalWrite(WhatsAppControlledESP8266PinD0, 0);
    return "*Done: Wifi Switch One Turned ON*";
  }

  else if (query == "Switch1 off" || query == "switch1 off") 
  {

    //if returns 1, means switch is already off
    if(digitalRead(WhatsAppControlledESP8266PinD0))
    {
      return "*Wifi Switch One is already Turned OFF*";
    }
    digitalWrite(WhatsAppControlledESP8266PinD0, 1);
    return "*Done: Wifi Switch One Turned OFF*";
  }
  
  else if (query == "Switch1 status" || query == "switch1 status")
  {
    return digitalRead(WhatsAppControlledESP8266PinD0) ? "*Wifi Switch One is OFF*" : "*Wifi Switch One is ON*";
  }
  else if (query == "Help" || query == "help")
  {
    return "Welcome to Home Automation Services. \nYou can ask me to turn on an IoT device, turn it off or ask the status of it :) \nFor example, type 'switch1 on', and I will turn on the connected IoT device. Hope you enjoy our service ! \n*_Home Automation, by Abhinav Anand_*"; 
  }  
  else return "*PLEASE TYPE A VALID QUERY !*";
}
