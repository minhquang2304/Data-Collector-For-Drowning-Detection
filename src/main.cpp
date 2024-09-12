/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <esp_now.h>
#include <WiFi.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"

#define FREQUENCY_HZ 50
#define INTERVAL_MS (1000 / (FREQUENCY_HZ + 1))
#define ACC_RANGE 1 // 0: -/+2G; 1: +/-4G

// convert factor g to m/s2 ==> [-32768, +32767] ==> [-2g, +2g]
#define CONVERT_G_TO_MS2 (9.81 / (16384.0 / (1. + ACC_RANGE)))

MPU6050 imu;
int16_t ax, ay, az;
int32_t altitude;

// REPLACE WITH YOUR RECEIVER MAC Address
uint8_t broadcastAddress[] = {0x34, 0x85, 0x18, 0x25, 0xCA, 0x30};

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message
{

  float x;
  float y;
  float z;

} struct_message;

// Create a struct_message called myData
struct_message myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
{
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup()
{
  // Init Serial Monitor
  Serial.begin(115200);
  Serial.println("Initializing I2C devices...");
  Wire.begin();
  imu.initialize();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK)
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  // Add peer
  if (esp_now_add_peer(&peerInfo) != ESP_OK)
  {
    Serial.println("Failed to add peer");
    return;
  }
  delay(300);

  // Set MCU 6050 OffSet Calibration
  imu.setXAccelOffset(-363);
  imu.setYAccelOffset(1391);
  imu.setZAccelOffset(1364);
  imu.setXGyroOffset(80);
  imu.setYGyroOffset(34);
  imu.setZGyroOffset(37);
  imu.setFullScaleAccelRange(ACC_RANGE);
}

void loop()
{
  // Set values to send
  imu.getAcceleration(&ax, &ay, &az);

  // converting to m/s2
  float ax_m_s2 = ax * CONVERT_G_TO_MS2;
  float ay_m_s2 = ay * CONVERT_G_TO_MS2;
  float az_m_s2 = az * CONVERT_G_TO_MS2;

  myData.x = ax_m_s2;
  myData.y = ay_m_s2;
  myData.z = az_m_s2;

  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&myData, sizeof(myData));

  if (result == ESP_OK)
  {
    Serial.println("Sent with success");
  }
  else
  {
    Serial.println("Error sending the data");
  }
  delay(20);
}