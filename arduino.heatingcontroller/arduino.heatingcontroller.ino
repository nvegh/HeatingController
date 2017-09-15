#include "U8glib.h"

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);  

void setup() {  
  
  u8g.setColorIndex(1); // Instructs the display to draw with a pixel on. 
}

void loop() {  
  u8g.firstPage();
  do {  
    draw();
  } while( u8g.nextPage() );
  delay(1000);   
}
  
void draw(){
  //https://code.google.com/archive/p/u8glib/wikis/fontsize.wiki
  //u8g.setFont(u8g_font_fur49n);
  //u8g.setFont(u8g_font_fur42n);

  char* temp = "75";

  byte pos = 0;
  if (u8g.getStrWidth(temp) > 82) pos = 20;
  u8g.setFont(u8g_font_fur42n);
  u8g.drawStr(82-u8g.getStrWidth(temp) + pos, 63, temp);
  
  //draw degree n celsius
  u8g.drawCircle(90+pos, 26, 4);
  u8g.drawCircle(90+pos, 26, 5);
  //char* c = "C";
  //u8g.setFont(u8g_font_osr35);   
  //u8g.drawStr(128-u8g.getStrWidth(c), 64, c);


  
  u8g.setFont(u8g_font_profont15);
  //u8g.setPrintPos(0, 9);
  //u8g.print("265 C");
  u8g.drawStr(0, 9, "PUMPA");
  
  char* t2 = "235 C";
  u8g.drawStr(128-u8g.getStrWidth(t2), 9, t2);
}
