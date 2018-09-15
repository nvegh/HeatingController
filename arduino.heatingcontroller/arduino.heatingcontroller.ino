#include "bitmaps.h"
#include <TimerOne.h>
#include <MAX6675_Thermocouple.h>
#include "U8glib.h"

#define thermo1_SCK_PIN 3
#define thermo1_CS_PIN  4
#define thermo1_SO_PIN  5

#define thermo2_SCK_PIN 6
#define thermo2_CS_PIN  7
#define thermo2_SO_PIN  8

#define buttonPin 12
#define buttonLEDPin 11

#define relaySetPin 9
#define relayResetPin 10

#define SerialPrintln(a) (Serial.println(a))
#define SerialPrint(a) (Serial.print(a))

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);
MAX6675_Thermocouple* thermocouple1 = NULL;   //water
MAX6675_Thermocouple* thermocouple2 = NULL;   //fireplace

double water_temp = 0.0;
double fireplace_temp = 0.0;

enum Setting {
  KI = 0,
  BE = 1,
  AUTO = 2
};

enum autoStatus {
  PUMP_OFF = 0,
  WACTHING = 1,
  PUMP_ON = 2,
  WATER_HOT = 3
};

struct pumpState {
  boolean actualState;
  autoStatus autoState;
  unsigned long autoOnTime;   //fire start time
  double autoOnWaterTemp;     //pump start temp
};

struct drawScreen {
  uint8_t *bitmap;
  unsigned long timeFrame;
  char *stat;
  char *autoState;
};

drawScreen screen = {splash_screen, 3500, "KI", "0"};
pumpState pump = {LOW, PUMP_OFF, 0, 0};

Setting setting = KI;

boolean buttonPressed = false;
boolean statusChanged = true;  //for forcing pin cahnges on start

unsigned long relayTime = 0;
boolean coilON = false;

void setup() {

  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on.

  thermocouple1 = new MAX6675_Thermocouple(thermo1_SCK_PIN, thermo1_CS_PIN, thermo1_SO_PIN, 20, 10);
  thermocouple2 = new MAX6675_Thermocouple(thermo2_SCK_PIN, thermo2_CS_PIN, thermo2_SO_PIN, 15, 7);

  water_temp = thermocouple1->readCelsius();
  fireplace_temp = 40; //thermocouple2->readCelsius();

  Timer1.initialize(5000000);  //1sec - 1000000
  Timer1.attachInterrupt(controlIt);

  pinMode(buttonLEDPin, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);

  pinMode(relaySetPin, OUTPUT);
  pinMode(relayResetPin, OUTPUT);

  digitalWrite(relaySetPin, LOW);
  digitalWrite(relayResetPin, LOW);

  Serial.begin(9600);
}


void loop() {

  u8g.firstPage();
  do {
    draw();
  } while ( u8g.nextPage() );

  if (digitalRead(buttonPin) == LOW) {
    buttonPressed = true;
  }

  if (digitalRead(buttonPin) == HIGH && buttonPressed) {  //on button release
    buttonPressed = false;
    statusChanged = true;

    if (!(millis() > screen.timeFrame))  //display current status
    {
      if (setting == 2 ) {
        setting = 0;
      } else {
        setting = setting + 1;
      }
    }

    screen.timeFrame = millis() + 2000;

    switch (setting) {
      case KI:
        screen.bitmap = off_screen;
        screen.stat = "KI";
        break;
      case BE:
        screen.bitmap = on_screen;
        screen.stat = "BE";
        break;
      case AUTO:
        screen.bitmap = auto_screen;
        screen.stat = "A";
        break;
    }
  }

  if (statusChanged && millis() > screen.timeFrame)  //change screen over
  {
    switch (setting) {
      case KI:
        pumpOFF();
        //pump.autoState = PUMP_OFF;
        break;
      case BE:
        pumpON();
        break;
      case AUTO:
        if (pump.autoState == PUMP_ON || pump.autoState == WATER_HOT) {
          pumpON();
        }
        else {
          pumpOFF();
        }
        break;
    }
    statusChanged = false;
  }

  if (coilON && relayTime < millis())
  {
    digitalWrite(relayResetPin, LOW);
    digitalWrite(relaySetPin, LOW);
    coilON = false;
  }
}

