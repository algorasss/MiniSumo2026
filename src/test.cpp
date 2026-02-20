#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "ClosedCube_TCA9548A.h"

#define TCA9548A_I2C_ADDRESS 0x70
HardwareSerial BTSerial(2);
ClosedCube::Wired::TCA9548A tca; // init the multiplexer right away
Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

const uint8_t NUM_SENSORS = 8; // we have 5 sensors

const int motorA1 = PB13; // пін для правого мотора вперед
const int motorA2 = PB14; // пін для правого мотора назад
const int motorB1 = PA6;  // пін для лівого мотора вперед
const int motorB2 = PA5;  // пін для лівого мотора назад

const int lineSensor1 = PA15; //
const int lineSensor2 = PB3;  //

const int buttonPin = PC15;

bool buttonState = false;
bool lastButtonState = false;

uint16_t distances[NUM_SENSORS]; // start from 1 not 0

int timesRan = 0;

void printAddress(uint8_t address)  {
	BTSerial.print("0x");
	if (address<0x10) {
		BTSerial.print("0");
	}
	BTSerial.println(address,HEX);
}

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
}
void InitSensorsTemp()
{
    tca.address(TCA9548A_I2C_ADDRESS);

    uint8_t returnCode = 0;
    uint8_t address;
    uint8_t numberOfDevices;

    for (uint8_t channel = 0; channel < TCA9548A_MAX_CHANNELS; channel++)
    {
        BTSerial.print("Scanning channel #");
        BTSerial.print(channel);
        BTSerial.println("...");

        returnCode = tca.selectChannel(channel);
        numberOfDevices = 0;

        if (returnCode == 0)
        {
            // for (address = 0x01; address < 0x7F; address++)
            // {
                Wire.beginTransmission(address);
                Wire.write(1 << channel);
                returnCode = Wire.endTransmission();
                BTSerial.println(returnCode);
                if (returnCode == 0)
                {
                    BTSerial.print("I2C device = ");
                    printAddress(address);

                    numberOfDevices++;
                }
                else if (returnCode == 4)
                {
                    BTSerial.print("Unknown error at ");
                    printAddress(address);
                }
                else
                {
                    BTSerial.print(returnCode);
                    BTSerial.println("Fail");
                }
                if (numberOfDevices == 0)
                    BTSerial.println("No I2C devices found\n");
                else
                {
                    BTSerial.print("Error scan channel (Code:");
                    BTSerial.print(returnCode);
                    BTSerial.println(")");
                }
            //}
            BTSerial.println("where am i");
        } else
        {
            BTSerial.print("Error scan channel (Code:");
            BTSerial.print(returnCode);
            BTSerial.println(")");
        }
    }
}
void InitSensors()
{
    uint8_t returnCode = 0;
    uint8_t numberOfDevices;

    for (uint8_t channel = 0; channel < NUM_SENSORS; channel++)
    {
        BTSerial.print("Scanning channel #");
        BTSerial.print(channel);
        BTSerial.println("...");

        for(uint8_t address = 0x01; address < 0x7F; address++)
        {
            BTSerial.print("Trying address: ");
            BTSerial.println(address, HEX);
            Wire.beginTransmission(address);
            Wire.write(1 << channel);
            returnCode = Wire.endTransmission();
            BTSerial.println(returnCode);
        }
    }
}

void TestLineSensors(bool isFirst)
{
    // maybe after detecting a white line, delay cuz its tweaking
    if (isFirst)
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
        if (sensor1 == 1)
        {
            SerialUSB.println("WWWW");
        }
    }
    else
    {
        int sensor2 = digitalRead(lineSensor2);
        SerialUSB.print("sensor 2 value: ");
        SerialUSB.println(sensor2);
        if (sensor2 == 1)
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

    BTSerial.begin(9600);
    BTSerial.println("setting up...");

    Wire.begin(PB7, PB6); // SDA, SCL
    Wire.setClock(100000);

    InitSensors();
}
void loop()
{
    //BTSerial.println("looping...");
    //delay(100);
    // buttonState = digitalRead(buttonPin);
    // if (buttonState == LOW && lastButtonState == HIGH)
    // {
    //     delay(50);
    //     SerialUSB.println("Re-initializing sensors...");
    //     Wire.end();
    //     delay(100);
    //     Wire.begin();
    //     Wire.setClock(400000);
    //     InitSensors();
    //     // delay(1000);
    // }
    // lastButtonState = buttonState;
    // test motors code
    //  if(timesRan < 5)
    //  {
    //      timesRan++;
    //      TestMotor();
    //  }

    // test line sensors
    // TestLineSensors(false); //line sensor

    // test enemy sensors
    //TestSensors();
}
// make program start on button click, if button is clicked while code is running, switch to the next mode, if it is held for 2 second, stop