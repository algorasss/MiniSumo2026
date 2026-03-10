#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "ClosedCube_TCA9548A.h"

#define TCA9548A_I2C_ADDRESS 0x70
HardwareSerial BTSerial(2);
ClosedCube::Wired::TCA9548A tca(TCA9548A_I2C_ADDRESS); //init the multiplexer right away
Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

//---PINS---
const int motorL1 = PB13;     // пін для мотора 1
const int motorL2 = PB14;     // пін для мотора 1
const int motorR1 = PA6;      // пін для мотора 2
const int motorR2 = PA5;      // пін для мотора 2
const int lineSensorL = PA15; // пін для датчика лінії 1
//idk which one of these is left and which is right
const int lineSensorR = PB3;  // пін для датчика лінії 2
const int buttonPin = PC15;   // пін для кнопки
const uint8_t NUM_SENSORS = 4; // we have 4 sensors

//---SPEEDS---
int attackSpeed = 200; // speed for motors
int curveMainSpeed = 180; // speed for the main motor when doing a curve attack
int curveSubSpeed = 100; // speed for the secondary motor when doing a curve attack
int rotateInPlaceSpeed = 130; // speed for rotating in place

///---BUTTON---
bool lastButtonState = false; // попередній стан кнопки
bool isProgramRunning = false;  // стан програми (запущено чи ні)
bool buttonState = false;     // поточний стан кнопки


//---SENSOR CHECKS---
uint16_t distances[NUM_SENSORS];
// far 60 - 750
bool enemyLeftFar;
bool enemyFrontLeftFar;
bool enemyRightFar;
bool enemyFrontRightFar;
bool enemyFrontFar;
// close 1 - 60
bool enemyLeftClose;
bool enemyFrontLeftClose;
bool enemyRightClose;
bool enemyFrontRightClose;
bool enemyFrontClose;
// clear > 750
bool enemyRightClear;
bool enemyLeftClear;
bool enemyFrontLeftClear;
bool enemyFrontRightClear;
bool enemyFrontClear;
bool enemySidesClear;
bool enemyRightSideClear;
bool enemyLeftSideClear;

//---FSM STATES---
enum SubState
{ SEARCHING, AVOID_EDGE, ATTACK, ROTATE_IN_PLACE };
enum MoveDirection
{ LEFT, FRONT_LEFT, FRONT, FRONT_RIGHT, RIGHT, BACK_LEFT, BACK, BACK_RIGHT };
// MainState mainState = ON_RING;
SubState state = SEARCHING;
MoveDirection moveDirection = FRONT;


void Motors(int leftForward, int leftBackward, int rightForward, int rightBackward)
{
    analogWrite(motorL1, leftForward);
    analogWrite(motorL2, leftBackward);
    analogWrite(motorR1, rightForward);
    analogWrite(motorR2, rightBackward);
}
void Move(MoveDirection dir)
{
    // straight attacks
    if(dir == FRONT)
    {
      Motors(attackSpeed, 0, attackSpeed, 0);
    }
    else if(dir == BACK)
    {
      Motors(0, attackSpeed, 0, attackSpeed);
    }
    //curve attacks
    else if(dir == FRONT_LEFT)
    {
      Motors(curveSubSpeed, 0, curveMainSpeed, 0);
    }
    else if(dir == FRONT_RIGHT)
    {
      Motors(curveMainSpeed, 0, curveSubSpeed, 0);
    }
}
void RotateInPlace(MoveDirection dir)
{
    if(dir == LEFT)
    {
      Motors(0, rotateInPlaceSpeed, rotateInPlaceSpeed, 0);
    }
    else if(dir == RIGHT)
    {
      Motors(rotateInPlaceSpeed, 0, 0, rotateInPlaceSpeed);
    }
}


