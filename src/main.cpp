/**
 * @Hardwares: M5StickCPlus2
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5StickCPlus2: https://github.com/m5stack/M5StickCPlus2
 */

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_AS7331.h>
#include "M5StickCPlus2.h"

SfeAS7331ArdI2C myUVSensor;

float maxuva = 0;
float maxuvb = 0;
float maxuvc = 0;

#define BUTTON_A GPIO_NUM_37
#define BUTTON_B GPIO_NUM_39

void buttonTask(void *pvParameters);

// Function to convert voltage to percentage
int voltageToPercentage(int voltage)
{
    // Assuming 3.7V is 0% and 4.2V is 100%
    int percentage = map(voltage, 3700, 4200, 0, 100);
    return constrain(percentage, 0, 100);
}

int getStableBatteryPercentage()
{
    const int numReadings = 10;
    int totalVoltage = 0;

    for (int i = 0; i < numReadings; i++)
    {
        totalVoltage += StickCP2.Power.getBatteryVoltage();
        delay(10); // Small delay between readings
    }

    int averageVoltage = totalVoltage / numReadings;
    return voltageToPercentage(averageVoltage);
}

bool displayPaused = false;

void setup()
{
    auto cfg = M5.config();
    StickCP2.begin(cfg);

    Serial.begin(115200);
    delay(2500);
    Serial.println("M5StickCPlus2 initialized");

    StickCP2.Display.setBrightness(25);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextColor(RED, BLACK);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.setFont(&fonts::FreeSans12pt7b);
    StickCP2.Display.setTextSize(1);

    // pinMode(32, INPUT_PULLUP); // Enable internal pull-down resistor for pin 32.
    // pinMode(33, INPUT_PULLDOWN);
    pinMode(19, OUTPUT); // Set pin 19 as an output.

    Wire.begin(32, 33);

    if (myUVSensor.begin() == false)
    {
        Serial.println("Sensor failed to begin. Please check your wiring!");
        Serial.println("Halting...");
        while (1)
            ;
    }

    if (kSTkErrOk != myUVSensor.setBreakTime(250))
    {
        Serial.println("Sensor did not set break time properly.");
        Serial.println("Halting...");
        while (1)
            ;
    }

    // Set measurement mode and change device operating mode to measure.
    if (myUVSensor.prepareMeasurement(MEAS_MODE_CONT) == false)
    {
        Serial.println("Sensor did not get set properly.");
        Serial.println("Spinning...");
        while (1)
            ;
    }

    Serial.println("Set mode to continuous. Starting measurement...");

    // Begin measurement.
    if (kSTkErrOk != myUVSensor.setStartState(true))
        Serial.println("Error starting reading!");

    xTaskCreate(
        buttonTask,    // Function that should be called
        "Button Task", // Name of the task (for debugging)
        4096,          // Stack size (in words, not bytes)
        NULL,          // Parameter to pass to the function
        1,             // Task priority
        NULL           // Task handle
    );
}

void buttonTask(void *pvParameters)
{
    // Configure GPIO37 for the button
    pinMode(BUTTON_A, INPUT_PULLUP); // Button is active LOW
    pinMode(BUTTON_B, INPUT_PULLUP); // Button is active LOW

    bool buttonAPressed = false;
    bool buttonBPressed = false;

    for (;;)
    {
        if (digitalRead(BUTTON_A) == LOW && !buttonAPressed) // GPIO37 is active LOW
        {
            StickCP2.Speaker.tone(8000, 20);
            maxuva = 0;
            maxuvb = 0;
            maxuvc = 0;
            buttonAPressed = true;
        }
        if (digitalRead(BUTTON_A) == HIGH && buttonAPressed)
        {
            buttonAPressed = false;
        }

        if (digitalRead(BUTTON_B) == LOW && !buttonBPressed) // GPIO39 is active LOW
        {
            StickCP2.Speaker.tone(5000, 20);

            static int brightness = 25;
            brightness += 25;
            if (brightness > 100)
            {
                brightness = 25;
            }
            StickCP2.Display.setBrightness(brightness);
            buttonBPressed = true;
        }
        if (digitalRead(BUTTON_B) == HIGH && buttonBPressed)
        {
            buttonBPressed = false;
        }

        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void loop()
{
    StickCP2.update(); // Update button states

    // if (StickCP2.BtnB.wasClicked())
    // {
    //     StickCP2.Display.clear();
    //     // StickCP2.Speaker.tone(8000, 20);
    // }

    // if (StickCP2.BtnA.wasPressed())
    // {
    //     displayPaused = !displayPaused; // Toggle display update state
    //     maxuva = 0;
    //     maxuvb = 0;
    //     maxuvc = 0;
    // }

    // if (StickCP2.BtnB.wasReleased())
    // {
    //     StickCP2.Speaker.tone(8000, 20);
    //     StickCP2.Display.clear();
    //     StickCP2.Display.drawString("A Btn Released",
    //                                 StickCP2.Display.width() / 2,
    //                                 StickCP2.Display.height() / 2);
    // }

    if (!displayPaused)
    {
        StickCP2.Display.setTextSize(1);

        // StickCP2.Display.fillRect(0, 0, 240, 135, BLACK);
        int percentage = getStableBatteryPercentage();
        StickCP2.Display.setCursor(10, 20);
        StickCP2.Display.printf("Bat: %d%%\n", percentage);
        StickCP2.Display.setCursor(11, 40);
        StickCP2.Display.setTextSize(0.7);
        StickCP2.Display.printf("%dmV\n", StickCP2.Power.getBatteryVoltage());
        StickCP2.Display.setTextSize(1);

        if (kSTkErrOk != myUVSensor.readAll())
            Serial.println("Error reading UV.");

        // Serial.print("UVA:");
        // Serial.print(myUVSensor.getUVA());
        // Serial.print(" UVB:");
        // Serial.print(myUVSensor.getUVB());
        // Serial.print(" UVC:");
        // Serial.print(myUVSensor.getUVC());
        // Serial.print(" temp:");
        // Serial.println(myUVSensor.getTemp());

        float uva = myUVSensor.getUVA();
        float uvb = myUVSensor.getUVB();
        float uvc = myUVSensor.getUVC();

        if (uva > maxuva)
            maxuva = uva;
        if (uvb > maxuvb)
            maxuvb = uvb;
        if (uvc > maxuvc)
            maxuvc = uvc;

        StickCP2.Display.setTextSize(0.7);
        StickCP2.Display.setCursor(10, 65);
        StickCP2.Display.printf("UV-A: %.1f\n", uva);
        StickCP2.Display.setCursor(10, 84);
        StickCP2.Display.printf("UV-B: %.1f\n", uvb);
        StickCP2.Display.setCursor(10, 102);
        StickCP2.Display.printf("UV-C: %.1f\n", uvc);
        StickCP2.Display.setCursor(10, 118);
        StickCP2.Display.printf("Temp: %.1f C", myUVSensor.getTemp());

        StickCP2.Display.setCursor(140, 45);
        StickCP2.Display.print("Max:");
        StickCP2.Display.setCursor(140, 65);
        StickCP2.Display.printf("%.1f", maxuva);
        StickCP2.Display.setCursor(140, 84);
        StickCP2.Display.printf("%.1f", maxuvb);
        StickCP2.Display.setCursor(140, 102);
        StickCP2.Display.printf("%.1f", maxuvc);
    }

    delay(1000);
}