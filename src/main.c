#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#include <ESP8266mDNS.h>        // Include the mDNS library - utilizado para host.local - avahi/bonjour...etc

#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>


#ifndef STASSID
#define STASSID "ssid"
#define STAPSK  "123456"
#endif

const char* ssid = STASSID;
const char* password = STAPSK;



#include <WiFiUdp.h>


#include <ESP8266FtpServer.h>


#include <SPI.h>
#include <SD.h>
#define CS_PIN  2


// #define SS_PIN  2;
//padrão esp8266:
// #define MOSI_PIN  12;
// #define MISO_PIN  13;
// #define SCK_PIN  14;

FtpServer ftpSrv;   //set #define FTP_DEBUG in ESP8266FtpServer.h to see ftp verbose on serial



const char* mqttServer = "192.168.2.1";
const int mqttPort = 1883;
const char* mqttUser = "ivan";
const char* mqttPassword = "654321";

uint8_t my_str[6];


//WiFiServer server(80);


const char* ssid2 = "box_sd";
const char* pass2 = "123123";

// uint8_t status_gpio = 0;
const int saida = 15;//GPIO
const int led = 16;//GP16
const int rele=  5;//gpio05
String msg;
uint8_t ousaida,ouled;
// #include "FS.h"


ADC_MODE(ADC_TOUT);
uint16_t valor_lido = 0;

//udp----------------

WiFiUDP ntpUDP;

void enviaMsgUDP(String msg) {

  char resposta[msg.length()+1];
  msg.toCharArray(resposta, msg.length()+1);
  //strcpy(resposta, "abhigyan:LS1");
  // strcpy(resposta, msg);
  ntpUDP.beginPacket("192.168.2.255", 8766);
  ntpUDP.write(resposta);
  ntpUDP.endPacket();

}

//---fim udp---------------------

//prototipos privados 
void callback(char* topic, byte* payload, unsigned int length);
void clearstring();
void reconnect();
void reconnectWiFi();
// void printDirectory(File dir, int numTabs) ;

WiFiClient espClient;
PubSubClient clientMQT(mqttServer, mqttPort, callback, espClient);
int led_status=0;

File dataFile;            // Objeto responsável por escrever/Ler do cartão SD
uint32_t blocosporcluster,clustertamanho,totalclus;


ESP8266WebServer *server;


// Get MIME content type from file extension
String getContentType(String filename) {
    if (server->hasArg("download")) {
        return "application/octet-stream";
    } else if (filename.endsWith(".htm")) {
        return "text/html";
    } else if (filename.endsWith(".html")) {
        return "text/html";
    } else if (filename.endsWith(".css")) {
        return "text/css";
    } else if (filename.endsWith(".js")) {
        return "application/javascript";
    } else if (filename.endsWith(".png")) {
        return "image/png";
    } else if (filename.endsWith(".gif")) {
        return "image/gif";
    } else if (filename.endsWith(".jpg")) {
        return "image/jpeg";
    } else if (filename.endsWith(".ico")) {
        return "image/x-icon";
    } else if (filename.endsWith(".svg")) {
        return "image/svg+xml";
    } else if (filename.endsWith(".xml")) {
        return "text/xml";
    } else if (filename.endsWith(".pdf")) {
        return "application/x-pdf";
    } else if (filename.endsWith(".zip")) {
        return "application/x-zip";
    } else if (filename.endsWith(".gz")) {
        return "application/x-gzip";
    }
    return "text/plain";
}

// Check if a given file exists
bool exists(String path){
    bool yes = false;
    //File file = FILESYSTEM.open(path, "r");
    File file = SD.open(path, "r");
    if (!file.isDirectory()) {
        yes = true;
    }
    file.close();
    return yes;
}

// This method is invoked to handle GET requests on static files
bool handleFileRead(String path) {
    if (path.endsWith("/")) {
        path += "index.html";
    }
    String contentType = getContentType(path);
    if (exists(path)) {
        File file = SD.open(path, "r");
        server->streamFile(file, contentType);
        file.close();
        return true;
    }
    return false;
}

// This method is invoked to handle a reboot command
void handleReboot() {
    server->send(200, "text/html", "Restarting...");
    ESP.restart();
}

// This method is invoked to handle signal generator setup
void handleSetup() {

  
}

