#ifndef CONFIG_H
#define CONFIG_H
#define CONFIG_HOSTNAME  "test"

#include "./parse_gen.h";
#include "./logger.h";

namespace DevRelief {
    const char * DEFAULT_CONFIG = R"config({ 
        "name": "Test Controller",
        "addr": "unset",
        "scenes": [],
        "strips": [],
        "brightness": 40
    })config";

    class Config {
        public:
            Config() {
                m_logger = new Logger("Config",100);
                strcpy(name,CONFIG_HOSTNAME);
                for(auto i=0;i<3;i++) {
                    stripPins[i] = -1;
                    stripLeds[0] = 0;
                }
                brightness = 40;
            }

            bool read(ObjectParser parser){
                size_t idx;
                ArrayParser stripArray;
                ObjectParser obj;
                bool result = parser.readStringValue("name",name);
                stripCount = 0;
                parser.getArray("strips",stripArray);
                
                while(stripArray.nextObject(&obj)){
                    obj.readIntValue("pin",&stripPins[stripCount]);
                    obj.readIntValue("leds",&stripLeds[stripCount]);
                    stripCount++;
                }
                parser.readIntValue("brightness",&brightness);
                m_logger->debug("config read success");
                return true;
            }

            char     name[100];
            char     addr[32];
            size_t stripCount;
            int  stripPins[4];
            int  stripLeds[4];
            int  brightness;
            Logger * m_logger;
    };
}
#endif