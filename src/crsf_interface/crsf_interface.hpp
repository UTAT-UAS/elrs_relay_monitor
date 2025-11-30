#ifndef CRSF_INTERFACE_HPP  
#define CRSF_INTERFACE_HPP

#include <Arduino.h>


#define LINK_ID 0x14

class CrsfInterface
{
    public: 
        CrsfInterface();
        ~CrsfInterface() = default;
        void begin();
        void readCrsfData(); 
        
    private:
        void processCrsfFrame();

        HardwareSerial CrsfLink{2};
        uint8_t rxBuffer_[128];
        uint8_t rxBufferCount_ = 0;
        const uint32_t baudRate_ = 420000;
        const bool invertOptions_ = false;
};
#endif