#ifndef DRSCRIPT_H
#define DRSCRIPT_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";
#include "./config.h";
#include "./standard.h";
#include "./list.h";
#include "./led_strip.h";

namespace DevRelief {
    Logger ScriptLogger("Script",SCRIPT_LOGGER_LEVEL);
    Logger ScriptCommandLogger("ScriptCommand",SCRIPT_LOGGER_LEVEL);

    class ScriptState {
        public:
            ScriptState(IHSLStrip* strip) {
                m_strip = strip;
                m_startLed=0;
                m_endLed=-1;
                m_repeatOn=-1;
                m_repeatOff=-1;
            }

            void eachLed(auto&& lambda) ;

            void setStart(int s) { m_startLed = s;}
            void setEnd(int s) { m_endLed = s;}
            void setRepeatOn(int s) { m_repeatOn = s;m_repeatOff = s;}
            void setRepeatOff(int s) { m_repeatOff = s;}
        private:
            IHSLStrip* m_strip;
            int m_startLed;
            int m_endLed;
            int m_repeatOn;
            int m_repeatOff;
    };

    class ScriptCommand {
        public: 
            ScriptCommand(){
                m_logger = &ScriptCommandLogger;
            }    
            virtual ~ScriptCommand() {

            }
            virtual void step(ScriptState& state)=0;

        protected:
            Logger* m_logger;
    };

    class HSLCommand : public ScriptCommand {
        public: 
            HSLCommand(int h=-1, int s = -1, int l=-1) {
                m_hue = h;
                m_saturation = s;
                m_lightness = l;
            }

            void setHue(int hue) { m_hue = hue;}
            int getHue(int hue) { return m_hue;}
            void setLightness(int lightness) { m_lightness = lightness;}
            int getLightness(int lightness) { return m_lightness;}
            void setSaturation(int saturation) { m_saturation = saturation;}
            int getSaturation(int saturation) { return m_saturation;}

            virtual ~HSLCommand() {

            }

            virtual void step(ScriptState& state) {
                state.eachLed([&](IHSLStrip* strip, int idx){
                    if (m_hue >= 0) {
                        strip->setHue(idx,m_hue);
                    }
                    if (m_saturation>= 0) {
                        strip->setSaturation(idx,m_saturation);
                    }
                    if (m_lightness>= 0) {
                        strip->setLightness(idx,m_lightness);
                    }
                });
            }

        private:
            int m_hue;
            int m_saturation;
            int m_lightness;
    };

    class RGBCommand : public ScriptCommand {
        public: 
            RGBCommand(int r=0,int g=0, int b=0) {
                m_logger->debug("RGBCommand %d,%d,%d",r,g,b);
                m_red = r;
                m_green = g;
                m_blue = b;
                CRGB rgb(m_red,m_green,m_blue);
                m_logger->debug("\t rgb %d,%d,%d",rgb.red,rgb.green,rgb.blue);
            }

            void setRed(int red) { m_red = red;}
            int getRed(int red) { return m_red;}
            void setGreen(int green) { m_green = green;}
            int getGreen(int green) { return m_green;}
            void setBlue(int blue) { m_blue = blue;}
            int getBlue(int blue) { return m_blue;}

            virtual ~RGBCommand() {

            }

            virtual void step(ScriptState& state) {
                state.eachLed([&](IHSLStrip* strip, int idx){
                    strip->setRGB(idx,CRGB(m_red,m_green,m_blue));
                });
            }

        private:
            uint8_t m_red;
            uint8_t m_blue;
            uint8_t m_green;
    };

    class Script {
        public:
            Script() {
                m_logger = &ScriptLogger;
            }

            ~Script() { 
                delete m_logger;
            }

            void clear() {
                m_commands.clear();
            }

            void add(ScriptCommand* cmd) {
                m_commands.add(cmd);
            }

            void step(ScriptState& state) {
                m_commands.each([&](ScriptCommand*cmd){
                    cmd->step(state);
                });
            }

        private:
            Logger* m_logger;
            PtrList<ScriptCommand*> m_commands;
    };

    void ScriptState::eachLed(auto&& lambda) {
        int count = m_strip->getCount();
        for(int i=0;i<count;i++) {
            lambda(m_strip,i);
        }
    }
}
#endif