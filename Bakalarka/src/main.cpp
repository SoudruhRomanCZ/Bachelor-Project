/*resources of the used tutorials (most likely in czech, can be found in english or in arduino libraries)
https://navody.dratek.cz/navody-k-produktum/arduino-wifi-modul-nrf24l01.html -> 
->https://howtomechatronics.com/tutorials/arduino/how-to-build-an-arduino-wireless-network-with-multiple-nrf24l01-modules/ 
or 
->https://github.com/nRF24/RF24Network
https://navody.dratek.cz/navody-k-produktum/ultrazvukovy-meric-vzdalenosti-hy-srf05.html
https://navody.dratek.cz/navody-k-produktum/arduino-joystick-ps2.html
https://navody.dratek.cz/navody-k-produktum/gyroskop-a-akcelerometr.html
https://docs.arduino.cc/built-in-examples/basics/AnalogReadSerial
https://howtomechatronics.com/tutorials/arduino/rotary-encoder-works-use-arduino/ 
http://www.pjrc.com/teensy/td_libs_Encoder.html
*/
// included libraries for used periferies
#include <Arduino.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "SRF05.h"
#include <NewPing.h>
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <MCP23017.h>
/*
// structure the data that will be send out via nRF24L01 Wifi Module
struct PacketData 
{
  byte rPotValue;    
  byte switch1Value;
  byte switch2Value;
  byte switch3Value;
  byte switch4Value;  
};
PacketData data;
*/
//define the Arduino pins configuration
#define MCP23017_ADDR 0x27
MCP23017 mcp = MCP23017(MCP23017_ADDR);
// pinout of mcp23017 I/0 expander https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library
#define LED1 0     // MCP23XXX pin LED is attached to PA0
#define LED2 1     // MCP23XXX pin LED is attached to PA1
#define LED3 2     // MCP23XXX pin LED is attached to PA2
#define LED4 3     // MCP23XXX pin LED is attached to PA3
#define LED5 4     // MCP23XXX pin LED is attached to PA4
#define LED6 5     // MCP23XXX pin LED is attached to PA5

#define BUTTON1 6  // MCP23XXX pin button is attached to PA6
#define BUTTON2 7  // MCP23XXX pin button is attached to PA7
#define BUTTON3 12  // MCP23XXX pin button is attached to PB4
#define BUTTON4 13  // MCP23XXX pin button is attached to PB5
#define BUTTON5 14  // MCP23XXX pin button is attached to PB6
#define BUTTON6 15  // MCP23XXX pin button is attached to PB7

#define SWITCH1a 8 // MCP23XXX pin switch1a is connected to PB0
#define SWITCH1b 9 // MCP23XXX pin switch1b is connected to PB1
#define SWITCH2a 10 // MCP23XXX pin switch2a is connected to PB2
#define SWITCH2b 11 // MCP23XXX pin switch2b is connected to PB3

// defining arduino digital pins
// pins 0 a 1 are used for rx and tx of UART cominication,
// so it is prefered to not connect them if you dont want to experience some unexpected events
#define pinTrigger    2
#define BUZ_PIN       3
#define pinEcho       4
#define ENC_CLK_PIN   5
#define ENC_DT_PIN    6
#define ENC_SW_PIN    7
#define Joy_SW_PIN    8
#define CE 9
#define CSN 10

// pins D11,D12,D13 are reserved for SPI comunication used in nRF24L01 wifi module
//D13 is CLK, D12 is MISO, D11 is MOSI
// nRF24L01 Wifi Module inicialization
RF24 radio(CE, CSN);
RF24Network network(radio);      // Network uses that radio
const uint16_t this_node = 00;   // Address of our node == main node == in Octal format
const uint16_t other_node = 01;  // Address of the other node == next board == students arduinos? == in Octal format
struct payload_t {  // Structure of our payload
  unsigned long ms;
  unsigned long counter;
};
const unsigned long interval = 2000;  // How often (in ms) to send 'hello world' to the other unit
unsigned long last_sent;     // When did we last send?
unsigned long packets_sent;  // How many have we sent already

// analog pins
#define MIC_PIN A0
#define pinX A1
#define pinY A2
#define POT_PIN A3
// analog pins A4 and A5 are also called SDA and SCL and are used for I2C communication
// A4 is SDA and A5 is SCL, can also find on top of Digital pins, D13->GND->Ref->SDA->SCL

