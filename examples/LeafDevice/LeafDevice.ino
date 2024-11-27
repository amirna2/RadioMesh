#include <Arduino.h>
#include <string>
#include <arduino-timer.h>
#include <RadioMesh.h>
#include "device_info.h"

const uint8_t SENSOR_TOPIC = 0x10;

std::string toHex(const byte *data, int size);
bool sendSensorData();
bool runSensor(void *);
void RxCallback(const RadioMeshPacket *packet, int err);
void TxCallback(const RadioMeshPacket *packet, int err);
bool setupRadio();
bool setupDisplay();
void displayText(std::string message, int x, int y, bool clear, int delayMs);

IDevice *device = nullptr;
IRadio *radio = nullptr;
IDisplay *display = nullptr;

// Default display configuration
#ifdef USE_DISPLAY
OledDisplayParams displayParams = OledDisplayParams(SCL_OLED, SDA_OLED, RST_OLED, WARP_FONT_SMALL);
#endif

DeviceBuilder builder;
bool setupOk = false;

auto timer = timer_create_default();
const int INTERVAL_MS = 15000;
uint32_t counter = 1000;
bool isIncluded = false;

// convert a vector of bytes to a hex string
std::string toHex(const byte *data, int size)
{
    std::string buf;
    buf.reserve(size * 2); // Each byte will become two characters
    static const char hex[] = "0123456789ABCDEF";

    for (int i = 0; i < size; i++) {
        buf.push_back(hex[(data[i] >> 4) & 0xF]);
        buf.push_back(hex[data[i] & 0xF]);
    }

    return buf;
}

void TxCallback(const RadioMeshPacket *packet, int err)
{
      if (err != RM_E_NONE) {
         logerr_ln("TX Failed [%d]", err);
      } else {
         loginfo_ln("Packet sent successfully");
      }
}

// Callback function for received packets
void RxCallback(const RadioMeshPacket *packet, int err)
{
   if (packet == nullptr) {
      logerr_ln("RX Failed [%d]", err);
      return;
   }
   if (isIncluded && MessageTopicUtils::isIncludeOpen(packet->topic)) {
      return;
   }
   std::string source = toHex(packet->sourceDevId.data(), packet->sourceDevId.size());
   std::string destination = toHex(packet->destDevId.data(), packet->destDevId.size());
   if (err != RM_E_NONE) {

      logerr_ln("RX Failed [%d], from:%s, to:%s", err, source.c_str(), destination.c_str());
      return;
   }

   loginfo_ln("---------------------------------------------");
   loginfo_ln("Received packet ID: 0x%s [ RSSI: %ddBm, SNR: %.2fdB ]",
              toHex(packet->packetId.data(), packet->packetId.size()).c_str(), radio->getRSSI(), radio->getSNR());
   loginfo_ln("  Topic: 0x%02X", packet->topic);
   loginfo_ln("  Source: %s ", source.c_str());
   loginfo_ln("  Destination: %s", destination.c_str());
   loginfo_ln("  Device Type: 0x%02X", packet->deviceType);
   loginfo_ln("  Hop Count: %d", packet->hopCount);
   loginfo_ln("  Data CRC: 0x%04X", packet->packetCrc);
   loginfo_ln("  Data: %s", RadioMeshUtils::toString(packet->packetData).c_str());
   loginfo_ln("---------------------------------------------");

   if (MessageTopicUtils::isIncludeOpen(packet->topic) && !isIncluded) {
      isIncluded = true;
      timer.every(INTERVAL_MS, runSensor);
   }
}

bool setupDisplay()
{
   display = device->getDisplay();
   if (display == nullptr) {
      logerr_ln("ERROR  display is null");

      return false;
   } else {
      // Setup the display with the built-in configuration
      if (display->setup() != RM_E_NONE) {
         logerr_ln("ERROR  display setup failed");
         return false;
      }
      display->clear();
   }
   return true;
}

void displayText(std::string message, int x=10, int y=40, bool clear=true, int delayMs=1000)
{
#ifdef USE_DISPLAY
   if (clear) {
      display->clear();
   }
   display->drawString(x, y, message);
   delay(delayMs);
#endif
}

bool setupRadio()
{
   radio = device->getRadio();
   if (radio == nullptr) {
      logerr_ln("ERROR  radio is null");
      return false;
   } else {
      // Setup the radio with the built-in configuration
      if (radio->setup() != RM_E_NONE) {
         logerr_ln("ERROR  radio setup failed");
         return false;
      }
   }

   return true;
}

bool sendSensorData()
{
    bool result = false;
    std::vector<byte> buffer = RadioMeshUtils::numberToBytes(counter);

    loginfo_ln("[DEVICE]  Sending sensor data %d", counter);
    int err = device->sendData(SENSOR_TOPIC, buffer);
    if (err == RM_E_NONE) {
        loginfo_ln("[DEVICE]  Sensor data sent");
        result = true;
        displayText(std::to_string(counter), 10, 20, true, 1000);
        counter++;
    } else {
        logerr_ln("[DEVICE]  ERROR - Failed to send sensor data");
        return false;
    }
    return result;
}

bool runSensor(void *)
{  if (!isIncluded) {
      loginfo_ln("[DEVICE]  Waiting for inclusion");
      return true;
   }
   loginfo_ln("[DEVICE]  Running sensor");
   return sendSensorData();
}

void setup()
{
#ifdef USE_HELTEC_WIFI_LORA_32_V3
LoraRadioParams radioParams = LoraRadioPresets::HELTEC_WIFI_LORA_32_V3;
#else
LoraRadioParams radioParams = LoraRadioPresets::HELTEC_CUBECELL;
#endif
   // Create the device first using the Device Builder
   // This device does not have wifi capabilities
   // and does not act as a router initially
   device = builder.start()
                   .withLoraRadio(radioParams)
                   .withRelayEnabled(false)
                   .withRxPacketCallback(RxCallback)
                   .withAesCrypto(key, iv)
#ifdef USE_DISPLAY
                   .withOledDisplay(displayParams)
#endif
                   .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::STANDARD);

   if (device == nullptr) {
      logerr_ln("ERROR  device is null");
      setupOk = false;
      return;
   }

   // Setup the device components with the built-in configuration
 #if defined(USE_DISPLAY) && !defined(WARP_NO_DISPLAY)
   setupOk = setupDisplay();
   if (!setupOk) {
      logerr_ln("ERROR  display setup failed");
      return;
   }
#endif
   setupOk = setupRadio();
   if (!setupOk) {
      logerr_ln("ERROR  radio setup failed");
      return;
   }

   setupOk = true;
   loginfo_ln("INFO  setup complete");
}

void loop() {
   if (!setupOk) {
      return;
   }
   device->run();
   timer.tick();
}

