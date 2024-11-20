#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>

#define HOSTNAME "ESP32-2432S024"
#define TIMEZONE "MSK-3"

#define TFT_CS   15
#define TFT_RST  -1
#define TFT_DC    2
#define TFT_MISO 12  // Data in
#define TFT_MOSI 13  // Data out
#define TFT_CLK  14  // Clock out
#define TFT_BL   27  // back.light

#define CTP_INT 5
#define CTP_RST 4
#define CTP_SCL 3
#define CTP_SDA 2

#define RGBLR 16  // red
#define RGBLG 17  // green
#define RGBLB 4   // blue
#define LSENS 34  // photoresistor

#define TRACE(...) Serial.printf(__VA_ARGS__)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

const char* wssid = "YOUR_SSID"; // Change this to your WiFi SSID
const char* wpass = "YOUR_PASSWORD"; // Change this to your WiFi password
const char* ntpHost[] = {"0.ru.pool.ntp.org", "1.ru.pool.ntp.org", "2.ru.pool.ntp.org"};
const int serial_baud = 115200;

bool initWiFi(const char*, const char*, int, int);
bool isWiFiOn();
String getTimeStr(int);
String scanWiFi();
void screenBoot();
void drawClock();
void drawPanel();
void drawScreen(int);

bool initWiFi(const char *wssid, const char *wpass,
              int max_tries = 10, int pause = 500)
{
  int i = 0;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
  WiFi.disconnect();
  delay(125);
  TRACE("Connecting to WiFi: %s\n", wssid);
  WiFi.begin(wssid, wpass);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  do {
    delay(pause);
    TRACE(".");
    i++;
  } while (!isWiFiOn() && i < max_tries);
  if (WiFi.status() == WL_CONNECTED) {
    TRACE("\nConnected to %s; RSSI = %ddBm; Channel %d\n", wssid, WiFi.RSSI(), WiFi.channel());
    TRACE("IP address local: %s\n", WiFi.localIP().toString().c_str());
    WiFi.setAutoReconnect(true);
    WiFi.persistent(true);
  }
  return isWiFiOn();
}

bool isWiFiOn() {
  return (WiFi.status() == WL_CONNECTED);
}

String scanWiFi() {
  String scanResult(' ');
  int n = WiFi.scanNetworks();
  if (n == 0) {
   scanResult += "No networks found";
  } else {
  scanResult += n;
  scanResult += " networks found at ";
  scanResult += getTimeStr(0).c_str();
  scanResult += '\n';
  int lengthMax = 0;
  for (int l = 0; l < n; l++) {
    if (WiFi.SSID(l).length() >= lengthMax) {
      lengthMax = WiFi.SSID(l).length();
    }
  }
  scanResult += " Nr |";
  scanResult +=  " SSID";
  String spaceStr = "";
  for (int m = 0; m < lengthMax - 3; m++) {
    spaceStr += ' ';
  }
  scanResult += spaceStr;
  scanResult += "|RSSI | CH | Encrypt\n";
  for (int i = 0; i < n; i++) {
    scanResult += ' ';
    if (i < 9) {
      scanResult += '0';
    }
    scanResult += i + 1;
    scanResult += " | ";
    int lengthAdd = lengthMax - WiFi.SSID(i).length();
    String cmStr = "";
    for (int k = 0; k < lengthAdd; k++) {
      cmStr += ' ';
    }
    scanResult += WiFi.SSID(i).c_str();
    scanResult += cmStr;
    scanResult += " | ";
    scanResult += WiFi.RSSI(i);
    scanResult += " | ";
    scanResult += WiFi.channel(i);
    if (WiFi.channel(i) < 10) {
      scanResult += ' ';
    }
    scanResult += " | ";
    switch (WiFi.encryptionType(i)) {
      case WIFI_AUTH_OPEN:
      scanResult += "open";
      break;
      case WIFI_AUTH_WEP:
      scanResult += "WEP";
      break;
      case WIFI_AUTH_WPA_PSK:
      scanResult += "WPA";
      break;
      case WIFI_AUTH_WPA2_PSK:
      scanResult += "WPA2";
      break;
      case WIFI_AUTH_WPA_WPA2_PSK:
      scanResult += "WPA+WPA2";
      break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
      scanResult += "WPA2-EAP";
      break;
      case WIFI_AUTH_WPA3_PSK:
      scanResult += "WPA3";
      break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
      scanResult += "WPA2+WPA3";
      break;
      case WIFI_AUTH_WAPI_PSK:
      scanResult += "WAPI";
      break;
      default:
      scanResult += "unk";
    }
    scanResult += '\n';
    delay(10);
  }
 }
 return scanResult;
}

String getTimeStr(int numStr)
{
  time_t tnow = time(nullptr);
  struct tm *timeinfo;
  time(&tnow);
  timeinfo = localtime(&tnow);
  char timeStrRet[32];
  switch (numStr)
  {
  case 0:
    strftime(timeStrRet, 32, "%T", timeinfo); // ISO8601 (HH:MM:SS)
    break;
  case 1:
    strftime(timeStrRet, 32, "%R", timeinfo); // HH:MM
    break;
  case 3:
    strftime(timeStrRet, 32, "%H", timeinfo); // HH
    break;
  case 4:
    strftime(timeStrRet, 32, "%M", timeinfo); // MM
    break;
  case 5:
    strftime(timeStrRet, 32, "%S", timeinfo); // SS
    break;
  case 6:
    strftime(timeStrRet, 32, "%a %d %h", timeinfo); // Thu 23 Aug
    break;
  case 7:
    strftime(timeStrRet, 32, "%d.%m.%g", timeinfo); // 23.08.24
    break;
  case 8:
    strftime(timeStrRet, 32, "%d %h", timeinfo); // 23 Aug
    break;
  case 9:
    strftime(timeStrRet, 32, "%c", timeinfo); // Thu Aug 23 14:55:02 2001
    break;
  }

  return timeStrRet;
}