// inicializace měřícího modulu z knihovny
#define maxVzdalenost 450
NewPing sonar(pinTrigger, pinEcho, maxVzdalenost);
// middle Variables of JoyStick
int nulaX, nulaY;
// variable for potentionmeter read values
int  pot_value;
const int MPU_addr=0x68; // I2C address of the gyroscope and accel meter 0x68 or 0x69, defined by AD0
// used another gyroscope so the address might be different
// inicializace proměnných, do kterých se uloží data
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

// defined constants of encoder
 int counter = 0; 
 int aState;
 int aLastState;  

void setup() {
  // I2C communication inicialization for Gyroscope
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  
  // MPC I/O expansion initialization
  // kinda improvised so from Gyro code
  Wire.begin();
  Wire.beginTransmission(MCP23017_ADDR);
  Wire.write(0x6C);
  Wire.write(0);
  Wire.endTransmission(true);

  // configure LED pin for output
  mcp.pinMode(LED1, OUTPUT);
  mcp.pinMode(LED2, OUTPUT);
  mcp.pinMode(LED3, OUTPUT);
  mcp.pinMode(LED4, OUTPUT);
  mcp.pinMode(LED5, OUTPUT);
  mcp.pinMode(LED6, OUTPUT);
  // configure button pin for input with build in pull up resistor so no other resistor is needed on DPS
  mcp.pinMode(BUTTON1, INPUT_PULLUP);
  mcp.pinMode(BUTTON2, INPUT_PULLUP);
  mcp.pinMode(BUTTON3, INPUT_PULLUP);
  mcp.pinMode(BUTTON4, INPUT_PULLUP);
  mcp.pinMode(BUTTON5, INPUT_PULLUP);
  mcp.pinMode(BUTTON6, INPUT_PULLUP);
  // encoder inicialization
  pinMode(ENC_CLK_PIN,INPUT);
  pinMode(ENC_DT_PIN,INPUT);
  aLastState = digitalRead(ENC_CLK_PIN);     
  // inicializing JoyStick button
  pinMode(Joy_SW_PIN, INPUT_PULLUP);
  // read and inicialize X and Y values of JoyStick
  nulaX = analogRead(pinX);
  nulaY = analogRead(pinY);
  //potenciometr
  pot_value= analogRead(POT_PIN);
  // serial monitor speed which PC will communicate with Arduino
  Serial.begin(115200);
  //there is no inicialization of rotary encoder
  // nRF24L01 Wifi Module inicialization of network
  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!"));
    while (1) {
      // hold in infinite loop
    }
  }
  radio.setChannel(90);
  network.begin(/*node address*/ this_node);
}

void distance() {
    // načtení vzdálenosti v centimetrech do vytvořené proměnné vzdalenost
  int vzdalenost = sonar.ping_cm();
  // pauza před dalším měřením
  delay(50);
  // pokud byla detekována vzdálenost větší než 0,
  // provedeme další měření
  if (vzdalenost > 0) {
    vzdalenost = 0;
    // pro získání stabilnějších výsledků provedeme 5 měření
    // a výsledky budeme přičítat do proměnné vzdalenost
    for (int i = 0; i < 5; i++) {
      vzdalenost += sonar.ping_cm();
      delay(50);
    }
    // v proměnné vzdálenost máme součet posledních 5 měření
    // a musíme tedy provést dělení 5 pro získání průměru
    vzdalenost = vzdalenost / 5;
    // vytištění informací po sériové lince
    Serial.print("Vzdalenost mezi senzorem a predmetem je ");
    Serial.print(vzdalenost);
    Serial.println(" cm.");
  }
  // pokud byla detekována vzdálenost 0, je předmět mimo měřící rozsah,
  // tedy příliš blízko nebo naopak daleko
  else {
    Serial.println("Vzdalenost mezi senzorem a predmetem je mimo merici rozsah.");
    delay(500);
  }
}

void joystick() {
  // vytvoření proměnných pro uložení
  // hodnot pro osy x, y a stav tlačítka
  int aktX, aktY, stavTlac;
  // načtení analogových hodnot osy x a y
  aktX = analogRead(pinX) - nulaX;
  aktY = analogRead(pinY) - nulaY;
  // načtení stavu tlačítka
  stavTlac = digitalRead(Joy_SW_PIN);
  if (aktX > 0) {
    aktX = map(aktX, 0, 1023-nulaX, 0, 100);
  }
  else {
    aktX = map(aktX, 0, -nulaX, 0, -100);
  }
  if (aktY > 0) {
    aktY = map(aktY, 0, 1023-nulaY, 0, 100);
  }
  else {
    aktY = map(aktY, 0, -nulaY, 0, -100);
  }
  // vytištění informací o souřadnicích
  // po sériové lince
  Serial.print("Souradnice X,Y = ");
  Serial.print(aktX);
  Serial.print(", ");
  Serial.print(aktY);
  // kontrola stavu tlačítka, v případě stisku
  // vytiskneme informaci po sériové lince
  if(stavTlac == LOW) {
    Serial.print(" | Tlacitko stisknuto.");
  }
  Serial.println();
  // volitelná pauza pro přehledný tisk
  // delay(1000);
}

