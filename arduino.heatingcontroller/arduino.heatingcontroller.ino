#include <TimerOne.h>
#include <MAX6675_Thermocouple.h>
#include "U8glib.h"

#define thermo1_SCK_PIN 3
#define thermo1_CS_PIN  4
#define thermo1_SO_PIN  5

#define thermo2_SCK_PIN 6
#define thermo2_CS_PIN  7
#define thermo2_SO_PIN  8

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);  
MAX6675_Thermocouple* thermocouple1 = NULL;   //water
MAX6675_Thermocouple* thermocouple2 = NULL;   //fireplace

double water_temp = 0.0;
double fireplace_temp = 0.0;
boolean relay_switch = LOW;

double filteredReading = 0;


void setup() {
  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on. 

  thermocouple1 = new MAX6675_Thermocouple(thermo1_SCK_PIN, thermo1_CS_PIN, thermo1_SO_PIN, 20, 10);
  thermocouple2 = new MAX6675_Thermocouple(thermo2_SCK_PIN, thermo2_CS_PIN, thermo2_SO_PIN, 15, 7);

  water_temp = thermocouple1->readCelsius();
  //fireplace_temp = thermocouple2->readCelsius();

  Timer1.initialize(10000000);  //1sec - 1000000
  Timer1.attachInterrupt(controlIt);

  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(9600);
}


void loop() { 

  u8g.firstPage();
  do {  
    draw();
  } while( u8g.nextPage() );
  //delay(1000);   
}


void controlIt()
{
  readTemps(); 

  Serial.println(water_temp);
  //water_temp = water_temp + 0.5;


  //logic to switch the relay on
  relay_switch = water_temp > 30;
  
  digitalWrite(LED_BUILTIN, relay_switch);
}

void readTemps()
{
  water_temp = .85 * water_temp + .15 * thermocouple1->readCelsius();
   //fireplace_temp = 0.85 * fireplace_temp + .15 * thermocouple2->readCelsius();
}
  
void draw(){
  //https://code.google.com/archive/p/u8glib/wikis/fontsize.wiki
  //u8g.setFont(u8g_font_fur49n);
  //u8g.setFont(u8g_font_fur42n);

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
  if (relay_switch) u8g.drawStr(0, 9, "PUMPA");

  u8g.drawStr(128-u8g.getStrWidth(cFT), 9, cFT);
}