// This method is invoked to stop a signal generator
void handleStop() {
    String type;// = "square";
    String ativ;
    // ledc_channel_t pwm_channel = LEDC_CHANNEL_0;
    // dac_channel_t dac_channel = DAC_CHANNEL_1;
    bool highspeed = true;

    for (uint8_t i = 0; i < server->args(); i++) {
        if (server->argName(i) == "type") { type = server->arg(i); continue; }
        if (server->argName(i) == "ativ") { ativ = server->arg(i); continue; }

        // if (server->argName(i) == "pwm_channel") { pwm_channel = (ledc_channel_t)atoi(server->arg(i).c_str()); continue; }
        // if (server->argName(i) == "dac_channel") { dac_channel = (dac_channel_t)atoi(server->arg(i).c_str()); continue; }
        // if (server->argName(i) == "highspeed") { highspeed = (server->arg(i) == "true"); continue; }
        server->send(400, "text/html", "Invalid parameter name: " + server->argName(i)); return;
    }

     if (ativ == "stop")
     {

         server->send(200, "text/html", "Stopped successfully.");
     }

            
    server->send(400, "text/html", "Invalid type.");
}

// This method is invoked to get or set the configuration
void handleConfig() {
    switch (server->method())
    {
    case HTTP_GET:
        Serial.println("Getting configuration...");

        server->send(200, "text/html", "{\"ssid\":\"" + String(ssid) + "\"," + "\"local_ip\":\"" + "local_ip.toString() "+ "\"," + "\"gateway\":\"" + "gateway.toString()" + "\"," + "\"subnet\":\"" + "subnet.toString()" + "\"}");
        break;
    case HTTP_POST:
        Serial.println("Setting configuration...");

        for (uint8_t i = 0; i < server->args(); i++) {
            if (server->argName(i) == "ssid"){
                //ssid = server->arg(i);
                Serial.println("Setting ssid to '" + String(ssid) + "'");
                //settings->StoreString(SSID_SETTING, ssid);
            } else if (server->argName(i) == "password"){
                //password = server->arg(i);
                Serial.print("Setting password to '");
                #ifdef SHOW_PASSWORDS
                    Serial.print(password);
                #else
                    Serial.print("****");
                #endif
                Serial.println("'");
                //settings->StoreString(PASSWORD_SETTING, password);
            } else if (server->argName(i) == "local_ip"){
                //local_ip.fromString(server->arg(i));
                //Serial.println("Setting local_ip to '" + "local_ip.toString()" + "'");
                //settings->StoreIp(LOCAL_IP_SETTING, local_ip);
            } else if (server->argName(i) == "gateway"){
                //gateway.fromString(server->arg(i));
                //Serial.println("Setting gateway to '" +" gateway.toString()" + "'");
                //settings->StoreIp(GATEWAY_SETTING, gateway);
            } else if (server->argName(i) == "subnet"){
                //subnet.fromString(server->arg(i));
                //Serial.println("Setting subnet to '" + "subnet.toString()" + "'");
                //settings->StoreIp(SUBNET_SETTING, subnet);
            } else {
                server->send(400, "text/html", "Invalid parameter: " + server->argName(i));
            }
        }
        //settings->Commit();

        server->send(200, "text/html", "Configuration updated.");
        break;
    default:
        server->send(405, "text/html", "Invalid method.");
        break;
    }
}




// int vv_pos1;
// This method is invoked if the requested resource is not found
void HandleNotFound() {
    if (!handleFileRead(server->uri())) {
        String message = "File Not Found\n\n";
        message += "URI: ";
        message += server->uri();
        message += "\nMethod: ";
        message += (server->method() == HTTP_GET) ? "GET" : "POST";
        message += "\nArguments: ";
        message += server->args();
        message += "\n";

        for (uint8_t i = 0; i < server->args(); i++) {
            message += " " + server->argName(i) + ": " + server->arg(i) + "\n";
        }

        server->send(404, "text/plain", message);
    }
}

// Initialize the web server and set up the API endpoints
void SetupWebserver() {
    server = new ESP8266WebServer(80);
    server->on("/config", handleConfig);
    server->on("/setup", handleSetup);
    server->on("/stop", handleStop);
    server->on("/reboot", handleReboot);
    //server->on("/slider",handleSlider);
    //server->on("/slidecarro",handleSliderCarro);
    //server->on("/telemetria",handleTelemetria);
    server->onNotFound(HandleNotFound);
    server->begin();
}

