#include <Arduino.h>
#include "painlessMesh.h"

#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

int inputPin = 26;

DynamicJsonDocument doc(1024);
void sendMessage(); // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage()
{
  delay(180000);
  String msg = "";
  // 0 = LUZ
  // 1 = OBSCURIDAD
  int val = digitalRead(inputPin);
  doc["id"] = "lopyligth";

  if(val == 1)
    doc["data"] = "ON";
  else
    doc["data"] = "OFF";
  
  delay(1000);

  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  Serial.println(msg);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

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
  // Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup()
{
  Serial.begin(115200);

  pinMode(inputPin, INPUT);
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