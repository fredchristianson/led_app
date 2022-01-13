#ifndef DR_DATA_LOADER_H
#define DR_DATA_LOADER_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"
#include "./data.h"
#include "./file_system.h"
#include "./config.h"


namespace DevRelief {
const char * ERROR_NO_REQUEST = "Nothing has been loaded";

Logger DataLoaderLogger("DataLoader",DATA_LOADER_LOGGER_LEVEL);



class DataLoader;

class LoadResult {
    public:
    LoadResult() {
        m_type = FILE_UNKNOWN_TYPE;
        m_json = NULL;
        m_jsonRoot = NULL;
        m_success = false;
        m_error = ERROR_NO_REQUEST;
    }

    ~LoadResult() {
        delete m_jsonRoot;
    }

    DRFileBuffer& getBuffer() { return m_buffer;}
    const char * getText() {
        return m_buffer.text();
    }

    JsonElement* getJson() {
        return m_jsonRoot == NULL ? NULL : m_jsonRoot->getTopElement();
    }

    JsonRoot* getJsonRoot() {
        return m_jsonRoot;
    }

    void setJsonRoot(JsonRoot* json) {
        m_jsonRoot = json;
        m_json = m_jsonRoot ? m_jsonRoot->getTopElement()->asObject() : NULL;
    }

    void setSuccess(bool success) { m_success = success;}
    bool isSuccess() { return m_success;}
    const char * error() { return m_error;}


private:
    friend DataLoader;
    DRFileBuffer m_buffer;    
    FileType m_type;
    JsonRoot* m_jsonRoot;
    JsonElement* m_json;
    bool m_success;
    const char * m_error;
};


class DataLoader {
public:
    DataLoader()  {
        m_logger = &DataLoaderLogger;
    }
    virtual ~DataLoader() {
    }

    bool writeJsonFile(const char * path,JsonElement* json) {
        m_logger->debug("write JSON file %s",path);
        DRString buffer;
        m_logger->debug("\tgen JSON");
        JsonGenerator gen(buffer);
        gen.generate(json);
        m_logger->debug("\twrite JSON: %s",buffer.text());
        return m_fileSystem.write(path,buffer.text());
    }

    bool loadFile(const char * path,LoadResult & result) {
        result.m_type = m_fileSystem.getFileType(path);
        
        if (!m_fileSystem.read(path,result.getBuffer())){
            m_logger->error("cannot read file %s",path);
            result.m_success = false;
            result.m_error = "file read() failed";
        } else {

            if (result.m_type == FILE_JSON) {                
                m_logger->debug("got json file");

                JsonParser parser;
                m_logger->debug("\tparse file %s",result.getBuffer().text());
                JsonRoot* root = parser.read(result.getBuffer().text());
                m_logger->debug("\tset root %s",(root == NULL ? "NULL" : "found"));
                m_logger->debug("\ttop 0x%04X",(root == NULL ? 0 : root->getTopElement()));
                m_logger->debug("\ttop obj 0x%04X",(root == NULL ? 0 : root->getTopElement()->asObject()));
                result.setJsonRoot(root);
                result.m_success = root != NULL;
                result.m_error = root == NULL ? "JSON parse failed" : NULL;
            } else {
                m_logger->debug("not JSON file %s",path);
                result.m_success = true;
                result.m_error = "File is not .json";
            }
       }
       return result.m_success;
    }

protected:


    Logger * m_logger;
    static DRFileSystem m_fileSystem;
private:

};

DRFileSystem DataLoader::m_fileSystem;

class ConfigDataLoader : public DataLoader {
    public:
        ConfigDataLoader() {
            
        }

        bool addScripts(Config&config) {
            LinkedList<DRString> files;
            m_logger->debug("adding scripts");
            if (m_fileSystem.listFiles("/script",files)){
                m_logger->debug("\tcall config.setScripts");
                config.setScripts(files);
            }
            return true;
        }

        bool initialize(Config& config) {
            config.clearScripts();
            config.clearPins();
            config.setBrightness(40);
            config.setMaxBrightness(100);
            addScripts(config);
            return true;
        }

        bool deleteConfig(const char * path = "/config.json") {
            m_fileSystem.deleteFile(path);
        }
        bool saveConfig(Config& config, const char * path = "/config.json"){
            SharedPtr<JsonRoot> jsonRoot = toJson(config);
            return writeJsonFile(path,jsonRoot->getTopElement());
        }

        bool toJsonString(Config&config,DRString& result) {
            SharedPtr<JsonRoot> jsonRoot = toJson(config);
            JsonGenerator gen(result);
            return gen.generate(jsonRoot.get());
            
        }

        bool updateConfig(Config& config, const char * jsonText){
            JsonParser parser;
            m_logger->always("read config");
            JsonRoot * root = parser.read(jsonText);
            if (root == NULL) {
                m_logger->always("no JSON");
                return false;
            } else {
                m_logger->always("get Config from JSON");
                if (readJson(config,root)) {
                    m_logger->always("save Config");
                    return saveConfig(config);
                }
            }
            return false;
        }

