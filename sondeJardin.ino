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
#define DEBUG
#define PROJECT "jardin"
#define BMP280_I2C_ADDRESS 0x76
#define TOPIC_BUFFER_SIZE (100)
#define PAYLOAD_BUFFER_SIZE (100)
#define SLEEP 30e6
#define NBLOOPCONNECTWIFI 30
#define WILLTOPIC "status/"
int sleeping = 1; // 0 no deepsleep 

// init Debug telnet
RemoteDebug Debug;
// Set web server port number to 80
ESP8266WebServer server(80);

// init wifi
WiFiClient espClient;
PubSubClient client(espClient);
// PubSubClient client();
//variable de mqtt
unsigned long lastMsg = 0;
char topic[TOPIC_BUFFER_SIZE];
char payload[PAYLOAD_BUFFER_SIZE];
int value = 0;

void publishModel(PubSubClient &client, char *name, char *mach, int value)
{
  snprintf(topic, TOPIC_BUFFER_SIZE, "%s/%d/%s/%s", PROJECT, random(1, 20), mach, name);
  snprintf(payload, PAYLOAD_BUFFER_SIZE, "%d", value);
  client.publish(topic, payload);
}

void publishModel(PubSubClient &client, char *name, char *mach, float value)
{
  snprintf(topic, TOPIC_BUFFER_SIZE, "%s/%d/%s/%s", PROJECT, random(1, 20), mach, name);
  snprintf(payload, PAYLOAD_BUFFER_SIZE, "%lf", value);
  client.publish(topic, payload);
}

// convertisseur string char
char *stringToChar(String str)
{
  char tmp[str.length() + 1];
  for (int i = 0; i < str.length(); i++)
  {
    tmp[i] = str[i];
  }
  tmp[str.length()] = '\0';
  return tmp;
}

// fonction pour mettre en pause l'ESP
void espSleeping()
{
  if (sleeping == 1)
  {
    delay(5000);
    ESP.deepSleep(SLEEP);
  }
  
}

void setup_wifi()
{
  delay(10);
  // We start by connecting to a WiFi network
  #ifdef DEBUG
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  #endif

  WiFi.begin(ssid, password);
  int cpt = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    if (cpt > NBLOOPCONNECTWIFI)
    {
      espSleeping();
    }
    delay(500);
    #ifdef DEBUG
    Serial.print(".");
    #endif
    cpt++;
  }

  randomSeed(micros());
  #ifdef DEBUG
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("adresse Mac");
  Serial.println(WiFi.macAddress());
  #endif
}

void callback(char *topic, byte *payload, unsigned int length)
{
  // topic upload
    if (strcmp(topic, "upload") == 0 && payload[0] == '1'){
        unsigned int delaiBeforeLoopEnd = 60000;
        #ifdef DEBUG
        Serial.println("je vais télévérsé");
        #endif
        unsigned long now = millis();
        if (now - lastMsg > delaiBeforeLoopEnd)
        {
          lastMsg = now;
          ArduinoOTA.handle();
          #ifdef DEBUG
          Serial.println("on attend");
        #endif
        }
        #ifdef DEBUG
        Serial.println("j'ai reçu le upload");
        #endif
    }else{
        #ifdef DEBUG
        Serial.println("inverse!!!!!");
        #endif
    }

    // topic sleeping
    if (strcmp(topic, "sleeping") == 0 && payload[0] == '1'){
        sleeping = 1;
    }else{
        sleeping = 0;
    }
}

void reconnect()
{
  int checkNum = 0;
  // Loop until we're reconnected
  while (!client.connected())
  {
    #ifdef DEBUG
    Serial.print("Attempting MQTT connection...");
        #endif

    // Attempt to connect
    if (client.connect("monesp8266","login","pwd"))
    {
      #ifdef DEBUG
      Serial.println("connected");
        #endif
    }
    else
    {
      if (checkNum > 5)
      {
        espSleeping();
      }
      #ifdef DEBUG
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
        #endif
      // Wait 5 seconds before retrying
      checkNum++;
      delay(5000);
    }
  }
}

void setup()
{
  pinMode(BUILTIN_LED, OUTPUT); // Initialize the BUILTIN_LED pin as an output
  Serial.begin(115200);
  setup_wifi();
  // client.setClient(espClient);
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // initialisation de la librairie de debug
  Debug.begin("monEsp");
  ArduinoOTA.setHostname("test_sonde_jardin"); // on donne une petit nom a notre module
  ArduinoOTA.begin();                          // initialisation de l'OTA
  // initialisation du serveur
  server.on("/", []() {
    // a chaque requete recue, on envoie un message de debug
    Debug.println("request received");
    server.send(200, "text/plain", "ok :)");
  });
  server.begin();
}

void loop()
{

  Debug.handle();
  char mach[25];
  int line = random(1, 20);
  strcpy(mach, stringToChar(WiFi.macAddress()));

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
  Adafruit_BMP280 bmp280;

  bmp280.begin(BMP280_I2C_ADDRESS);
  float temp = bmp280.readTemperature(); // get temperature
  float pres = bmp280.readPressure();    // get pressure
  

  server.handleClient();
  
  // on souscrit à tous les topics
  // TODO voir si ça n'encombre pas trop l'ESP
  client.subscribe("#");
  // test de flag

  
  // list des publish
  publishModel(client, "temp", mach, temp);
  publishModel(client, "hydroTerre", mach,analogRead(A0) );
  publishModel(client, "presAtmo", mach, pres);
  client.subscribe("upload",1);
  espSleeping();
  

  ArduinoOTA.handle();
}
