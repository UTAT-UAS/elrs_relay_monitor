#include <Arduino.h>
#include "ADCReader.hpp"

ADCReader batteryADC(GPIO_NUM_14, ADC2_CHANNEL_6);

constexpr float BATTERY_SCALE_M = 1.0f;

constexpr float BAT_WARN_THRESHOLD = 7.0f;
constexpr float BAT_CRIT_THRESHOLD = 6.0f;

enum BatteryState {BAT_OK, BAT_WARN, BAT_CRIT};

BatteryState lastBatState = BAT_OK;

void setup() {
    Serial.begin(115200);
    CrsfLink.begin(baudRate, SERIAL_8N1, 16, -1, invertOptions);
    batteryADC.begin();
}

float measureBatteryVoltage() {
  float Vadc = batteryADC.readAveragedVoltage(); 
  float Vin = Vadc * BATTERY_SCALE_M;
  Serial.printf("[BAT] Vadc = %.3f V, Vin ≈ %.3f V\n", Vadc, Vin); //For debugging
  return Vin;
}

void handleBatteryStatus(float Vin) {
    BatteryState newState;

    if (Vin < BAT_CRIT_THRESHOLD) {
        newState = BAT_CRIT;
    } else if (Vin < BAT_WARN_THRESHOLD) {
        newState = BAT_WARN;
    } else {
        newState = BAT_OK;
    }

    if (newState != lastBatState) {
        lastBatState = newState;

        switch (newState) {
            case BAT_OK:
                Serial.printf("[BAT] OK: %.2f V\n", Vin);
                // Something to be done?
                break;

            case BAT_WARN:
                Serial.printf("[BAT] WARNING: %.2f V (below %.1f V)\n",
                              Vin, BAT_WARN_THRESHOLD);
                // Something to be done?
                break;

            case BAT_CRIT:
                Serial.printf("[BAT] CRITICAL: %.2f V (below %.1f V)\n",
                              Vin, BAT_CRIT_THRESHOLD);
                // Something to be done?
                break;
        }
    }
}

void loop() {
    readCrsfData();
  
  static unsigned long lastCheck = 0;
  unsigned long now = millis();

  if (now - lastCheck > 200) {   // 200 ms interval
    float Vin = measureBatteryVoltage();
    handleBatteryStatus(Vin);
    lastCheck = now;
  }
}
