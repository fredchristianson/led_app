#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\script_data_loader.h"
#ifndef DR_SCRIPT_DATA_LOADER_H
#define DR_SCRIPT_DATA_LOADER_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"
#include "./data.h"
#include "./file_system.h"
#include "./config.h"
#include "./script/script.h"
#include "./script/script_position.h"
#include "./script/json_names.h"
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
            m_logger->debug("jsonToScript");

            if (jsonRoot == NULL || jsonRoot->getTopObject()== NULL) {
                return NULL;
            }
            
            JsonObject* obj = jsonRoot->getTopObject();
            Script* script = new Script();

            m_logger->debug("convert JSON object to Script");
            script->setName(jsonString(obj,S_NAME,"unnamed"));
            script->setFrequencyMSec(jsonInt(obj,S_FREQUENCY,0));

            JsonArray * arr = obj->getArray("commands");
            jsonToCommands(arr,script->getContainer());
                       
            m_logger->debug("created Script");
            return script;

        }

        void jsonToCommands(JsonArray* arr, ICommandContainer* parent) {
            if (arr == NULL) {
                return;
            }
            arr->each([&](JsonElement*item) {
            
                m_logger->debug("\tgot json command");
                JsonObject*obj = item->asObject();
                
                if (obj) {
                    const char * type = obj->get("type","unknown");
                    m_logger->debug("\t\ttype: %s",type);
                    ScriptCommandBase* cmd = NULL;
                    
                    if (matchName(S_RGB,type)) {
                        cmd= jsonToRGBCommand(obj);
                    } else if (matchName(S_HSL,type)) {
                        cmd=jsonToHSLCommand(obj);
                    } else if (matchName(S_XHSL,type)) {
                        cmd=jsonToXHSLCommand(obj);
                    }  else if (matchName(S_VALUES,type)) {
                        m_logger->debug("read Values command");
                        cmd=jsonToValueCommand(obj);
                    } else if (matchName(S_POSITION,type)) {
                       cmd=jsonToPositionCommand(obj);
                    
                    } else if (matchName(S_SEGMENT,type)){
                        cmd=jsonToSegment(obj);
                    } else if (matchName(S_CREATE,type)){
                        cmd=jsonToCreate(obj);
                    } else {
                        m_logger->error("unknown ScriptCommand type %s",type);
                        m_logger->info(obj->toJsonString().text());
                    } 
                    
                    if (cmd != NULL) {

                        m_logger->debug("add command %s",cmd->getType());
                        ScriptPosition*pos = jsonToPosition(obj);
                        if (pos != NULL) {
                            m_logger->debug("got position object");
                            cmd->setScriptPosition(pos);
                        }

                        JsonObject*valJson = obj->getChild("values");
                        
                        if (valJson != NULL) {
                            m_logger->debug("\tgot values json");
                            jsonGetValues(valJson,cmd);
                        }
                        m_logger->debug("\tadd child to script");
                        parent->add(cmd);
                        m_logger->debug("\tadded");
                        
                    }
                    
                } else {
                    m_logger->error("\t\tcommand is not an object");
                }
                
                
            });
        }

        const char * jsonString(JsonObject* obj,const char * name, const char * defaultValue) {
            JsonProperty* prop = obj->getProperty(name);
            if (prop){
                const char * val = prop->get(defaultValue);
                return val;
            }
            return defaultValue;
        }

        int jsonInt(JsonObject* obj,const char * name, int defaultValue) {
            JsonProperty* prop = obj->getProperty(name);
            if (prop){
                int val = prop->get(defaultValue);
                return val;
            }
            return defaultValue;
        }


        ScriptSegmentContainer* jsonToSegment(JsonObject* json) {
            ScriptSegmentContainer* seg = new ScriptSegmentContainer();
            JsonArray* arr = json->getArray("commands");
            jsonToCommands(arr,seg);
            return seg;
        }
        ScriptTemplate* jsonToCreate(JsonObject* json) {
            JsonObject* templ =json->getPropertyValue("template")->asObject();
            if (templ == NULL) {
                return NULL;
            }
            ScriptTemplate *templateContainer = new ScriptTemplate();
            templateContainer->setCount(jsonToValue(json,"count"));            
            templateContainer->setMinCount(jsonToValue(json,"min-count"));            
            templateContainer->setMaxCount(jsonToValue(json,"max-count"));
            templateContainer->setStartChance(jsonToValue(json,"start-chance"));
            templateContainer->setEndChance(jsonToValue(json,"end-chance"));
            JsonArray* arr = templ->getArray("commands");
            jsonToCommands(arr,templateContainer);

            JsonObject* values = templ->getPropertyValue("values")->asObject();
            if (values) {
                values->eachProperty([&](const char* name, JsonElement*value){
                    if (!Util::equal("type",name)) {
                        IScriptValue * scriptValue = jsonToValue(value);
                        if (scriptValue == NULL) {
                            m_logger->error("unable to get ScriptValue from %s",value->toJsonString().text());
                        } else {
                            m_logger->debug("\tadd ScriptValue for %s",name);
                            templateContainer->addValue(name,scriptValue);
                        }
                    }
                });
            }
            return templateContainer;
        }

        PositionCommand* jsonToPositionCommand(JsonObject* json) {
            m_logger->debug("create PositionCommand");
            PositionCommand* cmd = new PositionCommand();
            ScriptPosition* pos = jsonToPosition(json);
            cmd->setScriptPosition(pos);
            return cmd;
        }

