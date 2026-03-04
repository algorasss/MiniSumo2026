#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VL53L0X.h"
#include "ClosedCube_TCA9548A.h"

#define TCA9548A_I2C_ADDRESS 0x70
HardwareSerial BTSerial(2);
ClosedCube::Wired::TCA9548A tca; // init the multiplexer right away
Adafruit_VL53L0X sensor = Adafruit_VL53L0X();

const uint8_t NUM_SENSORS = 4; // we have 4 sensors

const int motorA1 = PB13; // пін для правого мотора вперед
const int motorA2 = PB14; // пін для правого мотора назад
const int motorB1 = PA6;  // пін для лівого мотора вперед
const int motorB2 = PA5;  // пін для лівого мотора назад

const int lineSensor1 = PA15; //
const int lineSensor2 = PB3;  //

const int buttonPin = PC15;

bool buttonState = false;
bool lastButtonState = false;

uint16_t distances[NUM_SENSORS];

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
            distances[channel] = measure.RangeMilliMeter;
            BTSerial.print("Distance on sensor ");
            BTSerial.print(channel);
            BTSerial.print(": ");
            BTSerial.println(measure.RangeMilliMeter);
        }
        else
        {
            distances[channel] = 1000;
            BTSerial.print("Sensor error ");
            BTSerial.println(measure.RangeStatus);
        }
    }
}
void InitSensorsAlt()
{
    tca.address(TCA9548A_I2C_ADDRESS);

    for (uint8_t channel = 0; channel < NUM_SENSORS; channel++)
    {
        BTSerial.print("Scanning channel #");
        BTSerial.println(channel);

        uint8_t returnCode = tca.selectChannel(channel);
        if (returnCode != 0)
        {
            BTSerial.print("ERROR selecting channel, code: ");
            BTSerial.println(returnCode);
            continue;
        }

        uint8_t numberOfDevices = 0;
        for (uint8_t address = 0x01; address < 0x7F; address++)
        {
            Wire.beginTransmission(address);
            returnCode = Wire.endTransmission();
            if (returnCode == 0)
            {
                BTSerial.print("Found device at 0x");
                BTSerial.println(address, HEX);
                numberOfDevices++;
            }
        }

        if (numberOfDevices == 0)
            BTSerial.println("No I2C devices found on this channel");
    }
}
void InitSensors()
{
    tca.address(TCA9548A_I2C_ADDRESS);
    for (uint8_t channel = 0; channel < NUM_SENSORS; channel++)
    {
        BTSerial.print("Сканування сенсора на каналі ");
        BTSerial.print(channel);
        BTSerial.print("... ");

        if (tca.selectChannel(channel) == 0)
        {
            BTSerial.println("канал вибрано, ініціалізація сенсора...");
            if (!sensor.begin())
            {
                BTSerial.println("❌ Не знайдено VL53L0X");
            }
            else
            {
                BTSerial.println("✅ Сенсор знайдено.");
            }
        }
        else
        {
            BTSerial.println("❌ Помилка вибору каналу");
        }
    }
}


void TestLineSensors(bool isFirst)
{
    // maybe after detecting a white line, delay cuz its tweaking
    if (isFirst)
    {
        int sensor1 = digitalRead(lineSensor1);
        SerialUSB.print("sensor 1 value: ");
        SerialUSB.println(sensor1);
        if (sensor1 == 1)
        {
            SerialUSB.println("1");
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

    Wire.begin(); // SDA, SCL
    Wire.setClock(400000);

    InitSensors();
}
void loop()
{
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
    //delay(500);
}