void ScanSensors()
{
  for (uint8_t channel = 0; channel < NUM_SENSORS; channel++)
  {
    if (tca.selectChannel(channel) != 0)
    {
      distances[channel] = 1000;
      BTSerial.print("ERROR selecting channel ");
      BTSerial.println(channel);
      continue;
    }

    VL53L0X_RangingMeasurementData_t measure;
    sensor.rangingTest(&measure, false);

    if (measure.RangeStatus == 0)
    {
      if (measure.RangeMilliMeter == 0)
      {
        distances[channel] = 1000; // if the sensor cant see, set distance to 1000 instead of 0
        BTSerial.print("Range is 0 on channel ");
        BTSerial.println(channel);
      }
      else
      {
        distances[channel] = measure.RangeMilliMeter;
        BTSerial.print("Distance on sensor ");
        BTSerial.print(channel);
        BTSerial.print(": ");
        BTSerial.println(measure.RangeMilliMeter);
      }
    }
    else
    {
      distances[channel] = 1000;
      BTSerial.print("No valid measurement on sensor ");
      BTSerial.println(channel);
    }
  }
  // far 60 - 750
  enemyLeftFar = distances[0] > 60 && distances[0] < 750;
  enemyFrontLeftFar = distances[1] > 60 && distances[1] < 750;
  enemyRightFar = distances[3] > 60 && distances[3] < 750;
  enemyFrontRightFar = distances[2] > 60 && distances[2] < 750;
  enemyFrontFar = enemyFrontLeftFar && enemyFrontRightFar;
  // close 1 - 60
  enemyLeftClose = distances[0] <= 60 && distances[0] > 1;
  enemyFrontLeftClose = distances[1] <= 60 && distances[1] > 1;
  enemyRightClose = distances[3] <= 60 && distances[3] > 1;
  enemyFrontRightClose = distances[2] <= 60 && distances[2] > 1;
  enemyFrontClose = enemyFrontLeftClose && enemyFrontRightClose;
  // clear > 750
  enemyRightClear = distances[3] > 750;
  enemyLeftClear = distances[0] > 750;
  enemyFrontLeftClear = distances[1] > 750;
  enemyFrontRightClear = distances[2] > 750;
  enemyFrontClear = enemyFrontLeftClear && enemyFrontRightClear;
  enemySidesClear = enemyRightClear && enemyLeftClear;
  enemyRightSideClear = enemyRightClear && enemyFrontRightClear;
  enemyLeftSideClear = enemyLeftClear && enemyFrontLeftClear;
}

void setup()
{
  pinMode(motorL1, OUTPUT);
  pinMode(motorL2, OUTPUT);
  pinMode(motorR1, OUTPUT);
  pinMode(motorR2, OUTPUT);
  pinMode(lineSensorL, INPUT);
  pinMode(lineSensorR, INPUT);
  pinMode(buttonPin, INPUT_PULLUP); // is high on default, low when pressed

  BTSerial.begin(9600);
  Wire.begin();
  Wire.setClock(400000);
  BTSerial.println("setting up...");

  
  for (uint8_t channel = 0; channel < NUM_SENSORS; channel++) //checking sensors on all channels of the multiplexer
  {
    BTSerial.print("scanning sensor on channel ");
    BTSerial.print(channel);
    BTSerial.println("... ");
    bool isChannelSelected = (tca.selectChannel(channel) == 0);

    if (isChannelSelected) // channel selected successfully
    { 
      BTSerial.println("OK channel selected");
      if (sensor.begin())
      {
        BTSerial.println("OK sensor found");  
        sensor.configSensor(Adafruit_VL53L0X::VL53L0X_SENSE_HIGH_SPEED);
        VL53L0X_RangingMeasurementData_t measure;
        sensor.rangingTest(&measure, false);
        BTSerial.print("Distance on sensor "); BTSerial.print(channel); BTSerial.print(": "); BTSerial.println(measure.RangeMilliMeter);
      }
      else
      {
        BTSerial.println("ERROR finding sensor");
      }
    }
    else BTSerial.println("ERROR selecting channel");
  }
}

