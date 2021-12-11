#ifndef DRSCRIPTEXECUTOR_H
#define DRSCRIPTEXECUTOR_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";
#include "./config.h";
#include "./standard.h";
#include "./script.h";
namespace DevRelief {

    class ScriptExecutor {
        public:
            ScriptExecutor() {
                m_logger = new Logger("ScriptExecutor",SCRIPT_EXECUTOR_LOGGER_LEVEL);
                m_script = NULL;
                m_ledStrip = NULL;
            }

            ~ScriptExecutor() { 
                delete m_logger;
                delete m_ledStrip;
            }

            void turnOff() {
                white(0);
            }

            void white(uint8_t level) {
                // level is 0-100
                m_ledStrip->clear();
                m_ledStrip->setBrightness(40);
                m_logger->debug("Set whilte level %d",level);
                for(int i=0;i<m_ledStrip->getCount();i++) {
                    m_ledStrip->setSaturation(i,level);
                    m_ledStrip->setLightness(i,100);
                    m_ledStrip->setHue(i,0);
                }
                m_ledStrip->show();
            }
            void setScript(Script * script) {
                m_script = script;
            }

            void configChange(Config& config) {
                setupLeds(config);
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
                        DRLedStrip * real = new PhyisicalLedStrip(pin->number,pin->ledCount);
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