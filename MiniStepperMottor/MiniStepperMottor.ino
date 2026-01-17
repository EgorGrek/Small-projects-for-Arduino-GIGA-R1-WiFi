#include <Stepper.h>

// Укажите количество шагов на один оборот. 
// Для этой модели (PM08-2) это обычно 20 шагов (угол 18 градусов).
const int stepsPerRevolution = 20; 

// Инициализируем библиотеку на пинах 8, 9, 10, 11
// Подключите входы драйвера (IN1, IN2, IN3, IN4) к этим пинам
Stepper myStepper(stepsPerRevolution, 4, 5, 6, 7);

void setup() {
  // Устанавливаем скорость (RPM). Для таких малюток 30-60 — нормально.
  myStepper.setSpeed(60);
  Serial.begin(9600);
}

void loop() {
  // Поворот на один оборот в одну сторону
  Serial.println("Вращение по часовой...");
  myStepper.step(stepsPerRevolution);
  
  myStepper.step(stepsPerRevolution);
  
  myStepper.step(stepsPerRevolution);
  
  myStepper.step(stepsPerRevolution);
  delay(2000);

  // Поворот на один оборот в другую сторону
  Serial.println("Вращение против часовой...");
  myStepper.step(-stepsPerRevolution);
  delay(2000);
}