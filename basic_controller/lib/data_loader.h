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

Logger DataLoaderLogger("DataLoader",DEBUG_LEVEL);



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
        delete m_json;
    }

    DRFileBuffer& getBuffer() { return m_buffer;}
    const char * getTest() {
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
    }

    void setSuccess(bool success) { m_success = success;}
    bool isSuccess() { return m_success;}
    const char * error() { return m_error;}


private:
    friend DataLoader;
    DRFileBuffer m_buffer;    
    FileType m_type;
    JsonRoot* m_jsonRoot;
    JsonRoot* m_json;
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
        DRBuffer buffer;
        JsonGenerator gen(buffer);
        gen.generate(json);
        return m_fileSystem.write(path,buffer);
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
                m_logger->debug("\tparse file");
                JsonRoot* root = parser.read(result.getBuffer().text());
                m_logger->debug("\tset root %s",(root == NULL ? "NULL" : "found"));
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
private:
    static DRFileSystem m_fileSystem;

};

DRFileSystem DataLoader::m_fileSystem;

class ConfigDataLoader : public DataLoader {
    public:
        ConfigDataLoader() {
            
        }

        
        bool initialize(Config& config) {
            config.clearScripts();
            config.clearPins();
            config.setBrightness(40);
            config.setMaxBrightness(100);
            return true;
        }

        bool saveConfig(Config& config, const char * path = "/config.json"){
            JsonRoot root;
            JsonObject * json = root.createObject();
            json->set("hostName",config.getHostname());
            json->set("ipAddress",config.getAddr());
            json->set("brightness",config.getBrightness());
            json->set("maxBrightness",config.getMaxBrightness());
            json->set("runningScript",config.getRunningScript());
            JsonArray* pins = new JsonArray(root);
            json->set("pins",pins);
            config.getPins().each( [&](LedPin* &pin) {
                m_logger->debug("handle pin %d",pin->number); 
                JsonObject* pinElement = pins->createObjectElement();
                pinElement->set("number",pin->number);
                pinElement->set("ledCount",pin->ledCount);
                pinElement->set("reverse",pin->reverse);
                pins->addItem(pinElement);
            });
            m_logger->debug("pins done");

            JsonArray* scripts = new JsonArray(root);
            json->set("scripts",scripts);
            config.getScripts().each( [&](DRString &script) {
                m_logger->debug("handle script %s",script.get()); 
                
                scripts->add(script.get());
            });
            m_logger->always("scripts done");

            return writeJsonFile(path,json);
        }

        bool loadConfig(Config& config, const char * path = "/config.json"){
            LoadResult result;
            m_logger->debug("initialize config");
            initialize(config);
            m_logger->debug("load file");
            if (loadFile(path,result)){
                m_logger->debug("process json");
                if (!readJson(config,result)) {
                    result.setSuccess(false);
                }

            } else {
                m_logger->error("Config json not found");
            }
            return result.isSuccess();
        }

        bool readJson(Config& config, LoadResult& result) {
            m_logger->debug("readJson.  getJson object");
            JsonRoot * root = result.getJsonRoot();

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
            m_logger->always("pins: %s",pins->toJsonString());
            if (pins) {
                pins->each([&](JsonElement*&item) {
                    m_logger->debug("got pin");
                    JsonObject* pin = item->asObject();
                    m_logger->always("\tpin: %s",pin->toJsonString());
                    if (pin){
                        m_logger->debug("add pin %d",pin->get("number",-1));
                        config.addPin(pin->get("number",-1),pin->get("ledCount",0),pin->get("reverse",false)); 
                    } else {
                        m_logger->error("pin is not an Object");
                    }
                });
            } else {
                m_logger->debug("no pins found");
            }

            
            JsonArray* scripts = object->getArray("scripts");
            if (scripts) {
                scripts->each([&](JsonElement*&item) {
                    DRString name;
                    if (item->getDRStringValue(name)){
                        m_logger->debug("add script %s",name.get());
                        config.addScript(name);
                    }
                });
            }
            return result.isSuccess();
        }

    protected:
        void setDefaults() {
            
        }
    private:
        
};
};
#endif