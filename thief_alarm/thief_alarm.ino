#include <LiquidCrystal.h>

const int trigPin = 10;
const int echoPin = 9;
const int alarmBuzzer = 6;
const int alarmLed = 7;

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); 

long duration, distance;

void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(alarmBuzzer, OUTPUT);
  pinMode(alarmLed, OUTPUT);
}

void loop() {
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
//  lcd.setCursor(0, 0);

  digitalWrite(trigPin, LOW);
  delayMicroseconds(5);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);

  // Convert the time into a distance
  distance = (duration/2) * 0.034;
  Serial.println("Distance: ");
  Serial.print(distance);
  Serial.print(" cm\n");
  
//  lcd.setCursor(0, 0);
  lcd.clear();
  if(distance <= 35){
    lcd.print("Alarm On");
    digitalWrite(alarmLed, HIGH);
    digitalWrite(alarmBuzzer, HIGH);
    delay(500);
    digitalWrite(alarmLed, LOW);
    digitalWrite(alarmBuzzer, LOW);
  }else{
    digitalWrite(alarmBuzzer, LOW);
    lcd.print("Alarm Off");
  }

  delay(500);
}
