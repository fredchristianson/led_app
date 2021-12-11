#ifndef CONFIG_H
#define CONFIG_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./data.h";
#include "./list.h";
#include "./util.h";

namespace DevRelief {
    Logger ConfigLogger("Config",CONFIG_LOGGER_LEVEL);

    class LedPin {
        public:
        LedPin(int n, int c, bool r) {
            ConfigLogger.debug("create LedPin 0x%04X  %d %d %d",this,n,c,r);
            number=n;
            ledCount=c;
            reverse=r;
        }

        ~LedPin() {
            ConfigLogger.debug("destroy LedPin 0x%04X %d %d %d",this,number,ledCount,reverse);
        }

        int number;
        int ledCount;
        bool reverse;
    };



    class Config {
        public:
            static Config* getInstance() { return instance;}
            static void setInstance(Config*cfg) { Config::instance = cfg;}

            Config() {
                m_logger = &ConfigLogger;
                hostName = HOSTNAME;
                runningScript = NULL;
                runningParameters = NULL;
                brightness = 40;
                maxBrightness = 100;
                buildVersion = BUILD_VERSION;
                buildDate = BUILD_DATE;
                buildTime = BUILD_TIME;

            }

          
            void setAddr(const char * ip){
                ipAddress = ip;
            }

            
            const DRString& getAddr() const { return ipAddress;}

            void clearPins() {
                pins.clear();
            }


            void addPin(int number,int ledCount,bool reverse=false) {
                m_logger->debug("addPin %d %d %d",number,ledCount,reverse);
                pins.add(new LedPin(number,ledCount,reverse));
            }
            const LedPin* getPin(size_t idx) const { return pins[idx];}
            size_t getPinCount() const { return pins.size();}
            const PtrList<LedPin*>& getPins() { 
                m_logger->always("return pins");
                return pins;
            }

            int getBrightness() const { return brightness;}
            void setBrightness(int b) { brightness = b;}
            int getMaxBrightness() const { return maxBrightness;}
            void setMaxBrightness(int b) { maxBrightness = b;}

            void clearScripts() {
                scripts.clear();
            }

            size_t getScriptCount()  const{ return scripts.size();}
            
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

            void setScripts(LinkedList<DRString>& list) {
                scripts.clear();
                list.each([&](DRString&name) {
                    addScript(name.get());
                });
            }
            const LinkedList<DRString>& getScripts()  const{ return scripts;}

            const DRString& getHostname() const { return hostName;}
            void setHostname(const char * name) {
                hostName = name;
            }
            void setRunningScript(const char * name) {
                runningScript = name;
            }
            const DRString& getRunningScript()  const{ return runningScript;}

            const DRString& getBuildVersion()const { return buildVersion;}
            const DRString& getBuildDate()const { return buildDate;}
            const DRString& getBuildTime()const { return buildTime;}
    private:            
            DRString     hostName;
            DRString     ipAddress;
            DRString    buildVersion;
            DRString    buildDate;
            DRString    buildTime;
            PtrList<LedPin*>  pins;
            LinkedList<DRString>   scripts;
            DRString runningScript;
            JsonElement * runningParameters;
            int  brightness;
            int  maxBrightness;
            Logger * m_logger;
            static Config* instance;

    };
    Config* Config::instance;
}


#endif