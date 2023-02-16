#include <Arduino.h>
#include <BLEDevice.h>
#include <ArduinoJson.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include "painlessMesh.h"

//BLE server name
#define bleServerName "BLEMESH"

//MESH
#define MESH_PREFIX "whateverYouLike"
#define MESH_PASSWORD "somethingSneaky"
#define MESH_PORT 5555
 
// Defining the service and characteristics
#define SERVICE_UUID "91bad492-b950-4226-aa2b-4ede9fa42f59"
#define CHARACTERISTIC_UUID_TX "ea12ce72-e2f6-44c5-ac3f-51fb791a7c99"
#define CHARACTERISTIC_UUID_RX "89844d3b-e63d-479e-af2c-1f97e6fc5651"

BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;
BLEServer* pServer = NULL;
float txtValue = 0;

unsigned long lastTime = 0;
unsigned long timerDelay = 30000;

Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

String mensaje ="";
String mensajeMesh ="";

class MyCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) {
    mensaje= "";
    std::string rxValue = pCharacteristic->getValue();
    StaticJsonDocument<256> messageJSON;
    deserializeJson(messageJSON, rxValue);

    if (rxValue.length() > 0) {
      Serial.println("*********");
      Serial.print("New value: ");
      for (int i = 0; i < rxValue.length(); i++)
      {
        Serial.print(rxValue[i]);
        mensaje += rxValue[i];
      }
      Serial.println();
      Serial.println("*********");
    }
    Serial.println("");
  }
};

void sendMessage(); // Prototype so PlatformIO doesn't complain

Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void sendMessage(){
  mesh.sendBroadcast(mensaje);
  taskSendMessage.setInterval(random(TASK_SECOND * 1, TASK_SECOND * 5));
}

void receivedCallback(uint32_t from, String &msg){
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  
  pCharacteristic->setValue(msg.c_str());
  pCharacteristic->notify();   
  Serial.printf("*");
}

void newConnectionCallback(uint32_t nodeId){
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback(){
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset){
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);

  BLEDevice::init(bleServerName);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE characteristics
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_TX,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  
  // BLE2902 needed to notify
  pCharacteristic->addDescriptor(new BLE2902());
  // Characteristic for receiving end
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID_RX,
    BLECharacteristic::PROPERTY_WRITE
  );
  pCharacteristic->setCallbacks(new MyCallbacks());
  // Strat the service
  pService->start();
  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x0);  // set value to 0x00 to not advertise this parameter
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");

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

void loop() {
  if (!deviceConnected && oldDeviceConnected) {
      delay(500); // give the bluetooth stack the chance to get things ready
      pServer->startAdvertising(); // restart advertising
      Serial.println("start advertising");
      oldDeviceConnected = deviceConnected;
  }
  if (deviceConnected && !oldDeviceConnected) {
      oldDeviceConnected = deviceConnected;
  }
  mesh.update();
}