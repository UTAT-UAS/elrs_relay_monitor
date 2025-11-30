#include <HardwareSerial.h>
#include "crsf_interface/crsf_interface.hpp"


CrsfInterface crsfInterface;

void setup() {
    Serial.begin(115200);
    delay(1000);
    crsfInterface.begin();
}

void loop() {
    crsfInterface.readCrsfData();
}

