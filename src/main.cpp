/*
    !Do not reload the board just make a switch between STA/AP where you can relaunch one on demand and turn off another one

    Make a reset page as well
*/

#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <ESP32Ping.h>
#include <SPIFFS.h>
#include <EEPROM.h>
#include <AsyncElegantOTA.h>
#include <esp_wifi.h>

#define EEPROM_SIZE 1

const int ledPin = 2;
uint32_t chipId = 0;
uint32_t authId = 0;
int wifiTimeout = 0;
bool wifiState = false;
bool authTrigger = false;
String pageLoaded = "false";

const char* ssid = "";
const char* password = "";
const char* ap_ssid = "Domovyk";
const char* ap_password = "beehappy";
IPAddress AP_LOCAL_IP(192, 168, 4, 1);
IPAddress AP_GATEWAY_IP(192, 168, 4, 2);
IPAddress AP_NETWORK_MASK(255, 255, 255, 0);

const char* ssid_input = "ssid_input";
const char* pass_input = "pass_input";
String ssid_var;
String pass_var;
String secret_var;
String name_var;
const char* ssidPath = "/ssid.txt";
const char* passPath = "/password.txt";
const char* secretPath = "/secret.txt";
const char* namePath = "/name.txt";

const char* remote_host = "www.google.com"; //ping host

#define BOT_TOKEN "5800593194:AAFLU2VGDUC0MzzwJi865dBUuRDL6RCCCa0" //Domovyk (domovyk_ua_bot)

WiFiClientSecure secured_client;
UniversalTelegramBot bot(BOT_TOKEN, secured_client);
const unsigned long BOT_MTBS = 10000; // mean time between scan messages
unsigned long bot_lasttime; // last time messages' scan has been done

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

//get device unique id
void getChipId(){
  for(int i=0; i<17; i=i+8) {
    chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  }
}

String readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);
    File file = fs.open(path);
    if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
    }
    
    String fileContent;
    while(file.available()){
    fileContent = file.readStringUntil('\n');
    Serial.println(fileContent);
    break; 
    }
    return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, FILE_WRITE);
  if(!file){
  Serial.println("- failed to open file for writing");
  return;
  }
  if(file.print(message)){
  Serial.println("- file written");
  } else {
  Serial.println("- frite failed");
  }
}

//initiatlize SPIFFS
void initSPIFFS() {
 if (!SPIFFS.begin()) {
 Serial.println("An error has occurred while mounting SPIFFS");
 }
 Serial.println("SPIFFS mounted successfully");
}

//initialize WiFi connection
void initWiFi() {
 //delay(5000);
 //WiFi.disconnect(); 
 WiFi.mode(WIFI_MODE_APSTA);
  if (!WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY_IP, AP_NETWORK_MASK)) {
    Serial.println("AP Config Failed");
    return;
  }
 WiFi.softAP(ap_ssid, ap_password);
 Serial.println("Access Point IP: ");
 Serial.println(WiFi.softAPIP());

 ssid_var = readFile(SPIFFS, ssidPath).c_str();
 pass_var = readFile(SPIFFS, passPath).c_str();

 char ssid_char[ssid_var.length()+1];
 ssid_var.toCharArray(ssid_char, ssid_var.length()+1);
 char pass_char[pass_var.length()+1];
 pass_var.toCharArray(pass_char, pass_var.length()+1);

  if (ssid_char == NULL || pass_char == NULL) {
      Serial.println("No WiFi Credentials Found.");
  } else {
      WiFi.begin(ssid_char, pass_char);
      WiFi.setSleep(false);
      secured_client.setCACert(TELEGRAM_CERTIFICATE_ROOT);
      Serial.println("Connecting to WiFi");
      while (WiFi.status() != WL_CONNECTED) {        
        wifiTimeout = wifiTimeout + 1;
        Serial.print('.');
        delay(1000);
        if (wifiTimeout == 10) {
          Serial.println("\nWrong WiFi Credentials or No Network Found.");
          wifiTimeout = 0;
          wifiState = false;
          return;
        }
      }
      Serial.println("WiFi Local IP: ");
      Serial.println(WiFi.localIP());
      wifiState = true;
  }
 
}

