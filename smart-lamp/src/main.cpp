#include <HardwareSerial.h>
#include <lamp.hpp>

#define LAMP_PIN 23

LampCtx *lamp;
volatile uint32_t seed = 0;

void setup() {
    Serial.begin(115200);
    lamp = new LampCtx(8, 10);
    lamp->setBrightness(64);
}

void loop() {
    seed++;
    delay(75);
    lamp->refresh();
    if (seed % 100 == 0) {
        lamp->nextEffect();
    }
}
