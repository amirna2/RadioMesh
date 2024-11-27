#include <Arduino.h>
#include <RadioMesh.h>
#include <string>
#include <arduino-timer.h>
#include "device_info.h"
#include <vector>

std::string toHex(const byte *data, int size);
void RxCallback(const RadioMeshPacket *packet, int err);
void TxCallback(const RadioMeshPacket *packet, int err);

void displaySetupMsg(std::string message, int x, int y, bool clear, int delayMs);
std::string getWifiSignal();
bool setupAccessPoint();
bool setupDisplay();
bool setupRadio();
bool setupWifiConnector();
bool broadcastInclusion(void*);

IDevice *device = nullptr;
IRadio *radio = nullptr;
IDisplay *display = nullptr;
IWifiConnector *wifiConnector = nullptr;
IWifiAccessPoint *wifiAP = nullptr;
ICrypto *crypto = nullptr;

DeviceBuilder builder;
bool setupOk = false;

auto timer = timer_create_default();
const int INCLUSION_BROADCAST_INTERVAL = (15 * 1000);  // 15 seconds
bool inclusionMode = false;

int counter = 1;
bool txComplete = true;  // Flag to track if previous TX is done

// Default wifi configuration
WifiParams wifiParams = {WIFI_SSID,WIFI_PASSWORD};
// Default display configuration
OledDisplayParams displayParams = OledDisplayParams(SCL_OLED, SDA_OLED, RST_OLED, RM_FONT_SMALL);
// Default access point configuration
WifiAccessPointParams apParams = {WIFI_AP_SSID, WIFI_AP_PASSWORD, WIFI_AP_IP_ADDRESS};

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


bool broadcastInclusion(void*) {
    if (device) {
        loginfo_ln("Broadcasting inclusion open message");
        int rc = device->sendInclusionOpen();
        if (rc != RM_E_NONE) {
            logerr_ln("Failed to send inclusion open message [%d]", rc);
        }
    }
    return true;  // Keep timer running
}

void TxCallback(const RadioMeshPacket *packet, int err)
{
      if (err != RM_E_NONE) {
         logerr_ln("TX Failed [%d]", err);
         display->setFont(RM_FONT_TINY);
         display->clear();
         display->drawString(0, 30, "TX failed"); display->drawString(45, 30, "Error: " + std::to_string(err));
      } else {
         loginfo_ln("TX Success. Packet Topic: %s", MessageTopicUtils::topicToString(packet->topic).c_str());
      }
}

