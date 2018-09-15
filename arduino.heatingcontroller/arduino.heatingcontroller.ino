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
boolean waterHot_trigger = LOW;

enum Status {
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

struct pumpTrigger {
  boolean state;
  unsigned long onTime;
  double onWaterTemp;
};

struct timerTrigger {
  boolean state;
  unsigned long onTime;
};

struct drawScreen {
  uint8_t *bitmap;
  unsigned long timeFrame;
  char *stat;
};

drawScreen screen = {splash_screen, 3500, "KI"};
pumpTrigger pump = {LOW, 0, 0};
timerTrigger autoOn = {LOW, 0};
Status _status = KI;
autoStatus _autoStatus = PUMP_OFF;

boolean buttonPressed = false;
boolean statusChanged = true;  //for forcing pin cahnges on start
unsigned long relayTime;
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
      if (_status == 2 ) {
        _status = 0;
      } else {
        _status = _status + 1;
      }
    }

    screen.timeFrame = millis() + 2000;

    switch (_status) {
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
    switch (_status) {
      case KI:
        pumpOFF();
        break;
      case BE:
        pumpON();
        break;
      case AUTO:
        if (_autoStatus == PUMP_ON || _autoStatus == WATER_HOT) { pumpON(); }
        else { pumpOFF(); }
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
  if (fireplace_temp >= 42 && autoOn.state == LOW && pump.state == LOW) {
    autoOn.state = HIGH;
    autoOn.onTime = millis();
    SerialPrintln("auto triggered");
    _autoStatus = WACTHING;
    delay(5);
  }

  if (autoOn.state == HIGH) {
    if (millis() - autoOn.onTime < 600000) { //10 minutes
      SerialPrintln("auto trigger time ON");
      if (fireplace_temp >= 50) {
        SerialPrintln("RELAY ON!!");
        autoOn.state = LOW;
        if (_status == AUTO) setPump(HIGH);
        _autoStatus = PUMP_ON;
      }
    }
    else {
      autoOn.state = LOW;
      SerialPrintln("auto trigger time OFF");
      if (_status == AUTO) _autoStatus = PUMP_OFF;
    }
  }

  if (pump.state == HIGH && waterHot_trigger == LOW)
  {
    if (millis() - pump.onTime > 1200000 && water_temp < pump.onWaterTemp + 5) //after 20 mins water still not warmer than +5 degrees
    {
      if (_status == AUTO) setPump(LOW);
      waterHot_trigger = LOW;
      SerialPrintln("Watwr not warming in 20 mins - PUMP OFF");
      _autoStatus = PUMP_OFF;
    }
  }

  if (pump.state == HIGH && water_temp >= 30) {
    waterHot_trigger = HIGH;
    SerialPrintln("waterHot_trigger HIGH");
    _autoStatus = WATER_HOT;
  }

  if (waterHot_trigger == HIGH && water_temp <= 25) {
    if (_status == AUTO) setPump(LOW);
    waterHot_trigger = LOW;
    SerialPrintln("PUMP OFF!!");
    _autoStatus = PUMP_OFF;
  }
}

void printStats()
{
  SerialPrint("Water: ");
  SerialPrint(water_temp);
  SerialPrint(", Fire: ");
  SerialPrint(fireplace_temp);
  SerialPrint(", autoOn.state: ");
  SerialPrint(autoOn.state);
  SerialPrint(", autoOn.state time: ");
  SerialPrint(millis() - autoOn.onTime);
  SerialPrint(", pump.state: ");
  SerialPrintln(pump.state);
}

void readTemps()
{
  water_temp = .85 * water_temp + .15 * thermocouple1->readCelsius();
  //fireplace_temp = 0.85 * fireplace_temp + .15 * thermocouple2->readCelsius();
}

void setPump(boolean value)
{
  pump.state = value;
  pump.onTime = millis();
  pump.onWaterTemp = water_temp;
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
  if (pump.state) u8g.drawStr(0, 9, "P");

  u8g.drawStr(40, 9, screen.stat);
  u8g.drawStr(128 - u8g.getStrWidth(cFT), 9, cFT);
}

void pumpON()
{
  digitalWrite(buttonLEDPin, HIGH);
  digitalWrite(relaySetPin, HIGH);
  coilON = true;
  relayTime = millis() + 1000;
}

void pumpOFF()
{
  digitalWrite(buttonLEDPin, LOW);
  digitalWrite(relayResetPin, HIGH);
  coilON = true;
  relayTime = millis() + 1000;
}
