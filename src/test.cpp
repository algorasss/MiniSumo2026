#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "ClosedCube_TCA9548A.h"

#define TCA9548A_I2C_ADDRESS 0x70
ClosedCube::Wired::TCA9548A tca(TCA9548A_I2C_ADDRESS); //init the multiplexer right away
Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

const uint8_t NUM_SENSORS = 5; // we have 5 sensors

const int motorA1 = PB13;     // пін для правого мотора вперед
const int motorA2 = PB14;     // пін для правого мотора назад
const int motorB1 = PA6;      // пін для лівого мотора вперед
const int motorB2 = PA5;      // пін для лівого мотора назад

const int lineSensor1 = PA15; //
const int lineSensor2 = PB3;  //

const int buttonPin = PC15;

bool buttonState = false;
bool lastButtonState = false;

uint16_t distances[NUM_SENSORS]; // start from 1 not 0

int timesRan = 0;

void StopMotors()
{
    analogWrite(motorA1, 0);
    analogWrite(motorA2, 0);
    analogWrite(motorB1, 0);
    analogWrite(motorB2, 0);
}
void TestMotor()
{
    analogWrite(motorA1, 100);
    analogWrite(motorA2, 0);
    delay(1000);
    analogWrite(motorA1, 150);
    analogWrite(motorA2, 0);
    delay(1000);
    analogWrite(motorA1, 190);
    analogWrite(motorA2, 0);
    SerialUSB.println("fast right forward");

    delay(500);
    StopMotors();
    delay(1000);

    analogWrite(motorA1, 0);
    analogWrite(motorA2, 100);
    delay(1000);
    analogWrite(motorA1, 0);
    analogWrite(motorA2, 150);
    delay(1000);
    analogWrite(motorA1, 0);
    analogWrite(motorA2, 190);
    SerialUSB.println("fast right backward");

    delay(500);
    StopMotors();
    delay(1000);


    analogWrite(motorB1, 100);
    analogWrite(motorB2, 0);
    delay(1000);
    analogWrite(motorB1, 150);
    analogWrite(motorB2, 0);
    delay(1000);
    analogWrite(motorB1, 190);
    analogWrite(motorB2, 0);
    SerialUSB.println("fast left forward");

    delay(500);
    StopMotors();
    delay(1000);

    analogWrite(motorB1, 0);
    analogWrite(motorB2, 100);
    delay(1000);
    analogWrite(motorB1, 0);
    analogWrite(motorB2, 150);
    delay(1000);
    analogWrite(motorB1, 0);
    analogWrite(motorB2, 190);
    SerialUSB.println("fast left backward");

    delay(500);
    StopMotors();
    delay(1000);
}


void TestSensors()
{
    for (uint8_t channel = 1; channel < NUM_SENSORS + 1; channel++) // checking sensors on all channels of the multiplexer
    {

        SerialUSB.print(channel);

        tca.selectChannel(channel);
        delay(2);
        VL53L0X_RangingMeasurementData_t measure;
        sensor.rangingTest(&measure, true);
        // if (measure.RangeMilliMeter < 50)
        // {
            SerialUSB.print(" range: ");
            SerialUSB.print(measure.RangeMilliMeter);
            SerialUSB.print(" max range: ");
            SerialUSB.print(measure.RangeDMaxMilliMeter);
            SerialUSB.print(" fractional range: ");
            SerialUSB.print(measure.RangeFractionalPart);
            SerialUSB.print(" time stamp: ");
            SerialUSB.println(measure.TimeStamp);
        //}
    }
}
void InitSensors()
{
    for (uint8_t channel = 1; channel < NUM_SENSORS + 1; channel++) // checking sensors on all channels of the multiplexer
    {
        SerialUSB.print("SETUP sensor on channel ");
        SerialUSB.print(channel);
        SerialUSB.println("... ");
        tca.selectChannel(channel);
        delay(2);
        if (sensor.begin())
        {
            SerialUSB.println("s OK");
        }
        else
        {
            SerialUSB.println("s FAILED");
        }
    }
}


void TestLineSensors(bool isFirst)
{
    //maybe after detecting a white line, delay cuz its tweaking
    if(isFirst)
    {
        // for(int i = 0; i < 5; i++)
        // {
        //     bool sensor1 = digitalRead(lineSensor1);
        //     SerialUSB.print("sensor 1 value: ");
        //     SerialUSB.println(sensor1);
        //     //delay(1000);
        // }
        int sensor1 = digitalRead(lineSensor1);
        SerialUSB.print("sensor 1 value: ");
        SerialUSB.println(sensor1);
        if(sensor1 == 1)
        {
            SerialUSB.println("WWWW");
        }
    }
    else
    {
        int sensor2 = digitalRead(lineSensor2);
        SerialUSB.print("sensor 2 value: ");
        SerialUSB.println(sensor2);
        if(sensor2 == 1)
        {
            SerialUSB.println("WWWW");
        }
    }
}

void setup()
{
    pinMode(motorA1, OUTPUT);
    pinMode(motorA2, OUTPUT);
    pinMode(motorB1, OUTPUT);
    pinMode(motorB2, OUTPUT);

    pinMode(lineSensor1, INPUT);
    pinMode(lineSensor2, INPUT);

    pinMode(buttonPin, INPUT_PULLUP);

    SerialUSB.begin(115200);
    SerialUSB.println("setting up...");

    Wire.begin();
    Wire.setClock(400000);

    InitSensors();
}
void loop()
{
    buttonState = digitalRead(buttonPin);
    if (buttonState == LOW && lastButtonState == HIGH)
    {
        delay(50);
        SerialUSB.println("Re-initializing sensors...");
        Wire.end();
        delay(100);
        Wire.begin();
        Wire.setClock(400000);
        InitSensors();
        //delay(1000);
    }
    lastButtonState = buttonState;
    //test motors code
    // if(timesRan < 5)
    // {
    //     timesRan++;
    //     TestMotor();
    // }

    //test line sensors
    //TestLineSensors(false); //line sensor

    //test enemy sensors
    TestSensors();
}
//make program start on button click, if button is clicked while code is running, switch to the next mode, if it is held for 2 second, stop