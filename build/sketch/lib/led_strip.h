#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\led_strip.h"
#ifndef DRLED_STRIP_H
#define DRLED_STRIP_H
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
#include "./color.h"

namespace DevRelief {

Logger* ledLogger = new Logger("LED",10);


class DRLedStrip {
    public:
        DRLedStrip() {
            m_logger = ledLogger;
            validCheck=0x123fe;
        }

        virtual ~DRLedStrip() {

        }

        virtual void clear() =0;
        virtual void setBrightness(uint16_t brightness)=0;
        virtual void setColor(uint16_t index,const CRGB& color)=0;
        virtual size_t getCount()=0;
        virtual void show()=0;

        virtual void setColor(uint16_t index, CHSL& color) {
            return setColor(index,HSLToRGB(color));
        }

        long validCheck;

    protected:
        Logger* m_logger;
};

class AdafruitLedStrip : public DRLedStrip {
    public: 
        AdafruitLedStrip(int pin, uint16_t ledCount){
            m_logger = new Logger("AdafruitLED",20);
            m_logger->debug("create AdafruitLedStrip %d %d",pin,ledCount);
            m_controller = new Adafruit_NeoPixel(ledCount,pin,NEO_GRB+NEO_KHZ800);
            m_controller->setBrightness(40);
            m_controller->begin();
        }

        ~AdafruitLedStrip() {
            m_logger->debug("delete AdafruitLedStrip");
            delete m_controller;
        }

        virtual void clear() {
            m_logger->debug("clear AdafruitLedStrip");
            if (m_controller == NULL) {
                m_logger->error("NULL controller");
                return;
            }
            m_controller->clear();
        };
        virtual void setBrightness(uint16_t brightness) {
            m_controller->setBrightness(brightness);
        }

        virtual void setColor(uint16_t index, const CRGB& color){
            m_controller->setPixelColor(index,m_controller->Color(color.red,color.green,color.blue));
        }

        virtual size_t getCount() { return m_controller->numPixels();}
        virtual void show() {
            m_logger->debug("show strip %d, %d",m_controller->getPin(),m_controller->numPixels());
            //m_controller->setBrightness(40);
            //m_controller->setPixelColor(1,m_controller->Color(200,100,50));
            m_controller->show();
        }
    private:
        Adafruit_NeoPixel * m_controller;
};

class PhyisicalLedStrip : public AdafruitLedStrip {
    public:
        PhyisicalLedStrip(int pin, uint16_t ledCount): AdafruitLedStrip(pin,ledCount) {

        }
};

class CompoundLedStrip : public DRLedStrip {
    public:
        CompoundLedStrip() {
            strips[0] = NULL;
            strips[1] = NULL;
            strips[2] = NULL;
            strips[3] = NULL;
            count = 0;
            m_logger = new Logger("CompoundStrip",20);
            m_logger->info("create CompoundLedStrip");
        }

        ~CompoundLedStrip() {
            m_logger->debug("delete CompoundLedStrip");
            for(int i=0;i<count;i++) {
                m_logger->debug("\tdelete component LedStrip %d",i);
                delete strips[i];
            }
        }
        void add(DRLedStrip * strip) {
            if (count < 4) {
                strips[count++] = strip;
            } else {
                m_logger->error("too many strips added to CompoundLedStrip");
            }
        }

        
        void clear() {
            m_logger->debug("clear() %d components",count);
            for(int i=0;i<count;i++) {
                if (strips[i] == NULL) {
                    m_logger->error("NULL component script %d",i);
                } else {
                    //m_logger->debug("clear strip %d",i);
                    strips[i]->clear();
                }
            }
        };
        virtual void setBrightness(uint16_t brightness) {
            for(int i=0;i<count;i++) {
                strips[i]->setBrightness(brightness);
            }
        };