/**/
        ScriptPosition* jsonToPosition(JsonObject* json) {
            JsonElement* position = json->getPropertyValue("position");
            if (position != NULL) {
                json = position->asObject();
            }
            if (json == NULL) {
                return NULL;
            }

            if (json->getProperty("start") || json->getProperty("count")||
                json->getProperty("skip")||json->getProperty("reverse")||json->getProperty("offset")||json->getProperty("strip")){
                ScriptPosition* pos = new ScriptPosition();

                pos->setStartValue(jsonToValue(json,"start"));
                pos->setCountValue(jsonToValue(json,"count"));
                pos->setEndValue(jsonToValue(json,"end"));
                pos->setSkipValue(jsonToValue(json,"skip"));
                const char * unit = json->get("unit","inherit");
                
                if (Util::equal(unit,"pixel")){
                    pos->setUnit(POS_PIXEL);
                } else if (Util::equal(unit,"percent")){
                    pos->setUnit(POS_PERCENT);
                } else {
                    pos->setUnit(POS_INHERIT);
                }
                const char * type = json->get("type","relative");
                pos->setPositionType(positionTypeToInt(type));
                pos->setWrap(jsonToValue(json,"wrap"));
                pos->setReverse(jsonToValue(json,"reverse"));
                pos->setOffset(jsonToValue(json,"offset"));
                pos->setStripNumber(jsonToValue(json,"strip"));
                return pos;
            }
            return NULL;
        }

        PositionType positionTypeToInt(const char * type){
            PositionType pt = POS_ABSOLUTE;
            if (type == NULL) { return pt;}
            if (Util::equal(type,"absolute")){
                pt = POS_ABSOLUTE;
            } else if (Util::equal(type,"relative")){
                pt = POS_RELATIVE;
            } else if (Util::equal(type,"after")){
                pt = POS_AFTER;
            } else if (Util::equal(type,"strip")){
                pt = POS_STRIP;
            }
            return pt;
        }

        ScriptValueCommand* jsonToValueCommand(JsonObject* json) {
            ScriptValueCommand* cmd = new ScriptValueCommand();
            jsonGetValues(json,cmd);
            return cmd;
        }

        void jsonGetValues(JsonObject*json, ScriptCommandBase*cmd) {
            m_logger->debug("read values");
            json->eachProperty([&](const char* name, JsonElement*value){
                if (!Util::equal("type",name)) {
                    IScriptValue * scriptValue = jsonToValue(value);
                    if (scriptValue == NULL) {
                        m_logger->error("unable to get ScriptValue from %s",value->toJsonString().text());
                    } else {
                        m_logger->debug("\tadd ScriptValue for %s",name);
                        cmd->addValue(name,scriptValue);
                    }
                }
            });
            m_logger->debug("\tdone");

        }
       /* */
        RGBCommand* jsonToRGBCommand(JsonObject* json) {
            RGBCommand* cmd = new RGBCommand();
            cmd->setRed(jsonToValue(json,"red"));
            cmd->setGreen(jsonToValue(json,"green"));
            cmd->setBlue(jsonToValue(json,"blue"));
            cmd->setOperation(jsonString(json,"op","replace"));

            return cmd;
        }


        HSLCommand* jsonToHSLCommand(JsonObject* json) {
            HSLCommand* cmd = new HSLCommand();
            cmd->setHue(jsonToValue(json,"hue"));
            cmd->setLightness(jsonToValue(json,"lightness"));
            cmd->setSaturation(jsonToValue(json,"saturation"));
            cmd->setOperation(jsonString(json,"op","replace"));
            return cmd;
        }

        XHSLCommand* jsonToXHSLCommand(JsonObject* json) {
            XHSLCommand* cmd = new XHSLCommand();
            cmd->setHue(jsonToValue(json,"hue"));
            cmd->setLightness(jsonToValue(json,"lightness"));
            cmd->setSaturation(jsonToValue(json,"saturation"));
            cmd->setOperation(jsonString(json,"op","replace"));
            cmd->setIn(jsonToValue(json,"in"));
            cmd->setOut(jsonToValue(json,"out"));
            return cmd;
        }



        ValueAnimator* jsonToValueAnimator(JsonObject* json) {
            m_logger->test("get ValueAnimator from %s",json->toJsonString().get());
            JsonElement * jsonValue = json->getPropertyValue("animate");
            JsonObject* animate = NULL;
            if (jsonValue == NULL) {
                // see if the animate properties are promoted ot parent
                if (json->getProperty("duration") || json->getProperty("speed") || json->getProperty("unfold") || json->getProperty("ease")|| json->getProperty("ease-in")|| json->getProperty("ease-out")) {
                    animate = json;
                }
            } else {
                animate = jsonValue->asObject();
            }
            m_logger->test("use animate object 0x%04X",animate);
            ValueAnimator* animator = NULL;
            if (animate) {
                m_logger->debug("create ValueAnimator");
                JsonObject* obj = animate;
                IScriptValue* duration = jsonToValue(obj,"duration");
                IScriptValue* speed = jsonToValue(obj,"speed");
                IScriptValue* delayResponse = jsonToValue(obj,"delay-value");
                if (delayResponse == NULL) {
                    // try old name if new one is missing
                    delayResponse = jsonToValue(obj,"delayValue");
                }
                if (speed != NULL) {
                    TimeValueAnimator* aspeed = new SpeedValueAnimator(speed);
                    aspeed->setRepeatValue(jsonToValue(obj,"repeat"));
                    aspeed->setRepeatDelayValue(jsonToValue(obj,"delay"));
                    aspeed->setDelayResponseValue(delayResponse);
                    animator = aspeed;
                } else if (duration) {
                    TimeValueAnimator* aduration = new DurationValueAnimator(duration);
                    aduration->setRepeatValue(jsonToValue(obj,"repeat"));
                    aduration->setRepeatDelayValue(jsonToValue(obj,"delay"));
                    aduration->setDelayResponseValue(delayResponse);
                    animator = aduration;
                } else {
                    animator = new PositionValueAnimator();
                }

                animator->setUnfold(jsonToValue(obj,"unfold"));
                animator->setEase(jsonToValue(obj,"ease"));
                animator->setEaseIn(jsonToValue(obj,"ease-in"));
                animator->setEaseOut(jsonToValue(obj,"ease-out"));
            }
            m_logger->debug("return value animator 0x%04X",animator);
            return animator;
        }
    
        IScriptValue* jsonToValue(JsonObject* json, const char * name) {
            JsonElement * jsonValue = json->getPropertyValue(name);
            if (jsonValue == NULL) {
                m_logger->never("No value found for %s",name);
                return NULL;
            }
            return jsonToValue(jsonValue);
        }

        IScriptValue* jsonToValue(JsonElement* jsonValue) {
            IScriptValue* scriptValue = NULL;
            if (jsonValue->isNull()) {
                scriptValue = new ScriptNullValue();
            } else if (jsonValue->isString()) {
                const char * val = jsonValue->getString();
                if (Util::startsWith(val,"var(")){
                    scriptValue = parseVarName(val);
                } else if (Util::startsWith(val,"sys(")){
                    scriptValue = parseSysVarName(val);
                } else {
                    scriptValue = new ScriptStringValue(val);
                }
            } else if (jsonValue->isObject()) {
                scriptValue = jsonObjectToValue(jsonValue->asObject());
               
            } else if (jsonValue->isBool()) {
                m_logger->debug("creating ScriptBoolValue()");
                scriptValue = new ScriptBoolValue(jsonValue->getBool());
                m_logger->debug("\tdone ScriptBoolValue()");
                m_logger->debug("\ttoString()");
                m_logger->debug("%s",scriptValue->toString().get());
            } else if (jsonValue->isNumber()) {
                scriptValue = new ScriptNumberValue(jsonValue->getFloat());
            } else if (jsonValue->isArray()) {
                scriptValue = jsonArrayToFunction(jsonValue->asArray());
            }
            return scriptValue;
        }

        IScriptValue* jsonArrayToFunction(JsonArray* arr) {
            int cnt = arr->getCount();
            if (cnt == 0) { return NULL;}
            JsonElement* jsonName = arr->getAt(0);
            const char * name = jsonName->getString();
            if (name == NULL) {
                m_logger->error("function array needs a string as first element");
                return NULL;
            }
            FunctionArgs* args = getFunctionArgs(arr,1);
            return new ScriptFunction(name,args);
        }

        IScriptValue* jsonToPattern(JsonObject*obj) {
            m_logger->never("jsonToPattern");
            if (obj == NULL) { 
                m_logger->never("no object");
                return NULL;
            }
            JsonArray* elements = obj->getArray("pattern");
            if (elements == NULL) {
                m_logger->never("no pattern array");
                return NULL;
            }
            auto animator = jsonToValueAnimator(obj);
            auto patternValue = new PatternValue(animator);
            auto extend = jsonString(obj,"extend","repeat");
            PatternExtend extendInt = (PatternExtend)Util::mapText2Int("repeat:0,stretch:1,clip:2,none:2",extend,0);
            m_logger->never("extend %s==>%d",extend,extendInt);
            patternValue->setExtend(extendInt); //repeat-stretch-none
            elements->each([&](JsonElement*item){
                m_logger->never("check item");

                ScriptPatternElement* pattern = NULL;
                if (item->isObject()) {
                    m_logger->never("\tparse object");
                    pattern = patternElementFromObject(item->asObject());
                } else if (item->isString()){
                    m_logger->never("\tparse string");
                    pattern = patternElementFromString(item);
                } else {
                    m_logger->never("\tuse number");
                    if (item->isNull()) {
                        pattern = new ScriptPatternElement(1,new ScriptNullValue());
                    } else {
                        pattern = new ScriptPatternElement(1,new ScriptNumberValue(item->getFloat()));
                    }
                }
                patternValue->addElement(pattern);
            });
            return patternValue;
        };

        ScriptPatternElement* patternElementFromObject(JsonObject* obj) {
            IScriptValue* value = jsonToValue(obj,"value");
            if (value == NULL) {
                return NULL;
            }
            int repeat = jsonInt(obj,"count",1);
            return new ScriptPatternElement(repeat,value);
        }

        ScriptPatternElement* patternElementFromString(JsonElement* element) {
            const char * str = element->getString();
            if (str == NULL) { return NULL;}
            const char *  x = strchr(str,'x');
            int val = 0;
            int count = 1;
            if (x == NULL) {
                val = atof(str);
            } else {
                count = atoi(str);
                val = atof(x+1);
            }
            return new ScriptPatternElement(count,new ScriptNumberValue(val));
        }

        IScriptValue* jsonObjectToValue(JsonObject* obj) {
            JsonObject*valueObject = obj;
            JsonElement * funcName = obj->getPropertyValue("function");
            IScriptValue* scriptValue = NULL;
            if (funcName) {
                FunctionArgs* args = jsonObjectToFunctionArgs(obj);
                scriptValue = new ScriptFunction(funcName->getString(),args);
            } else {
                IScriptValue* pattern = jsonToPattern(valueObject);
                if (pattern == NULL) {
                    IScriptValue * start = jsonToValue(valueObject,"start");
                    if (start == NULL) {
                        start = jsonToValue(valueObject,"value");
                    }
                    IScriptValue * end = jsonToValue(valueObject,"end");
                    auto rangeValue = new ScriptRangeValue(start,end);
                    rangeValue->setAnimator(jsonToValueAnimator(valueObject));
                    scriptValue = rangeValue;
                } else {
                    scriptValue = pattern;
                }
            }
            return scriptValue;
        }

        FunctionArgs* jsonObjectToFunctionArgs(JsonObject* obj) {
            JsonArray* argArray = obj->getArray("args");
            return getFunctionArgs(argArray);
        }

        FunctionArgs* getFunctionArgs(JsonArray* argArray,int skip=0) {
            if (argArray == NULL) { return NULL;}
            FunctionArgs* args = new FunctionArgs();
            argArray->each([&](JsonElement*element){
                if (skip == 0){
                    IScriptValue*val = jsonToValue(element);
                    args->add(val);
                } else {
                    skip -= 1;
                }
            });
            return args;
        }
