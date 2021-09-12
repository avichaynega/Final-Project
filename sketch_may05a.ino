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

char ssid[] = "**********";
char password[] = "*********";

const char* brokerUsername = "afmproject2021@gmail.com";
const char* brokerPassword = "*********";

const char* broker = "mqtt.dioty.co";

MFRC522 mfrc522(SS_PIN, RST_PIN);  // Create MFRC522 instance
WiFiClient espClient;
PubSubClient client(espClient);

const byte led_gpio = 32;
const byte actuator_gpio = 15;
const byte actuator_gpio_c = 2;

const float engine_time = 1000;

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
  
  SPI.begin();      // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522
  mfrc522.PCD_DumpVersionToSerial();  // Show details of PCD - MFRC522 Card Reader details

  client.setServer(broker, 1883);
  client.setCallback(callback);
  client.connect("ESP8266Client", brokerUsername, brokerPassword);

  client.subscribe("/afmproject2021@gmail.com/afm/debug");
  
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
    messageTemp += (char)message[i];
  }
  Serial.print(messageTemp);
  Serial.println();

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
    if(messageTemp == "on"){
      Serial.println("on");
      digitalWrite(led_gpio, HIGH);
    }
    else if(messageTemp == "off"){
      Serial.print("Changing output to ");
      Serial.println("off");
      digitalWrite(led_gpio, LOW);
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    //Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client", brokerUsername, brokerPassword)) {
      // Subscribe
      client.subscribe("/afmproject2021@gmail.com/afm/output");
      client.subscribe("/afmproject2021@gmail.com/afm/debug");
    } else {

      // Possible values for client.state()
      //#define MQTT_CONNECTION_TIMEOUT     -4
      //#define MQTT_CONNECTION_LOST        -3
      //#define MQTT_CONNECT_FAILED         -2
      //#define MQTT_DISCONNECTED           -1
      //#define MQTT_CONNECTED               0
      //#define MQTT_CONNECT_BAD_PROTOCOL    1
      //#define MQTT_CONNECT_BAD_CLIENT_ID   2
      //#define MQTT_CONNECT_UNAVAILABLE     3
      //#define MQTT_CONNECT_BAD_CREDENTIALS 4
      //#define MQTT_CONNECT_UNAUTHORIZED    5 
      
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
  delay(engine_time);

  // close gpio 2
  digitalWrite(actuator_gpio_c, LOW);

  // turn on 15 gpio
  digitalWrite(actuator_gpio, HIGH);   // turn the LED off (LOW voltage level)
  delay(1000);

  // turn on 2 gpio to contract the actuator
  digitalWrite(actuator_gpio_c, HIGH);   // turn the LED off (LOW voltage level)
  delay(engine_time * 2);

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

    dbg("WiFi Connected!");
  }

//  dbg("AFM Online");

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

  dbg("ID Identified");
  
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

    dbg("REFILL");
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
      dbg("Plate is not empty!");
    }

    // rf without refill and without change of weight
    else {
      dbg("Plate is not empty!");
      emitFrequency();
      weight_now = weight;
      emitWeight();
    }
  }


}

void dbg(char* msg) {
  Serial.print("Debug: ");
  Serial.println(msg);
  boolean rc = client.publish("/afmproject2021@gmail.com/afm/debug", msg);
  Serial.println(rc);

}

void emitFrequency() {
 
  char fString[8];
  dtostrf(counter, 1, 2, fString);
  Serial.print("Frequency: ");
  Serial.println(counter);
  delay(1000);
  client.publish("/afmproject2021@gmail.com/afm/frequency", fString); 
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
  client.publish("/afmproject2021@gmail.com/afm/delta_weight", dwString);

  char wString[8];
  dtostrf(weight_now, 1, 2, wString);
  Serial.print("Weight: ");
  Serial.println(weight_now);
  client.publish("/afmproject2021@gmail.com/afm/weight", wString);
  
}



