#include <ESP8266HTTPClient.h>
#include <BearSSLHelpers.h>
#include <CertStoreBearSSL.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
//#include <WiFiClientSecureAxTLS.h>
//#include <WiFiClientSecureBearSSL.h>
#include <WiFiServer.h>
#include <WiFiServerSecure.h>
//#include <WiFiServerSecureAxTLS.h>
//#include <WiFiServerSecureBearSSL.h>
#include <WiFiUdp.h>
#include <SPI.h>
#include <PubSubClient.h>
#include <ESP8266WebServer.h>
#include <ArduinoOTA.h>

boolean PublishOK = false;
const char * ssid = "ssid";
const char * password = "password";

#define ROSSO 14
#define VERDE 4
#define BLU   12

#define INPUT_LAMPADINA 5

//circa mezzora
#define SendLoop 9000

int ValuePrec;

const char* mqtt_server = "192.168.1.12";
WiFiClient espClient;
PubSubClient client(espClient);



/*OTA*/
ESP8266WebServer oserver(80);
/* Style */
String style =
"<style>#file-input,input{width:100%;height:44px;border-radius:4px;margin:10px auto;font-size:15px}"
"input{background:#f1f1f1;border:0;padding:0 15px}body{background:#3498db;font-family:sans-serif;font-size:14px;color:#777}"
"#file-input{padding:0;border:1px solid #ddd;line-height:44px;text-align:left;display:block;cursor:pointer}"
"#bar,#prgbar{background-color:#f1f1f1;border-radius:10px}#bar{background-color:#3498db;width:0%;height:10px}"
"form{background:#fff;max-width:258px;margin:75px auto;padding:30px;border-radius:5px;text-align:center}"
".btn{background:#3498db;color:#fff;cursor:pointer}</style>";

/* Login page */
String loginIndex = 
"<form name=loginForm>"
"<h1>Serra Login</h1>"
"<input name=userid placeholder='User ID'> "
"<input name=pwd placeholder=Password type=Password> "
"<input type=submit onclick=check(this.form) class=btn value=Login></form>"
"<script>"
"function check(form) {"
"if(form.userid.value=='admin' && form.pwd.value=='admin')"
"{window.open('/serverIndex')}"
"else"
"{alert('Error Password or Username')}"
"}"
"</script>" + style;
 
/* Server Index Page */
String serverIndex = 
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
"<input type='file' name='update' id='file' onchange='sub(this)' style=display:none>"
"<label id='file-input' for='file'>   Choose file...</label>"
"<input type='submit' class=btn value='Update'>"
"<br><br>"
"<div id='prg'></div>"
"<br><div id='prgbar'><div id='bar'></div></div><br></form>"
"<script>"
"function sub(obj){"
"var fileName = obj.value.split('\\\\');"
"document.getElementById('file-input').innerHTML = '   '+ fileName[fileName.length-1];"
"};"
"$('form').submit(function(e){"
"e.preventDefault();"
"var form = $('#upload_form')[0];"
"var data = new FormData(form);"
"$.ajax({"
"url: '/update',"
"type: 'POST',"
"data: data,"
"contentType: false,"
"processData:false,"
"xhr: function() {"
"var xhr = new window.XMLHttpRequest();"
"xhr.upload.addEventListener('progress', function(evt) {"
"if (evt.lengthComputable) {"
"var per = evt.loaded / evt.total;"
"$('#prg').html('progress: ' + Math.round(per*100) + '%');"
"$('#bar').css('width',Math.round(per*100) + '%');"
"}"
"}, false);"
"return xhr;"
"},"
"success:function(d, s) {"
"console.log('success!') "
"},"
"error: function (a, b, c) {"
"}"
"});"
"});"
"</script>" + style;
/*END OTA*/





void setup() 
{
    
  pinMode(LED_BUILTIN, OUTPUT);
  
  pinMode(INPUT_LAMPADINA, INPUT);
  
  pinMode(ROSSO, OUTPUT);
  pinMode(BLU, OUTPUT);
  pinMode(VERDE, OUTPUT);
  
  digitalWrite(ROSSO, HIGH);
  
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  IPAddress ip(192, 168, 1, 7);
  IPAddress gateway(192, 168, 1, 254);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(ip, gateway, subnet);
  
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    digitalWrite(ROSSO, LOW);
    delay(500);
    digitalWrite(ROSSO, HIGH);
    Serial.println("Connecting..");
  }
  ResetLeds();
  client.setServer(mqtt_server, 1883);
  
  /*OTA*/