        virtual void setColor(uint16_t index,const CRGB& color)  {
            int strip = 0;
            uint16_t oindex = index;
            while(strip < count && strip < 4 && strips[strip] != NULL && index >= strips[strip]->getCount()) {
                index -= strips[strip]->getCount();
                strip++;
            }
            if (strip >= count || strip >= 4) {
                m_logger->error("strip too big %d %d",strip,oindex);
                return;
            }

            if (strips[strip] == NULL) {
                m_logger->error("missing strip %d %d",oindex,strip);
                return;
            }
            if (strips[strip]->validCheck != 0x123fe) {
                m_logger->error(" strip not valid  %d %d",oindex,strip);
                return;
            }
            // if (index == 0) {
            //     m_logger->error("set color %d %d %d %d: %d,%d,%d",index,count,strips[strip]->getCount(),strip,color.red,color.green,color.blue);
            // }
            if (index<strips[strip]->getCount()){
                strips[strip]->setColor(index,color);
            } else {
                m_logger->error("bad index %d %d %d",index,strip,(strips[strip] == NULL ? -1 : strips[strip]->getCount()));
            }
        };
        virtual size_t getCount() {
            size_t ledcount = 0;
            for(int i=0;i<count;i++) {
                if (strips[i] == NULL) {
                    m_logger->error("strip %d is NULL",i);
                } else {
                   // m_logger->debug("get count strip %d",i);
                    ledcount += strips[i]->getCount();
                }
            }
            m_logger->debug("getcount()=%d",ledcount);
            return ledcount;
        }

        virtual void show() {
            m_logger->debug("show() %d",count);
            for(int i=0;i<count;i++) {
                strips[i]->show();
            }
        }

    private:
        DRLedStrip* strips[4]; // max of 4 strips;
        size_t      count;
};

class AlteredStrip : public DRLedStrip {
    public:
        AlteredStrip(DRLedStrip * base) {
            m_base = base;
        }   

        ~AlteredStrip() {
            delete m_base;
        }

        
        virtual void clear() {
            m_base->clear();
        };
        virtual void setBrightness(uint16_t brightness) {
            m_base->setBrightness(brightness);
        }

        virtual void setColor(uint16_t index, const CRGB& color){
            m_base->setColor(translateIndex(index),translateColor(color));
        }

        virtual size_t getCount() { return translateCount(m_base->getCount());}
        virtual void show() {m_base->show();}

    protected:
        virtual uint16_t translateIndex(uint16_t index) { return index;}
        virtual uint16_t translateCount(uint16_t count) { return count;}
        virtual CRGB translateColor(const CRGB& color) { return color;}
    
        DRLedStrip * m_base;
};

class ReverseStrip: public AlteredStrip {
    public:
        ReverseStrip(DRLedStrip* base): AlteredStrip(base) {
            m_logger->debug("create ReverseStrip");
        }

        ~ReverseStrip() {
            m_logger->debug("delete ReverseStrip");
        }

    protected:
        uint16_t translateIndex(uint16_t index) { 
            return getCount()-index-1;
        }
};

class RotatedStrip: public AlteredStrip {
    public:
        RotatedStrip(DRLedStrip* base): AlteredStrip(base) { m_rotationCount = 0;}

    protected:
        uint16_t translateIndex(int16_t index) { 
            size_t count =  getCount();
            return (index + count + m_rotationCount) % count;
        }

    private: 
        int16_t m_rotationCount;
};

enum HSLOperation {  
    REPLACE=0,
    ADD=1,
    SUBTRACT=2,
    AVERAGE=3,
    MIN=4,
    MAX=5
};

class IHSLStrip {
    public:
        virtual void setHue(int index, int16_t hue, HSLOperation op)=0;
        virtual void setSaturation(int index, int16_t hue, HSLOperation op)=0;
        virtual void setLightness(int index, int16_t hue, HSLOperation op)=0;
        virtual void setRGB(int index, CRGB& rgb, HSLOperation op)=0;
        virtual size_t getCount()=0;
};

class HSLStrip: public AlteredStrip, public IHSLStrip{
    public:
        HSLStrip(DRLedStrip* base): AlteredStrip(base) { 
            m_count = 0;
            m_hue = NULL;
            m_saturation = NULL;
            m_lightness = NULL;
            m_logger = new Logger("HSLStrip",20);
        }

        ~HSLStrip() {
            reallocHSLData(0);
        }

        void setRGB(int index, CRGB& rgb,HSLOperation op) {
            CHSL hsl = RGBToHSL(rgb);
            setHue(index,hsl.hue,op);
            setLightness(index,hsl.saturation,op);
            setLightness(index,hsl.lightness,op);
        }

        void setHue(int index, int16_t hue, HSLOperation op) {
            if (index<0 || index>=m_count) {
                m_logger->error("HSL Hue index out of range %d (0-%d)",index,m_count);
                return;
            } 
            if (index == 0) {
                m_logger->debug("hue %d %d",index,hue);
            }
            m_hue[index] = hue;
            return;
            m_hue[index] = clamp(0,359,performOperation(op,m_hue[index],hue));
            if (index == 0) {
                m_logger->debug("setHue %d %d %d",index,hue,op);
            }
        }

