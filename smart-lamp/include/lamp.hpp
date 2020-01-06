#ifndef SMART_LAMP_LAMP_HPP
#define SMART_LAMP_LAMP_HPP

#include <FastLED.h>

#define LAMP_PIN 23

class LampCtx;

class LampEffect {
public:
    virtual void draw(LampCtx *lamp, uint32_t seed) = 0;
};

class MatrixEffect : public LampEffect {
public:
    void draw(LampCtx *lamp, uint32_t seed) override;
};

class ColorsEffect : public LampEffect {
public:
    int8_t hue = 0;

    void draw(LampCtx *lamp, uint32_t seed) override;
};

class SnowEffect : public LampEffect {
public:
    void draw(LampCtx *lamp, uint32_t seed) override;
};

class FireEffect : public LampEffect {
    unsigned char *line;
    bool loadingLine = true;
    int pcnt = 0;
    uint8_t width;
    uint8_t height;
    unsigned char matrixValue[8][16];

    const unsigned char valueMask[8][16] PROGMEM = {
            {32,  0,   0,   0,   0,   0,   0,   32,  32,  0,   0,   0,   0,   0,   0,   32},
            {64,  0,   0,   0,   0,   0,   0,   64,  64,  0,   0,   0,   0,   0,   0,   64},
            {96,  32,  0,   0,   0,   0,   32,  96,  96,  32,  0,   0,   0,   0,   32,  96},
            {128, 64,  32,  0,   0,   32,  64,  128, 128, 64,  32,  0,   0,   32,  64,  128},
            {160, 96,  64,  32,  32,  64,  96,  160, 160, 96,  64,  32,  32,  64,  96,  160},
            {192, 128, 96,  64,  64,  96,  128, 192, 192, 128, 96,  64,  64,  96,  128, 192},
            {255, 160, 128, 96,  96,  128, 160, 255, 255, 160, 128, 96,  96,  128, 160, 255},
            {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}
    };

    const unsigned char hueMask[8][16] PROGMEM = {
            {1, 11, 19, 25, 25, 22, 11, 1, 1, 11, 19, 25, 25, 22, 11, 1},
            {1, 8,  13, 19, 25, 19, 8,  1, 1, 8,  13, 19, 25, 19, 8,  1},
            {1, 8,  13, 16, 19, 16, 8,  1, 1, 8,  13, 16, 19, 16, 8,  1},
            {1, 5,  11, 13, 13, 13, 5,  1, 1, 5,  11, 13, 13, 13, 5,  1},
            {1, 5,  11, 11, 11, 11, 5,  1, 1, 5,  11, 11, 11, 11, 5,  1},
            {0, 1,  5,  8,  8,  5,  1,  0, 0, 1,  5,  8,  8,  5,  1,  0},
            {0, 0,  1,  5,  5,  1,  0,  0, 0, 0,  1,  5,  5,  1,  0,  0},
            {0, 0,  0,  1,  1,  0,  0,  0, 0, 0,  0,  1,  1,  0,  0,  0}
    };
public:
    FireEffect(uint8_t width, uint8_t height) {
        this->width = width;
        this->height = height;
        line = new unsigned char[width];
        memset(matrixValue, 0, sizeof(matrixValue));
    }

    void draw(LampCtx *lamp, uint32_t seed) override;

private:
    void generateLine() {
        for (uint8_t x = 0; x < width; x++) {
            line[x] = random(64, 255);
        }
    }

    void shiftUp() {
        for (uint8_t y = height - 1; y > 0; y--) {
            for (uint8_t x = 0; x < width; x++) {
                uint8_t newX = x;
                if (x > 15) newX = x - 15;
                if (y > 7) continue;
                matrixValue[y][newX] = matrixValue[y - 1][newX];
            }
        }

        for (uint8_t x = 0; x < width; x++) {
            uint8_t newX = x;
            if (x > 15) newX = x - 15;
            matrixValue[0][newX] = line[newX];
        }
    }

    void drawFrame(LampCtx *lamp);
};

class LampCtx {

    CRGB *leds;
    uint32_t seed = 0;
    LampEffect **effects;
    uint8_t effectsCount;
    uint8_t effectsCurrent;

public:
    int8_t width;
    int8_t height;
    int16_t count;

    LampCtx(int8_t width, int8_t height) {
        this->width = width;
        this->height = height;
        count = width * height;
        leds = new CRGB[count];
        initLEDs();
        fillAll(CRGB(0x00, 0xFF, 0x00));
        apply();
        effectsCount = 4;
        effects = new LampEffect *[effectsCount];
        effects[0] = new SnowEffect();
        effects[1] = new ColorsEffect();
        effects[2] = new MatrixEffect();
        effects[3] = new FireEffect(width, height);
        effectsCurrent = 0;
    };

    void setEffect(uint8_t index) {
        if (index > 0 && index < effectsCount) {
            effectsCurrent = index;
        }
    }

    void nextEffect() {
        if (effectsCurrent == effectsCount - 1) {
            effectsCurrent = 0;
        } else {
            effectsCurrent++;
        }
    }

    void refresh() {
        effects[effectsCurrent]->draw(this, seed);
        seed++;
    }

    void fillAll(CRGB color) {
        for (auto i = 0; i < count; i++) {
            leds[i] = color;
        }
    }

    void drawPixel(int8_t x, int8_t y, CRGB color) {
        if (!isValidAccess(x, y)) {
            return;
        }
        leds[index(x, y)] = color;
    }