/*return index page which is stored in serverIndex */
  oserver.on("/", HTTP_GET, []() {
    oserver.sendHeader("Connection", "close");
    oserver.send(200, "text/html", loginIndex);
  });
  oserver.on("/serverIndex", HTTP_GET, []() {
    oserver.sendHeader("Connection", "close");
    oserver.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  oserver.on("/update", HTTP_POST, []() {
    oserver.sendHeader("Connection", "close");
    oserver.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = oserver.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  oserver.begin();
/*END OTA*/
}

void ResetLeds() 
{
  digitalWrite(ROSSO, LOW);
  digitalWrite(VERDE, LOW);
  digitalWrite(BLU, LOW);
}

int counterVolte = SendLoop-10;


void reconnect() {
  int o = 0;
  // Loop until we're reconnected
  while (!client.connected()) 
  {
    // Attempt to connect
    while (WiFi.status() != WL_CONNECTED) 
    {
      digitalWrite(ROSSO, HIGH);
      delay(100);
      ResetLeds();
      delay(100);
    }
    
    if (client.connect("GateClient","mqtt_user","password")) 
    {
      Serial.println("connected");
    } 
    else 
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying        
      delay(5000);
    }
  }
}

boolean sendMQTT(unsigned char value)
{
  boolean pub;
  String tempString;
  char buffertempString[100];
  tempString = "{\"Status\" : \"" + String(value)+"\", ";
  tempString += "\"Signal\" : \"" + String(WiFi.RSSI())+"\"";
  tempString += "}";
  tempString.toCharArray(buffertempString,100);
  pub = client.publish("Cancello", buffertempString);
  return pub;  
}

void loop() 
{
  oserver.handleClient();
  counterVolte++;
  if(counterVolte < SendLoop) 
  {
    if (WiFi.status() == WL_CONNECTED) 
    {
      digitalWrite(VERDE, HIGH);
      if (!client.connected()) 
      {
        reconnect();
      }
      client.loop();
    } 
    else 
    {
        while (WiFi.status() != WL_CONNECTED) 
        {
          digitalWrite(ROSSO, HIGH);
          delay(100);
          ResetLeds();
          delay(100);
        }
    }
    
    
    delay(100);
    ResetLeds();
    delay(100);

    if ((ValuePrec != digitalRead(INPUT_LAMPADINA)) || ((counterVolte > 10) && (PublishOK == false))) //ogni 2 secondi se la risposta è negativa
    {
      counterVolte = 30000;
      ValuePrec = digitalRead(INPUT_LAMPADINA);
    }
  }
  else
  {
    if (WiFi.status() == WL_CONNECTED) 
    {
      if (!client.connected()) 
      {
        reconnect();
      }
      client.loop();
      if (client.connected()) 
      {
        ResetLeds();
        digitalWrite(BLU, HIGH);
        if (digitalRead(INPUT_LAMPADINA)) 
        {
          delay(500);
          PublishOK = sendMQTT(1);
        } 
        else
        {
          delay(500);
          PublishOK = sendMQTT(0);
        }
    
        if (PublishOK == false) 
        {
          delay(300);
          ResetLeds();
          delay(300);
          digitalWrite(BLU, HIGH);
          digitalWrite(ROSSO, HIGH);
          delay(300);
          ResetLeds();
          digitalWrite(ROSSO, HIGH);
          delay(300);
          ResetLeds();
          digitalWrite(BLU, HIGH);
          digitalWrite(ROSSO, HIGH);
          delay(300);
          Serial.println("invalid reponse");
        }
        else
        {
          delay(500);
          if (digitalRead(INPUT_LAMPADINA)) 
          {
            delay(500);
            PublishOK = sendMQTT(1);
          } 
          else
          {
            delay(500);
            PublishOK = sendMQTT(0);
          }
          
          if (PublishOK == false) 
          {
            delay(300);
            ResetLeds();
            delay(300);
            digitalWrite(BLU, HIGH);
            digitalWrite(ROSSO, HIGH);
            delay(300);
            ResetLeds();
            digitalWrite(ROSSO, HIGH);
            delay(300);
            ResetLeds();
            digitalWrite(BLU, HIGH);
            digitalWrite(ROSSO, HIGH);
            delay(300);
            Serial.println("invalid reponse");
          }
        }
        ResetLeds();
      }
      else
      {
          counterVolte = 30000;
          return;
      }
    } 
    else 
    {
      Serial.println("not connected");
      counterVolte = 30000;
      return;
    }
    counterVolte = 0;
  }
  

  
}
