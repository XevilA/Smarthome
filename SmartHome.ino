#define BLYNK_PRINT Serial
#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <Servo.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Keypad.h>
#include <IRremote.h>

// WiFi & Blynk
char auth[] = "BLYNK_AUTH_TOKEN";
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// DHT11 Sensor
#define DHTPIN D4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LCD Display (I2C)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Keypad (4x4)
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {D0, D1, D2, D3};
byte colPins[COLS] = {D5, D6, D7, D8};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// IR Remote
IRrecv irrecv(D9);
decode_results results;

// Sensors & Actuators
int flameSensor = A0;
int rainSensor = A1;
int gasSensor = A2;
int lightSensor = A3;
int buzzer = D10;

Servo garageDoor, homeDoor, windowServo;

// Blynk Virtual Pins
#define V_TEMPERATURE V1
#define V_HUMIDITY V2
#define V_GAS V3
#define V_FLAME V4
#define V_LIGHT V5
#define V_RAIN V6
#define V_BUZZER V7
#define V_GARAGE_DOOR V8
#define V_HOME_DOOR V9
#define V_WINDOW V10

// อ่านค่าจากเซ็นเซอร์และส่งไปที่ Blynk
void readSensors() {
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();
  int flame = analogRead(flameSensor);
  int rain = analogRead(rainSensor);
  int gas = analogRead(gasSensor);
  int light = analogRead(lightSensor);

  Blynk.virtualWrite(V_TEMPERATURE, temperature);
  Blynk.virtualWrite(V_HUMIDITY, humidity);
  Blynk.virtualWrite(V_GAS, gas);
  Blynk.virtualWrite(V_FLAME, flame);
  Blynk.virtualWrite(V_LIGHT, light);
  Blynk.virtualWrite(V_RAIN, rain);

  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C  ");

  lcd.setCursor(0, 1);
  lcd.print("Humid: ");
  lcd.print(humidity);
  lcd.print("%  ");

  delay(2000);
}

// ควบคุม Servo Motors ผ่าน Blynk
BLYNK_WRITE(V_GARAGE_DOOR) {
  int value = param.asInt();
  garageDoor.write(value ? 90 : 0);
}

BLYNK_WRITE(V_HOME_DOOR) {
  int value = param.asInt();
  homeDoor.write(value ? 90 : 0);
}

BLYNK_WRITE(V_WINDOW) {
  int value = param.asInt();
  windowServo.write(value ? 90 : 0);
}

// เปิด/ปิด Buzzer Alarm
BLYNK_WRITE(V_BUZZER) {
  int state = param.asInt();
  digitalWrite(buzzer, state);
}

// ควบคุมผ่าน Keypad
void handleKeypad() {
  char key = keypad.getKey();
  if (key) {
    Serial.print("Key Pressed: ");
    Serial.println(key);
    if (key == 'A') {
      garageDoor.write(90);
    } else if (key == 'B') {
      homeDoor.write(90);
    } else if (key == 'C') {
      windowServo.write(90);
    } else if (key == 'D') {
      digitalWrite(buzzer, HIGH);
      delay(1000);
      digitalWrite(buzzer, LOW);
    }
  }
}

// ควบคุมผ่าน IR Remote
void handleIRRemote() {
  if (irrecv.decode(&results)) {
    Serial.println(results.value, HEX);
    if (results.value == 0xFFA25D) {
      garageDoor.write(90);
    } else if (results.value == 0xFF629D) {
      homeDoor.write(90);
    } else if (results.value == 0xFFE21D) {
      windowServo.write(90);
    } else if (results.value == 0xFF22DD) {
      digitalWrite(buzzer, HIGH);
      delay(1000);
      digitalWrite(buzzer, LOW);
    }
    irrecv.resume();
  }
}

// ตั้งค่า
void setup() {
  Serial.begin(115200);
  Blynk.begin(auth, ssid, pass);

  pinMode(flameSensor, INPUT);
  pinMode(rainSensor, INPUT);
  pinMode(gasSensor, INPUT);
  pinMode(lightSensor, INPUT);
  pinMode(buzzer, OUTPUT);
  
  dht.begin();
  lcd.begin();
  lcd.backlight();

  garageDoor.attach(D11);
  homeDoor.attach(D12);
  windowServo.attach(D13);

  Blynk.virtualWrite(V_GARAGE_DOOR, 0);
  Blynk.virtualWrite(V_HOME_DOOR, 0);
  Blynk.virtualWrite(V_WINDOW, 0);

  irrecv.enableIRIn(); // เริ่มต้น IR Remote

  Serial.println("Smart Home Ready!");
}

void loop() {
  Blynk.run();
  readSensors();
  handleKeypad();
  handleIRRemote();
  delay(5000);
}
