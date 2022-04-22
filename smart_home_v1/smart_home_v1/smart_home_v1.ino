#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h> 
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "AdafruitIO_WiFi.h"

#include "config.h"

//------------- pin setup starts ----------
#define SS_PIN 4
#define RST_PIN 5
#define ldr_pin A0
int led_pin = 2;

//------------- pin setup ends ----------

//------------- Adafruit IO configuration starts ----------
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

// set up the 'door_status' feed
AdafruitIO_Feed *door_status = io.feed("door_status");
AdafruitIO_Feed *light_status = io.feed("light_status");
//------------- Adafruit IO configuration ends ----------

//------------- rfid paramters settings starts ----------
MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class
MFRC522::MIFARE_Key key; //using the scope resolution operator :: to access MIFARE_Key variable of the MFRC522

// Init array that will store new NUID 
byte nuidPICC[4];

String NUID_dec, NUID_hex;
String RKey;
//------------- rfid paramters settings ends ----------

//------------- global variables starts ----------
Servo servo;
int counter;
int door_stat;
int light_intensity = 0;
int current_light_status;
int prev_light_status;
//------------- global variables ends ------------

void setup() { 
  Serial.begin(9600);
  pinMode(led_pin, OUTPUT);

  door_status->get();
  light_status->get();

  initializeRfid();
  
  servo.attach(15);

  connectWifi();
  
  // wait for serial monitor to open
  while(! Serial);

  connectAio();
  
  // set up a message handler for the count feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  door_status->onMessage(handleMessage);
  light_status->onMessage(handleLightMessage);
}


void loop() {
  io.run();

  readLdrValue();

  trigger_light();

  publishLightStatus();
  
  // Reset the loop if no new card present on the sensor/reader.
  if ( ! rfid.PICC_IsNewCardPresent()){
    return;
  }

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial()){
    return;
  }

  //for getting the nuid in its raw format.
  for (byte i = 0; i < 4; i++) {
    nuidPICC[i] = rfid.uid.uidByte[i];
  }

  displayCardUid();
  
  if (compareNUID(nuidPICC, 123, 216, 74, 59) || compareNUID(nuidPICC, 25, 152, 153, 193)){
    openDoor();

    //call count-down here
    countDown();

    lookDoor();
  
  }else{
    Serial.println("Access Denied");  
    delay(1000);
  }

  Serial.println();

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();
}


//method for comparing the NUID with the NUID from the card. 
boolean compareNUID(byte x[4], byte x0, byte x1, byte x2, byte x3)
{
  if (x[0] == x0 && x[1] == x1 && x[2] == x2 && x[3] == x3){
    return true; 
  }else{
    return false; 
  }

}

void countDown(){
  for(counter = 10; counter > 0; counter--){
    Serial.println(counter);
    
    delay(1000);
  }
}


void lookDoor(){
  servo.write(0);
  Serial.println("Door Locked");
  door_stat = 0;
  Serial.println("Door Status: "); 
  Serial.print(door_stat);
  Serial.println("");

  // save door_stat to the 'door_status' feed on Adafruit IO
  Serial.print("sending -> ");
  Serial.println(door_stat);
  door_status->save(door_stat);
  
  delay(1000);
}

void openDoor(){
  servo.write(270);
  door_stat = 1;
  Serial.println("Access Granted, WELCOME!");
  Serial.println("Door Status: ");
  Serial.print(door_stat);
  Serial.println("");

  // save door_stat to the 'door_status' feed on Adafruit IO
  Serial.print("sending -> ");
  Serial.println(door_stat);
  door_status->save(door_stat);
  
  delay(1000);
}

void handleMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());

  if(data->toPinLevel() == HIGH){
    Serial.println("HIGH");
    servo.write(270);
    door_stat = 1;
    Serial.println("Access Granted, WELCOME!");
    Serial.println("Door Status: ");
    Serial.print(door_stat);
    Serial.println("");
  }else{
    Serial.println("LOW");
    servo.write(0);
    Serial.println("Door Locked");
    door_stat = 0;
    Serial.println("Door Locked");
    Serial.println("Door Status: "); 
    Serial.print(door_stat);
    Serial.println("");
  }
  
//  digitalWrite(led_pin, data->toPinLevel());
}

void handleLightMessage(AdafruitIO_Data *data) {

  Serial.print("received <- ");
  Serial.println(data->value());

  if(data->toPinLevel() == HIGH){
    Serial.println("HIGH");
    digitalWrite(led_pin, HIGH);
    current_light_status = 1;
  }else{
    Serial.println("LOW");
    digitalWrite(led_pin, LOW);
    current_light_status = 0;
  }

  Serial.println("Light Status: ");
  Serial.print(door_stat);
  Serial.println("");

}

void connectAio(){
  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();

  
  // wait for an MQTT connection
  // NOTE: when blending the HTTP and MQTT API, always use the mqttStatus
  // method to check on MQTT connection status specifically
//  while(io.mqttStatus() < AIO_CONNECTED) {
//    Serial.print(".");
//    delay(500);
//  }
  
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}


void initializeRfid(){
  SPI.begin(); // Init SPI bus(for rfid protocol)
  rfid.PCD_Init(); // Init MFRC522 
  
  //Define Key
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  RKey = String(key.keyByte[0], HEX) + " "
           +String(key.keyByte[1], HEX) + " "
           +String(key.keyByte[2], HEX) + " " 
           +String(key.keyByte[3], HEX) + " "
           +String(key.keyByte[4], HEX) + " "
           +String(key.keyByte[5], HEX);
  RKey.toUpperCase();
  Serial.print(F("MFRC522 Key: "));
  Serial.println(RKey);
}

void connectWifi(){
  //Wifi Connection
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.println("Connecting");
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
}

void displayCardUid(){
  //displaying the PICC(Periferal Integrated Circuit Card) type
  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak); //get PICC type and store in a variable
  Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  //displaying the Non-Unique Identity in decimal
  NUID_dec = String(rfid.uid.uidByte[0])+ " " 
           +String(rfid.uid.uidByte[1]) + " "
           +String(rfid.uid.uidByte[2]) + " "
           +String(rfid.uid.uidByte[3]);
  Serial.print("NUID Tag (DEC): "); 
  Serial.println(NUID_dec); 

  //displaying the Non-Unique Identity in dexi-decimal
  NUID_hex = String(rfid.uid.uidByte[0], HEX) + " "
           +String(rfid.uid.uidByte[1], HEX) + " "
           +String(rfid.uid.uidByte[2], HEX) + " " 
           +String(rfid.uid.uidByte[3], HEX);
  NUID_hex.toUpperCase();
  Serial.print("NUID Tag (HEX): "); 
  Serial.println(NUID_hex);
}

void readLdrValue(){
  light_intensity = analogRead(ldr_pin);
  Serial.println("LDR Value: ");
  Serial.print(light_intensity);
  delay(1000);
}

void trigger_light(){
  if(light_intensity >= 500){
    current_light_status = 1;
//    digitalWrite(led_pin, HIGH);
//    Serial.println("\nLight On");
//    Serial.println("\nCurrent Light Status: ");
//    Serial.println(current_light_status);
  }else{
    current_light_status = 0;
//    digitalWrite(led_pin, LOW);
//    Serial.println("\nLight Off");
//    Serial.println("\nCurrent Light Status: ");
//    Serial.println(current_light_status);
  }
}

void publishLightStatus(){
  if(current_light_status != prev_light_status){
    // save door_stat to the 'door_status' feed on Adafruit IO
    Serial.print("sending -> ");
    Serial.println(current_light_status);
    light_status->save(current_light_status);
    prev_light_status = current_light_status;
  }
  
}
