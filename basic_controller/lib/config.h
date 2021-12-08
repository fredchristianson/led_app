#ifndef CONFIG_H
#define CONFIG_H
#define CONFIG_HOSTNAME  "Lanai"

#include "./parse_gen.h";
#include "./logger.h";
#include "./data.h";

namespace DevRelief {
    Logger ConfigLogger("Config",WARN_LEVEL);
    const char * DEFAULT_CONFIG = R"config({ 
        "name": "CONFIG_HOSTNAME",
        "addr": "unset",
        "scripts": [],
        "strips": [],
        "brightness": 40
        ]
    })config";

    class ConfigPin {

    };

    class Config : Data {
        public:
            Config() {
                m_logger = &ConfigLogger;
                strcpy(name,CONFIG_HOSTNAME);
                for(auto i=0;i<3;i++) {
                    stripPins[i] = -1;
                    stripLeds[0] = 0;
                }
                brightness = 40;
                scriptCount = 0;
            }

          
            void setAddr(const char * ip){
                strcpy(addr,ip);
            }

            void setScripts(String* scripts, int count) {
                int pos = 0;
                uint8_t* data = scriptNames.reserve(count*20); // estimate 20 chars per name.  may be extended
                scriptCount = count;
                for(int i=0;i<count;i++) {
                    auto name = scripts[i].c_str();
                    m_logger->debug("add script: %s",name);
                    auto len = strlen(name);
                    data = scriptNames.reserve(pos+len+1);
                    memcpy(data+pos,name,len);
                    pos += len;
                    data[pos] = 0;
                    pos += 1;
                    scriptNames.setLength(pos);
                }
                data[pos] = 0;
               // m_logger->debug("script names: %s",data);
               // m_logger->debug("script names: %s",scriptNames.text());
            }

            size_t getStripCount() { return stripCount;}
            int getPin(size_t idx) { return stripPins[idx];}
            int getLedCount(size_t idx) { return stripLeds[idx];}
            bool isReversed(size_t idx) { return stripReverse[idx];}
            char     name[50];
            char     startupScript[50];
            char     addr[32];
            size_t stripCount;
            int  stripPins[4];
            int  stripLeds[4];
            bool stripReverse[4];
            int  brightness;
            int scriptCount;
            DRBuffer scriptNames;
            Logger * m_logger;
    };
}
#endif