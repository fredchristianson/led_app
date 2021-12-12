#ifndef DR_SCRIPT_DATA_LOADER_H
#define DR_SCRIPT_DATA_LOADER_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"
#include "./data.h"
#include "./file_system.h"
#include "./config.h"
#include "./script.h"
#include "./data_loader.h"


namespace DevRelief {

Logger ScriptLoaderLogger("DataLoader",SCRIPT_LOADER_LOGGER_LEVEL);
const char * SCRIPT_PATH_BASE="/script/";


class ScriptDataLoader : public DataLoader {
    public:
        ScriptDataLoader() {
            m_logger = & ScriptLoaderLogger;
        }


        bool initialize(Script& script) {

            return true;
        }

        DRString getPath(const char * name) {
            DRString path= SCRIPT_PATH_BASE;
            path += name;
            path += ".json";
            m_logger->debug("getPath(%s)==>%s",name,path.text());
            return path;
        }
        bool deleteScript(const char * name) {

            m_logger->debug("Delete script %s",name);
            return m_fileSystem.deleteFile(getPath(name));
        }

        Script* writeScript(const char * name, const char * text){
            SharedPtr<JsonRoot> jsonRoot;
            jsonRoot = parse(text);
            Script* script = jsonToScript(jsonRoot.get());
            if (script == NULL) {
                return NULL;
            }
            m_fileSystem.write(getPath(name),text);
            return script;
        }

        JsonRoot* parse(const char * text) {
            JsonParser parser;
            JsonRoot* root = parser.read(text);
            return root;
        }

        bool save(Script& script, const char * name){
            SharedPtr<JsonRoot> jsonRoot = toJson(script);
            return writeJsonFile(getPath(name),jsonRoot->getTopElement());
        }

        bool toJsonString(Script& script,DRString& result) {
            SharedPtr<JsonRoot> jsonRoot = toJson(script);
            JsonGenerator gen(result);
            return gen.generate(jsonRoot.get());
            
        }

        bool updateScript(const char * name, Script& script, const char * jsonText){
            return m_fileSystem.write(getPath(name),jsonText);
            /*
            JsonParser parser;
            JsonRoot * root = parser.read(jsonText);
            if (root == NULL) {
                return false;
            } else {
                if (readJson(script,root)) {
                    return save(script);
                }
            }
            return false;
            */
        }

        SharedPtr<JsonRoot> toJson(Script& script) {
            JsonRoot* root=new JsonRoot;  
            JsonObject * json = root->createObject();
            /*
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
            config.getPins().each( [&](LedPin* &pin) {
                m_logger->debug("handle pin %d",pin->number); 
                JsonObject* pinElement = pins->createObjectElement();
                pinElement->set("number",pin->number);
                pinElement->set("ledCount",pin->ledCount);
                pinElement->set("reverse",pin->reverse);
                pins->addItem(pinElement);
            });
            m_logger->debug("pins done");

            JsonArray* scripts = root->createArray();
            json->set("scripts",scripts);
            config.getScripts().each( [&](DRString &script) {
                m_logger->debug("handle script %s",script.get()); 
                
                scripts->add(script.get());
            });
            */
            m_logger->debug("scripts done");

            return root;
        }
        bool loadScriptJson(const char * name,LoadResult& result){
            m_logger->debug("load script %s",name);
            if (loadFile(getPath(name),result)){
                m_logger->debug("got JSON root=0x%04X, top=0x%04X",result.getJsonRoot(),result.getJson());
                return true;
            } else {
                m_logger->error("script json not found");
            }
            return false;
        }

        bool readJson(Script& script, JsonRoot* root) {
            m_logger->debug("readJson.  getJson object");
          
            return true;
        }

        Script* jsonToScript(LoadResult& load){
            return jsonToScript(load.getJsonRoot());
        }
        Script* jsonToScript(JsonRoot* jsonRoot){
            if (jsonRoot == NULL || jsonRoot->getTopObject()== NULL) {
                return NULL;
            }
            JsonObject* obj = jsonRoot->getTopObject();
            Script* script = new Script();
            m_logger->debug("convert JSON object to Script");

            JsonArray * arr = obj->getArray("commands");
            arr->each([&](JsonElement*item) {
                m_logger->debug("\tgot json command");
                JsonObject*obj = item->asObject();
                if (obj) {
                    DRString type = obj->get("type","unknown");
                    m_logger->debug("\t\ttype: %s",type.text());
                    ScriptCommand* cmd = NULL;
                    if (strcmp(type,"rgb")==0) {
                        cmd=new RGBCommand(obj->get("red",0),obj->get("green",0),obj->get("blue",0));
                    } else if (strcmp(type,"hsl")==0) {
                        cmd=new HSLCommand(obj->get("hue",-1),obj->get("saturation",-1),obj->get("lightness",-1));
                    } else {
                        m_logger->error("unknown ScriptCommand type %s",type.text());
                    }
                    if (cmd != NULL) {
                        script->add(cmd);
                    }
                } else {
                    m_logger->error("\t\tcommand is not an object");
                }
            });
            return script;
        }

    protected:
        void setDefaults() {
            
        }
    private:
        
};
};
#endif