#ifndef DR_DATA_H
#define DR_DATA_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"


namespace DevRelief {

Logger PathLogger("JsonPath",WARN_LEVEL);
Logger DataLogger("Data",WARN_LEVEL);
class JsonPath {
    public:
        JsonPath() {
           m_firstProperty = NULL;
           m_logger = &PathLogger;
        }
        JsonPath(JsonElement&top, const char * name,bool create) {
           m_logger = &PathLogger;
           m_firstProperty = NULL;
           m_logger->debug("parse path %s",name);
           parse(top,name,create);
        }
        ~JsonPath() {
            clear();
        }

        void clear() {
            m_logger->debug("clear JsonPath");
            JsonProperty *del = m_firstProperty;
            while(del != NULL) {
                del->forgetValue();
                del = del->getNext();
            }
            delete m_firstProperty;
            m_firstProperty = NULL;
        }

        JsonProperty* parse(JsonElement&top, const char * name,bool createPath){
            m_logger->debug("JsonPath::parse");
            clear();

            m_logger->debug("\tcreate first element");
            m_firstProperty = new JsonProperty(*(top.getRoot()),"/",&top);
            m_logger->debug("\tsplit name");
            
            const char ** parts = buffer.split(name,"/");
            if (parts == NULL || parts[0] == NULL) {
                m_logger->debug("\tno parts found");
                return NULL;
            }
            JsonObject* obj = top.asObject();
            JsonElement* child = NULL;
            JsonProperty* lastProp=m_firstProperty;
            for(int idx=0;idx<buffer.count();idx++){
                const char * next = buffer.getAt(idx);
                m_logger->debug("\tpart %s",next);
                child = obj == NULL ? NULL : obj->getPropertyValue(next);
                if (createPath && child == NULL) {
                    m_logger->debug("\tcreate child %s",next);
                    if ( idx+1<buffer.count()) {
                        // create JsonObject for every parent.  leave the last value null;
                        m_logger->debug("\tcreate JsonObject for child");
                        child = new JsonObject(*(top.getRoot()));
                    }
                    obj->add(next,child);
                }
                m_logger->debug("\tcreate prop");
                JsonProperty* prop = new JsonProperty(*(top.getRoot()),next,child);
                lastProp=prop;
                m_logger->debug("\taddEnd");
                m_firstProperty->addEnd(prop);
                m_logger->debug("\tadded");
                obj = child == NULL ? NULL : child->asObject();
            }
            return lastProp;
        }

        JsonObject* getLastParent() {
            m_logger->debug("getLastParent");
            JsonProperty* prop = m_firstProperty;
            JsonObject* parent = NULL;
            while(prop != NULL && prop->getNext() != NULL) {
                m_logger->debug("\tget next %s",prop->getName());
                parent = prop->getValue()->asObject();
                prop = prop->getNext();
            }
            if (parent == NULL) {
                m_logger->error("not found");
                return NULL;
            } 
            if (parent == NULL) {
                m_logger->debug("\tno parent");
                return NULL;
            }
            return parent;
        }

        JsonProperty* getFirstProperty() { return m_firstProperty;}
        JsonProperty* getLastProperty() { 
            JsonProperty * prop = m_firstProperty;
            while(prop != NULL && prop->getNext() != NULL) {
                prop = prop->getNext();
            }
            return prop;
        }
    private:
    DRStringBuffer buffer;
    JsonProperty* m_firstProperty;
    Logger * m_logger;
};


class Data : public JsonRoot {
public:
    Data() : m_obj(*this) {
        m_logger = &DataLogger;
        m_logger->debug("Data is JSON type %d -- %d",this->getType(),m_obj.getType());
        add(&m_obj);
    }
    virtual ~Data() {
        forget(&m_obj);
    }

    bool addProperty(const char * name,bool b) { 
        JsonPath path(m_obj,name,true);
        JsonObject* parent = path.getLastParent();
        if (parent != NULL) {
            m_logger->info("adding bool to parent");
            parent->add(name,b);
            return true;
        } else {
            m_logger->error("cannot find last parent %s",name);
            return false;
        }
    }
    bool addProperty(const char * name,int b) { 
        m_logger->debug("add int prop %s %d",name,b);
        dump();
        JsonPath path(m_obj,name,true);
        m_logger->debug("created JsonPath");
        JsonObject* parent = path.getLastParent();
        m_logger->debug("got last parent");
        if (parent != NULL) {
            m_logger->info("adding int to parent");
            JsonProperty* last = path.getLastProperty();
            if (last == NULL) {
                m_logger->error("last property not created for %s",name);
                return false;
            }
            const char * pname = last->getName();
            parent->add(pname,b);
            dump();
            return true;
        } else {
            m_logger->error("cannot find last parent %s",name);
            return false;
        }
    }
    /*
    bool addProperty(const char * name,double b) { 
        JsonObject* parent = getJsonParentByName(name, true);
        if (parent != NULL) {
            parent->add(name,b);
            return true;
        }
        return false;
    }
    bool addProperty(const char * name,const char * b) { 
        JsonObject* parent = getJsonParentByName(name, true);
        if (parent != NULL) {
            parent->add(name,b);
            return true;
        }
        return false;        
    }
*/

    int getInt(const char * name, int defaultValue=0) {
        m_logger->debug("get int prop %s %d",name);
        JsonPath path;
        JsonProperty* prop = path.parse(m_obj,name,false);
        int val=defaultValue;
        if (prop != NULL) {
            m_logger->debug("\tgot property %s",prop->getName());
            JsonElement*e = prop->getValue();
            if (e && e->getIntValue(val,defaultValue)){
                m_logger->debug("\tgot value %d",val);
            } else {
                m_logger->debug("\tfailed to get value");
                m_logger->debug("\telement type %d",(e == NULL ? -1 : e->getType()));
            }
            
        }

        return val;
    }
    
    void dump() {
        if (m_logger->getLevel() < DEBUG_LEVEL) {
            return;
        }
        DRBuffer buf;
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