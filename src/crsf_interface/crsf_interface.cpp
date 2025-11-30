#include "crsf_interface.hpp"

CrsfInterface::CrsfInterface() {
    // Constructor
}

void CrsfInterface::begin() {
    CrsfLink.begin(baudRate_, SERIAL_8N1, 16, -1, invertOptions_);
}

void CrsfInterface::readCrsfData() {
  static unsigned long lastFrameTime = 0;
  
  // Timeout check - reset if no complete frame in 500ms
  if (rxBufferCount_ > 0 && (millis() - lastFrameTime > 500)) {
    rxBufferCount_ = 0;
  }
  
  while (CrsfLink.available()) {
    uint8_t data = CrsfLink.read();

    // Frame Assembly State Machine
    if (rxBufferCount_ == 0) {
      lastFrameTime = millis();
      rxBuffer_[rxBufferCount_++] = data;
    }
    else if (rxBufferCount_ == 1) {
      // CRSF frame length should be 2-64
      if (data >= 2 && data <= 64) {
        rxBuffer_[rxBufferCount_++] = data;
      } else {
        rxBufferCount_ = 0; // Reset on invalid length
      }
    }
    else {
      if (rxBufferCount_ < sizeof(rxBuffer_)) {
        rxBuffer_[rxBufferCount_++] = data;
      }
      // Check if a complete packet has been received
      // Packet length = Address(1) + Length(1) + Payload(N) + CRC(1)
      if (rxBufferCount_ == (rxBuffer_[1] + 2)) {
        processCrsfFrame();
        rxBufferCount_ = 0;
      }
    }
  }
}

void CrsfInterface::processCrsfFrame() {
  uint8_t frameType = rxBuffer_[2];
  uint8_t length = rxBuffer_[1];

  switch (frameType) {
      
    case LINK_ID:
      if (length >= 10) {
        uint8_t uplinkRssi1 = rxBuffer_[3];   // RSSI from Rx Antenna 1 (scaled 0-255)
        uint8_t uplinkRssi2 = rxBuffer_[4];   // RSSI from Rx Antenna 2 (scaled 0-255)
        uint8_t linkQuality = rxBuffer_[5];   // Link Quality (%)
        int8_t snr = (int8_t)rxBuffer_[6];    // Signal-to-Noise Ratio (signed)
        uint8_t txPower = rxBuffer_[9];       // Transmit Power level

        // In CRSF/ELRS: Higher RSSI value (200+) = stronger signal (close)
        // Convert to dBm: Strong signal around -50dBm, weak around -120dBm
        // Reverse scale: dBm = -(255 - RSSI) - offset
        // Typical range: RSSI 50 -> -120dBm, RSSI 220 -> -50dBm
        int rssi1_dbm = -130 + (uplinkRssi1 / 2);
        int rssi2_dbm = -130 + (uplinkRssi2 / 2);

        Serial.printf("[LINK] LQ: %d%%, RSSI: %d/%d (%ddBm/%ddBm), SNR: %ddB, TxPwr: %d\n",
                       linkQuality, uplinkRssi1, uplinkRssi2, rssi1_dbm, rssi2_dbm, snr, txPower);
      }
      break;

    default:
      break;
  }
}