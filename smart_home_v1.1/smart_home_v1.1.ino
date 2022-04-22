#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h> 
#include <DHT.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "AdafruitIO_WiFi.h"

#include "config.h"

//------------- pin setup starts ----------
int alarm = 5;
int fan = 2;
const int trigPin = 14;
const int echoPin = 12;

#define DHTPIN 4
#define DHTTYPE DHT11
//------------- pin setup ends ----------

//------------- Adafruit IO configuration starts ----------
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);

// set up the 'amarm_status' feed
AdafruitIO_Feed *alarm_status = io.feed("alarm_status");
AdafruitIO_Feed *temperature = io.feed("temperature");
AdafruitIO_Feed *humidity = io.feed("humidity");
AdafruitIO_Feed *fan_status = io.feed("fan_status");
//------------- Adafruit IO configuration ends ----------

//------------- DHT11 Sensors Setup Starts ----------
DHT dht(DHTPIN, DHTTYPE);

float temp = 0.0;
float hum = 0.0;
long duration, distance;

//------------- DHT11 Sensors Setup Ends ----------

//------------- global variables starts ----------
int current_alarm_stat;
int prev_alarm_stat;
int current_fan_stat;
int prev_fan_stat;
unsigned long lastTime = 0;
unsigned long timerDelay = 60000;
//------------- global variables ends ------------

void setup() { 
  Serial.begin(9600);
  dht.begin();
  pinMode(alarm, OUTPUT);
  pinMode(fan, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  alarm_status->get();
  fan_status->get();
  
  connectWifi();
  
  // wait for serial monitor to open
  while(! Serial);

  connectAio();
  
  // set up a message handler for the count feed.
  // the handleMessage function (defined below)
  // will be called whenever a message is
  // received from adafruit io.
  alarm_status->onMessage(handleMessage);
  fan_status->onMessage(handleFanMessage);
  
}

void loop() {
  io.run();
  
  float h = analogRead(A0);
  float h_percent = h/1023*100;
  Serial.println("Gas Raw Value: ");
  Serial.print(h);
  Serial.println("Gas Value in Percentage: ");
  Serial.print(h_percent);

  if(h_percent >= 31){
    current_alarm_stat = 1;
    Serial.println("HIG");
  }else{
    current_alarm_stat = 0;
    Serial.println("Alarm Off");
  }

  // save alarm_stat to the 'alarm_status' feed on Adafruit IO
  if(current_alarm_stat != prev_alarm_stat){
    Serial.println("sending -> ");
    Serial.print(current_alarm_stat);
    alarm_status->save(current_alarm_stat);
    prev_alarm_stat = current_alarm_stat;
  }

  //calculte the distance
  detectObstacle();

  //read temperature and relative humidity every 5seconds 
  if((millis() - lastTime) > timerDelay){
    //Read temperature
    float newT = dht.readTemperature();
    
    //If temperature read failed, don't change t value
    if(isnan(newT)){
      Serial.print("Failed to read from DHT sensor!");
    }else{
      if(temp != newT){
        temp = newT;
        Serial.println("Temperature: ");
        Serial.print(temp);

        //sending tmperature to adafruit dashboard
        Serial.println("sending -> ");
        Serial.print(temp);
        temperature->save(temp);
      }else{
        Serial.println("temerature not changed");
      }
      
    }

    //Read Humidity
    float newH = dht.readHumidity();
    //if humidity read failed, don't change h value
    if (isnan(newH)){
      Serial.println("Failed to read from DHT sensor!");
    }else{
      if(hum != newH){
        hum = newH;
        Serial.println("humidity: ");
        Serial.print(hum);

        //sending tmperature to adafruit dashboard
        Serial.println("sending -> ");
        Serial.print(hum);
        humidity->save(hum);
      }else{
        Serial.println("temerature not changed");
      }
      
    }

    lastTime = millis();
  }

  //trigger fan based on temperature
  if(temp >= 29){
    current_fan_stat = 1;
    Serial.println("Temperature below >= 29");
  }else{
    current_fan_stat = 0;
    Serial.println("Temperature < 29");
  }

  // save current_fan_stat to the 'fan_status' feed on Adafruit IO
  if(current_fan_stat != prev_fan_stat){
    Serial.println("sending -> ");
    Serial.print(current_fan_stat);
    fan_status->save(current_fan_stat);
    prev_fan_stat = current_fan_stat;
  }
  
  delay(1000);
}

void handleMessage(AdafruitIO_Data *data) {

  Serial.println("received <- ");
  Serial.print(data->value());
  Serial.println("\n");

  if(data->toPinLevel() == HIGH){
    Serial.println("Alarm On");
    digitalWrite(alarm, HIGH);
    current_alarm_stat = 1;
    Serial.println("Gas Alarm Status: ");
    Serial.print(current_alarm_stat);
    Serial.println("");
  }else{
    Serial.println("Alarm Off");
    digitalWrite(alarm, LOW);
    current_alarm_stat = 0;
    Serial.println("Gas Alarm Status: "); 
    Serial.print(current_alarm_stat);
    Serial.println("");
  }
  
}

void handleFanMessage(AdafruitIO_Data *data) {

  Serial.println("received <- ");
  Serial.print(data->value());
  Serial.println("\n");

  if(data->toPinLevel() == HIGH){
    Serial.println("FAN IS ON");
    digitalWrite(fan, HIGH);
    current_fan_stat = 1;
    Serial.println("");
  }else{
    Serial.println("FAN IS OFF");
    digitalWrite(fan, LOW);
    current_fan_stat = 0;
    Serial.println("");
  }

  Serial.println("Fan Status: ");
  Serial.print(current_fan_stat);
}

void connectAio(){
  Serial.print("Connecting to Adafruit IO");

  // connect to io.adafruit.com
  io.connect();
  
  // wait for a connection
  while(io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
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

void detectObstacle(){

  // The sensor is triggered by a HIGH pulse of 10 or more microseconds.
  // Give a short LOW pulse beforehand to ensure a clean HIGH pulse:
  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  // Read the signal from the sensor: a HIGH pulse whose
  // duration is the time (in microseconds) from the sending
  // of the ping to the reception of its echo off of an object.
  duration = pulseIn(echoPin, HIGH);

  // Convert the time into a distance
  distance = (duration/2) * 0.034;
  Serial.println("Distance: ");
  Serial.print(distance);
  Serial.print(" cm\n");
}