void controlIt()
{
  readTemps();
  setAuto();
  printStats();
}

void setAuto()
{
  unsigned long _millis = millis();

  if (fireplace_temp >= 42 && pump.autoState == PUMP_OFF) {
    pump.autoOnTime = _millis;
    pump.autoState = WACTHING;
    SerialPrintln("auto triggered");
  }

  if (pump.autoState == WACTHING) {
    if (_millis - pump.autoOnTime < 600000) { //10 minutes
      SerialPrintln("auto trigger time ON");
      if (fireplace_temp >= 50) {
        if (setting == AUTO) {
          pumpON();
        }

        pump.autoState = PUMP_ON;
        pump.autoOnWaterTemp = water_temp;
        pump.autoOnTime = _millis;

        SerialPrintln("RELAY ON!!");
      }
    }
    else {
      SerialPrintln("auto trigger time OFF");
      pump.autoState = PUMP_OFF;
    }
  }

  if (pump.autoState == PUMP_ON)
  {
    //after 20 mins water still not warmer than +5 degrees
    if (_millis - pump.autoOnTime > 1200000 && water_temp < pump.autoOnWaterTemp + 5)
    {
      if (setting == AUTO) pumpOFF();
      pump.autoState = PUMP_OFF;
      SerialPrintln("Watwr not warming in 20 mins - PUMP OFF");
    }

    if (water_temp >= 30) {
      pump.autoState = WATER_HOT;
      SerialPrintln("WATER_HOT");
    }
  }

  if (pump.autoState == WATER_HOT && water_temp <= 25) {
    if (setting == AUTO) pumpOFF();
    pump.autoState = PUMP_OFF;
    SerialPrintln("PUMP OFF!!");
  }

  switch (pump.autoState) {
    case PUMP_OFF:
      screen.autoState = "0";
      break;
    case WACTHING:
      screen.autoState = "1";
      break;
    case PUMP_ON:
      screen.autoState = "2";
      break;
    case WATER_HOT:
      screen.autoState = "3";
      break;
  }

}

void printStats()
{
  SerialPrint("Water: ");
  SerialPrint(water_temp);
  SerialPrint(", Fire: ");
  SerialPrint(fireplace_temp);
  SerialPrint(", auto state: ");
  SerialPrint(screen.autoState);
  SerialPrint(", pump.state: ");
  SerialPrintln(pump.actualState);
}

void readTemps()
{
  water_temp = .85 * water_temp + .15 * thermocouple1->readCelsius();
  //fireplace_temp = 0.85 * fireplace_temp + .15 * thermocouple2->readCelsius();
}


void draw() {
  //https://code.google.com/archive/p/u8glib/wikis/fontsize.wiki
  //u8g.setFont(u8g_font_fur49n);
  //u8g.setFont(u8g_font_fur42n);

  if (millis() <= screen.timeFrame) {
    u8g.drawBitmapP( 0, 0, 16, 64, screen.bitmap);
    return;
  }

  char cWT[3] = "00";
  dtostrf(water_temp, 2, 0, cWT);

  char cFT[3] = "00";
  dtostrf(fireplace_temp, 2, 0, cFT);


  u8g.setFont(u8g_font_fur42n);
  byte pos = 0;
  if (u8g.getStrWidth(cWT) > 82) pos = 20;
  u8g.drawStr(82 - u8g.getStrWidth(cWT) + pos, 63, cWT);

  //draw degree n celsius
  u8g.drawCircle(90 + pos, 26, 4);
  u8g.drawCircle(90 + pos, 26, 5);


  u8g.setFont(u8g_font_profont15);
  if (pump.actualState) u8g.drawStr(0, 9, "P");

  u8g.drawStr(30, 9, screen.autoState);
  u8g.drawStr(45, 9, screen.stat);
  u8g.drawStr(128 - u8g.getStrWidth(cFT), 9, cFT);
}

void pumpON()
{
  pump.actualState = HIGH;

  digitalWrite(buttonLEDPin, HIGH);

  digitalWrite(relaySetPin, HIGH);
  coilON = true;
  relayTime = millis() + 1000;
}

void pumpOFF()
{
  pump.actualState = LOW;

  digitalWrite(buttonLEDPin, LOW);

  digitalWrite(relayResetPin, HIGH);
  coilON = true;
  relayTime = millis() + 1000;
}


