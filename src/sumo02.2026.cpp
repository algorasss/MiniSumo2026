#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "ClosedCube_TCA9548A.h"

#define TCA9548A_I2C_ADDRESS 0x70
HardwareSerial BTSerial(2);
ClosedCube::Wired::TCA9548A tca(TCA9548A_I2C_ADDRESS); //init the multiplexer right away
Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

const int motorA1 = PB13;     // пін для мотора 1
const int motorA2 = PB14;     // пін для мотора 1
const int motorB1 = PA6;      // пін для мотора 2
const int motorB2 = PA5;      // пін для мотора 2
const int lineSensor1 = PA15; // пін для датчика лінії 1
//idk which one of these is left and which is right
const int lineSensor2 = PB3;  // пін для датчика лінії 2
const int buttonPin = PC15;   // пін для кнопки

const uint8_t NUM_SENSORS = 6; // we have 6 sensors

int motorSpeed = 80;
int motorSpeedback = 200; 
int motorSpeedrot = 100;
int speedRotate = 100;
int attackSpeed = 140;
int SpeedUP = 80;
bool lastButtonState = false; // попередній стан кнопки
bool isProgramRunning = false;  // стан програми (запущено чи ні)
bool buttonState = false;     // поточний стан кнопки

unsigned long lastRead = 0;
uint16_t distances[NUM_SENSORS]; // start from 1 not 0
bool sensorStatus[NUM_SENSORS];


void moveForward()
{
  analogWrite(motorA1, motorSpeed);
  analogWrite(motorA2, 0);
  analogWrite(motorB1, motorSpeed);
  analogWrite(motorB2, 0);
}
void speedUp()
{
  analogWrite(motorA1, SpeedUP); // Швидкість на мотор 1
  analogWrite(motorA2, 0);       // Відключення протилежного напрямку на моторі 1
  analogWrite(motorB1, SpeedUP); // Швидкість на мотор 2
  analogWrite(motorB2, 0);       // Відключення протилежного напрямку на моторі 2
}
void stopMotors()
{
  analogWrite(motorA1, 0); // Зупинка мотора 1
  analogWrite(motorA2, 0); // Зупинка мотора 1
  analogWrite(motorB1, 0); // Зупинка мотора 2
  analogWrite(motorB2, 0); // Зупинка мотора 2
}
void attack()
{
  analogWrite(motorA1, attackSpeed); // Швидкість на мотор 1
  analogWrite(motorA2, 0);           // Відключення протилежного напрямку на моторі 1
  analogWrite(motorB1, attackSpeed); // Швидкість на мотор 2
  analogWrite(motorB2, 0);           // Відключення протилежного напрямку на моторі 2
}
void rotate()
{
  analogWrite(motorA1, 0);             // Відключення протилежного напрямку на моторі 1
  analogWrite(motorA2, motorSpeedrot); // Швидкість на мотор 1
  analogWrite(motorB1, motorSpeedrot); // Швидкість на мотор 2
  analogWrite(motorB2, 0);             // Відключення протилежного напрямку на моторі 2
}
void rotateR()
{
  analogWrite(motorA1, 0);           // Відключення протилежного напрямку на моторі 1
  analogWrite(motorA2, speedRotate); // Швидкість на мотор 1
  analogWrite(motorB1, speedRotate); // Швидкість на мотор 2
  analogWrite(motorB2, 0);           // Відключення протилежного напрямку на моторі 2
}
void rotateL()
{
  analogWrite(motorA1, speedRotate); // Швидкість на мотор 1
  analogWrite(motorA2, 0);           // Відключення протилежного напрямку на моторі 1
  analogWrite(motorB1, 0);           // Відключення протилежного напрямку на моторі 2
  analogWrite(motorB2, speedRotate); // Швидкість на мотор 2
}
void moveBackward()
{
  analogWrite(motorA1, 0);              // Відключення напрямку вперед на моторі 1
  analogWrite(motorA2, motorSpeedback); // Швидкість на мотор 1
  analogWrite(motorB1, 0);              // Відключення напрямку вперед на моторі 2
  analogWrite(motorB2, motorSpeedback); // Швидкість на мотор 2
}

void printAddress(uint8_t address)
{
  BTSerial.print("0x");
  if (address < 0x10)
  {
    BTSerial.print("0");
  }
  BTSerial.println(address, HEX);
}

