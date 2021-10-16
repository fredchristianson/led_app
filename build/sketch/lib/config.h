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
                sceneCount = 0;
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

            void write(Generator& gen) {
                gen.startObject();
                gen.writeNameValue("name",name);
                gen.writeNameValue("addr",addr);
                gen.writeNameValue("stripCount",stripCount);
                gen.writeNameValue("brightness",brightness);

                gen.writeName("strips");
                gen.startProperty();
                gen.startArray();
                for(int i=0;i<stripCount;i++) {
                    gen.startProperty();
                    gen.startObject();
                    gen.writeNameValue("pin",stripPins[i]);
                    gen.writeNameValue("pin",stripLeds[i]);
                    gen.endObject();
                    gen.endProperty();
                }
                gen.endArray();
                gen.endProperty();

                
                gen.writeName("scenes");
                gen.startProperty();
                gen.startArray();
                auto scene = sceneNames.text();
                for(int i=0;i< sceneCount;i++) {
                    gen.writeArrayValue(scene);
                    scene += strlen(scene)+1;
                }
                gen.endArray();
                gen.endProperty();


                gen.endObject();
            }

            void setAddr(const char * ip){
                strcpy(addr,ip);
            }

            void setScenes(String* scenes, int count) {
                int pos = 0;
                uint8_t* data = sceneNames.reserve(count*20); // estimate 20 chars per name.  may be extended
                sceneCount = count;
                for(int i=0;i<count;i++) {
                    auto name = scenes[i].c_str();
                    m_logger->debug("add scene: %s",name);
                    auto len = strlen(name);
                    data = sceneNames.reserve(pos+len+1);
                    memcpy(data+pos,name,len);
                    pos += len;
                    data[pos] = 0;
                    pos += 1;
                    sceneNames.setLength(pos);
                }
                data[pos] = 0;
               // m_logger->debug("scene names: %s",data);
               // m_logger->debug("scene names: %s",sceneNames.text());
            }

            char     name[100];
            char     addr[32];
            size_t stripCount;
            int  stripPins[4];
            int  stripLeds[4];
            int  brightness;
            int sceneCount;
            DRBuffer sceneNames;
            Logger * m_logger;
    };
}
#endif