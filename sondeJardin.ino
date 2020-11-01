#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>

// Update these with values suitable for your network.
#define PROJECT "jardin"


const char* ssid = "thomas";
const char* password = "tiliatilia";
const char* mqtt_server = "176.166.1.64";

#define BMP280_I2C_ADDRESS  0x76

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define TOPIC_BUFFER_SIZE	(100)
#define PAYLOAD_BUFFER_SIZE  (100)
char topic[TOPIC_BUFFER_SIZE];
char payload[PAYLOAD_BUFFER_SIZE];
int value = 0;


// convertisseur string char
char * stringToChar(String str) {
  char tmp[str.length()+1];
  for (int i =0; i< str.length(); i++){
    tmp[i] = str[i];
  }
  tmp[str.length()] ='\0';
  return tmp;
}

void espSleeping(){
  delay(500);
  ESP.deepSleep(5e6);
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

}

void loop() {
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
  
  snprintf (topic, TOPIC_BUFFER_SIZE,"%s/%d/%s/",PROJECT,line,mach); 
  snprintf (payload, PAYLOAD_BUFFER_SIZE,"temp,%lf,pres,%lf,hydro,%d", temp,pres,analogRead(A0));
    client.publish(topic, payload);

    espSleeping();
    
}
