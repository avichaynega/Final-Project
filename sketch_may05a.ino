/*
 * --------------------------------------------------------------------------------------------------------------------
 * Example sketch/program showing how to read data from a PICC to serial.
 * --------------------------------------------------------------------------------------------------------------------
 * This is a MFRC522 library example; for further details and other examples see: https://github.com/miguelbalboa/rfid
 * 
 * Example sketch/program showing how to read data from a PICC (that is: a RFID Tag or Card) using a MFRC522 based RFID
 * Reader on the Arduino SPI interface.
 * 
 * When the Arduino and the MFRC522 module are connected (see the pin layout below), load this sketch into Arduino IDE
 * then verify/compile and upload it. To see the output: use Tools, Serial Monitor of the IDE (hit Ctrl+Shft+M). When
 * you present a PICC (that is: a RFID Tag or Card) at reading distance of the MFRC522 Reader/PCD, the serial output
 * will show the ID/UID, type and any data blocks it can read. Note: you may see "Timeout in communication" messages
 * when removing the PICC from reading distance too early.
 * 
 * If your reader supports it, this sketch/program will read all the PICCs presented (that is: multiple tag reading).
 * So if you stack two or more PICCs on top of each other and present them to the reader, it will first output all
 * details of the first and then the next PICC. Note that this may take some time as all data blocks are dumped, so
 * keep the PICCs at reading distance until complete.
 * 
 * @license Released into the public domain.
 * 
 * Typical pin layout used:
 * -----------------------------------------------------------------------------------------
 *             MFRC522      Arduino       Arduino   Arduino    Arduino          Arduino
 *             Reader/PCD   Uno/101       Mega      Nano v3    Leonardo/Micro   Pro Micro
 * Signal      Pin          Pin           Pin       Pin        Pin              Pin
 * -----------------------------------------------------------------------------------------
 * RST/Reset   RST          9             5         D9         RESET/ICSP-5     RST
 * SPI SS      SDA(SS)      10            53        D10        10               10
 * SPI MOSI    MOSI         11 / ICSP-4   51        D11        ICSP-4           16
 * SPI MISO    MISO         12 / ICSP-1   50        D12        ICSP-1           14
 * SPI SCK     SCK          13 / ICSP-3   52        D13        ICSP-3           15
 */

#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "HX711.h"
HX711 loadcell;

#define RST_PIN         22          // Configurable, see typical pin layout above
#define SS_PIN          5         // Cnfigurable, see typical pin layout above

const int LOADCELL_DOUT_PIN = 26;
const int LOADCELL_SCK_PIN = 27;

const long LOADCELL_DIVIDER = 1970.403;

char ssid[] = "ido";
char password[] = "0504571865";

// parents home
//char ssid[] = "IdosNet";
//char password[] = "0507332821";

const char* brokerUsername = "1337souless@gmail.com";
const char* brokerPassword = "b6820e33";
const char* broker = "mqtt.dioty.co";

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];

const byte led_gpio = 32;
const byte actuator_gpio = 15;
const byte actuator_gpio_c = 2;

float weight = 0;
float weight_now = 0;
float prev_weight = 0;
float counter = 0;
float food_counter = -1;

bool flag = true;
int status = WL_IDLE_STATUS;

