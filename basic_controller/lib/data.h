#ifndef DR_DATA_H
#define DR_DATA_H
#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"


namespace DevRelief {

Logger PathLogger("JsonPath",DATA_LOGGER_LEVEL);
Logger DataLogger("Data",DATA_LOGGER_LEVEL);
class JsonPath {
    public:
        JsonPath() {
           m_logger = &PathLogger;
        }

        ~JsonPath() {
            
        }


        bool getParent(JsonElement&top, const char * path,JsonObject*& parent,const char*&name) {
            m_logger->debug("Create property %s",path);
            parent = top.asObject();
            const char** parts = strings.split(path,"/");
            name = path;
            while(parent != NULL && *parts != NULL) {
                name = *parts;
                JsonProperty* childProp = parent->getProperty(*parts);
                JsonElement*child = (childProp==NULL) ? NULL : childProp->getValue();
                parts++;
                if (*parts != NULL) {
                    if (childProp == NULL || childProp->getValue()==NULL || childProp->getValue()->asObject() == NULL) {
                        child = new JsonObject(*(top.getRoot()));
                        parent->set(name,child);
                        parent = child->asObject();
                    } else {
                        parent = child->asObject();
                    }
                }
            }
            
            return *parts == NULL;
        }

        JsonElement* getPropertyValue(JsonElement&top, const char * path) {
            m_logger->debug("get property %s",path);
            const char** parts = strings.split(path,"/");
            JsonObject* parent = top.asObject();
            JsonElement* result = &top;
            while(parent != NULL && *parts != NULL) {
                JsonProperty* childProp = parent->getProperty(*parts);
                result = (childProp==NULL) ? NULL : childProp->getValue();
                parts++;
                if (*parts != NULL) {
                    parent = result->asObject();
                }
            }
            
            return result;
        }

    
    private:
    Logger * m_logger;
    DRStringBuffer strings;

};


class Data : public JsonRoot {
public:
    Data() : m_obj(*this) {
        m_logger = &DataLogger;
        add(&m_obj);
    }
    virtual ~Data() {
        forget(&m_obj);
    }


    bool addProperty(const char * path,JsonElement * child) { 
       m_logger->debug("add JsonElement prop %s",path);
        JsonPath jsonPath;
        JsonObject* parent;
        const char * name;
        if (jsonPath.getParent(m_obj,path,parent,name)) {
            parent->set(name,child);
            return true;
        }
        return false;
    }


    bool addProperty(const char * path,bool b) { 
       m_logger->debug("add int prop %s %d",path,b);
        JsonPath jsonPath;
        JsonObject* parent;
        const char * name;
        if (jsonPath.getParent(m_obj,path,parent,name)) {
            parent->set(name,b);
            return true;
        }
        return false;
    }

    bool addProperty(const char * path,int b) { 
        m_logger->debug("add bool prop %s %d",path,b);
        JsonPath jsonPath;
        JsonObject* parent;
        const char * name;
        if (jsonPath.getParent(m_obj,path,parent,name)) {
            parent->set(name,b);
            return true;
        }
        return false;
    }
    
    bool addProperty(const char * path,double b) { 
        m_logger->debug("add float prop %s %f",path,b);

        JsonPath jsonPath;
        JsonObject* parent;
        const char * name;
        if (jsonPath.getParent(m_obj,path,parent,name)) {
            parent->set(name,b);
            return true;
        }
        return false;
    }
    bool addProperty(const char * path,const char * b) { 
        m_logger->debug("add string prop %s %d",path,b);
        JsonPath jsonPath;
        JsonObject* parent;
        const char * name;
        if (jsonPath.getParent(m_obj,path,parent,name)) {
            parent->set(name,b);
            return true;
        }
        return false;      
    }


    int getInt(const char * path, int defaultValue=0) {
        m_logger->debug("get int prop %s",path);
        JsonPath jsonPath;
        JsonElement* element = jsonPath.getPropertyValue(m_obj,path);
        int val=defaultValue;
        if (element != NULL) {
            m_logger->debug("\tgot property");
            element->getIntValue(val,defaultValue);
        }

        return val;
    }