// Load configuration from EEPROM
void loadSettings() {
  Serial.println("Loading Settings from ROM...");

   //settings->LoadString(SSID_SETTING, &ssid);
   Serial.print("ssid: '");
   Serial.print(ssid);
   Serial.println("'");

  // settings->LoadString(PASSWORD_SETTING, &password);
   Serial.print("password: '");
   #ifdef SHOW_PASSWORDS
     Serial.print(password);
   #else
     Serial.print("****");
   #endif
   Serial.println("'");

  //settings->LoadIp(LOCAL_IP_SETTING, &local_ip);
  Serial.print("local_ip: '");
  //Serial.print(local_ip.toString());
  Serial.println("'");

  //settings->LoadIp(GATEWAY_SETTING, &gateway);
  Serial.print("gateway: '");
  //Serial.print(gateway.toString());
  Serial.println("'");

  //settings->LoadIp(SUBNET_SETTING, &subnet);
  Serial.print("subnet: '");
  //Serial.print(subnet.toString());
  Serial.println("'");
}

// Reset configuration defaults and store them in EEPROM
void resetDefaults() {
    ssid = "";
    password = "";

}


const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 2, 112);
IPAddress apIP2(192, 168, 2, 3);
//DNSServer dnsServer;

void setup() {

  // put your setup code here, to run once:
  Serial.begin(115200,SERIAL_8N1,SERIAL_TX_ONLY);
  // Serial.println("conectado..a...");
       pinMode(saida, OUTPUT);//
       pinMode(rele, OUTPUT);//
       // PIN_PULLUP_EN(rele);

  pinMode(led, OUTPUT);//
digitalWrite(saida, LOW);//desliga bomba
//Serial.println("conectado2...");


   WiFi.softAP(ssid2, pass2, 6, false);


    //WiFiManager
    //wifiManager.autoConnect("poco");
WiFi.hostname("box_sd");
    WiFi.begin(ssid, password);
  Serial.println("MAC:");
Serial.println(WiFi.macAddress() );
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    led_status = !led_status; // this inverts button mode: If Mode was True - it will make it False and viseversa
   
  digitalWrite(led, led_status); 
  }
digitalWrite(led,1);//acende led

    //if you get here you have connected to the WiFi
    Serial.println("conectado...");
    Serial.println(WiFi.localIP());

 
 ESP.wdtDisable();

// ESP.wdtEnable(3000);//default é 10 segundos...

   // ----------- inicio OTA ----------------
   // Port defaults to 8266
   // ArduinoOTA.setPort(8266);

    ArduinoOTA.setHostname("box_sd");


   ArduinoOTA.onStart([]() {
     String type;
     if (ArduinoOTA.getCommand() == U_FLASH) {
       type = "sketch";
     } else { // U_SPIFFS
       type = "filesystem";
     }

     // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
     Serial.println("Start updating " + type);
   });
   ArduinoOTA.onEnd([]() {
     Serial.println("\nEnd");
   });
   ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
     Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
   });
   ArduinoOTA.onError([](ota_error_t error) {
     Serial.printf("Error[%u]: ", error);
     if (error == OTA_AUTH_ERROR) {
       Serial.println("Auth Failed");
     } else if (error == OTA_BEGIN_ERROR) {
       Serial.println("Begin Failed");
     } else if (error == OTA_CONNECT_ERROR) {
       Serial.println("Connect Failed");
     } else if (error == OTA_RECEIVE_ERROR) {
       Serial.println("Receive Failed");
     } else if (error == OTA_END_ERROR) {
       Serial.println("End Failed");
     }
   });
   ArduinoOTA.begin();
   Serial.println("Ready");
   Serial.print("IP address: ");
   Serial.println(WiFi.localIP());
   //---------fim OTA ----------------



clientMQT.setServer(mqttServer, mqttPort);
//client.setCallback(callback);


 if (clientMQT.connect("boxSd", mqttUser, mqttPassword)) {
    // client.publish("pocoOut/botao","0");
    
    // clientMQT.publish("boxSd/botao","1");
    clientMQT.subscribe("boxSd/saida");
    clientMQT.subscribe("boxSd/hb");
    clientMQT.subscribe("boxSd/led");

  }
