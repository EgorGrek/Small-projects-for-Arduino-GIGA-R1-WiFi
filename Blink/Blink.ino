/*
  Blink

  Turns an LED on for one second, then off for one second, repeatedly.

  Most Arduinos have an on-board LED you can control. On the UNO, MEGA and ZERO
  it is attached to digital pin 13, on MKR1000 on pin 6. LED_BUILTIN is set to
  the correct LED pin independent of which board is used.
  If you want to know what pin the on-board LED is connected to on your Arduino
  model, check the Technical Specs of your board at:
  https://docs.arduino.cc/hardware/

  This example code is in the public domain.

  https://docs.arduino.cc/built-in-examples/basics/Blink/
*/

#include <Wire.h>


// Structure to hold the sensor data
struct DHTData {
  byte humidity;
  byte temperature;
  bool success;
};

enum Mode {
  JOYSTICK,
  SENSOR
};
Mode currentMode = JOYSTICK;

#define BLUE_LED D31
#define DHT_PIN D51 // temp/humidity sensor


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

  Serial.begin(9600);
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

  delay(2000);
}

byte val = 0;
void printDHTdataOnLCD(DHTData &data) {
  // Первая строка: Температура
  lcdSetCursor(0, 0);
  lcdPrint("Temp: ");
  char t_str[4];
  itoa(data.temperature, t_str, 10);
  lcdPrint(t_str);
  lcdPrint(" ");
  sendLCD(LCD_DATA, 0xDF); // символ градуса
  lcdPrint("C   "); // Пробелы в конце для очистки старых цифр

  // Вторая строка: Влажность
  lcdSetCursor(0, 1);
  lcdPrint("Humid: ");
  char h_str[4];
  itoa(data.humidity, h_str, 10);
  lcdPrint(h_str);
  lcdPrint(" %   ");
}
// the loop function runs over and over again forever
void loop() {

  if (digitalRead(JOY_SW) == LOW) {
    currentMode = (currentMode == JOYSTICK) ? SENSOR : JOYSTICK;
    sendLCD(LCD_COMMAND, 0x01); // Очистка экрана при смене режима
    delay(300); // Антидребезг
  }
  if (currentMode == JOYSTICK) {
    int xVal = analogRead(JOY_X);
    int yVal = analogRead(JOY_Y);

    lcdSetCursor(0, 0);
    lcdPrint("MODE: JOYSTICK  ");
    
    lcdSetCursor(0, 1);
    lcdPrint("X:");
    char xStr[5]; itoa(xVal, xStr, 10); lcdPrint(xStr);
    lcdPrint(" Y:");
    char yStr[5]; itoa(yVal, yStr, 10); lcdPrint(yStr);
    lcdPrint("    ");
  } else if (currentMode == SENSOR) {

    DHTData data = readDHT11();

    if (data.success) {
      Serial.print("Humidity: "); Serial.print(data.humidity); Serial.print("%  |  ");
      Serial.print("Temp: "); Serial.print(data.temperature); Serial.println("C");
      printDHTdataOnLCD(data);
    } else {
      Serial.println("Error reading sensor (Checksum/Timeout)");
      lcdSetCursor(0, 0);
      lcdPrint("Sensor Error    ");
      lcdSetCursor(0, 1);
      lcdPrint("                ");
    }
  }
  digitalWrite(LED_BUILTIN, HIGH);  // turn the LED off
  digitalWrite(BLUE_LED, HIGH);  // turn the LED on
  Serial.print("Blink HIGH\n");
  delay(1000);                      // wait for a second
  digitalWrite(LED_BUILTIN, LOW);   // turn the LED on 
  digitalWrite(BLUE_LED, LOW);  // turn the LED off
  Serial.print("Blink LOW\n");
  delay(1000);                      // wait for a second

}


// Function 1: Send the 18ms Start Signal required by DHT11
void sendStartSignal() {
  pinMode(DHT_PIN, OUTPUT);
  digitalWrite(DHT_PIN, LOW);
  delay(18); // Pull low for at least 18ms
  digitalWrite(DHT_PIN, HIGH);
  delayMicroseconds(40);
  pinMode(DHT_PIN, INPUT);
}

// Function 2: Logic to capture 40 bits of data based on pulse timing
bool captureBits(byte *data) {
  unsigned long startPulse = micros();
  // Wait for DHT Response (80us Low, 80us High)
  while(digitalRead(DHT_PIN) == HIGH && (micros() - startPulse) < 100);
  startPulse = micros();
  while(digitalRead(DHT_PIN) == LOW && (micros() - startPulse) < 100);
  startPulse = micros();
  while(digitalRead(DHT_PIN) == HIGH && (micros() - startPulse) < 100);

  for (int i = 0; i < 40; i++) {
    startPulse = micros();
    while(digitalRead(DHT_PIN) == LOW && (micros() - startPulse) < 100); // Wait for bit start
    startPulse = micros();
    
    while(digitalRead(DHT_PIN) == HIGH && (micros() - startPulse) < 100); // Measure HIGH pulse duration
    
    // A "0" is 26-28us; a "1" is 70us. We use 40us as the threshold
    if ((micros() - startPulse) > 40) {
      data[i / 8] |= (1 << (7 - (i % 8)));
    }
  }
  return true;
}

// Function 3: Wrapper to coordinate the read and verify data integrity
DHTData readDHT11() {
  byte rawData[5] = {0, 0, 0, 0, 0};
  DHTData currentRead = {0, 0, false};

  sendStartSignal();
  
  if (captureBits(rawData)) {
    // Checksum verification: Sum of first 4 bytes must equal the 5th byte
    if (rawData[4] == (rawData[0] + rawData[1] + rawData[2] + rawData[3])) {
      currentRead.humidity = rawData[0];
      currentRead.temperature = rawData[2];
      currentRead.success = true;
    }
  }
  return currentRead;
}