void setWiFiPowerSavingMode(){
    esp_wifi_set_ps(WIFI_PS_MAX_MODEM);
    //    esp_wifi_set_ps(WIFI_PS_NONE);
    //    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}

//upading text variables
String processor(const String& var){
 if(var == "chipId") {
  String stringId = String(chipId);  
  return stringId;
 }

 if(var == "wifiState") {
  String sta_wifi;
  if (wifiState) {
    sta_wifi = "Connected";  
  } else {
    sta_wifi = "Disconnected";     
  }
  return sta_wifi;
 }

 if(var == "wifiClr") {
  String wifiClr;
  if (wifiState) {
    wifiClr = "green";  
  } else {
    wifiClr = "red";     
  }
  return wifiClr;
 }

 if(var == "staIP") {
  String staIP;
  staIP = WiFi.localIP().toString().c_str();
  return staIP;
 }

 if(var == "apIP") {
  String apIP;
  apIP = WiFi.softAPIP().toString().c_str();
  return apIP;
 }

 if(var == "ssidVar") {
  String ssidSTR = readFile(SPIFFS, ssidPath).c_str();  
  return ssidSTR;
 }

 if(var == "passVar") {
  String passSTR = readFile(SPIFFS, passPath).c_str();  
  return passSTR;
 }

 if(var == "authState") {
  String authState = readFile(SPIFFS, namePath).c_str();
  if (authState == "null"){
    authState = "Tet-a-Tet";
  } else if (authState == "") {
    authState = "Disconnected";
  }
  return authState;
 }

  if(var == "authStateClr") {
  String authStateClr = readFile(SPIFFS, namePath).c_str();
  if (authStateClr == "") {
    authStateClr = "red";
  } else {
    authStateClr = "green";
  }
  return authStateClr;
 }

 return String();
}

void reboot(){
  ESP.restart();
}

bool authentication(String chat_id){
  secret_var = readFile(SPIFFS, secretPath).c_str();
  if (chat_id == secret_var) {
    return true;
  } else {
    return false;
  }
}

void handleNewMessages(int numNewMessages)
{


  for (int i = 0; i < numNewMessages; i++)
  {

  Serial.print("handleNewMessages ");
  Serial.println(numNewMessages);

    String chat_id = bot.messages[i].chat_id;
    String text = bot.messages[i].text;
    String chat_title = bot.messages[i].chat_title;
    
    if (authTrigger) {
      if (text == String(chipId)){
        bot.sendMessage(chat_id, "Домовик авторізован", "");
        SPIFFS.remove(secretPath);
        SPIFFS.remove(namePath);
        writeFile(SPIFFS, secretPath, chat_id.c_str());
        writeFile(SPIFFS, namePath, chat_title.c_str());
      } else {
        bot.sendMessage(chat_id, "Будь ласка перевірте авторизаційний код та спробуйте ще", "");
      }
      authTrigger = false;
    }

    String from_name = bot.messages[i].from_name;
    if (from_name == "")
      from_name = "Guest";

    if (text == "/auth")
    {
      bot.sendMessage(chat_id, "Введіть код авторизації Домовика", "");
      authTrigger = true;
    }  

    if (text == "/status")
    {
        if (!authentication(chat_id)) {
          bot.sendMessage(chat_id, "Не авторізовано!", "");  
        } else {
          bot.sendChatAction(chat_id, "typing");
          Ping.ping(remote_host);
          delay(2000);
          bot.sendMessage(chat_id, "Світло є!", "");
          String message = "Середня швидкість iнтернету: " + String(Ping.averageTime()) + " ms\n";
          bot.sendMessage(chat_id, message, "Markdown");
        }
    }  

    if (text == "/start")
    {
      String welcome = "Привіт " + from_name + ", я твій домовик.\n\n";
      welcome += "/auth : Авторизація Домовика\n";
      welcome += "/status : Перевірка світла та інтернету\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
 AwsFrameInfo *info = (AwsFrameInfo*)arg;
 if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
  data[len] = 0;
  pageLoaded = (char*)data;
  Serial.print("Is web page loaded: ");
  Serial.println(pageLoaded);
  if (pageLoaded == "true"){
    reboot();
  }
 }
}

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
 switch (type) {
  case WS_EVT_CONNECT:
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    break;
  case WS_EVT_DISCONNECT:
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    break;
  case WS_EVT_DATA:
    handleWebSocketMessage(arg, data, len);
    break;
  case WS_EVT_PONG:
  case WS_EVT_ERROR:
   break;
 }
}

void initWebSocket() {
 ws.onEvent(onEvent);
 server.addHandler(&ws);
}

void setup() {
  Serial.begin(115200);
  delay(50);
  getChipId();
  initSPIFFS();
  ssid_var = readFile(SPIFFS, ssidPath).c_str();
  pass_var = readFile(SPIFFS, passPath).c_str();
  secret_var = readFile(SPIFFS, secretPath).c_str();
  name_var = readFile(SPIFFS, namePath).c_str();
  //esp_wifi_set_ps(WIFI_PS_NONE); //wifi power saving is off
  //lru_purge_enable = true;
  initWiFi();
  setWiFiPowerSavingMode();
  pinMode(ledPin , OUTPUT);
  initWebSocket();

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(SPIFFS, "/index.html", "text/html", false, processor);
      return;
  });

  server.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request){
        SPIFFS.remove(ssidPath);
        SPIFFS.remove(passPath);        
        SPIFFS.remove(secretPath);
        SPIFFS.remove(namePath);
        delay(1);
        ssid_var = "";
        pass_var = "";
        secret_var = "";
        name_var = "";

        request->send(SPIFFS, "/reset.html", "text/html");
        return;
  });

  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
    int params = request->params();
      for(int i=0;i<params;i++){
        AsyncWebParameter* p = request->getParam(i);
        if(p->isPost()){
          if (p->name() == ssid_input) {
            ssid_var = p->value().c_str();
            Serial.print("SSID set to: ");
            Serial.println(ssid_var);
            // Write file to save value
            writeFile(SPIFFS, ssidPath, ssid_var.c_str());
          }
          if (p->name() == pass_input) {
            pass_var = p->value().c_str();
            Serial.print("PASS set to: ");
            Serial.println(pass_var);
            // Write file to save value
            writeFile(SPIFFS, passPath, pass_var.c_str());
          }
          //Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
        }
      }
  request->send(SPIFFS, "/wifi.html", "text/html");
  return;
  });

  AsyncElegantOTA.begin(&server); 
  
  server.serveStatic("/", SPIFFS, "/");
  server.begin();
  Serial.println("Server has been started");
}

void loop() {

  //ws.cleanupClients();

  while (!wifiState) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    digitalWrite(ledPin, LOW);
    delay(250);
  }

  delay(1);
  digitalWrite(ledPin, HIGH);

  if (millis() - bot_lasttime > BOT_MTBS)
  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages)
    {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      Serial.print("msg: ");Serial.println(numNewMessages);
    }

    bot_lasttime = millis();
  }
}