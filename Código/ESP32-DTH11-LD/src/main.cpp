#include <Arduino.h>
#include <Adafruit_Sensor.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "painlessMesh.h"

#define DHTPIN 14
#define DHTTYPE DHT11
#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

DHT dht(DHTPIN, DHTTYPE);

DynamicJsonDocument doc(1024);

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

// User stub|
void sendMessage(); // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage()
{
  delay(180000);
  String data = "";

  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t))  {
    Serial.println("Error obtaining DHT11 sensor data");
    return;
  }
  doc["id"] = "espdth11";
  doc["temperature"] = t;
  doc["humidity"] = h;
  serializeJson(doc, data);
  
  Serial.println(data);

  mesh.sendBroadcast(data);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

// Needed for painless library
void receivedCallback(uint32_t from, String &msg)
{
  Serial.printf("Mensaje", msg.c_str());
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup()
{
  Serial.begin(115200);
  dht.begin();

    // mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes(ERROR | STARTUP); // set before init() so that you can see startup messages

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}


void loop()
{
  mesh.update();  
}