void setup()
{
  pinMode(motorA1, OUTPUT);
  pinMode(motorA2, OUTPUT);
  pinMode(motorB1, OUTPUT);
  pinMode(motorB2, OUTPUT);
  pinMode(lineSensor1, INPUT);
  pinMode(lineSensor2, INPUT);
  pinMode(buttonPin, INPUT_PULLUP); // is high on default, low when pressed

  Serial.begin(115200);
  BTSerial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);
  Serial.println("setting up...");

  for (uint8_t channel = 1; channel < NUM_SENSORS; channel++) //checking sensors on all channels of the multiplexer
  {
    Serial.print("scanning sensor on channel ");
    Serial.print(channel);
    Serial.println("... ");
    //distances[channel] = 500; // pseudo number, gonna change later
    bool isChannelSelected = (tca.selectChannel(channel) == 0);

    if (isChannelSelected) // channel selected successfully
    { 
      Serial.println("OK channel selected");
      if (sensor.begin())
      {
        Serial.println("OK sensor found");
        sensor.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
        VL53L0X_RangingMeasurementData_t measure;
        sensor.rangingTest(&measure, true);
      }
      else
      {
        Serial.println("ERROR finding sensor");
      }
    }
    else Serial.println("ERROR selecting channel");
  }
}

void loop()
{
  //TODO add debugs
  //TODO maybe for optimization do it like an event or smth, look it up
  buttonState = digitalRead(buttonPin);

  if (buttonState == LOW && lastButtonState == HIGH) //if button was just pressed (active is low)
  {
    delay(50); // Затримка для антидребезгу кнопки
    if (!isProgramRunning)
    {
      delay(4950); // Затримка для входу в ринг, 5 секунд
      isProgramRunning = true; //starting the program
    }
    else
    {
      isProgramRunning = false; // stopping the program
      stopMotors();
    }
  }
  lastButtonState = buttonState;


  if (isProgramRunning)
  {
    /* LOGIC
    so, if one of line trackers detects a WHITE line, check for which one, left or right(probably 1 is left 2 is right)
    if left, rotate right, if right, rotate left
    if on black, check all sensors probably in no order, but this order could be optimal: middle, right left, right side left side
    and record all the values into distances array
    if else hell for now, later make a finite state machine
    placeholders for distances now
    if only middle detect enemy - ram at full speed; if middle and right - maybe left motor full, right motor a bit slower for turning;
    if middle left - the opposite; if right or right side or left or left side, jst rotate in place towards the enemy;
    if nothing - rotate in place to search for the enemy

    TODO think about logic, like if the car is in the left of the ring and cant see the enemy, rotate to right since its mora probable that the enemy is
    in the remaining part of ring, stuff like that.
    */


    // black is high, white is low
    int sensor1 = digitalRead(lineSensor1);
    int sensor2 = digitalRead(lineSensor2);

    if (sensor1 == HIGH || sensor2 == HIGH) // if both sensors are on ring(the ring should be black)
    {
      if (distances[3] < 90)
      {
        attack();
        BTSerial.println("Атака");
      }
      else if (distances[1] < 220 && distances[1] > 140)
      {
        speedUp();
      }
      else if (distances[1] < 350 && distances[1] > 220)
      {
        rotateR();
        BTSerial.println("Праворуч");
      }
      else if (distances[5] < 350 && distances[5] > 220)
      {
        rotateL();
        BTSerial.println("Ліворуч");
      }
      else if (distances[4] < 250 && distances[4] > 180)
      {
        rotateL();
        BTSerial.println("Доворот вліво");
      }
      else if (distances[2] < 250 && distances[2] > 180)
      {
        rotateR();
        BTSerial.println("Доворот вправо");
      }
      else
      {
        moveForward();
        BTSerial.println("Рух Вперед");
      }
    }
    else //if someone detects white(edge of the ring), we need to rotate to the opposite side to get back on the ring
    {
      if (sensor1 == LOW)
      {
        //rotate in place to the right
      }
      else if (sensor2 == LOW)
      {
        //rotate in place to the left
      }
      else //both sensors on white, 
      {
        //idk
      }
    }
    for (uint8_t channel = 1; channel < NUM_SENSORS; channel++)
    {
      // Вибір каналу
      if (tca.selectChannel(channel) != 0)
      {
        distances[channel] = 0;
        sensorStatus[channel] = false;
        continue;
      }
      VL53L0X_RangingMeasurementData_t measure;
      sensor.rangingTest(&measure, false);

      if (measure.RangeStatus != 4)
      {
        distances[channel] = measure.RangeMilliMeter;
        sensorStatus[channel] = true;
      }
      else
      {
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
  //maybe delay at the end of loop for smoother movement
}
