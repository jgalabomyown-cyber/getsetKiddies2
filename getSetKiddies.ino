// KiddieTracker_PHL.ino
#include <TinyGPS++.h>
#include <AltSoftSerial.h>
#include <SoftwareSerial.h>

const char APN[] = "internet.globe.com.ph";  // Globe APN
const char APN_USER[] = "";
const char APN_PASS[] = "";
const char SERVER_URL[] = "http://10.145.125.244/kiddie_tracker/track.php";
const char API_KEY[] = "MYSECRET123";
const char PHONE_FALLBACK[] = "+639XXXXXXXXX";  // Replace with your Globe number

AltSoftSerial simSerial; // SIM800L: RX=8, TX=9
SoftwareSerial ssGPS(4, 3); // GPS: TX=4 -> Arduino RX

TinyGPSPlus gps;
unsigned long lastSend = 0;
const unsigned long SEND_INTERVAL = 10000;
const float MOVE_THRESHOLD = 5.0;

double lastLat = 0, lastLng = 0;
bool haveLast = false;

void setup() {
  Serial.begin(115200);
  simSerial.begin(9600);
  ssGPS.begin(9600);
  Serial.println(F("KiddieTracker PHL Starting..."));
  delay(3000);
  sendAT("ATE0"); // echo off
}

void loop() {
  while (ssGPS.available()) gps.encode(ssGPS.read());

  if (gps.location.isUpdated()) {
    double lat = gps.location.lat();
    double lng = gps.location.lng();
    double spd = gps.speed.kmph();

    bool moved = false;
    if (!haveLast) moved = true;
    else if (distanceMeters(lat, lng, lastLat, lastLng) > MOVE_THRESHOLD) moved = true;

    if (moved || millis() - lastSend > SEND_INTERVAL) {
      if (sendToServer(lat, lng, spd)) {
        Serial.println(F("HTTP send OK"));
      } else {
        sendSMS(lat, lng, spd);
      }
      lastLat = lat; lastLng = lng; haveLast = true; lastSend = millis();
    }
  }
}

bool sendToServer(double lat, double lng, double spd) {
  char cmd[200];
  Serial.println(F("Opening GPRS..."));
  sendAT("AT+SAPBR=3,1,\"Contype\",\"GPRS\"");
  sprintf(cmd, "AT+SAPBR=3,1,\"APN\",\"%s\"", APN); sendAT(cmd);
  sendAT("AT+SAPBR=1,1");
  sendAT("AT+SAPBR=2,1");

  sendAT("AT+HTTPINIT");
  sendAT("AT+HTTPPARA=\"CID\",1");

  char url[200];
  sprintf(url, "%s?key=%s&lat=%.6f&lng=%.6f&spd=%.2f", SERVER_URL, API_KEY, lat, lng, spd);
  sprintf(cmd, "AT+HTTPPARA=\"URL\",\"%s\"", url);
  sendAT(cmd);
  if (!waitFor("+HTTPACTION: 0,", 15000)) return false;

  sendAT("AT+HTTPTERM");
  sendAT("AT+SAPBR=0,1");
  return true;
}

void sendSMS(double lat, double lng, double spd) {
  char msg[160];
  sprintf(msg, "Kid: https://maps.google.com/?q=%.6f,%.6f Speed:%.1f", lat, lng, spd);
  sendAT("AT+CMGF=1");
  char cmd[40];
  sprintf(cmd, "AT+CMGS=\"%s\"", PHONE_FALLBACK);
  simSerial.println(cmd);
  delay(500);
  simSerial.print(msg);
  simSerial.write(26); // Ctrl+Z
  waitFor("+CMGS", 10000);
}

void sendAT(const char *cmd) {
  simSerial.println(cmd);
  delay(500);
  while (simSerial.available()) Serial.write(simSerial.read());
}

bool waitFor(const char *target, unsigned long timeout) {
  unsigned long start = millis();
  String resp = "";
  while (millis() - start < timeout) {
    while (simSerial.available()) {
      char c = simSerial.read();
      resp += c;
      if (resp.indexOf(target) != -1) return true;
    }
  }
  return false;
}

double distanceMeters(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371000;
  double dLat = radians(lat2 - lat1);
  double dLon = radians(lon2 - lon1);
  double a = sin(dLat/2)*sin(dLat/2) + cos(radians(lat1))*cos(radians(lat2))*sin(dLon/2)*sin(dLon/2);
  double c = 2 * atan2(sqrt(a), sqrt(1-a));
  return R * c;
}
