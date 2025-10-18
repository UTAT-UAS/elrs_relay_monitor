#include <HardwareSerial.h>

// CRSF Protocol Constants
#define RADIO_ADDRESS 0xEA
#define UART_SYNC 0xC8
#define LINK_ID 0x14
#define BATTERY_ID 0x08

HardwareSerial CrsfLink(2); // Use UART2
uint8_t rxBuffer[128];
uint8_t rxBufferCount = 0;

// Baud rates to try
const uint32_t baudRates[] = {420000, 400000, 115200, 460800};
const bool invertOptions[] = {false, true};
int currentBaudIndex = 0;
int currentInvertIndex = 0;
unsigned long lastSwitchTime = 0;
unsigned long goodFrameCount = 0;

void readCrsfData();
void processCrsfFrame();
void tryNextConfig();

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("CRSF Auto-Detect Started");
  Serial.println("Testing baud rates and inversion...");
  
  // Start with first config
  CrsfLink.begin(baudRates[currentBaudIndex], SERIAL_8N1, 16, -1, invertOptions[currentInvertIndex]);
  Serial.printf("Testing: %lu baud, invert=%d\n", baudRates[currentBaudIndex], invertOptions[currentInvertIndex]);
  lastSwitchTime = millis();
}

void loop() {
  readCrsfData();
  
  // If no good frames after 5 seconds, try next config
  if (goodFrameCount == 0 && (millis() - lastSwitchTime > 5000)) {
    tryNextConfig();
  }
}

void tryNextConfig() {
  // Move to next configuration
  currentInvertIndex++;
  if (currentInvertIndex >= 2) {
    currentInvertIndex = 0;
    currentBaudIndex++;
    if (currentBaudIndex >= 4) {
      currentBaudIndex = 0;
    }
  }
  
  CrsfLink.end();
  delay(100);
  CrsfLink.begin(baudRates[currentBaudIndex], SERIAL_8N1, 16, -1, invertOptions[currentInvertIndex]);
  Serial.printf("\nTrying: %lu baud, invert=%d\n", baudRates[currentBaudIndex], invertOptions[currentInvertIndex]);
  
  rxBufferCount = 0;
  lastSwitchTime = millis();
  goodFrameCount = 0;
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
  
  // Count good frames
  goodFrameCount++;
  
  // Debug counter to show frames are being processed
  static unsigned long frameCount = 0;
  static unsigned long lastPrintTime = 0;
  frameCount++;
  
  // Print frame count every 2 seconds
//   if (millis() - lastPrintTime > 2000) {
//     Serial.printf("%lu baud, invert=%d - Frames: %lu (last 2s)\n", 
//                   baudRates[currentBaudIndex], invertOptions[currentInvertIndex], frameCount);
//     frameCount = 0;
//     lastPrintTime = millis();
//   }

  switch (frameType) {
    case 0x16: // RC Channels - most common frame
      // Just acknowledge we got it, don't spam output
      break;
      
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
        // Simple mapping
        int rssi2_dbm = -130 + (uplinkRssi2 / 2);

        Serial.printf("[LINK] LQ: %d%%, RSSI: %d/%d (%ddBm/%ddBm), SNR: %ddB, TxPwr: %d\n",
                       linkQuality, uplinkRssi1, uplinkRssi2, rssi1_dbm, rssi2_dbm, snr, txPower);
      }
      break;

    case BATTERY_ID:
      if (length >= 10) {
        // Battery voltage is a 16-bit value (little-endian), units: 0.1V
        uint16_t voltage = (rxBuffer[4] << 8) | rxBuffer[3];
        // Current is a 16-bit value, units: 0.1A
        uint16_t current = (rxBuffer[6] << 8) | rxBuffer[5];

        Serial.printf("[BATT] Voltage: %.1fV, Current: %.1fA\n",
                       voltage / 10.0, current / 10.0);
      }
      break;

    default:
      // Silently handle other frame types
      break;
  }
}