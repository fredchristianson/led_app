#ifndef CONFIG_H
#define CONFIG_H
#define CONFIG_HOSTNAME  "Lanai"

#include "./parse_gen.h";
#include "./logger.h";
#include "./data.h";
#include "./list.h";
#include "./util.h";

namespace DevRelief {
    Logger ConfigLogger("Config",DEBUG_LEVEL);
    const char * DEFAULT_CONFIG = R"config({ 
        "name": "CONFIG_HOSTNAME",
        "addr": "unset",
        "scripts": [],
        "strips": [],
        "brightness": 40
        ]
    })config";

    class LedPin {
        public:
        LedPin(int n, int c, bool r) {
            ConfigLogger.debug("create LedPin");
            number=n;
            ledCount=c;
            reverse=r;
        }

        ~LedPin() {
            ConfigLogger.debug("destroy LedPin");
        }

        int number;
        int ledCount;
        bool reverse;
    };



    class Config {
        public:
            static const Config* instance;
            static void setInstance(Config*cfg) { Config::instance = cfg;}

            Config() {
                m_logger = &ConfigLogger;
                m_logger->debug("create Config");
                m_logger->debug("\tset hostName");
                hostName = CONFIG_HOSTNAME;
                m_logger->debug("\tset runningScript");
                runningScript = NULL;
                m_logger->debug("\tset runningParameters");
                runningParameters = NULL;
                brightness = 40;
                maxBrightness = 100;
            }

          
            void setAddr(const char * ip){
                ipAddress = ip;
            }

            
            const DRString& getAddr() { return ipAddress;}

            void clearPins() {
                pins.clear();
            }


            void addPin(int number,int ledCount,bool reverse=false) {
                m_logger->debug("addPin %d %d %d",number,ledCount,reverse);
                pins.add(new LedPin(number,ledCount,reverse));
            }
            const LedPin* getPin(size_t idx) { return pins[idx];}
            size_t getPinCount() { return pins.size();}
            const LinkedList<LedPin*>& getPins() { return pins;}

            int getBrightness() { return brightness;}
            void setBrightness(int b) { brightness = b;}
            int getMaxBrightness() { return maxBrightness;}
            void setMaxBrightness(int b) { maxBrightness = b;}

            void clearScripts() {
                scripts.clear();
            }

            size_t getScriptCount() { return scripts.size();}
            
            bool addScript(const char * name) {
                m_logger->debug("add script %s",name);
                if (name == NULL || strlen(name) == 0) {
                    m_logger->warn("addScript requires a name");
                    return false;
                }
                
                scripts.add(DRString(name));
                m_logger->debug("\tadded");
                return true;
            }
            const LinkedList<DRString>& getScripts() { return scripts;}

            const DRString& getHostname() { return hostName;}
            void setHostname(const char * name) {
                hostName = name;
            }
            void setRunningScript(const char * name) {
                runningScript = name;
            }
            const DRString& getRunningScript() { return runningScript;}
    private:            
            DRString     hostName;
            DRString     ipAddress;
            PtrList<LedPin*>  pins;
            LinkedList<DRString>   scripts;
            DRString runningScript;
            JsonElement * runningParameters;
            int  brightness;
            int  maxBrightness;
            Logger * m_logger;
    };
    const Config* Config::instance;
}


#endif