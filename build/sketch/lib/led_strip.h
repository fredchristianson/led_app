#ifndef DRLED_STRIP_H
#define DRLED_STRIP_H
#include <FastLED.h>
FASTLED_USING_NAMESPACE;
#include "./logger.h"

namespace DevRelief {
    #define MAX_LEDS 300

    class DRLedStrip {
    public:
        DRLedStrip(int pin, uint16_t count) {
            m_logger = new Logger("DRLedStrip");
            //m_logger->debug("DRLedStrip create");
            Serial.println("create strip");
            m_pin = pin;
            m_count = count;
            m_colors = new CRGB[MAX_LEDS];
        
            m_controller = getController(pin,m_colors,count);
            FastLED.setBrightness(50);  // all strips have same max brightness at the FastLED global level.
            m_logger->debug("DRLedStrip created %d %d",pin,count);
            //Serial.println("strip created");
        }

        void setBrightness(int value) {
            FastLED.setBrightness(value);
        }

        CLEDController*  getController(int pin,CRGB* colors,int count) {
            switch(pin) {
                case 1: {
                    //m_logger->debug("Create FastLED on pin 1 (GPIO 5) with GRB");
                    //return &FastLED.addLeds<WS2812B,5,GRB>(colors,count);
                    m_logger->debug("Create FastLED on pin D41 (GPIO 2) with GRB");
                    return &FastLED.addLeds<WS2812B,2,GRB>(colors,count);
                }
                default: {
                    //m_logger->error("pin must be 1, 2, or 3.  got %d",pin);
                    return NULL;
                }
            }
        }
        
        void solid(CRGB color, int brightness) {
            ////m_logger->debug("set solid color %d %d %d",color.red,color.green,color.blue);
            for(int i=0;i<m_count;i++) {
                m_colors[i] = color;
            }
            FastLED.setBrightness(brightness);
            FastLED.show();
        }

        void clear() {
            for(int i=0;i<m_count;i++) {
                m_colors[i] = CRGB(0,0,0);
            }
        }

        void setColor(int index,CRGB color) {
            m_colors[index] = color;
        }


        void setHSV(int index,CHSV color) {
            m_colors[index].setHSV(color.hue,color.saturation,color.value);
            //CRGB rgb;
           // hsv2rgb_spectrum(color,rgb);
           // //m_logger->debug("hsv (%d,%d,%d)->rgb(%d,%d,%d)",color.h,color.s,color.v,rgb.r,rgb.g,rgb.b);
           // m_colors[index] = rgb;
        }


        CRGB* getColor() {
            return m_colors;
        }

        uint16_t getCount() {
            return m_count;
        }

        void setCount(int count) {
            m_count = count;
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