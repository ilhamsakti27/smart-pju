#include <SPI.h>
#include <Ethernet.h>      
#include <PubSubClient.h> 

const char* mqtt_server = "broker.hivemq.com"; // broker mqtt
const int mqtt_port =1883;                     // broker port

EthernetClient ethClient;
PubSubClient client(ethClient);
byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };

// definisi pin arduino
#define LED_PIN 6
int LDRSensor = 2;
#define currentLamp A0
#define voltageBattery A1
#define voltageSolar A2

// definisi nilai lain
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
#define LDR_MSG_BUFFER_SIZE (50)
#define VBATTERY_MSG_BUFFER_SIZE (50)
#define VSOLAR_MSG_BUFFER_SIZE (50)
#define ALAMP_MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
char ldrMsg[LDR_MSG_BUFFER_SIZE];
char batteryMsg[VBATTERY_MSG_BUFFER_SIZE];
char solarMsg[VSOLAR_MSG_BUFFER_SIZE];
char currentMsg[ALAMP_MSG_BUFFER_SIZE];
int value = 0;
 
float adc_voltage_battery = 0.0;
float in_voltage_battery = 0.0;
float R1 = 30000.0;
float R2 = 7500.0; 
float ref_voltage = 5.0;
int adc_value_battery = 0;

float adc_voltage_solar = 0.0;
float in_voltage_solar = 0.0;
int adc_value_solar = 0;

int nilaiadc_arus= 0;//nilai adc saat pembacaan arus yang menuju ke lampu dengan menggunakan sensor arus
int teganganoffset_arus = 2500; //nilai pembacaan offset saat tidak ada arus yang lewat
float tegangan_arus = 0.0;
float nilaiarus = 0.0;//nilai arus sesungguhnya pada lampu
int sensitivitas = 66; //tegantung sensor arus yang digunakan, yang ini 25 Ampere

// topic mqtt
const char* topic1 = "smart_pju_1";
const char* topic2 = "smart_pju_2";

// message subscribe topic
String topic1_input;
String var1, var2, var3, var4, var5;

//================================================ setup ================================================
void setup() {
  Serial.begin(9600);
//  pinMode(voltageBattery, INPUT);
  while (!Serial) delay(1);
  Ethernet.begin(mac);
  
  // Initialize pin
  pinMode(LED_PIN, OUTPUT);     
  pinMode(LDRSensor, INPUT);

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

    client.subscribe(topic2);   // subscribe the topic1
//    topic1_input = client.subscribe(topic2);   // subscribe the topic1
//    parseStringAndAssignValues(topic1_input, var1, var2, var3, var4, var5);

    
  } 
  else {
    Serial.print("\nfailed, rc=");
    Serial.println(client.state());
    }

}

void loop() {
  client.loop();
  
  // read LDR sensor
  int resultLDRsensor = digitalRead (LDRSensor);

  // read lamp current
  nilaiadc_arus = analogRead(A0);
  tegangan_arus = (nilaiadc_arus / 1024.0) * 5000;
  nilaiarus = ((tegangan_arus - teganganoffset_arus) / sensitivitas);
  char string_current_lamp[10];
  dtostrf(nilaiarus, 4, 2, string_current_lamp);

  // read battery voltage
  adc_value_battery = analogRead(voltageBattery);
  adc_voltage_battery = (adc_value_battery * ref_voltage) / 1024.0; 
  in_voltage_battery = (adc_voltage_battery / (R2/(R1+R2))) - 1; // RUMUS SEBENARNYA HARUSNYA GA ADA -1, DIHARUSKAN KRN ETHERNET SHIELD
  char string_voltage_battery[10];
  dtostrf(in_voltage_battery, 4, 2, string_voltage_battery);  // Konversi float ke string

  // read solar voltage
  adc_value_solar = analogRead(voltageSolar);
  adc_voltage_solar = (adc_value_solar * ref_voltage) / 1024.0; 
  in_voltage_solar = (adc_voltage_solar / (R2/(R1+R2))) - 1; 
  char string_voltage_solar[10];
  dtostrf(in_voltage_solar, 4, 2, string_voltage_solar);  

  // publish sensor values every 3 sec
  unsigned long now = millis();
  if (now - lastMsg > 3000) {

    // print results
    (resultLDRsensor) ? Serial.println("Darkness detected") : Serial.println("Light detected");
    
    Serial.print("Battery Voltage = ");
    Serial.println(string_voltage_battery);
    Serial.print("Solar Voltage = ");
    Serial.println(string_voltage_solar);
    Serial.print("Lamp Current = ");
    Serial.println(nilaiarus);

    // send sensor result to MQTT broker
    lastMsg = now;
    ++value;
//    snprintf (msg, MSG_BUFFER_SIZE, "request smart-pju-1 ke #%d", value); //perintah mempersiapkan data untuk dikirim ke mqtt broker
    snprintf (msg, MSG_BUFFER_SIZE, "request smart-pju-1 ke #%d", value); //perintah mempersiapkan data untuk dikirim ke mqtt broker
    snprintf (ldrMsg, LDR_MSG_BUFFER_SIZE, ";%d", resultLDRsensor); 
    snprintf (batteryMsg, VBATTERY_MSG_BUFFER_SIZE, ";%s", string_voltage_battery);
    snprintf (solarMsg, VSOLAR_MSG_BUFFER_SIZE, ";%s", string_voltage_solar);
    snprintf (currentMsg, ALAMP_MSG_BUFFER_SIZE, ";%s", string_current_lamp);

    // join the result
    // format msq: requestvalue; LDRsensor; lampCurrent; batteryVoltage; solarVoltage
    strcat(msg, ldrMsg);
    strcat(msg, batteryMsg);
    strcat(msg, solarMsg);
    strcat(msg, currentMsg);

    // publish message
//    Serial.print("Publish message: ");
//    Serial.println(msg);
//    client.publish(topic1, msg);    // comment dulu dek
  }
}

//======================================= This void is called every time we have a message from the broker =======================================
void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];
  
  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);
  
    // check the incomming message from topic 1
//    if( strcmp(topic,topic1) == 0){
//     if (incommingMessage.equals("1")) digitalWrite(LED_PIN, LOW);   // Turn the LED on 
//     else digitalWrite(LED_PIN, HIGH);  // Turn the LED off 
//  }
  
  parseStringAndAssignValues(payload, var1, var2, var3, var4, var5);
  Serial.print("var1 = ");
  Serial.println(var1);
  Serial.print("var2 = ");
  Serial.println(var2);
  Serial.print("var3 = ");
  Serial.println(var3);
  Serial.print("var4 = ");
  Serial.println(var4);
  Serial.print("var5 = ");
  Serial.println(var5);

  // bikin skenario

}

//======================================= publising as string =======================================
void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
}

// 
void parseStringAndAssignValues(String inputString, String& var1, String& var2, String& var3, String& var4, String& var5) {
  char *token;
  char *str;

  // Konversi String menjadi char array
  str = strdup(inputString.c_str());

  // Memisahkan setiap nilai berdasarkan tanda ";"
  token = strtok(str, ";");

  // Menyimpan setiap nilai ke dalam variabel yang sesuai
  if (token != NULL) {
    var1 = String(token);
    token = strtok(NULL, ";");
  }

  if (token != NULL) {
    var2 = String(token);
    token = strtok(NULL, ";");
  }

  if (token != NULL) {
    var3 = String(token);
    token = strtok(NULL, ";");
  }

  if (token != NULL) {
    var4 = String(token);
    token = strtok(NULL, ";");
  }

  if (token != NULL) {
    var5 = String(token);
  }
}
