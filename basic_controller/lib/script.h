#ifndef DRSCRIPT_H
#define DRSCRIPT_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";
#include "./config.h";
#include "./standard.h";
#include "./list.h";
#include "./led_strip.h";
#include "./animation.h";

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

    class IScriptValue {
        public:
            virtual int getInt(ScriptState & state, int percent)=0; // percent in [0,100]
    };

    class ScriptVariableValue : public IScriptValue {
        public:
            ScriptVariableValue(const char * value) : m_value(value) {
                
            }
        
            virtual int getInt(ScriptState& state, int percent){
                return 0;
            }
        protected:
            DRString m_value;
    };

    class ScriptIntValue : public IScriptValue {
        public:
            ScriptIntValue(int value) : m_value(value) {
                
            }
        
            virtual int getInt(ScriptState& state, int percent){
                return m_value;
            }
        protected:
            int m_value;
    };

    class ScriptRangeValue : public IScriptValue {
        public:
            ScriptRangeValue(IScriptValue* start,IScriptValue* end) {
                m_start = start;
                m_end = end;
            }

            ~ScriptRangeValue() {
                delete m_start;
                delete m_end;
            }

            virtual int getInt(ScriptState& state, int percent) {
                int start = m_start->getInt(state,percent);
                int end = m_end->getInt(state,percent);
                int diff = end-start;
                int result = start + diff*percent/100;
                return result;
            }

        protected: 
            IScriptValue* m_start;
            IScriptValue* m_end;
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
            HSLCommand(IScriptValue* h=NULL,IScriptValue* s=NULL,IScriptValue* l=NULL) {
                m_hue = h;
                m_saturation = s;
                m_lightness = l;
            }

            void setHue(IScriptValue* hue) { m_hue = hue;}
            IScriptValue* getHue(IScriptValue* hue) { return m_hue;}
            void setLightness(IScriptValue* lightness) { m_lightness = lightness;}
            IScriptValue* getLightness(IScriptValue* lightness) { return m_lightness;}
            void setSaturation(IScriptValue* saturation) { m_saturation = saturation;}
            IScriptValue* getSaturation(IScriptValue* saturation) { return m_saturation;}

            virtual ~HSLCommand() {

            }

            virtual void step(ScriptState& state) {
                int pos = 0;
                state.eachLed([&](IHSLStrip* strip, int idx){
                    if (m_hue >= 0) {
                        strip->setHue(idx,m_hue->getInt(state,pos));
                    }
                    if (m_saturation>= 0) {
                        strip->setSaturation(idx,m_saturation->getInt(state,pos));
                    }
                    if (m_lightness>= 0) {
                        strip->setLightness(idx,m_lightness->getInt(state,pos));
                    }
                });
            }

        private:
            IScriptValue* m_hue;
            IScriptValue* m_saturation;
            IScriptValue* m_lightness;
    };

    class RGBCommand : public ScriptCommand {
        public: 
            RGBCommand(IScriptValue* r=0,IScriptValue* g=0, IScriptValue* b=0) {
                m_red = r;
                m_green = g;
                m_blue = b;
            }

            void setRed(IScriptValue* red) { m_red = red;}
            IScriptValue* getRed(IScriptValue* red) { return m_red;}
            void setGreen(IScriptValue* green) { m_green = green;}
            IScriptValue* getGreen(IScriptValue* green) { return m_green;}
            void setBlue(IScriptValue* blue) { m_blue = blue;}
            IScriptValue* getBlue(IScriptValue* blue) { return m_blue;}

            virtual ~RGBCommand() {

            }

            virtual void step(ScriptState& state) {
                int pos = 0;
                state.eachLed([&](IHSLStrip* strip, int idx){
                    strip->setRGB(idx,CRGB(m_red->getInt(state,pos),m_green->getInt(state,pos),m_blue->getInt(state,pos)));
                });
            }

        private:
            IScriptValue* m_red;
            IScriptValue* m_blue;
            IScriptValue* m_green;
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