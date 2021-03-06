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

Logger ledLogger("LED",LED_LOGGER_LEVEL);


typedef enum HSLOperation {  
    REPLACE=0,
    ADD=1,
    SUBTRACT=2,
    AVERAGE=3,
    MIN=4,
    MAX=5
};
static const char * HSLOPTEXT[]={"replace","add","subtract","average","min","max"};

const char * HSLOpToText(HSLOperation op) {
    if (op>=REPLACE && op <= MAX) {
        return HSLOPTEXT[op];
    }
    ledLogger.error("Unknown HSL OP %d",op);
    return HSLOPTEXT[0];
};

HSLOperation TextToHSLOP(const char * text) {
    int pos = 0;
    while(strcasecmp(text,HSLOPTEXT[pos])!= 0 && pos <= MAX) {
        pos++;
    }
    if (pos <= MAX){
        return (HSLOperation)pos;
    }
    ledLogger.error("Unknown HSL OP text %s",text);
    return REPLACE;
}

class CompoundLedStrip;

class IHSLStrip {
    public:
        virtual void setHue(int index, int16_t hue, HSLOperation op=REPLACE)=0;
        virtual void setSaturation(int index, int16_t saturation, HSLOperation op=REPLACE)=0;
        virtual void setLightness(int index, int16_t lightness, HSLOperation op=REPLACE)=0;
        virtual void setRGB(int index, const CRGB& rgb, HSLOperation op=REPLACE)=0;
        virtual int getCount()=0;
        virtual int getStart()=0;
        virtual void clear()=0;
        virtual void show()=0;
};

class DRLedStrip {
    public:
        DRLedStrip() {
            m_logger = &ledLogger;
            validCheck=0x123fe;
        }

        virtual ~DRLedStrip() {

        }

        virtual void clear() =0;
        virtual void setBrightness(uint16_t brightness)=0;
        virtual void setColor(uint16_t index,const CRGB& color)=0;
        virtual int getCount()=0;
        virtual void show()=0;

        virtual void setColor(uint16_t index, CHSL& color) {
            return setColor(index,HSLToRGB(color));
        }

        long validCheck;
        // use getCompoundLedStrip to find a base virtual strip made of multiple other strips
        virtual CompoundLedStrip* getCompoundLedStrip()=0; 


    protected:
        Logger* m_logger;
};

class AdafruitLedStrip : public DRLedStrip {
    public: 
        AdafruitLedStrip(int pin, uint16_t ledCount, neoPixelType pixelType=NEO_GRB){
            m_logger = new Logger("AdafruitLED",ADAFRUIT_LED_LOGGER_LEVEL);
            m_logger->debug("create AdafruitLedStrip %d %d",pin,ledCount);
            
            m_controller = new Adafruit_NeoPixel(ledCount,pin,pixelType+NEO_KHZ800);
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
            if (index == 0) {
                m_logger->debug("setColor  %02X,%02X,%02X",color.red,color.green,color.blue);
            }
            m_controller->setPixelColor(index,m_controller->Color(color.red,color.green,color.blue));
        }

        virtual int getCount() { return m_controller->numPixels();}
        virtual void show() {
            m_logger->debug("show strip %d, %d",m_controller->getPin(),m_controller->numPixels());
            //m_controller->setBrightness(40);
            //m_controller->setPixelColor(10,m_controller->Color(200,100,50));
            m_controller->show();
        }

        virtual CompoundLedStrip* getCompoundLedStrip() { return NULL;}
    protected:
        Adafruit_NeoPixel * m_controller;
};

class PhyisicalLedStrip : public AdafruitLedStrip {
    public:
        PhyisicalLedStrip(int pin, uint16_t ledCount, neoPixelType pixelType,uint8_t maxBrightness): AdafruitLedStrip(pin,ledCount,pixelType) {
            m_maxBrightness = maxBrightness;
        }

        virtual void setBrightness(uint16_t brightness) {
            if (brightness > m_maxBrightness) {
                brightness = m_maxBrightness;
            }
            m_controller->setBrightness(brightness);
        }

    private:
        uint8_t m_maxBrightness;
};