clientMQT.publish("boxSd/hb","VIVO");


  Serial.print("\nInitializing SD card...");
  // verifica se o cartão SD está presente e se pode ser inicializado
  // if (!SD.begin(CS_PIN)) {
  if (!SD.begin(CS_PIN)) {
    Serial.println("Falha, verifique se o cartão está presente.");
    //programa encerrrado
    // return;
  }else{//cartao ok
 ftpSrv.begin("ivan","123456"); 
  dataFile = SD.open("/datalog.csv", FILE_WRITE); 
      // Serial.println("Cartão SD Inicializado para escrita :D ");
String leitura = "["+String(millis())+"] "+ESP.getResetInfo();    // Limpo campo contendo string que será armazenada em arquivo CSV
// leitura =  "teste1;teste2;teste3;";
    dataFile.println(leitura);  // Escrevemos no arquivos e pulamos uma linha
    dataFile.close();           // Fechamos o arquivo

  }



  blocosporcluster = SD.blocksPerCluster();    // clusters are collections of blocks
  // volumesize *= volume.clusterCount();      // we'll have a lot of clusters
  totalclus=SD.totalClusters();
  clustertamanho=SD.clusterSize(); 


    SetupWebserver();

}

 void reset_config(void) {
  //Reset das definicoes de rede

  delay(1500);
  ESP.reset();
}


int flg_sd_on_ultimo;
int envia_hb=0;
void loop() {

  ArduinoOTA.handle();

  ftpSrv.handleFTP();

// yield() ;
   ESP.wdtFeed();//limpa watch dog timer



if(envia_hb==1){

envia_hb=0;

StaticJsonDocument<300> doc;

  JsonArray saidab = doc.createNestedArray("saida");
  saidab.add(String(ousaida));
  

  JsonArray volumesizecx = doc.createNestedArray("blocosPorCluster");
  volumesizecx.add(String(blocosporcluster));

  JsonArray clustertamanhocx = doc.createNestedArray("clustertamanho");
  clustertamanhocx.add(String(clustertamanho));


  JsonArray totalcluscx = doc.createNestedArray("totalclusters");
  totalcluscx.add(String(totalclus));



  JsonArray sled = doc.createNestedArray("led");
  sled.add(String(ouled));
  
  
 char buffer[512];
//  char buffer[1024];
serializeJson(doc, buffer);
//  clientMQT.publish("boxSd/botao", buffer);
 clientMQT.publish("boxSd/hb", buffer);


 


envia_hb=0;
}

if (!clientMQT.connected())
{
  reconnect();
}

clientMQT.loop();


server->handleClient();


MDNS.update();
}


void callback(char* topic, byte* payload, unsigned int length) {
 


  for (int i = 0; i < length; i++) {

    my_str[i]=(char)payload[i];
  }
  String carga((char*)my_str);
  clearstring();


  String topico((char*)topic);
 enviaMsgUDP("topico=="+topico);
 enviaMsgUDP("carga=="+carga);
if(topico=="boxSd/saida"){
 if (carga=="1"){
   digitalWrite(saida,HIGH);

   ousaida=1;
 }else{
   digitalWrite(saida,LOW);

   ousaida=0;
 }
if (carga=="releon"){
 digitalWrite(rele,HIGH);
}
if (carga=="releoff"){
 digitalWrite(rele,LOW);
}
}


if(topico=="boxSd/led"){
 if (carga=="1"){
   digitalWrite(led,HIGH);
 }else{
   digitalWrite(led,LOW);
 }

}
 if(topico=="boxSd/hb"){
 
if(carga=="hb"){

envia_hb=1;
}
 }
 clearstring();
 
}
void clearstring(){

  for (int i = 0; i < 7; i++)
  {
    my_str[i]='\0';
  }


}

void reconnect(){
  
  while(!clientMQT.connected()){
    Serial.print("conectando MQTT...");
    
    String clientId="boxSd";
    clientId+=String(random(0xffff),HEX);
    if(clientMQT.connect(clientId.c_str())){
      Serial.print("conectado!");
     

    clientMQT.subscribe("boxSd/led");
    clientMQT.subscribe("boxSd/hb");
    clientMQT.subscribe("boxSd/botao");
    clientMQT.subscribe("boxSd/saida");
    
    }else{

      Serial.print("falhou codigo de erro= ");
      Serial.print(clientMQT.state());
      Serial.print("nova tentativa em 5 segundos");
      delay(1000);
      break;


    }

  }

}

