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

struct pumpTrigger {
  boolean state;
  unsigned long onTime;
  double onWaterTemp;
};

struct autoOnTrigger {
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
autoOnTrigger autoOn = {LOW, 0};
Status _status = KI;

boolean buttonPressed = false;


void setup() {
  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on. 

  thermocouple1 = new MAX6675_Thermocouple(thermo1_SCK_PIN, thermo1_CS_PIN, thermo1_SO_PIN, 20, 10);
  thermocouple2 = new MAX6675_Thermocouple(thermo2_SCK_PIN, thermo2_CS_PIN, thermo2_SO_PIN, 15, 7);

  water_temp = thermocouple1->readCelsius();
  fireplace_temp = 40; //thermocouple2->readCelsius();

  Timer1.initialize(5000000);  //1sec - 1000000
  Timer1.attachInterrupt(controlIt);

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP); 

  Serial.begin(9600);
}


void loop() { 

  u8g.firstPage();
  do {  
    draw();
  } while( u8g.nextPage() );

   if(digitalRead(buttonPin) == LOW) {
   buttonPressed = true;
  }

  if(digitalRead(buttonPin) == HIGH && buttonPressed) { //on release
     buttonPressed = false;
     if (_status == 2) {_status = 0;} else {_status = _status + 1;}

    screen.timeFrame = millis()+2000;
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
}


void controlIt()
{
  readTemps();

  fireplace_temp = fireplace_temp + 0.4;
  
  if (fireplace_temp >= 42 && autoOn.state == LOW && pump.state == LOW){
    autoOn.state = HIGH;
    autoOn.onTime = millis();
    Serial.println("auto triggered");  
    delay(5);
  }

  if (autoOn.state == HIGH) { 
    if (millis()-autoOn.onTime < 600000) {  //10 minutes
      Serial.println("auto trigger time ON");  
      if (fireplace_temp >= 50) {
          Serial.println("RELAY ON!!");
          autoOn.state = LOW;
          setPump(HIGH);
        }
    }
    else {
            autoOn.state = LOW;
            Serial.println("auto trigger time OFF");
         }
  }

  if (pump.state == HIGH && waterHot_trigger == LOW)
  {
    if (millis()-pump.onTime > 1200000 && water_temp < pump.onWaterTemp + 4)  //20 mins
     {
        setPump(LOW);
        waterHot_trigger = LOW;
        Serial.println("Watwr not warming in 20 mins - PUMP OFF");
      }
  }

  if (pump.state == HIGH && water_temp >= 30) { 
      waterHot_trigger = HIGH;
        Serial.println("waterHot_trigger HIGH");
  }
  
  if (waterHot_trigger == HIGH && water_temp <= 25) { 
        setPump(LOW);
        waterHot_trigger = LOW;
        Serial.println("PUMP OFF!!");
  }

  Serial.print("Water: ");
  Serial.print(water_temp);
  Serial.print(", Fire: ");
  Serial.print(fireplace_temp);
  Serial.print(", autoOn.state: ");
  Serial.print(autoOn.state);
  Serial.print(", autoOn.state time: ");
  Serial.print(millis()-autoOn.onTime);
  Serial.print(", pump.state: ");
  Serial.println(pump.state);  
  
   
  digitalWrite(LED_BUILTIN, pump.state);
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

void draw(){
  //https://code.google.com/archive/p/u8glib/wikis/fontsize.wiki
  //u8g.setFont(u8g_font_fur49n);
  //u8g.setFont(u8g_font_fur42n);

  if (millis()< screen.timeFrame) {u8g.drawBitmapP( 0, 0, 16, 64, screen.bitmap); return;}

  char cWT[3] = "00";
  dtostrf(water_temp, 2, 0, cWT);

  char cFT[3] = "00";
  dtostrf(fireplace_temp, 2, 0, cFT);
  
  
  u8g.setFont(u8g_font_fur42n);
  byte pos = 0;
  if (u8g.getStrWidth(cWT) > 82) pos = 20;
   u8g.drawStr(82-u8g.getStrWidth(cWT) + pos, 63, cWT);
  
  //draw degree n celsius
  u8g.drawCircle(90+pos, 26, 4);
  u8g.drawCircle(90+pos, 26, 5);

  
  u8g.setFont(u8g_font_profont15);
  if (pump.state) u8g.drawStr(0, 9, "P");

  u8g.drawStr(40, 9, screen.stat);  
  u8g.drawStr(128-u8g.getStrWidth(cFT), 9, cFT);
}