void loop()
{
  // TODO add debugs
  // TODO maybe for optimization do it like an event or smth, look it up
  buttonState = digitalRead(buttonPin);

  // TODO maybe separate function for button handling, but for now its ok
  if (buttonState == LOW && lastButtonState == HIGH) // if button was just pressed (active is low)
  {
    delay(50); // Затримка для антидребезгу кнопки
    if (!isProgramRunning)
    {
      BTSerial.println("starting program...");
      delay(4950); // Затримка для входу в ринг, 5 секунд
      BTSerial.println("START");
      isProgramRunning = true; // starting the program
    }
    else
    {
      BTSerial.println("STOP");
      isProgramRunning = false; // stopping the program
      Motors(0, 0, 0, 0);
    }
  }
  lastButtonState = buttonState;

  if (isProgramRunning)
  {
    /*
    Идеи 
    Если враг долго спереди близко, то отьехать назад и удариться в него опять
    */

    // black is high, white is low, edge is white
    int sensorL = digitalRead(lineSensorL);
    int sensorR = digitalRead(lineSensorR);

    ScanSensors();
    

    if (sensorL == 0 && sensorR == 0)
    {
      state = AVOID_EDGE;
      moveDirection = BACK;
    }
    else if (sensorL == 0 && sensorR == 1)
    {
      state = AVOID_EDGE;
      moveDirection = BACK_RIGHT;
    }
    else if (sensorL == 1 && sensorR == 0)
    {
      state = AVOID_EDGE;
      moveDirection = BACK_LEFT;
    }
    else
    {
      state = SEARCHING;
    }

    switch (state)
    {
    case SEARCHING:

      if ((enemyFrontFar || enemyFrontClose) && enemySidesClear) // if enemy is in front, go forward
      {
        moveDirection = FRONT;
        state = ATTACK;
      }
      else if (enemyFrontLeftClose && enemyLeftClear && enemyRightSideClear) // if enemy is in front left only, do a curved attack
      {
        moveDirection = FRONT_LEFT;
        state = ATTACK;
      }
      else if (enemyFrontRightClose && enemyRightClear && enemyLeftSideClear) // if enemy is in front right, curve attack
      {
        moveDirection = FRONT_RIGHT;
        state = ATTACK;
      }

      else if ((enemyLeftFar || enemyLeftClose || enemyFrontLeftFar) && enemyRightSideClear) // if enemy is in left, rotate towards him
      {
        moveDirection = LEFT;
        state = ROTATE_IN_PLACE;
      }
      else if ((enemyRightFar || enemyRightClose || enemyFrontRightFar) && enemyLeftSideClear) // if enemy is in right, rotate towards him
      {
        moveDirection = RIGHT;
        state = ROTATE_IN_PLACE;
      }
      
      else if(enemyRightSideClear && enemyLeftSideClear) // if no enemy is detected, forward 
      {
        moveDirection = FRONT;
        state = ATTACK;
      }
      else
      {
        BTSerial.println("LLLL bug searching for enemy");
      }
      break;

    case AVOID_EDGE:
      Move(BACK); // back up a bit
      delay(100);
      if(moveDirection == BACK)
      {
        state = ROTATE_IN_PLACE;
        moveDirection = RIGHT;
      }
      else if(moveDirection == BACK_RIGHT)
      {
        state = ROTATE_IN_PLACE;
        moveDirection = RIGHT;
      }
      else if(moveDirection == BACK_LEFT)
      {
        state = ROTATE_IN_PLACE;
        moveDirection = LEFT;
      }
      break;

    case ROTATE_IN_PLACE:
      RotateInPlace(moveDirection);
      delay(50);
      state = SEARCHING;
      break;

    case ATTACK:
      Move(moveDirection);
      delay(50);
      state = SEARCHING;
      break;
    }
    // maybe delay at the end of loop for smoother movement
  }
}