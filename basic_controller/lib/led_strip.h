#ifndef DRLED_STRIP_H
#define DRLED_STRIP_H
#include <Adafruit_NeoPixel.h>
#define STRIP1_NUMPIXELS STRIP1_LEDS
#define STRIP1_PIN 4
#define STRIP2_NUMPIXELS STRIP2_LEDS
#define STRIP2_PIN 5

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
            
            CHSL(uint16_t hue,uint16_t saturation,uint16_t lightness) {
                this->hue = hue;
                this->saturation = saturation;
                this->lightness = lightness;
            }

        public:
            uint16_t hue;  // 0-360
            uint16_t saturation; // 0-100
            uint16_t lightness;  // 0-100
            
    };

    class DRLedStrip {
    public:
        DRLedStrip() {
            m_logger = new Logger("DRLedStrip",80);
            m_logger->debug("DRLedStrip create");

        
            m_controller1 = getController(STRIP1_PIN,STRIP1_NUMPIXELS);
            m_controller1->setBrightness(40);  // all strips have same max brightness at the FastLED global level.

            if (STRIP2_PIN >= 0) {
                m_controller2 = getController(STRIP2_PIN,STRIP2_NUMPIXELS);
                m_controller2->setBrightness(40);  // all strips have same max brightness at the FastLED global level.
            } else {
                m_controller2 = NULL;
            }
        }

        void setBrightness(int value) {
            m_controller1->setBrightness(value);
            if (m_controller2 != NULL) {
                m_controller2->setBrightness(value);
            }
        }

        Adafruit_NeoPixel *  getController(int pin,int count) {
            m_logger->info("Create FastLED on pin D4 (GPIO 2) with GRB %d,%d",pin,count);
            auto neopixel = new Adafruit_NeoPixel(count,pin,NEO_GRB+NEO_KHZ800);
            neopixel->begin();
            return neopixel;
        }
        

        void clear() {
            m_logger->debug("clear strip");
            m_controller1->clear();
            if (m_controller2 != NULL) {
                m_controller2->clear();
            }
        }

        void setColor(uint16_t index,CRGB color) {
            if (index<4) {
                //m_logger->debug("setColor %d,(%d,%d,%d)",(int)index,(int)color.red,(int)color.green,(int)color.blue);
            }
            if (index == 0) {
                m_controller1->setPixelColor(index,m_controller1->Color(0,0,0));
            } else {
                if (index < STRIP1_NUMPIXELS) {
                    m_controller1->setPixelColor(index,m_controller1->Color(color.red,color.green,color.blue));
                } else if (m_controller2 != NULL) {
                    m_controller2->setPixelColor(index-STRIP1_NUMPIXELS,m_controller2->Color(color.red,color.green,color.blue));
                } else {
                    m_logger->error("index too high with no strip 2");
                }
            }

        }



        uint16_t getCount() {
            return STRIP1_NUMPIXELS + STRIP2_NUMPIXELS;
        }

        void setCount(int count) {
            m_logger->error("setCount not implemented");
        }

        void show() {
           // m_logger->debug("show strip");
            if (m_controller2 != NULL) {
                m_controller2->show();
            } else {
                m_logger->error("no strip 2");
            }
            m_controller1->show();

        }

    private:
        Logger * m_logger;    

        Adafruit_NeoPixel * m_controller1;
        Adafruit_NeoPixel * m_controller2;
    };

}


#endif 