void gyro_accel()
{
  // zapnutí přenosu
  Wire.beginTransmission(MPU_addr);
  // zápis do registru ACCEL_XOUT_H
  Wire.write(0x3B);
  Wire.endTransmission(false);
  // vyzvednutí dat z 14 registrů
  Wire.requestFrom(MPU_addr,14,true);
  AcX=Wire.read()<<8|Wire.read();    
  AcY=Wire.read()<<8|Wire.read();
  AcZ=Wire.read()<<8|Wire.read();
  Tmp=Wire.read()<<8|Wire.read();
  GyX=Wire.read()<<8|Wire.read();
  GyY=Wire.read()<<8|Wire.read();
  GyZ=Wire.read()<<8|Wire.read();
  // výpis surových dat z proměnných na sériovou linku
  Serial.print("AcX = "); Serial.print(AcX);
  Serial.print(" | AcY = "); Serial.print(AcY);
  Serial.print(" | AcZ = "); Serial.print(AcZ);
  // přepočtení teploty dle datasheetu
  Serial.print(" | Temp = "); Serial.print(Tmp/340.00+36.53);
  Serial.print(" | GyX = "); Serial.print(GyX);
  Serial.print(" | GyY = "); Serial.print(GyY);
  Serial.print(" | GyZ = "); Serial.println(GyZ);
}

void pot() {
  int potV = analogRead(POT_PIN);
  Serial.print("Potentionmeter value = ");
  Serial.println(potV);
}

void MCP() {
  //just inverting the buttons input so which LED will light when button is holded
  mcp.digitalWrite(LED1, !mcp.digitalRead(BUTTON1));
  mcp.digitalWrite(LED2, !mcp.digitalRead(BUTTON2));
  mcp.digitalWrite(LED3, !mcp.digitalRead(BUTTON3));
  mcp.digitalWrite(LED4, !mcp.digitalRead(BUTTON4));
  mcp.digitalWrite(LED5, !mcp.digitalRead(BUTTON5));
  mcp.digitalWrite(LED6, !mcp.digitalRead(BUTTON6));
}
void wifi_vysilac() {
  network.update();  // Check the network regularly
  unsigned long now = millis();
  // If it's time to send a message, send it!
  if (now - last_sent >= interval) {
    last_sent = now;
    Serial.print(F("Sending... "));
    payload_t payload = { millis(), packets_sent++ };
    RF24NetworkHeader header(/*to node*/ other_node);
    bool ok = network.write(header, &payload, sizeof(payload));
    Serial.println(ok ? F("ok.") : F("failed."));
  }
}

void wifi_prijimac(){
    network.update();  // Check the network regularly
  while (network.available()) {  // Is there anything ready for us?
    RF24NetworkHeader header;  // If so, grab it and print it out
    payload_t payload;
    network.read(header, &payload, sizeof(payload));
    Serial.print(F("Received packet: counter="));
    Serial.print(payload.counter);
    Serial.print(F(", origin timestamp="));
    Serial.println(payload.ms);
}
}

void encoder() {
   aState = digitalRead(ENC_CLK_PIN); // Reads the "current" state of the ENC_CLK_PIN
   // If the previous and the current state of the ENC_CLK_PIN are different, that means a Pulse has occured
   if (aState != aLastState){     
     // If the ENC_DT_PIN state is different to the ENC_CLK_PIN state, that means the encoder is rotating clockwise
     if (digitalRead(ENC_DT_PIN) != aState) { 
       counter ++;
     } else {
       counter --;
     }
     Serial.print("Position: ");
     Serial.println(counter);
   } 
   aLastState = aState; // Updates the previous state of the ENC_CLK_PIN with the current state
}

void led() {
// must think of the function for this segment, it will depend on the PureData outputs
}

void button() {
// must think of the function for this segment, it will depend on the PureData inputs
}

void loop() {
  //wifi_vysilac();
  //delay(1000);
  distance();
  delay(1000);
  gyro_accel();
  delay(1000);
  pot();
  delay(1000);
  encoder();
  delay(1000);
  MCP();
  delay(1000);
}