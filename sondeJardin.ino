#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
//OTA
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#include <RemoteDebug.h>

//constante
#include "constante.h"

// define 
#define PROJECT "jardin"
#define BMP280_I2C_ADDRESS  0x76
#define TOPIC_BUFFER_SIZE	(100)
#define PAYLOAD_BUFFER_SIZE  (100)
#define SLEEP 30e6
int sleeping = 0; // 0 no deepsleep 1 delay without delay

// init Debug telnet
RemoteDebug Debug;
// Set web server port number to 80
ESP8266WebServer server(80);

// init wifi
WiFiClient espClient;
PubSubClient client(espClient);

//variable de mqtt
unsigned long lastMsg = 0;
char topic[TOPIC_BUFFER_SIZE];
char payload[PAYLOAD_BUFFER_SIZE];
int value = 0;

void publishModel(PubSubClient client, char* name, int value){
  snprintf (topic, TOPIC_BUFFER_SIZE,"%s/%d/%s/%s",PROJECT,line,mach,name); 
  snprintf (payload, PAYLOAD_BUFFER_SIZE,"%d", value);
  client.publish(topic,payload);
}

void publishModel(PubSubClient client, char* name, float value){
  snprintf (topic, TOPIC_BUFFER_SIZE,"%s/%d/%s/%s",PROJECT,line,mach,name); 
  snprintf (payload, PAYLOAD_BUFFER_SIZE,"%lf", value);
  client.publish(topic,payload);
}



// convertisseur string char
char * stringToChar(String str) {
  char tmp[str.length()+1];
  for (int i =0; i< str.length(); i++){
    tmp[i] = str[i];
  }
  tmp[str.length()] ='\0';
  return tmp;
}

// fonction pour mettre en pause lESP
void espSleeping(){
  if (sleeping == 0){
    delay(5000);
    ESP.deepSleep(SLEEP);
  }
  // else
  // {
  //   Serial.println("je ne dors pas!!");
  // }
  
}

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);
  int cpt= 0;
  while (WiFi.status() != WL_CONNECTED) {
    if(cpt > 25){espSleeping();}
    delay(500);
    Serial.print(".");
    cpt++;
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("adresse Mac");
  Serial.println(WiFi.macAddress());
  
}

void callback(char* topic, byte* payload, unsigned int length) {

    Serial.print("payload : ");
    Serial.println(payload[0]);

   if(strcmp(topic,"upload") == 0 && payload[0] == '1'){
        Serial.println("je vais télévérsé");

   }
   else{
     Serial.println("inverse!!!!!");
   }

   if(strcmp(topic,"sleeping") == 0 && payload[0] == '1'){
      sleeping = 1;

   }
   else{
      sleeping =0 ;
   }
  
  
}

void reconnect() {
  int checkNum = 0;
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Attempt to connect
    if (client.connect("monesp8266")) {
      Serial.println("connected");
      
     
    } else {
      if(checkNum > 5){
        espSleeping();
      }
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      checkNum++;
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);


  // initialisation de la librairie de debug
  Debug.begin("monEsp");  
  ArduinoOTA.setHostname("test_sonde_jardin"); // on donne une petit nom a notre module
  ArduinoOTA.begin(); // initialisation de l'OTA
  // initialisation du serveur
  server.on("/", [](){
  // a chaque requete recue, on envoie un message de debug
    Debug.println("request received");
    server.send(200, "text/plain", "ok :)");
  });
  server.begin();

}

void loop() {
     
  Debug.handle();
  char mach[25];
  int line = random(1,20);
  strcpy(mach,stringToChar(WiFi.macAddress()));
   
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  Adafruit_BMP280  bmp280;
  
  bmp280.begin(BMP280_I2C_ADDRESS);
  float temp = bmp280.readTemperature();   // get temperature
  float pres = bmp280.readPressure();      // get pressure
  //float valut = 1003.5;
  
  server.handleClient();
  snprintf (topic, TOPIC_BUFFER_SIZE,"%s/%d/%s/temp",PROJECT,line,mach); 
  snprintf (payload, PAYLOAD_BUFFER_SIZE,"temp,%lf,pres,%lf,hydro,%d", temp,pres,analogRead(A0));
  



  // liste des souscriptions
  client.subscribe("upload");
  client.subscribe("sleeping");

      if(sleeping == 1){
          unsigned long now = millis();
        if (now - lastMsg > 2000)
        {
          lastMsg = now;
        client.publish(topic, payload);
        Serial.println("on ecrit");
        }
      }
      else{
        client.publish(topic, payload);
        espSleeping();
      }

    ArduinoOTA.handle();
}