String infoChip() {
  uint32_t total_internal_memory = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
  uint32_t free_internal_memory = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
  String infoChipStr[8];

  infoChipStr[0] += "Chip:";
  infoChipStr[0] += ESP.getChipModel();
  infoChipStr[0] += " R";
  infoChipStr[0] += ESP.getChipRevision();
  
  infoChipStr[1] += "CPU :";
  infoChipStr[1] += ESP.getChipCores();
  infoChipStr[1] += " cores @ ";
  infoChipStr[1] += ESP.getCpuFreqMHz();
  infoChipStr[1] += "MHz";
  
  infoChipStr[2] += "Flash frequency ";
  infoChipStr[2] += ESP.getFlashChipSpeed() / 1000000;
  infoChipStr[2] += "MHz";

  infoChipStr[3] += "Flash size: ";
  infoChipStr[3] += ESP.getFlashChipSize() * 0.001;
  infoChipStr[3] += "kb";

  infoChipStr[4] += "Flash size: ";
  infoChipStr[4] += ESP.getFlashChipSize() / 1024;
  infoChipStr[4] += " kib";

  infoChipStr[5] += "Free  heap: ";
  infoChipStr[5] += esp_get_free_heap_size() * 0.001;
  infoChipStr[5] += " kb";

  infoChipStr[6] += "Free  DRAM: ";
  infoChipStr[6] += free_internal_memory * 0.001;
  infoChipStr[6] += " kb";

  infoChipStr[7] += "Total DRAM: ";
  infoChipStr[7] += total_internal_memory * 0.001;
  infoChipStr[7] += " kb";

  String infoChipAll = "";
  for (int i = 0; i < 8; i++) {
    infoChipAll += i + 1;
    infoChipAll += ". ";
    infoChipAll += infoChipStr[i];
    infoChipAll += '\n';
  }
  return infoChipAll;
}

void screenBoot() {
  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, tft.width(), 20, ILI9341_NAVY);
  tft.fillRect(0, 220, tft.width(), 20, ILI9341_NAVY);
  tft.setCursor(0, 48);
  tft.setTextColor(ILI9341_DARKGREY, ILI9341_BLACK);
  tft.setTextSize(2);
  tft.printf("%s", infoChip().c_str());
}

void drawClock() {
  tft.setCursor(16, 3);
  tft.setTextColor(ILI9341_DARKCYAN, ILI9341_NAVY);
  tft.setTextSize(2);
  tft.printf("%s", getTimeStr(9).c_str());
}

void drawPanel() {
  tft.setCursor(18, 220);
  tft.setTextColor(ILI9341_DARKCYAN, ILI9341_NAVY);
  tft.setTextSize(1);
  tft.printf("WiFi: %s %s chan:%d %ddBm", wssid, WiFi.localIP().toString().c_str(), WiFi.channel(), WiFi.RSSI());
}

void drawScan() {
  tft.fillRect(0, 21, tft.width(), 198, ILI9341_NAVY);
  tft.setTextSize(1);
  tft.setCursor(0, 21);
  tft.printf("%s", scanWiFi().c_str());
  //tft.printf("\nScan complete at %s", getTimeStr(0).c_str());
}

void drawScreen(int refreshPeriod) {
  static int refreshTimer;
  static int refreshRate;
  if (millis() >= refreshTimer + refreshPeriod) {
    drawClock();
    drawPanel();
    refreshTimer = millis();
    refreshRate++;
  }
  if (millis() % 10000 == 0) {
    tft.setCursor(18, 230);
    tft.setTextColor(ILI9341_DARKCYAN, ILI9341_NAVY);
    tft.setTextSize(1);
    tft.fillRect(0, 220, tft.width(), 20, ILI9341_NAVY);
    tft.printf("[Refresh rate: %1.f avg]  [Free heap:%.1f kB]", refreshRate * 0.1, esp_get_free_heap_size() * 0.001);
    refreshRate = 0;
  }
}

void setup() {
  Serial.begin(serial_baud);
  while (!Serial) { ; };
  delay(200);
  TRACE("Serial0 running at %d baud\n", serial_baud);
  initWiFi(wssid, wpass, 10, 500);
  configTzTime(TIMEZONE, ntpHost[0], ntpHost[1], ntpHost[2]);
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, HIGH);
  tft.begin();
  delay(250);
  tft.setRotation(1);

  screenBoot();
  drawPanel();
  drawClock();
  delay(3000);

  pinMode(RGBLR, OUTPUT); digitalWrite(RGBLR, HIGH);
  pinMode(RGBLG, OUTPUT); digitalWrite(RGBLG, HIGH);
  pinMode(RGBLB, OUTPUT); digitalWrite(RGBLB, HIGH);

}

void loop(void)
{
  drawScreen(250);
  static int scanTimer;
  int scanPeriod = 300000;
  if (millis() >= scanTimer + scanPeriod) {
    drawScan();
    WiFi.scanDelete();
    scanTimer = millis();
  }
}
