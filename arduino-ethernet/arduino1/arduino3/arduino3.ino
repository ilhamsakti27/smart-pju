/* SUUUUUUUUUUUUUUUU
 * Virtuino IoT getting started example for ethernet connection
 * Broker: HiveMQ (Public connection)
 * Supported boards: Arduino and similar
 * Created by Ilias Lamprou
 */


#include <SPI.h>

#include <Ethernet.h>      // for the module ENC28J60 disable this line
//#include <UIPEthernet.h> // for the module ENC28J60 enable this line. You have to install the library UIPEthernet first

#include <PubSubClient.h>  // The MQTT library

const char* mqtt_server = "broker.hivemq.com"; // replace with your broker url
const int mqtt_port =1883; // unsecure port
// you don't need the username and password for public connection


EthernetClient ethClient;
PubSubClient client(ethClient);
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
#define LED_PIN 6
unsigned long lastMsg = 0;

int sensor1 = 0;   // temperature value
float sensor2 = 0;  // humidity
int command1 =0;    // led 

const char* sensor1_topic= "username_temperature";  // replace the prefix "username"
const char*  sensor2_topic="username_humidity";     // add a unique prefix instead of "username"
const char* command1_topic="username_led";
const char* topic1 = "smart_pju_1";
const char* topic2 = "smart_pju_2";


//================================================ setup
//================================================
void setup() {
  Serial.begin(9600);
  while (!Serial) delay(1);
  Ethernet.begin(mac);
  pinMode(LED_PIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output

  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());
  //--- init mqtt client
  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);
 // client.setBufferSize(255);  // default 128
  randomSeed(micros());
  String clientId = "EthernetClient-";   // Create a random client ID
  clientId += String(random(0xffff), HEX);
  if (client.connect(clientId.c_str())) {
    Serial.println("connected");

    client.subscribe(topic1);   // subscribe the topics here
    //client.subscribe(command2_topic);   
  } 
  else {
    Serial.print("\nfailed, rc=");
    Serial.println(client.state());
    }

}


//================================================ loop
//================================================
void loop() {
  client.loop();

  //---- example: how to publish sensor values every 5 sec
  unsigned long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    sensor1= random(50);       // replace the random value with your sensor value
    sensor2= 20+random(80);    // replace the random value  with your sensor value
    publishMessage(topic1,String(sensor1),true);  // publish the sensor1 value to the broker  
//    publishMessage(sensor2_topic,String(sensor2),true);
    
  }
}

//=======================================  
// This void is called every time we have a message from the broker

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];
  
  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);
  
  //--- check the incomming message
    if( strcmp(topic,command1_topic) == 0){
     if (incommingMessage.equals("1")) digitalWrite(LED_PIN, LOW);   // Turn the LED on 
     else digitalWrite(LED_PIN, HIGH);  // Turn the LED off 
  }
 //  check for other commands
 //  else  if( strcmp(topic,command2_topic) == 0){
 //    if (incommingMessage.equals("1")) {  } // do something else
 // }
}



//======================================= publising as string
void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
}
