//https://arduino.stackexchange.com/questions/37193/multiple-3-wire-spi-sensor-interfacing-with-arduino

#include <SPI.h>

#define SPI_1 9
#define SPI_2 10
#define SetPin 2
#define ResetPin 3

//Arduino SCK (pin 13) to all the SCK pins.
//Arduino MISO (pin 12) to all the DO (SO) pins



double readCelsius(uint8_t cs) {
    uint16_t v;

    digitalWrite(cs, LOW);
    v = SPI.transfer(0x00);
    v <<= 8;
    v |= SPI.transfer(0x00);
    digitalWrite(cs, HIGH);

    if (v & 0x4) {
        // uh oh, no thermocouple attached!
        return NAN; 
    }

    v >>= 3;

    return v*0.25;
}

void setup() {
    SPI.begin();
    pinMode(SPI_1, OUTPUT);
    pinMode(SPI_2, OUTPUT);
    digitalWrite(SPI_1, HIGH);
    digitalWrite(SPI_2, HIGH);


    pinMode(SetPin, OUTPUT);
    pinMode(ResetPin, OUTPUT);
    digitalWrite(SetPin, LOW);
    digitalWrite(ResetPin, LOW);
    
    Serial.begin(115200);
}

void loop() {
//    Serial.print(readCelsius(SPI_1));
//    Serial.print(" ");
//    Serial.println(readCelsius(SPI_2));
//    delay(1500);

digitalWrite(SetPin, HIGH);
delay(100);
digitalWrite(SetPin, LOW);

delay(8000);


digitalWrite(ResetPin, HIGH);
delay(100);
digitalWrite(ResetPin, LOW);

delay(8000);
}

