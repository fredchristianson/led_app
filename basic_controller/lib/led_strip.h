#ifndef DRLED_STRIP_H
#define DRLED_STRIP_H
#include <Adafruit_NeoPixel.h>
#define NUMPIXELS 150
#define PIN 2
//Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

#include "./logger.h"

namespace DevRelief {
    #define MAX_LEDS 300

    class CRGB {
        public: 
            CRGB() {
                red = 0;
                green = 0;
                blue = 0;
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
            
            CHSL(uint8_t hue,uint8_t saturation,uint8_t lightness) {
                this->hue = hue;
                this->saturation = saturation;
                this->lightness = lightness;
            }

        public:
            uint8_t hue;
            uint8_t saturation;
            uint8_t lightness;
            
    };

    class DRLedStrip {
    public:
        DRLedStrip(int pin, uint16_t count) {
            m_logger = new Logger("DRLedStrip",60);
            m_logger->debug("DRLedStrip create pin=%d, count=%d",pin,count);
            m_pin = pin;
            m_count = count;
        
            m_controller = getController(pin,count);
            m_controller->setBrightness(50);  // all strips have same max brightness at the FastLED global level.
        }

        void setBrightness(int value) {
            m_controller->setBrightness(value);
        }

        Adafruit_NeoPixel *  getController(int pin,int count) {
            m_logger->debug("Create FastLED on pin D4 (GPIO 2) with GRB %d,%d",pin,count);
            auto neopixel = new Adafruit_NeoPixel(count,pin,NEO_GRB+NEO_KHZ800);
            neopixel->begin();
            return neopixel;
        }
        

        void clear() {
            m_logger->debug("clear strip");
            m_controller->clear();
        }

        void setColor(uint16_t index,CRGB color) {
            if (index<4) {
                m_logger->debug("setColor %d,(%d,%d,%d)",(int)index,(int)color.red,(int)color.green,(int)color.blue);
            }
            m_controller->setPixelColor(index,m_controller->Color(color.red,color.green,color.blue));

        }

/*
        void setHSV(int index,CHSL color) {
            m_colors[index].setHSL(color.hue,color.saturation,color.value);
            //CRGB rgb;
           // hsv2rgb_spectrum(color,rgb);
           // //m_logger->debug("hsv (%d,%d,%d)->rgb(%d,%d,%d)",color.h,color.s,color.v,rgb.r,rgb.g,rgb.b);
           // m_colors[index] = rgb;
        }
*/


        uint16_t getCount() {
            return m_count;
        }

        void setCount(int count) {
            m_count = count;
        }

        void show() {
           // m_logger->debug("show strip");
            m_controller->show();
        }

    private:
        Logger * m_logger;    
        int m_pin;
        uint16_t m_count;
        Adafruit_NeoPixel * m_controller;
    };

}


#endif 