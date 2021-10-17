#ifndef CONFIG_H
#define CONFIG_H
#define CONFIG_HOSTNAME  "test"

#include "./parse_gen.h";
#include "./logger.h";

namespace DevRelief {
    const char * DEFAULT_CONFIG = R"config({ 
        "name": "Test Controller",
        "addr": "unset",
        "scripts": [],
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
                scriptCount = 0;
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
                    obj.readBoolValue("reverse",&stripReverse[stripCount],false);
                    stripCount++;
                }
                parser.readIntValue("brightness",&brightness);
                m_logger->debug("config read success");
                return true;
            }

            void write(Generator& gen) {
                gen.startObject();
                gen.writeNameValue("name",name);
                gen.writeNameValue("addr",addr);
                gen.writeNameValue("stripCount",(int)stripCount);
                gen.writeNameValue("brightness",brightness);

                gen.writeName("strips");
                gen.startProperty();
                gen.startArray();
                for(int i=0;i<stripCount;i++) {
                    gen.startProperty();
                    gen.startObject();
                    gen.writeNameValue("pin",stripPins[i]);
                    gen.writeNameValue("leds",stripLeds[i]);
                    gen.writeNameValue("reverse",stripReverse[i]);
                    gen.endObject();
                    gen.endProperty();
                }
                gen.endArray();
                gen.endProperty();

                
                gen.writeName("scripts");
                gen.startProperty();
                gen.startArray();
                auto script = scriptNames.text();
                for(int i=0;i< scriptCount;i++) {
                    gen.writeArrayValue(script);
                    script += strlen(script)+1;
                }
                gen.endArray();
                gen.endProperty();


                gen.endObject();
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
            char     name[100];
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