#define LINE_USER_ID ""
#define LINE_TOKEN ""

#include <WiFi.h>
#include <HTTPClient.h>
#include "ACS712.h"

char ssid[] = "";            // ชื่อ WiFi
char pass[] = "";       // รหัสผ่าน WiFi

ACS712 ACS(34, 3.3, 4095, 125);    // กำหนดค่า ACS712
int calibration_factor = 120;
const float voltage = 230.0;       // แรงดันไฟฟ้าคงที่ 230V

unsigned long lastAlertTime = 0;   // เวลาล่าสุดที่ส่งแจ้งเตือน
bool hasSentInitialAlert = false;  // ตรวจสอบว่าแจ้งเตือนครั้งแรกหรือยัง

void setup() {
  Serial.begin(115200);
  
  // เริ่มต้น WiFi
  WiFi.begin(ssid, pass);
  Serial.print("Connecting to WiFi");
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  float current = readCurrent();
  float power = voltage * (current / 1000.0); // คำนวณกำลังไฟฟ้า (Watt)

  bool isCurrentActive = current > 500;  // เช็คว่ามีการใช้งานหรือไม่ (เกิน 500mA)
  unsigned long currentTime = millis();  // เวลาปัจจุบัน

  // ✅ แจ้งเตือนครั้งแรกเมื่อเริ่มใช้งาน
  if (isCurrentActive && !hasSentInitialAlert) {
    sendMessage("⚡ ปลั๊กไฟถูกใช้งานครั้งแรก\n🔌 Voltage: " + String(voltage) + " V\n⚡ Current: " + String(current) + " mA\n💡 Power: " + String(power, 2) + " W");
    lastAlertTime = currentTime;  
    hasSentInitialAlert = true;  // ตั้งค่าให้รู้ว่าแจ้งเตือนครั้งแรกไปแล้ว
  }

  // ✅ แจ้งเตือนซ้ำทุก 5 นาทีหากยังมีการใช้งาน
  if (isCurrentActive && hasSentInitialAlert && (currentTime - lastAlertTime >= 300000)) {  
    sendMessage("⚡ ปลั๊กไฟกำลังถูกใช้งาน\n🔌 Voltage: " + String(voltage) + " V\n⚡ Current: " + String(current) + " mA\n💡 Power: " + String(power, 2) + " W");
    lastAlertTime = currentTime;  // อัปเดตเวลาแจ้งเตือนล่าสุด
  }

  // ✅ แจ้งเตือนเมื่อหยุดใช้งาน และรีเซ็ตตัวจับเวลา
  if (!isCurrentActive && hasSentInitialAlert) {
    sendMessage("✅ ปลั๊กไฟไม่ถูกใช้งานแล้ว\n🔌 Voltage: " + String(voltage) + " V");
    lastAlertTime = 0;  // รีเซ็ตเวลาแจ้งเตือน
    hasSentInitialAlert = false; // รีเซ็ตสถานะแจ้งเตือนครั้งแรก
  }

  delay(1000);  // หน่วงเวลาทุก 5 วินาที
}

float readCurrent() {
  float average = 0;
  for (int i = 0; i < 100; i++) {
    average += ACS.mA_AC();
  }
  float mA = (abs(average / 100.0) - calibration_factor);
  if (mA <= 5) mA = 0;

  float power = voltage * (mA / 1000.0); // คำนวณกำลังไฟฟ้า (Watt)

  Serial.print("🔌  Voltage: ");
  Serial.print(voltage);
  Serial.print(" V,⚡ Current: ");
  Serial.print(mA);
  Serial.print(" mA,💡 Power: ");
  Serial.print(power, 2);
  Serial.println(" W");
  Serial.println("...............................................");

  return mA;
}

void sendMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;

    String lineAPIUrl = "https://api.line.me/v2/bot/message/push";  // URL ของ LINE API
    String accessToken = LINE_TOKEN;  // ใช้ Token ที่กำหนด

    http.begin(lineAPIUrl);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "Bearer " + accessToken);

    String payload = "{";
    payload += "\"to\": \"" + LINE_USER_ID + "\",";
    payload += "\"messages\": [{";
    payload += "\"type\": \"text\",";
    payload += "\"text\": \"" + message + "\"";
    payload += "}]";
    payload += "}";

    int httpResponseCode = http.POST(payload);

    if (httpResponseCode == 200) {
      Serial.println("Message sent successfully");
    } else {
      Serial.printf("Error sending message. HTTP response code: %d\n", httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
