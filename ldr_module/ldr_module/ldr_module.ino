// Declare your LDR sensor out pin connected Arduino pin “D2” 
 int LDRSensor = 2;

void setup()
 {
  //Initialize Sensor (pin3) as an INPUT.
  pinMode (LDRSensor, INPUT);
  //Define baud rate for serial communication
  Serial.begin (9600);
 }
 
void loop()
 {
  //Read Digital output value from sensor using digitalRead()function
  int Sensordata = digitalRead (LDRSensor);
  //Print the sensor value on your serial monitor window
  Serial.print("Sensor value:");
  Serial.println(Sensordata);
//  Serial.print("Sensor value: %d", Sensordata);

  // Output LOW (0)= detect light
  // Output HIGH (1) = detect   
  //Delay for 1 second to get clear output on the serial monitor
  delay(1000);
 }
