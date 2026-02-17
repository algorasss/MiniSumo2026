
// Підключення бібліотек
#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "ClosedCube_TCA9548A.h"

#define TCA9548A_I2C_ADDRESS 0x70
HardwareSerial BTSerial(2); 
ClosedCube::Wired::TCA9548A tca(TCA9548A_I2C_ADDRESS);
Adafruit_VL53L0X lox = Adafruit_VL53L0X();
// Піни для підключення
const int motorA1 = PB13;  // пін для мотора 1
const int motorA2 = PB14;  // пін для мотора 1
const int motorB1 = PA6;   // пін для мотора 2
const int motorB2 = PA5;   // пін для мотора 2
const int lineSensor1 = PA15;  // пін для датчика лінії 1
const int lineSensor2 = PB3;   // пін для датчика лінії 2
const int buttonPin = PC15;    // пін для кнопки

const uint8_t NUM_SENSORS = 6;
// Змінні для регулювання швидкості моторів

//  HERE SPEEDS ???
int motorSpeed = 80;  // Початкова швидкість (максимальна)
int motorSpeedback = 200;  // Початкова швидкість (максимальна)
int motorSpeedrot = 100;  // Початкова швидкість (максимальна)
int speedRotate = 100;
int attackSpeed = 140;
int SpeedUP = 80;
//  END
bool lastButtonState = false; // попередній стан кнопки
bool programRunning = false;  // стан програми (запущено чи ні)
bool buttonState = false;     // поточний стан кнопки

unsigned long lastRead = 0;
uint16_t distances[NUM_SENSORS];
bool sensorStatus[NUM_SENSORS];

// Функція для руху моторів вперед з регулюванням швидкості
void moveForward() {
  analogWrite(motorA1, motorSpeed);  // Швидкість на мотор 1
  analogWrite(motorA2, 0);           // Відключення протилежного напрямку на моторі 1
  analogWrite(motorB1, motorSpeed);  // Швидкість на мотор 2
  analogWrite(motorB2, 0);           // Відключення протилежного напрямку на моторі 2
}
void speedUp() {
  analogWrite(motorA1, SpeedUP);  // Швидкість на мотор 1
  analogWrite(motorA2, 0);           // Відключення протилежного напрямку на моторі 1
  analogWrite(motorB1, SpeedUP);  // Швидкість на мотор 2
  analogWrite(motorB2, 0);           // Відключення протилежного напрямку на моторі 2
}
// Функція для зупинки моторів
void stopMotors() {
  analogWrite(motorA1, 0);  // Зупинка мотора 1
  analogWrite(motorA2, 0);  // Зупинка мотора 1
  analogWrite(motorB1, 0);  // Зупинка мотора 2
  analogWrite(motorB2, 0);  // Зупинка мотора 2
} 
void attack() {
  analogWrite(motorA1, attackSpeed);  // Швидкість на мотор 1
  analogWrite(motorA2, 0);  // Відключення протилежного напрямку на моторі 1
  analogWrite(motorB1, attackSpeed);  // Швидкість на мотор 2
  analogWrite(motorB2, 0);  // Відключення протилежного напрямку на моторі 2
}

//  HERE  ROTATE WHAT??
void rotate() {
  analogWrite(motorA1, 0);  // Відключення протилежного напрямку на моторі 1
  analogWrite(motorA2, motorSpeedrot);  // Швидкість на мотор 1
  analogWrite(motorB1, motorSpeedrot);  // Швидкість на мотор 2
  analogWrite(motorB2, 0);  // Відключення протилежного напрямку на моторі 2
}
//  END


void rotateR() {
  analogWrite(motorA1, 0);  // Відключення протилежного напрямку на моторі 1
  analogWrite(motorA2, speedRotate);  // Швидкість на мотор 1
  analogWrite(motorB1, speedRotate);  // Швидкість на мотор 2
  analogWrite(motorB2, 0);  // Відключення протилежного напрямку на моторі 2
}

void rotateL() {
  analogWrite(motorA1, speedRotate);  // Швидкість на мотор 1
  analogWrite(motorA2, 0);  // Відключення протилежного напрямку на моторі 1
  analogWrite(motorB1, 0);  // Відключення протилежного напрямку на моторі 2
  analogWrite(motorB2, speedRotate);  // Швидкість на мотор 2
}

// Функція для руху назад з регулюванням швидкості
void moveBackward() {
  analogWrite(motorA1, 0);  // Відключення напрямку вперед на моторі 1
  analogWrite(motorA2, motorSpeedback); // Швидкість на мотор 1
  analogWrite(motorB1, 0);  // Відключення напрямку вперед на моторі 2
  analogWrite(motorB2, motorSpeedback); // Швидкість на мотор 2
}

//  HERE WHAATS BT AND ADDRESS
void printAddress(uint8_t address)  {
  BTSerial.print("0x");
	if (address<0x10) {
    BTSerial.print("0");
	}
	BTSerial.println(address,HEX);
}
//  END


