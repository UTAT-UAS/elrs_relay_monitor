#include <HardwareSerial.h>

#define LINK_ID 0x14

HardwareSerial CrsfLink(2);
uint8_t rxBuffer[128];
uint8_t rxBufferCount = 0;

const uint32_t baudRate = 420000;
const bool invertOptions = false;

void readCrsfData();
void processCrsfFrame();
void tryNextConfig();

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  // Start with first config
  CrsfLink.begin(baudRate, SERIAL_8N1, 16, -1, invertOptions);
}

void loop() {
  readCrsfData();
}

void readCrsfData() {
  static unsigned long lastFrameTime = 0;
  
  // Timeout check - reset if no complete frame in 500ms
  if (rxBufferCount > 0 && (millis() - lastFrameTime > 500)) {
    rxBufferCount = 0;
  }
  
  while (CrsfLink.available()) {
    uint8_t data = CrsfLink.read();

    // Frame Assembly State Machine
    if (rxBufferCount == 0) {
      lastFrameTime = millis();
      rxBuffer[rxBufferCount++] = data;
    }
    else if (rxBufferCount == 1) {
      // CRSF frame length should be 2-64
      if (data >= 2 && data <= 64) {
        rxBuffer[rxBufferCount++] = data;
      } else {
        rxBufferCount = 0; // Reset on invalid length
      }
    }
    else {
      if (rxBufferCount < sizeof(rxBuffer)) {
        rxBuffer[rxBufferCount++] = data;
      }
      // Check if a complete packet has been received
      // Packet length = Address(1) + Length(1) + Payload(N) + CRC(1)
      if (rxBufferCount == (rxBuffer[1] + 2)) {
        processCrsfFrame();
        rxBufferCount = 0;
      }
    }
  }
}

void processCrsfFrame() {
  uint8_t frameType = rxBuffer[2];
  uint8_t length = rxBuffer[1];

  switch (frameType) {
      
    case LINK_ID:
      if (length >= 10) {
        uint8_t uplinkRssi1 = rxBuffer[3];   // RSSI from Rx Antenna 1 (scaled 0-255)
        uint8_t uplinkRssi2 = rxBuffer[4];   // RSSI from Rx Antenna 2 (scaled 0-255)
        uint8_t linkQuality = rxBuffer[5];   // Link Quality (%)
        int8_t snr = (int8_t)rxBuffer[6];    // Signal-to-Noise Ratio (signed)
        uint8_t txPower = rxBuffer[9];       // Transmit Power level

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