        SharedPtr<JsonRoot> toJson(Config&config) {
            JsonRoot* root=new JsonRoot;  
            JsonObject * json = root->createObject();
            json->set("buildVersion",config.getBuildVersion());
            json->set("buildDate",config.getBuildDate());
            json->set("buildTime",config.getBuildTime());
            json->set("hostname",config.getHostname());

            json->set("ipAddress",config.getAddr());
            json->set("brightness",config.getBrightness());
            json->set("maxBrightness",config.getMaxBrightness());
            json->set("runningScript",config.getRunningScript());
            JsonArray* pins = root->createArray();
            json->set("pins",pins);
            m_logger->debug("filling pins from config");
            config.getPins().each( [this,logger=m_logger,pins](LedPin* pin) {
                logger->debug("\thandle pin 0x%04X",pin); 
                logger->debug("\tnumber %d",pin->number); 
                JsonObject* pinElement = pins->createObjectElement();
                pinElement->set("number",pin->number);
                pinElement->set("ledCount",pin->ledCount);
                pinElement->set("reverse",pin->reverse);
                pinElement->set("maxBrightness",pin->maxBrightness);
                pinElement->set("pixelType",getPixelType(pin->pixelType));
                pins->addItem(pinElement);
            });
            m_logger->debug("pins done");

            JsonArray* scripts = root->createArray();
            json->set("scripts",scripts);
            config.getScripts().each( [&](DRString &script) {
                m_logger->debug("handle script %s",script.get()); 
                
                scripts->add(script.get());
            });
            m_logger->debug("scripts done");

            return root;
        }

        neoPixelType getPixelType(const char * name) {
            if (strcmp(name,"NEO_RGB") == 0) {return NEO_RGB;}
            if (strcmp(name,"NEO_RBG") == 0) {return NEO_RBG;}
            if (strcmp(name,"NEO_GRB") == 0) {return NEO_GRB;}
            if (strcmp(name,"NEO_GBR") == 0) {return NEO_GBR;}
            if (strcmp(name,"NEO_BRG") == 0) {return NEO_BRG;}
            if (strcmp(name,"NEO_BGR") == 0) {return NEO_BGR;}

            return NEO_GRB;
        }

        const char *getPixelType(neoPixelType type) {
            if (type==NEO_RGB) { return "NEO_RGB";}
            if (type==NEO_RBG) { return "NEO_RBG";}
            if (type==NEO_GRB) { return "NEO_GRB";}
            if (type==NEO_GBR) { return "NEO_GBR";}
            if (type==NEO_BRG) { return "NEO_BRG";}
            if (type==NEO_BGR) { return "NEO_BGR";}
            return "NEO_GRB";
        }

        bool loadConfig(Config& config, const char * path = "/config.json"){
            LoadResult result;
            m_logger->debug("initialize config");
            
            m_logger->debug("load file");
            if (loadFile(path,result)){
                m_logger->debug("process json");

                if (!readJson(config,result.getJsonRoot())) {
                    result.setSuccess(false);
                }

            } else {
                m_logger->error("Config json not found");
            }
            addScripts(config);
            return result.isSuccess();
        }

        bool readJson(Config& config, JsonRoot* root) {
            m_logger->debug("readJson.  getJson object");
           

            m_logger->debug("\tgot root %s",(root?"yes":"no"));
            if (root == NULL) {
                return false;
            }
            JsonElement * top = root->getTopElement();
            m_logger->debug("\tgot top %s",(top?"yes":"no"));
            if (top == NULL) {
                return false;
            }
            JsonObject*object = top->asObject();
            if (object == NULL) {
                m_logger->error("no JSON element found");
                return false;
            }
            m_logger->debug("get hostname");
            config.setHostname(object->get("hostname","unknown_host"));
            m_logger->debug("get ipAddress");
            config.setAddr(object->get("ipAddress","unknown_address"));
            m_logger->debug("get brightness");
            config.setBrightness(object->get("brightness",40));
            m_logger->debug("get maxBrightness");
            config.setMaxBrightness(object->get("maxBrightness",100));
            m_logger->debug("get runningScript");
            config.setRunningScript(object->get("runningScript",(const char*)NULL));
            config.clearPins();
            config.clearScripts();
            m_logger->debug("get pins");

            JsonArray* pins = object->getArray("pins");
            if (pins) {
                m_logger->debug("pins: %s",pins->toJsonString().get());
                pins->each([&](JsonElement*&item) {
                    m_logger->debug("got pin");
                    JsonObject* pin = item->asObject();
                    m_logger->debug("\tpin: %s",pin->toJsonString().get());
                    if (pin){
                        m_logger->debug("add pin %d",pin->get("number",-1));
                        LedPin* configPin = config.addPin(pin->get("number",-1),pin->get("ledCount",0),pin->get("reverse",false)); 
                        JsonProperty*maxBrightness = pin->getProperty("maxBrightness");
                        if (maxBrightness&& maxBrightness->getValue()){
                            configPin->maxBrightness = maxBrightness->getValue()->getInt();
                        }
                        JsonProperty*pixelType = pin->getProperty("pixelType");
                        if (pixelType&& pixelType->getValue()){
                            configPin->pixelType = getPixelType(pixelType->getValue()->getString());
                        }
                    } else {
                        m_logger->error("pin is not an Object");
                    }
                });
            } else {
                m_logger->debug("no pins found");
            }

            /* scripts are loaded from the filesystem, not stored*/
            m_logger->debug("\tdone reading JSON");
            return true;
        }

    protected:
        void setDefaults() {
            
        }
    private:
        
};
};
#endif