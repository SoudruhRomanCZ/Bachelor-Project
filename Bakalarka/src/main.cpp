#include <Arduino.h>


//define the Arduino pins configuration
#define WIFI_RX_PIN 0
#define WIFI_TX_PIN 1
#define POT_PIN 2
#define ENC_CLK_PIN 3
#define ENC_DT_PIN 4
#define ENC_SW_PIN 5
#define ULT_TRIG_PIN 6
#define ULT_ECHO_PIN 7
#define SW1_PIN1 8
#define SW1_PIN2 9
#define SW2_PIN1 10
#define SW2_PIN2 11
#define BUZ_PIN 12
#define MIC_PIN A0
// #define IO_expander A1 A2 

#define GYR_SDA_PIN A4
#define GYR_SCLK_PIN A5

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  int result = myFunction(2, 3);
}

void loop() {
  // put your main code here, to run repeatedly:
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}