    void drawPixel(int8_t x, int8_t y, uint32_t color) {
        if (!isValidAccess(x, y)) {
            return;
        }
        leds[index(x, y)] = CRGB(color);
    }

    int32_t getPixel(int8_t x, int8_t y) {
        if (!isValidAccess(x, y)) {
            return 0;
        }
        CRGB &pixel = leds[index(x, y)];
        return (((uint32_t) pixel.r << 16) | ((uint32_t) pixel.g << 8) | (uint32_t) pixel.b);
    }

    void setBrightness(uint8_t value) {
        brightness = value;
        FastLED.setBrightness(value);
        FastLED.show();
    }

    void apply() {
        FastLED.show();
    }

private:
    uint8_t brightness = 32;

    void initLEDs() {
        FastLED.addLeds<WS2812, LAMP_PIN, GRB>(leds, count);
        FastLED.setBrightness(brightness);
//        FastLED.set
//        FastLED.setMaxPowerInVoltsAndMilliamps(5, 1500);
        for (int i = 0; i < count; i++) {
            leds[i] = CRGB(128, 128, 128);
        }
        FastLED.show();
    };

    inline bool isValidAccess(int8_t x, int8_t y) {
        return x >= 0 && x < width && y >= 0 && y < height;
    }

    inline int16_t index(int8_t x, int8_t y) {
        // hardware fix
        if (x % 2 == 1 && x != 7) {
            return x * height + (height - y - 1);
        }
        return x * height + y;
    }
};

void MatrixEffect::draw(LampCtx *l, uint32_t seed) {
    const int8_t scale = 8;
    for (byte x = 0; x < l->width; x++) {
        uint32_t thisColor = l->getPixel(x, l->height - 1);
        if (thisColor == 0) {
            l->drawPixel(x, l->height - 1, 0x00FF00 * (random(0, scale) == 0));
        } else if (thisColor < 0x002000) {
            l->drawPixel(x, l->height - 1, 0);
        } else {
            l->drawPixel(x, l->height - 1, thisColor - 0x002000);
        }
    }
    for (byte x = 0; x < l->width; x++) {
        for (byte y = 0; y < l->height - 1; y++) {
            l->drawPixel(x, y, l->getPixel(x, y + 1));
        }
    }
    l->apply();
}

void ColorsEffect::draw(LampCtx *lamp, uint32_t seed) {
    hue += 1;
    lamp->fillAll(CRGB(CHSV(hue, 255, 255)));
    lamp->apply();
}

void SnowEffect::draw(LampCtx *lamp, uint32_t seed) {
    auto WIDTH = lamp->width;
    auto HEIGHT = lamp->height;
    int8_t scale = 10;
    for (byte x = 0; x < WIDTH; x++) {
        for (byte y = 0; y < HEIGHT - 1; y++) {
            lamp->drawPixel(x, y, lamp->getPixel(x, y + 1));
        }
    }
    for (byte x = 0; x < WIDTH; x++) {
        if (lamp->getPixel(x, HEIGHT - 2) == 0 && (random(0, scale) == 0)) {
            lamp->drawPixel(x, HEIGHT - 1, 0xE0FFFF - 0x101010 * random(0, 4));
        } else {
            lamp->drawPixel(x, HEIGHT - 1, 0x000000);
        }
    }
    lamp->apply();
}

void FireEffect::draw(LampCtx *lamp, uint32_t seed) {
    if (loadingLine) {
        loadingLine = false;
        generateLine();
    }
    if (pcnt >= 100) {
        shiftUp();
        generateLine();
        pcnt = 0;
    }
    drawFrame(lamp);
    pcnt += 30;
}

void FireEffect::drawFrame(LampCtx *lamp) {
    uint8_t scale = 16;
    int nextv;
    for (unsigned char y = height - 1; y > 0; y--) {
        for (unsigned char x = 0; x < width; x++) {
            uint8_t newX = x;
            if (x > 15) {
                newX = x - 15;
            }
            if (y < 8) {
                nextv =
                        ((100.0 - pcnt) * matrixValue[y][newX]
                         + pcnt * matrixValue[y - 1][newX]) / 100.0
                        - pgm_read_byte(&(valueMask[y][newX]));
                CRGB color = CHSV(
                        scale * 2.5 + pgm_read_byte(&(hueMask[y][newX])), // H
                        255, // S
                        (uint8_t) max(0, nextv) // V
                );
                lamp->drawPixel(x, y, color);
            } else if (y == 8) {
                if (random(0, 20) == 0 && lamp->getPixel(x, y - 1) != 0) {
                    lamp->drawPixel(x, y, lamp->getPixel(x, y - 1));
                } else {
                    lamp->drawPixel(x, y, 0);
                }
            } else {
                if (lamp->getPixel(x, y - 1) > 0) {
                    lamp->drawPixel(x, y, lamp->getPixel(x, y - 1));
                } else lamp->drawPixel(x, y, 0);
            }
        }
    }
    for (unsigned char x = 0; x < width; x++) {
        uint8_t newX = x;
        if (x > 15) newX = x - 15;
        CRGB color = CHSV(
                scale * 2.5 + pgm_read_byte(&(hueMask[0][newX])), // H
                255,           // S
                (uint8_t) (((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0) // V
        );
        lamp->drawPixel(newX, 0, color);
    }
    lamp->apply();
}


#endif //SMART_LAMP_LAMP_HPP
