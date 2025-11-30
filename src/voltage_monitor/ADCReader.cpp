#include "ADCReader.hpp"

ADCReader::ADCReader(gpio_num_t pin, adc2_channel_t channel, adc_atten_t atten)
    : _pin(pin), _channel(channel), _atten(atten) {
}

void ADCReader::begin() {
    // Configure ADC width & attenuation
    adc2_config_channel_atten(_channel, _atten);

    // Calibrate ADC
    esp_adc_cal_characterize(
        ADC_UNIT_1,
        _atten,
        ADC_WIDTH_BIT_12,
        1100,               // Default Vref (mV)
        &adc_chars
    );
}

int ADCReader::readRaw() {
    int raw;

    // Wi-Fi must be OFF
    // wifi_mode_t mode;
    // esp_wifi_get_mode(&mode);
    // if (mode != WIFI_MODE_NULL) {
    //     Serial.println("[ERROR] ADC2 cannot be read while Wi-Fi is ON.");
    //     return -1;
    // }

    if (adc2_get_raw(_channel, ADC_WIDTH_BIT_12, &raw) == ESP_OK)
        return raw;

    return -1;
}

float ADCReader::readVoltage() {
    int raw = readRaw();
    int mv = esp_adc_cal_raw_to_voltage(raw, &adc_chars);
    return mv / 1000.0;
}

float ADCReader::readAveragedVoltage(int samples) {
    int sum = 0;
    for (int i = 0; i < samples; i++) {
        sum += readVoltage();
        delayMicroseconds(200);
    }
    return (float)sum / samples;
}

float ADCReader::readBatteryVoltage(float Rtop, float Rbottom) {
    float Vadc = readAveragedVoltage();   // Already in volts
    return Vadc * (Rtop + Rbottom) / Rbottom;
}