        void setSaturation(int index, int16_t saturation, HSLOperation op) {
            if (index<0 || index>=m_count) {
                m_logger->error("HSL saturation index out of range %d (0-%d)",index,m_count);
                return;
            } 
            m_saturation[index] = clamp(0,100,performOperation(op,m_saturation[index],saturation));
        }

        void setLightness(int index, int16_t lightness, HSLOperation op) {
            if (index<0 || index>=m_count) {
                m_logger->error("HSL lightness index out of range %d (0-%d)",index,m_count);
                return;
            } 
            m_lightness[index] = clamp(0,100,performOperation(op,m_lightness[index],lightness));
        }

        void clear() {
            if (m_base == NULL) {
                m_logger->warn("HSLStrip does not have a base");
                return;
            }
            m_logger->debug("Clear HSLStrip");
            int count = m_base->getCount();
            m_logger->debug("HSLStrip realloc for %d leds",count);
            reallocHSLData(count);
            m_logger->debug("clear HSL values");
            for(int i=0;i<count;i++) {
                m_hue[i] = -1;
            }
            //memset(m_hue,-1,sizeof(int16_t)*count);
            memset(m_saturation,-1,sizeof(int8_t)*count);
            memset(m_lightness,-1,sizeof(int8_t)*count);
            m_base->clear();
        }

        void show() {
            m_logger->debug("show() %d",m_count);
            for(int idx=0;idx<m_count;idx++) {
                int hue = m_hue[idx];
                int sat = m_saturation[idx];
                int light = m_lightness[idx];
                if (hue < 0) {
                    light = 0;
                }
                if (false && idx < 20) {
                    m_logger->always("light: %d",light);
                }
                CHSL hsl(clamp(0,360,hue),defaultValue(0,100,sat,100),defaultValue(0,100,light,0));
                if (idx == 0) {
                    const CRGB rgb = HSLToRGB(hsl);
                    m_logger->debug("hsl(%d,%d,%d)->RGB(%d,%d,%d)",hsl.hue,hsl.saturation,hsl.lightness,rgb.red,rgb.green,rgb.blue);
                }
                m_base->setColor(idx,hsl);
            }
            m_base->show();
        }

        size_t getCount() { return AlteredStrip::getCount();}

    protected:
        void reallocHSLData(int count) {
            if ((count == 0 || count > m_count) && m_hue != NULL) {
                m_logger->debug("HSLStrip free %d %d",count,m_count);
                free(m_hue);
                free(m_saturation);
                free(m_lightness);
                m_hue = NULL;
                m_saturation = NULL;
                m_lightness = NULL;
            }
            if (count > 0 && m_hue == NULL) {
                m_logger->debug("HSLStrip malloc %d ",count);
                m_hue = (int16_t*) malloc(sizeof(int16_t)*count);
                m_saturation = (int8_t*) malloc(sizeof(int8_t)*count);
                m_lightness = (int8_t*) malloc(sizeof(int8_t)*count);
                m_count = count;
            } else {
                m_logger->debug("no need to malloc members %d",count);
            }
        }

        int16_t defaultValue(int min, int max, int val, int def) {
            if (val < min) {
                return def;
            } else if (val > max){
                return def;
            }
            return val;
        }
        int16_t clamp(int min, int max, int val) {
            if (val < min) {
                return min;
            } else if (val > max){
                return max;
            }
            return val;
        }
        int16_t performOperation(HSLOperation op, int16_t currentValue, int16_t operand)
        {
            if (currentValue < 0) {
                return operand;
            }
            switch (op)
            {
            case REPLACE:
                return operand;
            case ADD:
                return currentValue + operand;
            case SUBTRACT:
                return currentValue - operand;
            case AVERAGE:
                return (currentValue + operand)/2;
            case MIN:
                return currentValue < operand ? currentValue : operand;
            case MAX:
                return currentValue > operand ? currentValue : operand;
            default:
                m_logger->errorNoRepeat("unknown HSL operation %d",op);
            }
            return operand;
        }

    private:
        uint16_t m_count;
        int16_t * m_hue;
        int8_t  * m_saturation;
        int8_t  * m_lightness;
        HSLOperation m_op;
};

}

#endif 