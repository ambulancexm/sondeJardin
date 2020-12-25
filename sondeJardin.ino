

// sonde température
#include <DallasTemperature.h>
#include <OneWire.h>
#define ONE_WIRE_BUS D2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

#include <ESP8266WiFi.h>

#include <PubSubClient.h>



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
#define DEBUG 1
#define PROJECT "campingCar"
#define BMP280_I2C_ADDRESS 0x76 // valeur de l'adress I2C du capteur
#define TOPIC_BUFFER_SIZE (100)
#define PAYLOAD_BUFFER_SIZE (100)
#define SLEEP 0.5 // Temps de sommeil en minute
#define NBLOOPCONNECTWIFI 30
#define WILLTOPIC "status/"
#define LINE 1

/**
 * minute est un long qui attend un nomde de minute
 * si il est inferieur ou egal à 0 la valeur par defaut
 * est de 30 minnutes
 */
unsigned long minuteToSecond(long minute);

/**
 * generateur de fake line
 */
int getLine();

/**
 * creation d'un publish 
 * construction du topic avec le modèle :
 * projet,ligne,adresseMac,nom du topic
 * creation de la payload
 * value en int
 * 
 */
void publishModel(PubSubClient &client, char *name, char *mach, int value);
void publishModel(PubSubClient &client, char *name, char *mach, float value);

/**
 * convertisseur string char
 */
char *stringToChar(String str);

unsigned long minuteToSecond(long minute){
  if(minute > 0){
    return (minute * 6e7);

  }else
    return 1.8e9;
}


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

int getLine(){
  if(LINE == 0){
    return random(-20, -1);
  }
  return LINE;
}

void publishModel(PubSubClient &client, char *name, char *mach, int value){  
  snprintf(topic, TOPIC_BUFFER_SIZE, "%s/%d/%s/%s", PROJECT, getLine(), mach, name);
  snprintf(payload, PAYLOAD_BUFFER_SIZE, "%d", value);
  client.publish(topic, payload);
}

void publishModel(PubSubClient &client, char *name, char *mach, float value){
  snprintf(topic, TOPIC_BUFFER_SIZE, "%s/%d/%s/%s", PROJECT, getLine(), mach, name);
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
    // ESP.deepSleep(minuteToSecond(SLEEP));
    ESP.deepSleep(6e7);
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
    while (strcmp(topic, "retour/nodemcu") == 1 ){
        unsigned int delaiBeforeLoopEnd = 60000;
        unsigned long now = millis();
        if (now - lastMsg > delaiBeforeLoopEnd)
        {
          lastMsg = now;
          ArduinoOTA.handle();
          if (lastMsg%5 == 0){
            Serial.println("on attend");
          }
        }
        
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
     
      Serial.println("connected");
      client.publish("request","update");
      
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
  client.setServer(mqtt_server, mqtt_port);
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
  sensors.begin();
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
  
  sensors.requestTemperatures();
  


  server.handleClient();
  
  // on souscrit à tous les topics
  // TODO voir si ça n'encombre pas trop l'ESP
  // client.subscribe("#");
  // test de flag
  int tempTest = random(-10,35);
  
  // list des publish
  publishModel(client, "sensor/campingcar/temp/interieur", mach,sensors.getTempCByIndex(0));
//  publishModel(client, "hydroTerre", mach,analogRead(A0) );
//  publishModel(client, "presAtmo", mach, pres);
    //publishModel(client, "fakeIot",mach , tempTest);
  //client.subscribe("upload",1);
  espSleeping();
  

  ArduinoOTA.handle();
}
