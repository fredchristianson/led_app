#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\script_executor.h"
#ifndef DRSCRIPTEXECUTOR_H
#define DRSCRIPTEXECUTOR_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";
#include "./config.h";
#include "./standard.h";
#include "./script/script.h";
namespace DevRelief {

    Logger ScriptExecutorLogger("ScriptExecutor",SCRIPT_EXECUTOR_LOGGER_LEVEL);

    class ScriptExecutor {
        public:
            ScriptExecutor() {
                m_logger = &ScriptExecutorLogger;
                m_script = NULL;
                m_ledStrip = NULL;
            }

            ~ScriptExecutor() { 
                endScript();
                delete m_ledStrip;
                delete m_logger;
            }

            void turnOff() {
                endScript();
                white(0);
            }

            void white(uint8_t level) {
                endScript();
                if (m_ledStrip == NULL) {
                    return;
                }
                // level is 0-100
                m_ledStrip->clear();
                m_ledStrip->setBrightness(40);
                m_logger->debug("Set white level %d",level);
                for(int i=0;i<m_ledStrip->getCount();i++) {
                    m_ledStrip->setSaturation(i,100);
                    m_ledStrip->setLightness(i,level);
                    m_ledStrip->setHue(i,0);
                }
                m_ledStrip->show();
            }

            void solid(JsonObject* params) {
                endScript();
                if (m_ledStrip == NULL) {
                    return;
                }
                m_ledStrip->clear();
                m_ledStrip->setBrightness(40);
                int hue = params->get("hue",150);
                int saturation = params->get("saturation",100);
                int lightness = params->get("lightness",50);
                m_logger->debug("Set solid %d %d %d %d",m_ledStrip->getCount(),hue,saturation,lightness);
                for(int i=0;i<m_ledStrip->getCount();i++) {
                    m_ledStrip->setSaturation(i,saturation);
                    m_ledStrip->setLightness(i,lightness);
                    m_ledStrip->setHue(i,hue);
                }
                m_ledStrip->show();
            }


            void setScript(Script * script) {
                endScript();
                m_script = script;
                if (script != NULL) {
                    script->begin(m_ledStrip);
                }
            }

            void endScript() {
                delete m_script;
                m_script = NULL;
            }

            void configChange(Config& config) {
                turnOff();
                setupLeds(config);
            }

            void step() {
                if (m_ledStrip == NULL || m_script == NULL) {
                    return;
                }
                m_script->step();

            }
        private:

            void setupLeds(Config& config) {
                m_logger->debug("setup HSL Strip");
                if (m_ledStrip) {
                    delete m_ledStrip;
                }
                CompoundLedStrip*  compound = new CompoundLedStrip();
                const PtrList<LedPin*>& pins = config.getPins();
                int ledCount = 0;
                pins.each([&](LedPin* pin) {
                    m_logger->debug("\tadd pin 0x%04X %d %d %d",pin,pin->number,pin->ledCount,pin->reverse);
                    if (pin->number >= 0) {
                        DRLedStrip * real = new PhyisicalLedStrip(pin->number,pin->ledCount,pin->pixelType,pin->maxBrightness);
                        
                        if (pin->reverse) {
                            auto* reverse = new ReverseStrip(real);
                            compound->add(reverse);
                        } else {
                            compound->add(real);
                        }


                    }
                });

                m_ledStrip = new HSLStrip(compound);
                m_logger->info("created HSLStrip");
            }

            Logger* m_logger;
            Script* m_script;
            HSLStrip* m_ledStrip;
    };

}
#endif