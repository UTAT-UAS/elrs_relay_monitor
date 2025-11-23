#ifndef ADC_READER_H
#define ADC_READER_H

#include "Arduino.h"
#include "esp_adc_cal.h"

class ADCReader {
    private:
    gpio_num_t _pin;
    adc1_channel_t _channel;
    adc_atten_t _atten;

    esp_adc_cal_characteristics_t adc_chars;
};

public:
    ADCReader(gpio_num_t pin, adc1_channel_t channel, adc_atten_t atten = ADC_ATTEN_DB_11);

    void begin();
    uint32_t readRaw();
    float readVoltage();
    float readAveragedVoltage(int samples = 16);



#endif
