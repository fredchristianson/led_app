#ifndef DR_DATA_H
#define DR_DATA_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"


namespace DevRelief {

class JsonPath {
    public:
        JsonPath(JsonElement*start, const char * name) {
            m_firstElement = new JsonProperty(*(start->getRoot()),name,start,NULL);
            parse(root, name);
        }
        ~JsonPath() {
            JsonProperty *del = m_firstElement;
            while(del == NULL) {
                del->forgetValue();
                del = del->getNext();
            }
            delete m_firstElement;
        }

        bool parse(JsonElement*root, const char * name){
            char ** parts = buffer.split("/",name);
        }
    private:
    DRStringBuffer buffer;
    JsonProperty* m_firstElement;
};


class Data : public JsonRoot {
public:
    Data() : obj(*this) {
        m_logger = new Logger("Data",DEBUG_LEVEL);
    }
    virtual ~Data() {}

    bool addProperty(const char * name,bool b) { 
        JsonObject* parent = getJsonParentByName(name, true);
        if (parent != NULL) {
            obj.add(name,b);
            return true;
        }
        return false;
    }
    bool addProperty(const char * name,int b) { 
        JsonObject* parent = getJsonParentByName(name, true);
        if (parent != NULL) {
            m_logger->debug("got parent.  write int to name %s",name);
            JsonProperty* prop = parent->add(name,b);
            if (prop == NULL) {
                m_logger->error("failed to write property %s",name);
            } else {
                m_logger->debug("wrote property %s",name);
                return true;
            }
        }
        return false;
    }
    bool addProperty(const char * name,double b) { 
        JsonObject* parent = getJsonParentByName(name, true);
        if (parent != NULL) {
            obj.add(name,b);
            return true;
        }
        return false;
    }
    bool addProperty(const char * name,const char * b) { 
        JsonObject* parent = getJsonParentByName(name, true);
        if (parent != NULL) {
            obj.add(name,b);
            return true;
        }
        return false;        
    }

    JsonObject* getJsonParentByName(const char * &name,bool createPath) {
        m_logger->debug("getJsonParentByName %s",name);
        JsonObject* probe = &obj;
        JsonObject* parent = probe;
        DRBuffer buf;
        size_t len = strlen(name);
        char* part = (char*)buf.reserve(len+1);
        strcpy(part,name);
        char* end = part+len;

        while(probe != NULL && part < end) {
            char* sep = strchr(part,'/');
            parent = probe;
            if (sep != NULL) {
                *sep = 0;
                m_logger->debug("\tget property %s",part);
                JsonProperty * prop = probe->getProperty(part);
                if (prop == NULL){
                    if (createPath) {
                        m_logger->debug("\tcreate path JsonObject %s",part);
                        JsonObject * child = new JsonObject(*(probe->getRoot()));
                        probe->add(part,child);
                        probe = child;
                        parent = child;
                    } else {
                        m_logger->debug("\tproperty not found and not created");
                        probe = NULL;
                        parent = NULL;
                    }
                } else {
                    probe = prop->getValue()->asObject();
                    if (probe == NULL) {
                        m_logger->error("path part %s is not a JsonObject",part);
                    }
                }
                part = sep+1;

            } else {
                m_logger->debug("\tat last path name %s",part);
                parent = probe;
                probe = NULL;
                name = name + (part-(const char *)buf.data());
            }
        }
        m_logger->debug("%s parent", (parent == NULL ? "no" : "found"));
        return parent;
    }

    JsonElement* getJsonElementByName(const char * name) {
        m_logger->debug("getJsonElementByName %s",name);
        JsonObject* probe = &obj;
        JsonElement* result = NULL;
        DRBuffer buf;
        size_t len = strlen(name);
        char* part = (char*)buf.reserve(len+1);
        strcpy(part,name);
        char* end = part+len;

        while(probe != NULL && part < end) {
            char* sep = strchr(part,'/');
            if (sep != NULL) {
                *sep = 0;
            }
            m_logger->debug("\tget property %s",part);
            JsonProperty* prop = probe->getProperty(part);
            result = prop == NULL ? NULL : prop->getValue();
            if (result == NULL){
                m_logger->warn("\tno property for %s",part);
            }
            probe = result == NULL ? NULL : result->asObject();
            if (probe == NULL) {
                m_logger->info("\tpart %s is not a JsonObject",part);
            }
            part = sep+1;
        }
        if (part>= end && result != NULL) {
            m_logger->debug("found element %s of type %d",name,result->getType());
            return result;
        } else {
            m_logger->warn("\tpath part missing %s",part);
            return NULL;
        }

    }


    int getInt(const char * name, int defaultValue=0) {
        int val = defaultValue;
        JsonElement* value = getJsonElementByName(name);
        if (value != NULL) {
            m_logger->debug("got JsonElement to get int %d",value->getType());
            value->getIntValue(val,defaultValue);
        } else {
            m_logger->warn("JsonElement %s not found",name);
        }
        return val;
    }
protected:
    Logger * m_logger;
private:
    JsonObject obj;
};

class ApiResult : public Data {
    public:
        ApiResult(bool success=true) {
            addProperty("success",true);
            addProperty("message","success");
            addProperty("code",success ? 200:500);
        }
        ApiResult(bool success, const char * msg, ...) {
            va_list args;
            va_start(args,msg);
            addProperty("success",success);
            addProperty("code",success ? 200:500);
            setMessage(msg,args);
        }

        void setCode(int code) {
            addProperty("code",code);
        }

        void setMessage(const char *msg,...) {
            va_list args;
            va_start(args,msg);
            setMessage(msg,args);
        }

        void setMessage(const char * msg, va_list args) {
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
    private:
        
};
};
#endif