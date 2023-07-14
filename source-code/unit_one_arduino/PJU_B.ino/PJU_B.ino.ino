#define BLYNK_TEMPLATE_ID "TMPL64OWjelMT"
#define BLYNK_TEMPLATE_NAME "Smart PJU"
#define BLYNK_AUTH_TOKEN "2yVLYFBTSftQmW16o9_EzGS8hk1NcprN"

///* Comment this out to disable prints and save space */
//#define BLYNK_PRINT Serial

#include <SPI.h>
#include <Ethernet.h>      
#include <PubSubClient.h> 
#include <Blynk.h>
#include <BlynkSimpleEthernet.h>

const char* mqtt_server = "test.mosquitto.org"; // broker mqtt
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
#define relay1 8 // Relay bantuan untuk BATTERY
#define relay2 7 // Relay bantuan untuk LAMPU
#define relay3 5 // relay untuk menghidupkan lampu saat gelap

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

int nilaiadc_arus= 0;                   // nilai adc saat pembacaan arus yang menuju ke lampu dengan menggunakan sensor arus
int teganganoffset_arus = 2500;         // nilai pembacaan offset saat tidak ada arus yang lewat
float tegangan_arus = 0.0;
float nilaiarus = 0.0;                  // nilai arus sesungguhnya pada lampu
int sensitivitas = 66;                  // tegantung sensor arus yang digunakan, yang ini 25 Ampere

int arus = 0;
int teganganBaterai = 0;
int teganganPanel = 0;

// topic mqtt
const char* topic1 = "smart_pju_1";
const char* topic2 = "smart_pju_2";
const char* topic3 = "smart_pju_3";

void setup() {
  Serial.begin(9600);
  
  while (!Serial) delay(1);
  Ethernet.begin(mac);

  // koneksi ke blynk
  Blynk.begin(BLYNK_AUTH_TOKEN);
  
  // Initialize pin
  pinMode(LED_PIN, OUTPUT);     
  pinMode(LDRSensor, INPUT);
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);
  pinMode(relay3, OUTPUT);

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

    client.subscribe(topic1);   // subscribe the topic1
    client.subscribe(topic3);

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
  nilaiarus = abs(((tegangan_arus - teganganoffset_arus) / sensitivitas));
  arus = int(nilaiarus);

  // read battery voltage
  adc_value_battery = analogRead(voltageBattery);
  adc_voltage_battery = (adc_value_battery * ref_voltage) / 1024.0; 
  in_voltage_battery = abs((adc_voltage_battery / (R2/(R1+R2)))); // RUMUS SEBENARNYA HARUSNYA GA ADA -1, DIHARUSKAN KRN ETHERNET SHIELD
  teganganBaterai = int(in_voltage_battery);

  // read solar voltage
  adc_value_solar = analogRead(voltageSolar);
  adc_voltage_solar = (adc_value_solar * ref_voltage) / 1024.0; 
  in_voltage_solar = abs((adc_voltage_solar / (R2/(R1+R2)))); 
  teganganPanel = int(in_voltage_solar);

  // print results
    if (resultLDRsensor) {
      Serial.println("Darkness detected");
      digitalWrite(relay3, LOW);              // RELAY 3 HIDUP
    }
    else {
      Serial.println("Light detected");
      digitalWrite(relay3, HIGH);             // RELAY 3 MATI
    }

  
  // publish sensor values every 3 sec
  unsigned long now = millis();
  if (now - lastMsg > 30000) {

    Serial.print("Battery Voltage = ");
    Serial.println(in_voltage_battery);
    Serial.print("Tegangan Baterai = ");
    Serial.println(teganganBaterai);
    Serial.print("Solar Voltage = ");
    Serial.println(in_voltage_solar);
    Serial.print("Tegangan Panel = ");
    Serial.println(teganganPanel);

    // send sensor result to MQTT broker
    lastMsg = now;
    ++value;

    snprintf (msg, MSG_BUFFER_SIZE, "%d;", teganganBaterai); //perintah mempersiapkan data untuk dikirim ke mqtt broker
    snprintf (solarMsg, VSOLAR_MSG_BUFFER_SIZE, "%d;", teganganPanel); 
      

    // join the result
    // format msq: batteryVoltage; solarVoltage;
    strcat(msg, solarMsg);
     
    // buka RELAY 2 jika tegangan baterai di bawah 11V
    if(in_voltage_battery < 11) { 
      digitalWrite(relay2, LOW);              // RELAY 2 HIDUP
      Serial.print("in_voltage_battery : ");
      Serial.println(in_voltage_battery);
    }
    else {
      Serial.println("Battery more than 11V");
      digitalWrite(relay2, HIGH);             // RELAY 2 MATI
    }
  
    // buka RELAY 1 jika penel rusak
    if(in_voltage_solar < 11) {
      digitalWrite(relay1, LOW);              // RELAY 1 HIDUP
      Serial.print("in_voltage_solar : ");
      Serial.println(in_voltage_solar);
    }
    else {
      Serial.println("Solar Panel more than 11V");
      digitalWrite(relay1, HIGH);             // RELAY 1 MATI
    }
      

    // publish message
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish(topic2, msg);   
  }

  // kirim data ke blynk
  // V0 = Arus Lampu
  // V1 = Tegangan Baterai
  // V2 = Tegangan Panel Surya
  // V3 = LDR Sensor atau Photoresistor

  Blynk.virtualWrite(V0,nilaiarus);
  Blynk.virtualWrite(V1,in_voltage_battery);
  Blynk.virtualWrite(V2,in_voltage_solar);
  Blynk.virtualWrite(V3,resultLDRsensor);
  Blynk.run();
}

void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  String var1, var2, var3, var4, var5;
  for (int i = 0; i < length; i++) {
    incommingMessage+=(char)payload[i];
  }
  
  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);
  
  
  Serial.print("mesg: ");
  Serial.println(incommingMessage);

  // parse String
  // Convert the String to a character array
  char charArray[incommingMessage.length() + 1];
  incommingMessage.toCharArray(charArray, sizeof(charArray));
  
  // Tokenize the string using strtok()
  char* token = strtok(charArray, ";");
  
  // Iterate through the tokens
  int count = 0;
  while (token != NULL && count < 3) {
    // Assign each token to the corresponding variable
    switch (count) {
      case 0:
        var1 = token;
        break;
      case 1:
        var2 = token;
        break;
    }

    // Find the next token
    token = strtok(NULL, ";");
    count++;
  }

  Serial.print("batteryVoltage = ");
  Serial.println(var1);
  Serial.print("solarVoltage = ");
  Serial.println(var2);

  // skenario jika terjadi event di topic2=smart_pju_2
  if(strcmp(topic,topic1) == 0){ 
    Serial.println("Ini topic 1");

    if((var1.toInt() < 11 || var2.toInt() < 11) && teganganBaterai > 11) {
        digitalWrite(relay1, LOW);    // RELAY 1 HIDUP
    } 
    else {
      digitalWrite(relay1, HIGH);     // RELAY 1 MATI
    }
  }

  // skenario jika terjadi event di topic3=smart_pju_3
  if(strcmp(topic,topic3) == 0){ 
    Serial.println("Ini topic 3");
    
    if((var1.toInt() < 11 || var2.toInt() < 11) && teganganPanel > 11) {
        digitalWrite(relay1, LOW);    // RELAY 1 HIDUP
    }
    else {
      digitalWrite(relay1, HIGH);     // RELAY 1 MATI
    }
  } 
  
}

void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
}
