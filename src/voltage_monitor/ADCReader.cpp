#include "ADCReader.hpp"

ADCReader::ADCReader(gpio_num_t pin, adc1_channel_t channel, adc_atten_t atten)
    : _pin(pin), _channel(channel), _atten(atten) {
}

void ADCReader::begin() {
    // Configure ADC width & attenuation
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(_channel, _atten);

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
    return adc1_get_raw(_channel);
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
