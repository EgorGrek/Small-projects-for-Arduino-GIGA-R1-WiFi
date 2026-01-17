#include <Wire.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include "secrets.h"

enum Mode {
  JOYSTICK,
  SENSOR
};
Mode currentMode = JOYSTICK;
WiFiUDP Udp;

#define BLUE_LED D31
#define RED_LED D32
#define YELLOW_LED D33
#define GREEN_LED D35

#define JOY_X A0
#define JOY_Y A1
#define JOY_SW D41


// I2C Address for the AiP31068L controller [cite: 750]
const int LCD_ADDR = 0x7C >> 1; 

// Control Bytes [cite: 793, 822, 826]
// Co bit: 0 = last control byte, 1 = another follows. 
// RS bit: 0 = command, 1 = data.
const uint8_t LCD_COMMAND = 0x80; // 10000000: Command mode
const uint8_t LCD_DATA    = 0x40; // 01000000: Data mode
int XNormal;
int YNormal; 
const int Difference = 10; 
int X_CURRENT_POSITION = 512;
int Y_CURRENT_POSITION = 512;

void sendLCD(uint8_t control, uint8_t value) {
  Wire.beginTransmission(LCD_ADDR);
  Wire.write(control);
  Wire.write(value);
  Wire.endTransmission();
}

void lcdInit() {
  delay(50); 
  sendLCD(LCD_COMMAND, 0x38); // 2 строки, 5x8 точек [cite: 831, 833, 834]
  delayMicroseconds(40);
  sendLCD(LCD_COMMAND, 0x0C); // Дисплей ВКЛ, курсор ВЫКЛ [cite: 831, 835]
  delayMicroseconds(40);
  sendLCD(LCD_COMMAND, 0x01); // Очистка [cite: 841, 872]
  delay(2);
  sendLCD(LCD_COMMAND, 0x06); // Инкремент курсора [cite: 852, 872]
}

void lcdPrint(const char* str) {
  while (*str) {
    sendLCD(LCD_DATA, (uint8_t)*str++);
  }
}

void lcdSetCursor(uint8_t col, uint8_t row) {
  uint8_t addr = (row == 0 ? 0x80 : 0xC0) + col; // 0x80 - 1-я строка, 0xC0 - 2-я 
  sendLCD(LCD_COMMAND, addr);
}

// the setup function runs once when you press reset or power the board
void setup() {

  // initialize digital pins as an output.
  pinMode(LED_BUILTIN, OUTPUT); // active low
  pinMode(BLUE_LED, OUTPUT); // active high
  pinMode(RED_LED, OUTPUT); // active high
  pinMode(YELLOW_LED, OUTPUT); // active high
  pinMode(GREEN_LED, OUTPUT); // active high

  Serial.begin(115200);
  while (!Serial) {
    ; // wait
  }
  Serial.print("Hello world.\n");
  pinMode(JOY_SW, INPUT_PULLUP); // Кнопка джойстика с подтяжкой

  Wire.begin();// 1. Wait 50ms for the LCD internal reset routine [cite: 839]
  delay(100);
  
  lcdInit();

  char message[] = "Hello Yegor!";
  lcdPrint(message);
  XNormal = analogRead(JOY_X);
  YNormal = analogRead(JOY_Y);
  
  Serial.print("XNormal ");
  Serial.println(XNormal);
  Serial.print("YNormal ");
  Serial.println(YNormal);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Udp.begin(port);

  delay(2000);
}

byte val = 0;

// the loop function runs over and over again forever
void loop() {

  if (digitalRead(JOY_SW) == LOW) {
    // currentMode = (currentMode == JOYSTICK) ? SENSOR : JOYSTICK;
    // sendLCD(LCD_COMMAND, 0x01); // Очистка экрана при смене режима
    delay(300);
  }
  if (currentMode == JOYSTICK) {
    int xVal = analogRead(JOY_X);
    int yVal = analogRead(JOY_Y);

    lcdSetCursor(0, 0);
    lcdPrint("X:");
    char xStr[5]; itoa(X_CURRENT_POSITION, xStr, 10); lcdPrint(xStr);
    lcdPrint(" Y:");
    char yStr[5]; itoa(Y_CURRENT_POSITION, yStr, 10); lcdPrint(yStr);
    lcdPrint("    ");
    
    lcdSetCursor(0, 1);
    lcdPrint("X:");
    itoa(xVal, xStr, 10); lcdPrint(xStr);
    lcdPrint(" Y:");
    itoa(yVal, yStr, 10); lcdPrint(yStr);
    lcdPrint("    ");


    // Serial.print("xVal ");
    // Serial.println(xVal);
    // Serial.print("yVal ");
    // Serial.println(yVal);

    if ((xVal-XNormal) > Difference){
      X_CURRENT_POSITION++;
    }
    else if ((xVal-XNormal) < -Difference){
      X_CURRENT_POSITION--;
    }

    if ((yVal-YNormal) < -Difference){
      Y_CURRENT_POSITION++;
    }
    else if ((yVal-YNormal) > Difference){
      Y_CURRENT_POSITION--;
    }

    
    digitalWrite(BLUE_LED, LOW);

    if (X_CURRENT_POSITION < 512 && Y_CURRENT_POSITION < 512) 
    {
      digitalWrite(BLUE_LED, HIGH);

      digitalWrite(RED_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
    }

    if (X_CURRENT_POSITION < 512 && Y_CURRENT_POSITION > 512) 
    {
      digitalWrite(GREEN_LED, HIGH);

      digitalWrite(BLUE_LED, LOW);
      digitalWrite(RED_LED, LOW);
      digitalWrite(YELLOW_LED, LOW);
    }

    if (X_CURRENT_POSITION > 512 && Y_CURRENT_POSITION > 512) 
    { 
      digitalWrite(RED_LED, HIGH);

      digitalWrite(YELLOW_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(GREEN_LED, LOW);
    }

    if (X_CURRENT_POSITION > 512 && Y_CURRENT_POSITION < 512) 
    { 
      digitalWrite(YELLOW_LED, HIGH);

      digitalWrite(GREEN_LED, LOW);
      digitalWrite(BLUE_LED, LOW);
      digitalWrite(RED_LED, LOW);
    }

  }
  String dataToSend = "Hello from Giga! Uptime: " + String(millis() / 1000) + "s";
  
  Udp.beginPacket(pcIP, port);
  Udp.print(dataToSend);
  if (Udp.endPacket()) {
    Serial.println("Packet sent successfully!");
  } else {
    Serial.println("Packet failed to send. Check PC IP or Gateway.");
  }
  Serial.println("Sent: " + dataToSend);
  delay(2000);

}