void setup() {
  Serial.begin(115200);   // Initialize serial communications with the PC
  WiFi.begin(ssid, password);

  while (!Serial);

// disable wifi

  while (status != WL_CONNECTED){    // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4)

    Serial.print("Attempting to connect to WPA SSID: ");

    Serial.println(ssid);

    status = WiFi.begin(ssid, password);

    // wait 10 seconds for connection:

    delay(5000);
    
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // 3. Initialize library for hx711
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(LOADCELL_DIVIDER);
  loadcell.tare();
//  loadcell.set_offset(LOADCELL_OFFSET);
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

  client.setServer(broker, 1883);
  client.setCallback(callback);
  
  //Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
  pinMode(led_gpio, OUTPUT);
  pinMode(actuator_gpio, OUTPUT);
  pinMode(actuator_gpio_c, OUTPUT);
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
//    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.print(messageTemp);
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(led_gpio, HIGH);
    }
    else if(messageTemp == "off"){
  // Changes the output state according to the message
  if (String(topic) == "/1337souless@gmail.com/esp32/output") {
    Serial.print("Changing output to ");
      Serial.println("off");
      digitalWrite(led_gpio, LOW);
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", brokerUsername, brokerPassword)) {
      //Serial.println("connected");
      // Subscribe
      client.subscribe("/1337souless@gmail.com/esp32/output");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void actuator() {
  // using 2 relays to reverse the polarity of an actuator
  // to get it to open then close.
  // algo:
  // turn on gpio 2 to activate the actuator
  // turn off 2
  // turn on 15 to reverse polarity
  // turn on 2 to activate actuator
  // turn off 2
  // turn off 15

  // use 2 gpio to open the actuator
  digitalWrite(actuator_gpio_c, HIGH);

  Serial.println(F("Connection open for 5 seconds.."));
  delay(5000);

  // close gpio 2
  digitalWrite(actuator_gpio_c, LOW);

  // turn on 15 gpio
  digitalWrite(actuator_gpio, HIGH);   // turn the LED off (LOW voltage level)
  delay(1000);

  // turn on 2 gpio to contract the actuator
  digitalWrite(actuator_gpio_c, HIGH);   // turn the LED off (LOW voltage level)
  delay(5000);

  // turn off 2
  digitalWrite(actuator_gpio_c, LOW);   // turn the LED off (LOW voltage level)
  delay(1000);

  // turn off 15
  digitalWrite(actuator_gpio, LOW);
  Serial.println(F("Connection closed."));
  
  
}

void loop() {

  if (flag) {
    
    digitalWrite(led_gpio, HIGH);   // turn the LED off (LOW voltage level)
    delay(1000);
    digitalWrite(led_gpio, LOW);   // turn the LED off (LOW voltage level)
    flag = false;
  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // Look for new cards
  if ( ! mfrc522.PICC_IsNewCardPresent()) {         
    return;
  }

  // Select one of the cards
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Dump debug info about the card; PICC_HaltA() is automatically called
  mfrc522.PICC_DumpToSerial(&(mfrc522.uid));
  
  digitalWrite(led_gpio, HIGH);   // turn the LED on (HIGH voltage level)
  delay(1000);
  digitalWrite(led_gpio, LOW);

  // count rf frequency
  counter = counter + 1;

  // Acquire reading from weight sensor
  weight = loadcell.get_units(10);
  Serial.print("Weight: ");
  Serial.println(weight);

  // refill food
  if (weight < 10) {
    actuator();
    food_counter = food_counter + 1;

    weight_now = loadcell.get_units(10);
    prev_weight = weight_now;

    delay(500);
    
    emitWeight();
    emitFrequency();
    
  }
  else {
    
    // rf without refill and slight change of weight
    if (weight < prev_weight - 2) {
      emitFrequency();
      weight_now = loadcell.get_units(10);
      emitWeight();
      prev_weight = weight_now;
    }

    // rf without refill and without change of weight
    else {
      emitFrequency();
      weight_now = weight;
      emitWeight();
    }
  }


}

void emitFrequency() {
 
  char fString[8];
  dtostrf(counter, 1, 2, fString);
  Serial.print("Frequency: ");
  Serial.println(counter);
  delay(1000);
  client.publish("/1337souless@gmail.com/esp32/frequency", fString); 
}

void emitWeight() {
    // Convert the value to a char array
  char dwString[8];
  float dw;
  if (food_counter == 0) {
    dw = (weight_now - prev_weight);
  }
  else {
    dw = (weight_now - prev_weight) * food_counter;
  }
  dtostrf(dw, 1, 2, dwString);
  Serial.print("Delta Weight: ");
  Serial.println(dw);
  client.publish("/1337souless@gmail.com/esp32/delta_weight", dwString);

  char wString[8];
  dtostrf(weight_now, 1, 2, wString);
  Serial.print("Weight: ");
  Serial.println(weight_now);
  client.publish("/1337souless@gmail.com/esp32/weight", wString);
  
}


