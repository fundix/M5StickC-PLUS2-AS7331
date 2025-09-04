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
#include "M5GFX.h"

SfeAS7331ArdI2C myUVSensor;

M5Canvas canvas(&StickCP2.Display);

float maxuva = 0;
float maxuvb = 0;
float maxuvc = 0;

#define BUTTON_A GPIO_NUM_37
#define BUTTON_B GPIO_NUM_39

#define TAG "UV"

void buttonTask(void *pvParameters);

// Function to convert voltage (in mV) to percentage
int voltageToPercentage(int voltage_mV)
{
    // Map 3700 mV -> 0% and 4200 mV -> 100%
    float pct = ((float)voltage_mV - 3700.0f) / (4200.0f - 3700.0f) * 100.0f;
    if (pct < 0.0f)
        pct = 0.0f;
    if (pct > 100.0f)
        pct = 100.0f;
    return (int)(pct + 0.5f); // round to nearest int
}

int getStableBatteryPercentage()
{
    const int numReadings = 10;
    int total_mV = 0;

    for (int i = 0; i < numReadings; i++)
    {
        total_mV += (int)StickCP2.Power.getBatteryVoltage(); // mV per M5Unified
        delay(10);
    }

    int average_mV = total_mV / numReadings;
    return voltageToPercentage(average_mV);
}

bool displayPaused = false;

void setup()
{
    auto cfg = M5.config();
    // cfg.board = board_M5StickCPlus2;
    StickCP2.begin(cfg);

    Serial.begin(115200);
    delay(2500);
    Serial.println("M5StickCPlus2 initialized");

    StickCP2.Display.setBrightness(100);
    StickCP2.Display.setRotation(1);
    canvas.setTextColor(WHITE, BLACK);
    canvas.setTextDatum(middle_center);
    canvas.setFont(&fonts::FreeSans12pt7b);
    canvas.setTextSize(3);

    canvas.createSprite(StickCP2.Display.width(), StickCP2.Display.height());
    canvas.fillScreen(BLACK);

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

    if (ksfTkErrOk != myUVSensor.setBreakTime(112))
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
    if (ksfTkErrOk != myUVSensor.setStartState(true))
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

float calculateUVIndex(float uva, float uvb, float uvc)
{
    float k1 = 0.1;  // Váha pro UVA
    float k2 = 0.7;  // Váha pro UVB (hlavní složka)
    float k3 = 0.05; // Váha pro UVC (minimální vliv)

    float uv_index = (k1 * uva) + (k2 * uvb) + (k3 * uvc);
    return uv_index;
}

void drawUVScale(M5Canvas &canvas, float uvIndex)
{
    // Definice pozice a velikosti stupnice
    const int x = 10;
    const int y = 18;
    const int width = 220;
    const int height = 10;

    // Prahové hodnoty (vzhledem k citlivosti při Stargardtově chorobě)
    const float safeThreshold = 2.0;    // Pod touto hodnotou je riziko minimální
    const float cautionThreshold = 3.0; // Meziprodukt - doporučená ochrana
    const float maxThreshold = 6.0;     // Maximální hodnota stupnice

    // Nakreslíme obrys stupnice
    canvas.drawRect(x, y, width, height, WHITE);

    // Omezíme aktuální UV index pro výpočet vyplnění (aby nepřekročil maxThreshold)
    float displayValue = (uvIndex > maxThreshold) ? maxThreshold : uvIndex;

    // Vypočítáme šířku vyplněné části stupnice
    int fillWidth = (int)((displayValue / maxThreshold) * width);

    // Vybereme barvu podle hodnoty UV indexu:
    // - Pod safeThreshold = zelená (bez nutnosti ochrany)
    // - Mezi safeThreshold a cautionThreshold = žlutá (varování)
    // - Nad cautionThreshold = červená (vysoké riziko)

    uint16_t fillColor;
    if (uvIndex < safeThreshold)
    {
        fillColor = GREEN;
    }
    else if (uvIndex < cautionThreshold)
    {
        fillColor = YELLOW;
    }
    else
    {
        fillColor = RED;
    }

    // Vyplníme část stupnice
    canvas.fillRect(x + 1, y + 1, fillWidth - 2, height - 2, fillColor);

    // Vykreslíme text s aktuálním UV indexem
    canvas.setTextSize(0.5);
    canvas.setCursor(x + 100, y + height + 7);
    canvas.printf("UV index: %.1f", uvIndex);

    // Přidáme textovou informaci o nutnosti ochrany
    canvas.setCursor(x, y + height + 7);
    if (uvIndex >= safeThreshold)
    {
        canvas.print("vezmi Bryle");
    }
    else
    {
        canvas.print("Bez ochrany");
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

        canvas.fillScreen(BLACK);
        canvas.setTextSize(0.5);
        // canvas.fillRect(0, 0, 240, 135, BLACK);
        int percentage = getStableBatteryPercentage();
        canvas.setCursor(200, 10);
        canvas.printf("%d%%\n", percentage);
        canvas.setCursor(11, 10);
        canvas.setTextSize(0.5);
        int vbat_mV = StickCP2.Power.getBatteryVoltage(); // mV
        canvas.printf("%.2f V\n", vbat_mV / 1000.0f);
        // canvas.printf("%d mV\n", vbat_mV); // uncomment to see raw

        if (ksfTkErrOk != myUVSensor.readAllUV())
        {
            Serial.println("Error reading UV.");
        }

        float uva = myUVSensor.getUVA();
        float uvb = myUVSensor.getUVB();
        float uvc = myUVSensor.getUVC();

        float spektralni_faktor = 0.15;
        float uv_index = (uvb * spektralni_faktor) / 25.0;
        float uvi_total = ((uva + uvb) * spektralni_faktor) / 25.0;

        if (uva > maxuva)
            maxuva = uva;
        if (uvb > maxuvb)
            maxuvb = uvb;
        if (uvc > maxuvc)
            maxuvc = uvc;

        float uvIndex = calculateUVIndex(uva, uvb, uvc) / 25;

        ESP_LOGI(TAG, "uvIndex: %.2f", uvIndex);

        uint8_t y = 75;
        uint8_t line = 16;

        // canvas.setTextSize(1);
        // canvas.setCursor(10, 35);
        // canvas.printf("UV-I: %.0f\n", uv_index);
        // canvas.setCursor(100, 25);
        // canvas.printf("UV-T: %.0f\n", uvi_total);

        canvas.setTextSize(0.7);
        canvas.setCursor(10, y);
        canvas.printf("UV-A: %.0f\n", uva);
        canvas.setCursor(10, y + line);
        canvas.printf("UV-B: %.0f\n", uvb);
        canvas.setCursor(10, y + 2 * line);
        canvas.printf("UV-C: %.0f\n", uvc);
        canvas.setCursor(10, y + 3 * line);
        canvas.printf("Temp: %.1f C", myUVSensor.getTemp());

        canvas.setCursor(140, y - line);
        canvas.print("Max:");
        canvas.setCursor(140, y);
        canvas.printf("%.0f", maxuva);
        canvas.setCursor(140, y + line);
        canvas.printf("%.0f", maxuvb);
        canvas.setCursor(140, y + 2 * line);
        canvas.printf("%.0f", maxuvc);

        drawUVScale(canvas, uvIndex);

        canvas.pushSprite(0, 0);
    }

    delay(500);
}