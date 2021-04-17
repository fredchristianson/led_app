#ifndef DRLED_STRIP_H
#define DRLED_STRIP_H
#include <FastLED.h>
#include "./logger.h"

namespace DevRelief {
    #define MAX_LEDS 300

    class DRLedStrip {
    public:
        DRLedStrip(int pin, uint16_t count) {
            m_logger = new Logger("DRLedStrip");
            m_logger->debug("DRLedStrip created %d %d",pin,count);
            m_pin = pin;
            m_count = count;
            m_colors = new CRGB[MAX_LEDS];
        
            m_controller = getController(pin,m_colors,count);
            FastLED.setBrightness(50);  // all strips have same max brightness at the FastLED global level.
        }

        CLEDController*  getController(int pin,CRGB* colors,int count) {
            // I can only get 2 pins working on my ESP8266.  Adding a 3rd results in unwanted flashes
            switch(pin) {
                case 1: {
                    m_logger->debug("Create FastLED on pin 1");
                    return &FastLED.addLeds<NEOPIXEL,1>(colors,count);
                }
                case 2: {
                    m_logger->debug("Create FastLED on pin 2");
                    return &FastLED.addLeds<NEOPIXEL,2>(colors,count);
                }
                default: {
                    m_logger->error("pin must be 1, 2, or 3.  got %d",pin);
                    return NULL;
                }
            }
        }

        void clear() {
            for(int i=0;i<m_count;i++) {
                m_colors[i] = CRGB(0,0,0);
            }
        }

        void setColor(int index,CRGB color) {
            m_colors[index] = color;
        }

        CRGB* getColor() {
            return m_colors;
        }

        uint16_t getCount() {
            return m_count;
        }
        static void show() {
            FastLED.show();
        }

    private:
        Logger * m_logger;    
        int m_pin;
        uint16_t m_count;
        CRGB * m_colors;
        CLEDController* m_controller;
    };

}


#endif 