class CompoundLedStrip : public DRLedStrip {
    public:
        CompoundLedStrip() {
            strips[0] = NULL;
            strips[1] = NULL;
            strips[2] = NULL;
            strips[3] = NULL;
            count = 0;
            m_logger = new Logger("CompoundStrip",COMPOUND_STRIP_LOGGER_LEVEL);
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
        virtual int getCount() {
            size_t ledcount = 0;
            for(int i=0;i<count;i++) {
                if (strips[i] == NULL) {
                    m_logger->error("strip %d is NULL",i);
                } else {
                   // m_logger->debug("get count strip %d",i);
                    ledcount += strips[i]->getCount();
                }
            }
            m_logger->never("getcount()=%d",ledcount);
            return ledcount;
        }

        virtual void show() {
            m_logger->debug("show() %d",count);
            for(int i=0;i<count;i++) {
                strips[i]->show();
            }
        }

        virtual CompoundLedStrip* getCompoundLedStrip() { return this;}

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

        virtual int getCount() { return translateCount(m_base->getCount());}
        virtual void show() {m_base->show();}
        virtual CompoundLedStrip* getCompoundLedStrip() { return m_base ? m_base->getCompoundLedStrip() : NULL;}

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




class HSLStrip: public AlteredStrip, public IHSLStrip{
    public:
        HSLStrip(DRLedStrip* base): AlteredStrip(base) { 
            m_count = 0;
            m_hue = NULL;
            m_saturation = NULL;
            m_lightness = NULL;
            m_logger = new Logger("HSLStrip",HSL_STRIP_LOGGER_LEVEL);
            m_logger->debug("created HSLStrip with base 0x%04X",base);
        }

        ~HSLStrip() {
            reallocHSLData(0);
        }

        virtual int getStart() override { return 0;}

        void setRGB(int index, const CRGB& rgb,HSLOperation op) {
            CHSL hsl = RGBToHSL(rgb);
            m_logger->periodicNever(NEVER,5000,"setRGB %d (%d,%d,%d)->(%d,%d,%d)",index,rgb.red,rgb.green,rgb.blue,hsl.hue,hsl.saturation,hsl.lightness);
            setHue(index,hsl.hue,op);
            setSaturation(index,hsl.saturation,op);
            setLightness(index,hsl.lightness,op);
        }

        void setHue(int index, int16_t hue, HSLOperation op=REPLACE) {
            m_logger->never("HSL Hue %d %d",index,hue);
            if (index<0 || index>=m_count) {
                m_logger->periodic(ERROR_LEVEL,5000,"HSL Hue index out of range %d (0-%d)",index,m_count);
                return;
            } 
            if (index == 0) {
                m_logger->never("hue %d %d",index,hue);
            }
            m_hue[index] = clamp(0,359,performOperation(op,m_hue[index],hue));
            if (index == 0) {
                //m_logger->periodic(ERROR_LEVEL,5000,"setHue %d %d %d",index,hue,op);
            }
        }
 
        void setSaturation(int index, int16_t saturation, HSLOperation op=REPLACE) {
            if (index<0 || index>=m_count) {
                m_logger->periodic(ERROR_LEVEL,5000,"HSL saturation index out of range %d (0-%d)",index,m_count);
                return;
            } 
            if (saturation<0 || saturation>100) { return;}
            m_saturation[index] = clamp(0,100,performOperation(op,m_saturation[index],saturation));
        }

        void setLightness(int index, int16_t lightness, HSLOperation op=REPLACE) {
            if (index<=5) {
                m_logger->never("HSL Lightness op %d %d %d",op,index,lightness);
            }
            if (index<0 || index>=m_count) {
                m_logger->periodic(ERROR_LEVEL,5000,"HSL lightness index out of range %d (0-%d)",index,m_count);
                return;
            } 
            
            if (lightness<0 || lightness>100) { return;}
            int16_t l = performOperation(op,m_lightness[index],lightness);
            m_logger->never("op %d %d  %d->%d",op,m_lightness[index],lightness,l);
            m_lightness[index] = clamp(0,100,l);
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
                    m_logger->never("light: %d",light);
                }
                CHSL hsl(clamp(0,360,hue),defaultValue(0,100,sat,100),defaultValue(0,100,light,50));
                if (idx == 0) {
                    const CRGB rgb = HSLToRGB(hsl);
                    m_logger->debug("hsl(%d,%d,%d)->RGB(%d,%d,%d)",hsl.hue,hsl.saturation,hsl.lightness,rgb.red,rgb.green,rgb.blue);
                }
                m_base->setColor(idx,hsl);
            }
            m_base->show();
        }

        int getCount() { return AlteredStrip::getCount();}
        virtual IHSLStrip* getFirstHSLStrip() { return this;}
        virtual CompoundLedStrip* getCompoundLedStrip() { return m_base?m_base->getCompoundLedStrip() : NULL;}

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
                m_logger->never("SUBTRACT %d-%d=%d",currentValue,operand,currentValue-operand);
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