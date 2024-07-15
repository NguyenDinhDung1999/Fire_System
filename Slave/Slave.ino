#include <ESP8266WiFi.h>
#include <espnow.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <DHT.h>
LiquidCrystal_I2C lcd(0x27,16,2);

#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define Coi D5
#define Led D6
#define Led1 D7

float pinD0 = 16;
float pinD1 = 5;
// REPLACE WITH RECEIVER MAC Address
uint8_t broadcastAddress[] = {0xA8, 0x48, 0xFA, 0xDC, 0x6E, 0x2C};


// Set your Board ID (ESP8266 Sender #1 = BOARD_ID 1, ESP8266 Sender #2 = BOARD_ID 2, etc)
#define BOARD_ID 1

// Structure example to send data
// Must match the receiver structure
typedef struct struct_message {
    int id;
    int temp;
    int hum;
    int gas;
    int flame;
} struct_message;

// Create a struct_message called test to store variables to be sent
struct_message myData;

unsigned long lastTime = 0;
unsigned long timerDelay =5000;

// Insert your SSID
constexpr char WIFI_SSID[] = "OPPO";

int32_t getWiFiChannel(const char *ssid) {
  if (int32_t n = WiFi.scanNetworks()) {
    for (uint8_t i = 0; i < n; i++) {
      if (!strcmp(ssid, WiFi.SSID(i).c_str())) {
        return WiFi.channel(i);
      }
    }
  }
  return 0;
}

// Callback when data is sent
void OnDataSent(uint8_t *mac_addr, uint8_t sendStatus) {
  Serial.print("\r\nLast Packet Send Status: ");
  if (sendStatus == 0){
    Serial.println("Delivery success");
  }
  else{
    Serial.println("Delivery fail");
  }
}
 
void setup() {
  // Init Serial Monitor
  Serial.begin(9600);
  Wire.begin();
  dht.begin();
  Wire.begin(D4,D3);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();

  int32_t channel = getWiFiChannel(WIFI_SSID);

  pinMode(Coi, OUTPUT);
  pinMode(Led, OUTPUT);
  pinMode(Led1, OUTPUT);
  
  // Init ESP-NOW
  if (esp_now_init() != 0) {
    Serial.println("Error initializing ESP-NOW");
    return;
  } 
  // Set ESP-NOW role
  esp_now_set_self_role(ESP_NOW_ROLE_CONTROLLER);

  // Once ESPNow is successfully init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  esp_now_add_peer(broadcastAddress, ESP_NOW_ROLE_SLAVE, 1, NULL, 0);

}
void LCD(){
    lcd.init();
    lcd.clear();
    lcd.backlight();
    lcd.setCursor(0,0);
    lcd.print("ND:");
    lcd.setCursor(3,0);
    lcd.print(myData.temp);
    lcd.setCursor(5,0);
    lcd.print((char)223);
    lcd.print("C");
    lcd.setCursor(8,0);
    lcd.print("Lua:");
    lcd.setCursor(12,0);
    lcd.print(myData.flame);
    lcd.setCursor(0,1);
    lcd.print("DA:");
    lcd.setCursor(3,1);
    lcd.print(myData.hum);
    lcd.setCursor(5,1);
    lcd.print("%");
    lcd.setCursor(8,1);
    lcd.print("Gas:");
    lcd.setCursor(12,1);
    lcd.print(myData.gas);
}

void loop() {

  int h = dht.readHumidity();    
  int t = dht.readTemperature();
  int Value = digitalRead(pinD1);
  int flameValue = digitalRead(pinD0);
  
  if ((millis() - lastTime) > timerDelay) {
    // Set values to send
    
    Serial.print("Gas: ");
    Serial.println(Value);
    Serial.print("Lua: ");
    Serial.println(flameValue);
    Serial.print("Nhiet do: ");
    Serial.print(t);
    Serial.println("*C  ");
    Serial.print("Do am: ");
    Serial.print(h);
    Serial.println("%  ");
    
    myData.id = BOARD_ID;
    myData.temp = t ;
    myData.hum = h;
    myData.gas = Value;
    myData.flame = flameValue;

    // Send message via ESP-NOW
    esp_now_send(0, (uint8_t *) &myData, sizeof(myData));
    lastTime = millis();
  }

  LCD();

  if (Value == 0 || flameValue == 1 ){
    digitalWrite(Coi, HIGH);
    digitalWrite(Led1, HIGH);
    digitalWrite(Led, LOW);
  } else {
    digitalWrite(Coi, LOW);
    digitalWrite(Led1, LOW);
    digitalWrite(Led, HIGH);
  }

}
 
