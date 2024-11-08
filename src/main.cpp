/**
 * @file display.ino
 * @author SeanKwok (shaoxiang@m5stack.com)
 * @brief M5StickCPlus2 Display Test
 * @version 0.1
 * @date 2023-12-09
 *
 *
 * @Hardwares: M5StickCPlus2
 * @Platform Version: Arduino M5Stack Board Manager v2.0.9
 * @Dependent Library:
 * M5GFX: https://github.com/m5stack/M5GFX
 * M5Unified: https://github.com/m5stack/M5Unified
 * M5StickCPlus2: https://github.com/m5stack/M5StickCPlus2
 */

#include <Arduino.h>
#include "M5StickCPlus2.h"
#include <Wire.h>

// Function to convert voltage to percentage
int voltageToPercentage(int voltage) {
    // Assuming 3.7V is 0% and 4.2V is 100%
    int percentage = map(voltage, 3700, 4200, 0, 100);
    return constrain(percentage, 0, 100);
}

void setup() {
    auto cfg = M5.config();
    StickCP2.begin(cfg);
    StickCP2.Display.setRotation(1);
    StickCP2.Display.setTextColor(GREEN);
    StickCP2.Display.setTextDatum(middle_center);
    StickCP2.Display.setTextFont(&fonts::Orbitron_Light_24);
    StickCP2.Display.setTextSize(1);

    pinMode(32, INPUT_PULLDOWN);  // Enable internal pull-down resistor for pin 32.
}

void loop() {
    StickCP2.Display.clear();
    int vol = StickCP2.Power.getBatteryVoltage();
    int percentage = voltageToPercentage(vol);
    StickCP2.Display.setCursor(10, 30);
    StickCP2.Display.printf("BAT: %dmv", vol);
    StickCP2.Display.setCursor(10, 60);
    StickCP2.Display.printf("BAT: %d%%\n", percentage);

    StickCP2.Display.setCursor(10, 90);
    StickCP2.Display.printf("Analog:%d\n", analogRead(33));
    StickCP2.Display.setCursor(10, 120);
    StickCP2.Display.printf("Digital:%d\n", digitalRead(32));

    delay(1000);
}