void setup() {
  // Ініціалізація пінів
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(lineSensor1, INPUT);
  pinMode(lineSensor2, INPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // використання внутрішнього підтягу

  Serial.begin(115200);  // Ініціалізація серійного зв'язку для FT232RL
  BTSerial.begin(9600);
  Wire.begin();
  //  HERE WHY 400KHZ
  Wire.setClock(400000);
  //  END
  Serial.println("setting up...");
  //Serial.println("Ініціалізація мультиплексора TCA9548A...");
  //tca.address(TCA9548A_I2C_ADDRESS);

  // Ініціалізація сенсорів на каналах 0, 1, 2

  // HERE WHY 1 NOT 0
  for (uint8_t channel = 1; channel < NUM_SENSORS; channel++) {
    BTSerial.print("Сканування сенсора на каналі ");
    BTSerial.print(channel);
    BTSerial.print("... ");

    distances[channel] = 500; //pseudo number, gonna change later

    BTSerial.print(tca.selectChannel(channel));

    // create a var instead of calling the function twice

    if (tca.selectChannel(channel) == 0) { // channel selected successfully
       VL53L0X_RangingMeasurementData_t measure;
       lox.rangingTest(&measure, false);
      if (!lox.begin()) {
        BTSerial.println("❌ Не знайдено VL53L0X");
        Serial.println("❌ Не знайдено VL53L0X");
      } else {
        BTSerial.println("✅ Сенсор знайдено.");
        Serial.println("✅ Сенсор знайдено.");
        lox.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
      }
    } else {
      BTSerial.println("❌ Помилка вибору каналу");
      Serial.println("❌ Помилка вибору каналу");
    }
  }

  BTSerial.println("\nПочинаємо вимірювання...\n");
  // delay(15000);
}


void loop() {
  // Зчитування стану кнопки
  buttonState = digitalRead(buttonPin);

  // Якщо стан кнопки змінився (було натискання)
  if (buttonState == LOW && lastButtonState == HIGH) {  // Якщо кнопка натиснута
    delay(50);  // Затримка для антидребезгу кнопки
    if (!programRunning) {  // Якщо програма не запущена
      programRunning = true;  // Запускаємо програму
    } else {
      programRunning = false;  // Якщо програма запущена, зупиняємо її
      stopMotors();  // Зупиняємо мотори
    }
  }
  // Оновлення попереднього стану кнопки
  lastButtonState = buttonState;

  // Якщо програма активна
  if (programRunning) {
     // Зчитування значень з датчиків лінії
    int sensor1 = digitalRead(lineSensor1);
    int sensor2 = digitalRead(lineSensor2);
    // Якщо хоча б один датчик виявляє білу лінію
    if (sensor1 == LOW && sensor2 == LOW) {
      // Білу лінію виявлено
        BTSerial.print("   ");
       BTSerial.print(distances[5]);
        BTSerial.print("   ");
       BTSerial.print(distances[4]);
       BTSerial.print("   ");
       BTSerial.print(distances[3]);
        BTSerial.print("   ");
       BTSerial.print(distances[2]);
       BTSerial.print("  ");
       BTSerial.println(distances[1]);
      //  BTSerial.print("  Лівій фронтальний: ");
    //  if (true) Serial.print(" ");
       if (distances[3] < 90){
        attack();
       BTSerial.println("Атака");
      }
       else if (distances[1] < 220 && distances[1] > 140) {
         speedUp();
       }
       else if(distances[1] < 350 && distances[1] > 220){
        rotateR();
      BTSerial.println("Праворуч");
      }
      else if(distances[5] < 350 && distances[5] > 220){
        rotateL();
      BTSerial.println("Ліворуч");
      } 
       else if(distances[4] < 250 && distances[4] > 180){
       rotateL();
      BTSerial.println("Доворот вліво");
      }
      else if(distances[2] < 250 && distances[2] > 180){
       rotateR();
      BTSerial.println("Доворот вправо");
      }
      else {
        moveForward();
       BTSerial.println("Рух Вперед");
      }
    } else {
      //Інакше, продовжуємо рухатися вперед
      moveBackward();
      delay(200);
      rotate();
      delay(100);
      }
    for (uint8_t channel = 1; channel < NUM_SENSORS; channel++) {
    // Вибір каналу
    if (tca.selectChannel(channel) != 0) {
      distances[channel] = 0;
      sensorStatus[channel] = false;
      continue;
    }
     VL53L0X_RangingMeasurementData_t measure;
     lox.rangingTest(&measure, false);

    if (measure.RangeStatus != 4) {
      distances[channel] = measure.RangeMilliMeter;
      sensorStatus[channel] = true;
    } else {
      distances[channel] = 0;
      sensorStatus[channel] = false;
    }
  }
  if (distances[1] == 0)
  {
    distances[1] = 500;
  }
  if (distances[5] == 0)
  {
    distances[5] = 500;
  }
  if (distances[2] == 0)
  {
    distances[2] = 500;
  }
  if (distances[4] == 0)
  {
    distances[4] = 500;
  }
  if (distances[3] == 0)
  {
    distances[3] = 500;
  }
  }
}
