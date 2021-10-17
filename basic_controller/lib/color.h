#ifndef DRCOLOR_H
#define DRCOLOR_H
#include <Adafruit_NeoPixel.h>
#define STRIP1_NUMPIXELS STRIP1_LEDS
#define STRIP1_PIN 5
#define STRIP2_NUMPIXELS STRIP2_LEDS
#define STRIP2_PIN 4
#define STRIP3_NUMPIXELS STRIP3_LEDS
#define STRIP3_PIN 0
#define STRIP4_NUMPIXELS STRIP4_LEDS
#define STRIP4_PIN 2

//Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);




#include "./logger.h"

namespace DevRelief {

    class CRGB {
        public: 
            CRGB() {
                red = 0;
                green = 0;
                blue = 0;
            }

            CRGB(const CRGB& other) {
                red = other.red;
                blue = other.blue;
                green = other.green;
            }
            
            CRGB(uint8_t r,uint8_t g,uint8_t b) {
                red = r;
                green = g;
                blue = b;
            }

        public:
            uint8_t red;
            uint8_t green;
            uint8_t blue;
            
    };

    class CHSL {
        public: 
            CHSL() {
                hue = 0;
                saturation = 0;
                lightness = 0;
            }
            
            CHSL(uint16_t hue,uint16_t saturation,uint16_t lightness) {
                this->hue = hue;
                this->saturation = saturation;
                this->lightness = lightness;
            }

            CRGB toRGB() {
                return CHSL::HSLToRGB(*this);
            }

            static float HueToRGB(float v1, float v2, float vH) {
                if (vH < 0)
                    vH += 1;

                if (vH > 1)
                    vH -= 1;

                if ((6 * vH) < 1)
                    return (v1 + (v2 - v1) * 6 * vH);

                if ((2 * vH) < 1)
                    return v2;

                if ((3 * vH) < 2)
                    return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

                return v1;
            }

            static CRGB HSLToRGB(CHSL& hsl) {
                //m_logger->debug("hsl to rgb (%d,%d,%d)",(int)hsl.hue,(int)hsl.saturation,(int)hsl.lightness);
                unsigned char r = 0;
                unsigned char g = 0;
                unsigned char b = 0;

                float h = hsl.hue/360.0;
                float s = 1.0*hsl.saturation/100.0;
                float l = 1.0*hsl.lightness/100.0;
                if (s == 0)
                {
                    r = g = b = (unsigned char)(l * 255);
                }
                else
                {
                    float v1, v2;
                    float hue = (float)h;

                    v2 = (l < 0.5) ? (l * (1 + s)) : ((l + s) - (l * s));
                    v1 = 2 * l - v2;

                    r = (unsigned char)(255 * CHSL::HueToRGB(v1, v2, hue + (1.0f / 3)));
                    g = (unsigned char)(255 * CHSL::HueToRGB(v1, v2, hue));
                    b = (unsigned char)(255 * CHSL::HueToRGB(v1, v2, hue - (1.0f / 3)));
                }

                CRGB rgb(r, g, b);
                //m_logger->debug("hsl (%f,%f,%f)->rgb(%d,%d,%d)",(int)h,s,l,rgb.red,rgb.green,rgb.blue);
                return rgb;
            }

        public:
            uint16_t hue;  // 0-360
            uint16_t saturation; // 0-100
            uint16_t lightness;  // 0-100
            
    };

 

}


#endif 