    bool getBool(const char * path, bool defaultValue=0) {
        m_logger->debug("get int prop %s %d",path);
        JsonPath jsonPath;
        JsonElement* element = jsonPath.getPropertyValue(m_obj,path);
        bool val=defaultValue;
        if (element != NULL) {
            m_logger->debug("\tgot property");
            element->getBoolValue(val,defaultValue);
        }

        return val;
    }


    double getFloat(const char * path, double defaultValue=0) {
        m_logger->debug("get int prop %s ",path);
        JsonPath jsonPath;
        JsonElement* element = jsonPath.getPropertyValue(m_obj,path);
        double val=defaultValue;
        if (element != NULL) {
            m_logger->debug("\tgot property");
            element->getFloatValue(val,defaultValue);
        }

        return val;
    }

    
    const char * getString(const char * path,char * buffer, size_t maxLen,const char * defaultValue="") {
        m_logger->debug("get string prop %s ",path);
        JsonPath jsonPath;
        JsonElement* element = jsonPath.getPropertyValue(m_obj,path);
        const char * val;
        if (element != NULL && element->getStringValue(val,NULL)) {
            strncpy(buffer,val,maxLen);
        } else {
            strncpy(buffer,defaultValue,maxLen);
        }

        return buffer;
    }
    
    void dump() {
        if (m_logger->getLevel() < DEBUG_LEVEL) {
            return;
        }
        DRString buf;
        JsonGenerator gen(buf);
        gen.generate(this);
        m_logger->debug("JSON:");
        m_logger->debug(buf.text());
    }
protected:


    Logger * m_logger;
private:
    JsonObject m_obj;
};

class ApiResult : public Data {
    public:
        ApiResult(JsonElement *json) {
            m_logger->debug("create JSON ApiResult 0x%04X",json);
            addProperty("code",200);
            addProperty("success",true);
            addProperty("message","success");
            addProperty("data",json);
            mimeType = "text/json";
        }

        ApiResult(bool success=true) {
            addProperty("code",success ? 200:500);
            addProperty("success",true);
            addProperty("message","success");

        }
        ApiResult(bool success, const char * msg, ...) {
            va_list args;
            va_start(args,msg);
            addProperty("success",success);
            addProperty("code",success ? 200:500);
            setMessage(msg,args);
        }
        
        void setData(JsonElement*json) {
            addProperty("data",json);
        }
        bool toText(DRString& apiText){
            JsonGenerator gen(apiText);
            bool result = gen.generate(this);
            m_logger->debug("generated JSON: %s",apiText.text());
            return result;
        }
        void setCode(int code) {
            addProperty("code",code);
        }

        void setMessage(const char *msg,...) {
            va_list args;
            va_start(args,msg);
            setMessageArgs(msg,args);
        }

        void setMessageArgs(const char * msg, va_list args) {
            if (msg == NULL) {
                return;
            }
            int len = strlen(msg)*2+100;
            m_logger->debug("setting message len %d: %s",len,msg);
            DRBuffer message;
            message.reserve(len);
            vsnprintf((char*)message.data(),message.getMaxLength(),msg,args);
            m_logger->debug("formatted");
            m_logger->debug("add property %s",message.text());
            addProperty("message",message.text());
        }

        void setSuccess(bool success,int code=-1) {
            addProperty("success",success);
            if (code == -1) {
                code = success ? 200 : 500;
            }
            addProperty("code",code);
        }

        void send(Request* req){
            JsonObject* mem = createObject("memory");
            int heap = ESP.getFreeHeap();
            mem->set("stack",(int)ESP.getFreeContStack());
            mem->set("heap",heap);
            if (m_lastHeapSize != 0) {
                mem->set("lastHeap",(int)m_lastHeapSize);
                mem->set("heapChange",(int)heap-m_lastHeapSize);
            }
            m_lastHeapSize = heap;
            DRString result = toJsonString();
            req->send(getInt("code",200),mimeType.text(),result.text());
        }
    private:
        DRString mimeType;
        static int m_lastHeapSize;
        
};

int ApiResult::m_lastHeapSize = 0;

};
#endif