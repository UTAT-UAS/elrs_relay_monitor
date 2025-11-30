#ifndef ADC_READER_H
#define ADC_READER_H

#include "Arduino.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"

class ADCReader {
    private:
    gpio_num_t _pin;
    adc2_channel_t _channel;
    adc_atten_t _atten;

    esp_adc_cal_characteristics_t adc_chars;

    public:
    ADCReader(gpio_num_t pin, adc2_channel_t channel, adc_atten_t atten = ADC_ATTEN_DB_12);

    void begin();
    int readRaw();
    float readVoltage();
    float readAveragedVoltage(int samples = 16);
    float readBatteryVoltage(float Rtop, float Rbottom);

};




#endif
