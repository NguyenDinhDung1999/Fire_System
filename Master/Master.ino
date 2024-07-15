#include "FirebaseESP8266.h"
#include <ESP8266WiFi.h>
#include <espnow.h>
#include <ArduinoJson.h>

#define WIFI_SSID "OPPO"
#define WIFI_PASSWORD "1231231234"
#define FIREBASE_HOST "fas-firebase-default-rtdb.firebaseio.com"
#define FIREBASE_AUTH "oJJL4rcxw8HuC9txbcTdtNoBgmGrEHW0ZNYOrhKn"

#define RELAY_QUAT D5
#define RELAY_BOM D6  
#define BUTTON_QUAT D1
#define BUTTON_BOM D2

const int buttonPin = 13;
int CheDo = 0;
int gas0 = 0;
int flame0 = 0;
float temperature = 0;
float humidity = 0;
float lastTemperature = -999;
float lastHumidity = -999;

String data_quat = "";
String data_bom = "";

bool relay1Status = false;
bool relay2Status = false;

FirebaseData firebaseData;
String path = "System";
FirebaseJson json;

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    int id;
    int temp;
    int hum;
    int gas;
    int flame;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// Create a structure to hold the readings from each board
struct_message board1;

// Create an array with all the structures
struct_message boardsStruct[1] = {board1};

// Callback function that will be executed when data is received
void OnDataRecv(uint8_t * mac_addr, uint8_t *incomingData, uint8_t len) {
  char macStr[18];
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  Serial.println(macStr);
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Board ID %u: %u bytes\n", myData.id, len);
  // Update the structures with the new incoming data
  boardsStruct[myData.id - 1].temp = myData.temp;
  boardsStruct[myData.id - 1].hum = myData.hum;
  boardsStruct[myData.id - 1].gas = myData.gas;
  boardsStruct[myData.id - 1].flame = myData.flame;
  Serial.printf("temp value: %d \n", boardsStruct[myData.id-1].temp);
  Serial.printf("hum value: %d \n", boardsStruct[myData.id-1].hum);
  Serial.printf("gas value: %d \n", boardsStruct[myData.id-1].gas);
  Serial.printf("flame value: %d \n", boardsStruct[myData.id-1].flame);
  Serial.println();
}
 
void setup() {
  Serial.begin(9600);
  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // Khởi tạo kết nối Firebase
  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);
  
  pinMode(RELAY_QUAT, OUTPUT);
  pinMode(RELAY_BOM, OUTPUT);
  pinMode(BUTTON_QUAT, INPUT_PULLUP);
  pinMode(BUTTON_BOM, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  digitalWrite(RELAY_QUAT, relay1Status);
  digitalWrite(RELAY_BOM, relay2Status);
  
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(OnDataRecv);
}

void data (float tempBD, float humBD, int gasBD, int flameBD){
  if (tempBD != temperature){
    temperature = tempBD;
  }
  if (humBD != humidity){
    humidity = humBD;
  }
  if (gasBD != gas0){
    gas0 = gasBD;
  }
  if (flameBD != flame0){
    flame0 = flameBD;
  }
}


void loop()
{
  
  if (temperature != lastTemperature || humidity != lastHumidity) {
    // Gửi dữ liệu đến Firebase
    Firebase.setFloat (firebaseData, path + "/NhietDo", temperature);
    Firebase.setFloat (firebaseData, path + "/DoAm", humidity);
    
    // Lưu giá trị đọc mới vào biến lastTemperature và lastHumidity
    lastTemperature = temperature;
    lastHumidity = humidity;
  }
  
  
  if (gas0 == 0){
    Firebase.setString (firebaseData, path + "/GAS", "detection");
  }
  else {
    Firebase.setString (firebaseData, path + "/GAS", "no");
  }
  if (flame0 == 0){
    Firebase.setString (firebaseData, path + "/FIRE", "no");
  }
  else {
    Firebase.setString (firebaseData, path + "/FIRE", "detection");
  }

  
  // Access the variables for each board
  int TEMPbD = boardsStruct[0].temp;
  int HUMbD = boardsStruct[0].hum;
  int GASbD = boardsStruct[0].gas;
  int FLAMEbD = boardsStruct[0].flame;

  data(TEMPbD, HUMbD, GASbD, FLAMEbD);

  int buttonState = digitalRead(buttonPin);
  if (buttonState == LOW) {
    CheDo = 1 - CheDo;
    while (digitalRead(buttonPin) == LOW) {
      delay(10);
    }
  }
  
  if (CheDo == 0) {
    if (gas0 == 1 && flame0 == 0) {
      if (Firebase.getString(firebaseData, path + "/quat")) {
        data_quat = "";
        String data_quat1 = firebaseData.stringData();
        data_quat = (String)data_quat1[0];
        if (data_quat == "1")
          relay1Status = true;
        else if (data_quat == "0")
          relay1Status = false;
      }
      digitalWrite(RELAY_QUAT, relay1Status);
  
      if (Firebase.getString(firebaseData, path + "/bom")) {
        data_bom = "";
        String data_bom1 = firebaseData.stringData();
        data_bom = (String)data_bom1[0];
        if (data_bom == "1")
          relay2Status = true;
        else if (data_bom == "0")
          relay2Status = false;
      }
      digitalWrite(RELAY_BOM, relay2Status);
    } else {
      if (gas0 == 0) {
        relay1Status = true;
      } else {
        relay1Status = false;
      }
  
      if (flame0 == 1) {
        relay2Status = true;
      } else {
        relay2Status = false;
      }
    }
  } else {
    if (digitalRead(BUTTON_QUAT) == LOW) {
      delay(20);
      if (digitalRead(BUTTON_QUAT) == LOW) {
        relay1Status = !relay1Status;
        digitalWrite(RELAY_QUAT, relay1Status);
      }
      while (digitalRead(BUTTON_QUAT) == LOW) {
        delay(10);
      }
    }
  
    if (digitalRead(BUTTON_BOM) == LOW) {
      delay(20);
      if (digitalRead(BUTTON_BOM) == LOW) {
        relay2Status = !relay2Status;
        digitalWrite(RELAY_BOM, relay2Status);
      }
      while (digitalRead(BUTTON_BOM) == LOW) {
        delay(10);
      }
    }
  }
  
  
  digitalWrite(RELAY_QUAT, relay1Status);
  digitalWrite(RELAY_BOM, relay2Status);

  String CheDoString = (CheDo == 0) ? "td" : "tc";
  Firebase.setString(firebaseData, path + "/CheDo", CheDoString);

  
  String relay1StatusString = relay1Status ? "bat" : "tat";
  String relay2StatusString = relay2Status ? "bat" : "tat";
  Firebase.setString(firebaseData, path +  "/QUAT", relay1StatusString);
  Firebase.setString(firebaseData, path +  "/BOM", relay2StatusString);
}
