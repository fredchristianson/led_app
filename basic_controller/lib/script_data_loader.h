#ifndef DR_SCRIPT_DATA_LOADER_H
#define DR_SCRIPT_DATA_LOADER_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"
#include "./data.h"
#include "./file_system.h"
#include "./config.h"
#include "./script/script.h"
#include "./data_loader.h"
#include "./util.h"


namespace DevRelief {

Logger ScriptLoaderLogger("ScriptDataLoader",SCRIPT_LOADER_LOGGER_LEVEL);
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
            script->setName(jsonString(obj,S_NAME,"unnamed"));

            JsonArray * arr = obj->getArray("commands");
            arr->each([&](JsonElement*item) {
                m_logger->debug("\tgot json command");
                JsonObject*obj = item->asObject();
                if (obj) {
                    const char * type = obj->get("type","unknown");
                    m_logger->debug("\t\ttype: %s",type);
                    ScriptCommand* cmd = NULL;
                    if (matchName(S_RGB,type)) {
                        cmd= jsonToRGBCommand(obj);
                    } else if (matchName(S_HSL,type)) {
                        cmd=jsonToHSLCommand(obj);
                    }  else if (matchName(S_VALUES,type)) {
                        m_logger->debug("read Values command");
                        cmd=jsonToValueCommand(obj);
                    }  else if (matchName(S_POSITION,type)) {
                        cmd=jsonToPositionCommand(obj);
                    } else {
                        m_logger->error("unknown ScriptCommand type %s",type);
                        m_logger->info(obj->toJsonString().text());
                    }
                    if (cmd != NULL) {
                        JsonObject*posJson = obj->getChild("position");
                        
                        if (posJson != NULL) {
                            m_logger->debug("got position json");
                            IScriptPosition*pos = jsonToPosition(posJson);
                            if (pos != NULL) {
                                m_logger->debug("got position object");
                                cmd->setScriptPosition(pos);
                            }
                        }
                        script->add(cmd);
                    }
                } else {
                    m_logger->error("\t\tcommand is not an object");
                }
            });
            return script;
        }

        const char * jsonString(JsonObject* obj,const char * name, const char * defaultValue) {
            JsonProperty* prop = obj->getProperty(name);
            if (prop){
                const char * val = prop->get(defaultValue);
                return val;
            }
            return defaultValue;
        }


        PositionCommand* jsonToPositionCommand(JsonObject* json) {
            m_logger->debug("create PositionCommand");
            PositionCommand* cmd = new PositionCommand();
            IScriptPosition* pos = jsonToPosition(json);
            cmd->setScriptPosition(pos);
            return cmd;
        }


        ScriptPosition* jsonToPosition(JsonObject* json) {
            ScriptPosition* pos = new ScriptPosition();
            pos->setStartValue(jsonToValue(json,"start"));
            pos->setCountValue(jsonToValue(json,"count"));
            pos->setEndValue(jsonToValue(json,"end"));
            pos->setSkipValue(jsonToValue(json,"skip"));
            const char * type = json->get("unit","percent");
            if (Util::equal(type,"pixel")){
                pos->setUnit(POS_PIXEL);
            }
            pos->setWrap(jsonToValue(json,"wrap"));
            pos->setReverse(jsonToValue(json,"reverse"));
            pos->setAnimator(jsonToPositionAnimator(json,"animate"));
            return pos;
        }

        ScriptValueCommand* jsonToValueCommand(JsonObject* json) {
            ScriptValueCommand* cmd = new ScriptValueCommand();
            m_logger->debug("created ScriptValueCommand");
            json->eachProperty([&](const char* name, JsonElement*value){
                if (!Util::equal("type",name)) {
                    ScriptValue * scriptValue = jsonToValue(value);
                    if (scriptValue == NULL) {
                        m_logger->error("unable to get ScriptValue from %s",value->toJsonString().text());
                    } else {
                        m_logger->debug("add ScriptValue for %s",name);
                        cmd->add(name,scriptValue);
                    }
                }
            });
   
            return cmd;
        }
        RGBCommand* jsonToRGBCommand(JsonObject* json) {
            RGBCommand* cmd = new RGBCommand();
            cmd->setRed(jsonToValue(json,"red"));
            cmd->setGreen(jsonToValue(json,"green"));
            cmd->setBlue(jsonToValue(json,"blue"));
            return cmd;
        }


        HSLCommand* jsonToHSLCommand(JsonObject* json) {
            HSLCommand* cmd = new HSLCommand();
            cmd->setHue(jsonToValue(json,"hue"));
            cmd->setLightness(jsonToValue(json,"lightness"));
            cmd->setSaturation(jsonToValue(json,"saturation"));
            return cmd;
        }

        PositionAnimator* jsonToPositionAnimator(JsonObject* json, const char * name) {
            m_logger->test("get PositionAnimator from %s",json->toJsonString().get());
            JsonElement * jsonValue = json->getPropertyValue(name);
            m_logger->test("got %s property 0x%04X",name,jsonValue);
            PositionAnimator* animator = NULL;
            if (jsonValue != NULL && jsonValue->asObject()) {
                m_logger->debug("create PositionAnimator");
                JsonObject* obj = jsonValue->asObject();
                animator = new PositionAnimator();
                animator->setDuration(jsonToValue(obj,"duration"));
                animator->setSpeed(jsonToValue(obj,"speed"));
                animator->setRepeat(jsonToValue(obj,"repeat"));
                animator->setRepeatDelay(jsonToValue(obj,"delay"));
            }
            m_logger->test("return animator 0x%04X",animator);
            return animator;
        }

        ScriptValue* jsonToValue(JsonObject* json, const char * name) {
            JsonElement * jsonValue = json->getPropertyValue(name);
            if (jsonValue == NULL) {
                //m_logger->debug("No value found for %s",name);
                return NULL;
            }
            return jsonToValue(jsonValue);
        }

        ScriptValue* jsonToValue(JsonElement* jsonValue) {
            ScriptValue* scriptValue = NULL;
            if (jsonValue->isString()) {
                const char * val = jsonValue->getString();
                if (Util::startsWith(val,"var(")){
                    scriptValue = parseVarName(val);
                } else if (Util::startsWith(val,"func:")){
                    scriptValue = parseFunctionName(val);
                } else {
                    scriptValue = new ScriptStringValue(val);
                }
            } else if (jsonValue->isObject()) {
                JsonObject*valueObject = jsonValue->asObject();
                ScriptValue * start = jsonToValue(valueObject,"start");
                if (start == NULL) {
                    start = jsonToValue(valueObject,"value");
                }
                ScriptValue * end = jsonToValue(valueObject,"end");
                scriptValue = new ScriptRangeValue(start,end);
            } else if (jsonValue->isBool()) {
                scriptValue = new ScriptBoolValue(jsonValue->getBool());
            } else if (jsonValue->isNumber()) {
                scriptValue = new ScriptNumberValue(jsonValue->getFloat());
            }
            return scriptValue;
        }

        ScriptVariableValue* parseVarName(const char * val) {
            if (val == NULL) { return NULL;}
            const char * lparen = strchr(val,'(');
            const char * rparen = strchr(val,')');
            if (lparen == NULL || rparen == NULL) {
                m_logger->error("bad variable name: %s",val);
                return NULL;
            }
            auto result = DRString(lparen+1,(rparen-lparen)-1);
            m_logger->debug("got variable name %s",result.text());
            double defaultValue = 0;
            const char * def = strchr(val,'|');
            if (def != NULL) {
                m_logger->debug("found default value %s",def);
                while(*def != 0 && !isdigit(*def)) {
                    def++;
                }
                if (*def != 0) {
                    m_logger->debug("convert to float %s",def);
                    defaultValue = atof(def);
                }
            } else {
                m_logger->debug("no defaul value");
            }
            return new ScriptVariableValue(result.text(),defaultValue);
        }

        ScriptFunctionValue* parseFunctionName(const char * val) {
            return new ScriptFunctionValue(val);
        }
      
    protected:
        void setDefaults() {
            
        }
    private:
        
};
};
#endif