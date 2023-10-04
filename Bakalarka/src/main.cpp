#include <Arduino.h>
#include "I2Cdev.h"
#include "MPU6050.h"
#include "Wire.h"
#include "SRF05.h"
#include <NewPing.h>
#include <SPI.h>
#include "RF24.h"

//define the Arduino pins configuration
#define WIFI_RX_PIN 0
#define WIFI_TX_PIN 1
// nRF24L01 vysílač
// https://navody.dratek.cz/navody-k-produktum/arduino-wifi-modul-nrf24l01.html
// nastavení propojovacích pinů
#define CE 6
#define CS 7
// inicializace nRF s piny CE a CS
RF24 nRF(CE, CS);
// nastavení adres pro přijímač a vysílač,
// musí být nastaveny stejně v obou programech!
byte adresaPrijimac[]= "prijimac00";
byte adresaVysilac[]= "vysilac00";
// konec wifi inicializace
#define POT_PIN A3 // https://docs.arduino.cc/built-in-examples/basics/AnalogReadSerial
#define ENC_CLK_PIN 3
#define ENC_DT_PIN 4
#define ENC_SW_PIN 5
#define SW1_PIN 8
#define SW2_PIN 9
// #define BUZ_PIN 13
// Ultrazvukový modul HY-SRF05 pro měření vzdálenosti
// https://navody.dratek.cz/navody-k-produktum/ultrazvukovy-meric-vzdalenosti-hy-srf05.html
// připojení potřebné knihovny
// nastavení propojovacích pinů
#define pinTrigger    10
#define pinEcho       11
#define maxVzdalenost 450
// inicializace měřícího modulu z knihovny
NewPing sonar(pinTrigger, pinEcho, maxVzdalenost);
// Arduino Joystick PS2
// https://navody.dratek.cz/navody-k-produktum/arduino-joystick-ps2.html
// nastavení propojovacích pinů modulu
#define pinX A0
#define pinY A1
#define pinKey 12
// proměnné pro uložení hodnoty
// středu joysticku
int nulaX, nulaY;

// Arduino gyroskop a akcelerometr 1
// https://navody.dratek.cz/navody-k-produktum/gyroskop-a-akcelerometr.html
// inicializace proměnné pro určení adresy senzoru
// 0x68 nebo 0x69, dle připojení AD0
const int MPU_addr=0x68;
// inicializace proměnných, do kterých se uloží data
int16_t AcX,AcY,AcZ,Tmp,GyX,GyY,GyZ;

//#define MIC_PIN A0
// #define IO_expander A1 A2 

void setup() {
  // komunikace přes I2C sběrnici
  Wire.begin();
  Wire.beginTransmission(MPU_addr);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  // komunikace přes sériovou linku rychlostí 115200 baud
  Serial.begin(115200);
  // inicializace tlačítka joysticku
  pinMode(pinKey, INPUT_PULLUP);
  // komunikace přes sériovou linku rychlostí 9600 baud
  // tuto rychlost používá ultrazvuk a joystick
  // Serial.begin(9600);
  // načtení a uložení hodnot pro x a y osy
  nulaX = analogRead(pinX);
  nulaY = analogRead(pinY);

  //potenciometr

  //wifi
  // zapnutí komunikace nRF modulu
  nRF.begin();
  // nastavení výkonu nRF modulu,
  // možnosti jsou RF24_PA_MIN, RF24_PA_LOW, RF24_PA_HIGH and RF24_PA_MAX,
  // pro HIGH a MAX je nutný externí 3,3V zdroj
  nRF.setPALevel(RF24_PA_LOW);
  // nastavení zapisovacího a čtecího kanálu
  nRF.openWritingPipe(adresaVysilac);
  nRF.openReadingPipe(1,adresaPrijimac);
  // začátek příjmu dat
  nRF.startListening();
}

void distance() {
  // načtení vzdálenosti v centimetrech do vytvořené proměnné vzdalenost
  int vzdalenost = sonar.ping_cm();
  // pauza před dalším měřením
  delay(500);
  // pokud byla detekována vzdálenost větší než 0,
  // provedeme další měření
  if (vzdalenost > 0) {
    vzdalenost = 0;
    // pro získání stabilnějších výsledků provedeme 5 měření
    // a výsledky budeme přičítat do proměnné vzdalenost
    for (int i = 0; i < 5; i++) {
      vzdalenost += sonar.ping_cm();
      delay(100);
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
    // delay(500);
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
  stavTlac = digitalRead(pinKey);
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
  Serial.print("Potentionmeter value = ");Serial.println(potV);
}

void wifi() {
  // for smyčka pro postupné odeslání
  // hodnot 0 až 3 pro načtení všech dat
  // z přijímače
  for (int i=0; i < 4; i++ ) {
    // ukončení příjmu dat
    nRF.stopListening();
    // vytisknutí aktuální volby po sériové lince
    Serial.print("Posilam volbu ");
    Serial.println(i);
    // uložení startovního času komunikace
    unsigned long casZacatek = micros();
    // odeslání aktuální volby, v případě selhání
    // vytištění chybové hlášky na sériovou linku
    if (!nRF.write( &i, sizeof(i) )){
       Serial.println("Chyba při odeslání!");
    }
    // přepnutí do příjmu dat pro další komunikaci
    nRF.startListening();
    // uložení času začátku čekání
    unsigned long casCekaniOdezvy = micros();
    // proměnná s uložením stavu čekání na odezvu
    // od přijímače - "timeout"
    boolean timeout = false;
    // čekací while smyčka na odezvu od přijímače
    while ( ! nRF.available() ){
      // pokud do 200 000 mikrosekund nepřijde odezva,
      // ukonči čekání a nastav timeout
      if (micros() - casCekaniOdezvy > 200000 ){
          timeout = true;
          break;
      }      
    }
    // kontrola stavu timeoutu
    if ( timeout ){
      // v případě vypršení čekací smyčky,
      // vytiskni informaci o chybě spojení
      Serial.println("Chyba při prijmu, vyprseni casu na odezvu!");
    }
    // v opačném případě ulož přijatou zprávu a vypiš ji po sériové lince
    else{
        // proměnná pro uložení přijatých dat
        unsigned long prijataData;
        // příjem dat se zápisem do proměnné prijataData
        nRF.read( &prijataData, sizeof(prijataData) );
        // uložení času konce komunikace
        unsigned long casKonec = micros();
        // výpis dat z komunikace po sériové lince
        // včetně délky trvání spojení
        Serial.print("Odeslana volba: ");
        Serial.print(i);
        Serial.print(", prijata data: ");
        Serial.println(prijataData);
        Serial.print("Delka spojeni: ");
        Serial.print(casKonec - casZacatek);
        Serial.println(" mikrosekund.");
    }
  }
}

void encoder() {

}

void led() {

}

void button() {

}

void loop() {
  distance();
  delay(1000);
  joystick();
  delay(1000);
  gyro_accel();
  delay(1000);
  pot();
  delay(1000);
}