// Callback function for received packets
void RxCallback(const RadioMeshPacket *packet, int err)
{
   if (packet == nullptr) {
      logerr_ln("RX Failed [%d]", err);
      display->setFont(RM_FONT_TINY);
      display->clear();
      display->drawString(0, 30, "RX failed"); display->drawString(45, 30, "Error: " + std::to_string(err));
      return;
   }

   std::string source = toHex(packet->sourceDevId.data(), packet->sourceDevId.size());
   std::string destination = toHex(packet->destDevId.data(), packet->destDevId.size());

   std::vector<byte> rxData = crypto->decrypt(packet->packetData);

   if (err != RM_E_NONE) {
      logerr_ln("RX Failed [%d], from:%s, to:%s", err, source.c_str(), destination.c_str());
      display->setFont(RM_FONT_TINY);
      display->clear();
      display->drawString(0, 30, "RX failed"); display->drawString(45, 30, "Error: " + std::to_string(err));
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
   uint32_t data = RadioMeshUtils::bytesToNumber<uint32_t>(rxData);
   loginfo_ln("  Data: %d", data);
   loginfo_ln("---------------------------------------------");

   display->setFont(RM_FONT_TINY);
   display->clear();
   if (wifiConnector != nullptr) {
         display->drawString(90,10, getWifiSignal());
   }
   display->drawString(0, 30, "pkt id: "); display->drawString(45, 30, toHex(packet->packetId.data(), packet->packetId.size()));
   display->drawString(0, 40, "from:"); display->drawString(30, 40, source);
   display->drawString(0, 50, "to:"); display->drawString(30, 50, destination);
   display->drawNumber(0, 60, data);
}

// Display a message on the OLED display
void displaySetupMsg(std::string message, int x=10, int y=40, bool clear=true, int delayMs=1000)
{
   if (clear) {
      display->clear();
   }
   display->drawString(x, y, message);
   delay(delayMs);
}

std::string getWifiSignal()
{
   SignalIndicator si = wifiConnector->getSignalIndicator();
   return RadioMeshUtils::wifiSignalToString(si);
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

bool setupRadio()
{
   radio = device->getRadio();
   if (radio == nullptr) {
      logerr_ln("ERROR  radio is null");
      displaySetupMsg("E: radio null");
      return false;
   } else {
      // Setup the radio with the built-in configuration
      if (radio->setup() != RM_E_NONE) {
         logerr_ln("ERROR  radio setup failed");
         displaySetupMsg("E: radio setup");
         return false;
      }
   }

   return true;
}

bool setupWifiConnector()
{
   wifiConnector = device->getWifiConnector();
   if (wifiConnector == nullptr) {
      logerr_ln("ERROR  wifiConnector is null");
      displaySetupMsg("E: wifi null");
      return false;
   } else {
      // Setup the wifi connector with the built-in configuration
      int retries = 0;
      while (wifiConnector->connect() != RM_E_NONE) {
         logerr_ln("ERROR: WifiConnector setup failed.");
         displaySetupMsg("E: wifi setup: " + std::to_string(retries));
         retries++;
         if (retries > 3) {
            return false;
         }
      }
   }
   std::string ip = wifiConnector->getIpAddress();
   loginfo_ln("IP Address: %s", ip.c_str());
   displaySetupMsg("IP: " + ip, 10, 20, true, 1000);
   return true;
}

bool setupAccessPoint()
{
   wifiAP = device->getWifiAccessPoint();
   if (wifiAP == nullptr) {
      logerr_ln("ERROR  wifiAP is null");
      displaySetupMsg("E: wifiAP null");
      return false;
   } else {
      // Setup the wifi access point with the built-in configuration
      if (wifiAP->setup() != RM_E_NONE) {
         logerr_ln("ERROR: WifiAP setup failed.");
         displaySetupMsg("E: wifiAP setup");
         return false;
      }
      if (wifiAP->start() != RM_E_NONE) {
         logerr_ln("ERROR: WifiAP start failed.");
         displaySetupMsg("E: wifiAP failed");
         return false;
      }
   }
   return true;
}

void setup()
{
   // Create the device first using the Device Builder
   // The HUB device has wifi capabilities so it can connect to a wifi network to send data
   // and also create an access point to allow other devices to connect to it
   device = builder.start()
                   .withLoraRadio(LoraRadioPresets::HELTEC_WIFI_LORA_32_V3)
                   //.withWifi(wifiParams)
                   //.withWifiAccessPoint(apParams)
                   .withRxPacketCallback(RxCallback)
                   .withTxPacketCallback(TxCallback)
                   .withAesCrypto(key, iv)
                   .withOledDisplay(displayParams)
                   .build(DEVICE_NAME, DEVICE_ID, MeshDeviceType::HUB);

   if (device == nullptr) {
      logerr_ln("ERROR  device is null");
      setupOk = false;
      return;
   }

   // Setup the device components with the built-in configuration

   crypto = device->getCrypto();
   if (crypto == nullptr) {
      logerr_ln("ERROR  crypto is null");
      return;
   }
   setupOk = setupDisplay();
   if (!setupOk) {
      logerr_ln("ERROR  display setup failed");
      return;
   }
   setupOk = setupRadio();
   if (!setupOk) {
      logerr_ln("ERROR  radio setup failed");
      return;
   }
   displaySetupMsg("I: radio ok", 10, 20, false, 1000);

#if 0
   setupOk = setupWifiConnector();
   if (!setupOk) {
      logerr_ln("ERROR  wifi setup failed");
      return;
   }

   displaySetupMsg("I: wifi ok", 10, 30, false, 1000);

   setupOk = setupAccessPoint();
   if (!setupOk) {
      logerr_ln("ERROR  wifiAP setup failed");
      return;
   }

   displaySetupMsg("I: wifiAP ok", 10, 40, false, 1000);
#endif

   if (device->enableInclusionMode(true) == RM_E_NONE) {
      loginfo_ln("Inclusion mode enabled");
      inclusionMode = true;
      timer.every(INCLUSION_BROADCAST_INTERVAL, broadcastInclusion, nullptr);
   } else {
      logerr_ln("Failed to enable inclusion mode");
      inclusionMode = false;
   }

   setupOk = true;
   loginfo_ln("INFO  setup complete");

   displaySetupMsg("Setup OK!");
}

void loop() {
   if (!setupOk) {
      return;
   }
   device->run();
   timer.tick(); // Run the inclusion broadcast timer
}