/**/
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
            auto varValue = new ScriptVariableValue(result.text());
            if (def != NULL) {
                m_logger->debug("found default value %s",def);
                const char * digit = def;
                while(*digit != 0 && !isdigit(*digit)) {
                    digit++;
                }
                if (*digit != 0) {
                    m_logger->debug("convert to float %s",digit);
                    defaultValue = atof(digit);
                    varValue->setDefaultValue(defaultValue);

                } else {
                    const char * tf = def;
                    // look for string starting with  't' (true) or 'f' (false)
                    while(*tf != 0 && *tf != 't' && *tf != 'f') {
                        tf++;
                    }
                    if (*tf != 0) {
                        varValue->setDefaultValue((*tf == 't'?1 : 0));
                    }
                }
            } else {
                m_logger->debug("no defaul value");
            }
            return varValue;
        }

        ScriptValue* parseSysVarName(const char * val) {
            if (val == NULL) { return NULL;}
            const char * lparen = strchr(val,'(');
            const char * rparen = strchr(val,')');
            if (lparen == NULL || rparen == NULL) {
                m_logger->error("bad variable name: %s",val);
                return NULL;
            }
            auto result = DRString(lparen+1,(rparen-lparen)-1);
            m_logger->debug("got system variable name %s",result.text());
            auto varValue = new ScriptSystemValue(result.text());

            return varValue;
        }

      
    protected:
        void setDefaults() {
            
        }
    private:
};
};
#endif
