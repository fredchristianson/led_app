#ifndef PARSE_GEN_H
#define PARSE_GEN_H

#include "./logger.h";
#include "./buffer.h";
#include "./util.h";


namespace DevRelief {

char skipBuf[100];
char intValBuff[32];

class JsonBase;


class MemLogger {
    public:
    MemLogger() {
        m_logger = new Logger("Memory",WARN_LEVEL);

    }

    void construct(const char * type, JsonBase* object);

    void destruct(const char * type, JsonBase* object);

    void construct(const char * type, const char * name, JsonBase* object);
    

    void destruct(const char * type, const char * name,JsonBase* object) ;

    void allocString(const char * type, size_t len, JsonBase* object);

    void freeString(const char * type,JsonBase* object);

    private:
    Logger* m_logger;

};

MemLogger mem;

class PGLogger: public Logger {
    public:
    PGLogger(const char * module, int level) : Logger(module,level) {
        strcpy(skipBuf,"Logger ");
        strcat(skipBuf,module);
    }
};

Logger JSONLogger("Json",WARN_LEVEL);
Logger ParserLogger("JsonParser",WARN_LEVEL);
Logger GeneratorLogger("JsonGenerator",WARN_LEVEL);


typedef enum TokenType {
    TOK_EOD=1000,
    TOK_ERROR=1001,
    TOK_START=1002,
    TOK_NULL=1003,
    TOK_OBJECT_START=1,
    TOK_OBJECT_END=2,
    TOK_ARRAY_START=3,
    TOK_ARRAY_END=4,
    TOK_STRING=5,
    TOK_INT=6,
    TOK_FLOAT=7,
    TOK_COLON=8,
    TOK_COMMA=9,
    TOK_TRUE,
    TOK_FALSE
};

typedef enum JsonType {
    JSON_UNKNOWN=999,
    JSON_NULL=998,
    JSON_INTEGER=100,
    JSON_FLOAT=101,
    JSON_STRING=102,
    JSON_BOOLEAN=103,
    JSON_VARIABLE=104,
    JSON_OBJECT=5,
    JSON_ARRAY=6,
    JSON_ROOT=7,
    JSON_PROPERTY=8,
    JSON_ARRAY_ITEM=9
};

class JsonRoot;
class JsonElement;
class JsonArray;
class JsonObject;

class JsonBase {
    public:
        JsonBase() {
            mem.construct("JsonBase",this);
            m_jsonId = -1;
        }
        virtual ~JsonBase() {
            mem.destruct("JsonBase",this);
        }
        virtual JsonRoot* getRoot() = 0;
        virtual bool add(JsonElement* child)=0;
        virtual JsonType getType()=0;
        virtual Logger* getLogger() =0;

        virtual bool isArray() { return false;}
        virtual bool isObject() { return false;}
        
        virtual JsonArray* asArray() { return NULL;}
        virtual JsonObject* asObject() { return NULL;}

        virtual bool getIntValue(int& value,int defaultValue) {
            value = defaultValue;
            return false;
        }

        virtual bool getFloatValue(double & value, double defaultValue) {
            value = defaultValue;
            return false;
        }

        virtual bool getBoolValue(bool & value, bool defaultValue) {
            value = defaultValue;
            return false;
        }

        virtual bool getDRStringValue(DRString& val) {
            val = "";
            return false;
        }

        virtual bool getStringValue(char * value, size_t maxLen,const char* defaultValue=NULL) {
            if (defaultValue != NULL) {
                strncpy(value,defaultValue,maxLen);
            } else {
                value[0] = 0;
            }
            return false;
        }

        int getJsonId() { return m_jsonId;}
        void setJsonId(int id) { m_jsonId = id;}
        DRString toJsonString();
    private:
        int m_jsonId;
};

class JsonObject;
class JsonArray;

class JsonRoot : public JsonBase {
    public:
        JsonRoot() : JsonBase() {
            m_value = NULL;
            m_logger = &JSONLogger;
            mem.construct("JsonRoot",this);
            m_nextJsonId = 1;
            setJsonId(this);
        }

        void setJsonId(JsonBase* element) {
            element->setJsonId(m_nextJsonId++);
        }

        virtual ~JsonRoot();
        virtual JsonType getType() { return JSON_ROOT;}
        char * allocString(const char * val, size_t len) {
            if (len == 0) {
                return NULL;
            }
            char * str = (char*)malloc(len+1);
            strncpy(str,val,len+1);
            str[len] = 0;
            mem.allocString(str,len,this);
        
        
            return str;
        }

        void freeString(const char * val) {
            mem.freeString(val,this);
            free((void*)val);
        }

        virtual JsonRoot* getRoot() { return this;}

        virtual bool add(JsonElement* child) { 
            if (m_value != NULL) { 
                m_logger->error("adding multiple children to JsonRoot is not allowed");
                return false;
            }
            m_value = child;
            return true;
        }

        void forget(JsonElement* child) {
            if (child != m_value) {
                m_logger->error("forget() called with wrong child");
            }
            m_value = NULL;
        }

        JsonObject* createObject();
        JsonArray* createArray();

        JsonElement* getTopElement(){
            return m_value;
        }
        
        void setTopElement(JsonElement*top){
            m_value = top;
        }

        virtual Logger* getLogger() { return m_logger;}
    protected:
        int m_nextJsonId;
        JsonElement * m_value;
        Logger* m_logger;
};

class JsonElement : public JsonBase {
    public:
        JsonElement(JsonRoot& root,JsonType t) :m_root(root) {
            m_type = t;
            root.setJsonId(this);
            m_logger = m_root.getLogger();
            mem.construct("JsonElement",this);

        }
        virtual ~JsonElement() {
            mem.destruct("JsonElement",this);
         }

        virtual JsonRoot* getRoot() { return & m_root;}
        virtual Logger* getLogger() { return m_root.getLogger();}
        virtual bool add(JsonElement* child) { 
            return false;
        }

        virtual JsonType getType() { return m_type;}

        JsonObject* createObjectElement();
        JsonArray* createArrayElement();

    protected:
        JsonRoot& m_root;
        Logger * m_logger;
        JsonType m_type;
};



class JsonProperty : public JsonElement {
    public:
        JsonProperty(JsonRoot& root, const char * name,size_t nameLength,JsonElement* value) : JsonElement(root,JSON_PROPERTY) {
            m_name = root.allocString(name,nameLength);
            m_value = value;
            m_next = NULL;
            m_logger->info("JsonProperty for type %d",m_value->getType());
            mem.construct("JsonProperty",name,this);
        }
        
        JsonProperty(JsonRoot& root, const char * name,JsonElement* value) : JsonElement(root,JSON_PROPERTY) {
            m_logger->info("construct property %s",name);
            m_logger->info("\tvalue type %d",(value == NULL ? JSON_NULL : value->getType()));
            size_t nameLength = strlen(name)+1;
            m_name = root.allocString(name,nameLength);
            m_value = value;
            m_next = NULL;
            m_logger->info("\tconstrcted JsonProperty %s for type %d",m_name,(m_value == NULL ? JSON_NULL :m_value->getType()));
            mem.construct("JsonProperty",name,this);
        }
        virtual ~JsonProperty() { 
            delete m_next;
            m_root.freeString(m_name); 
            delete m_value;
            mem.destruct("JsonProperty",m_name,this);
        }

        JsonProperty* getNext() { return m_next;}
        JsonElement * getValue() { return m_value;}
        const char * getName() { return m_name;}
        
        int getCount() { return m_next == NULL ? 1 : 1+m_next->getCount();}
        JsonElement* getAt(size_t idx) {
            return idx==0 ? m_value : m_next == NULL ? NULL :m_next->getAt(idx-1);
        }

        void setNext(JsonProperty*  next) {
            m_logger->info("setNext property [%d]-->[%d]",getJsonId(),(next == NULL ? -1 : next->getJsonId()));
            if (m_next != NULL) {
                delete m_next;
            }
            m_next = next;
        }

        void addEnd(JsonProperty* next) {
            if (m_next == NULL) {
                m_logger->info("addEnd last [%d]-->[%d]",getJsonId(),(next == NULL ? -1 : next->getJsonId()));
                m_next = next;
            } else {
                m_logger->info("addEnd middle [%d]-->[%d]",getJsonId(),(next == NULL ? -1 : next->getJsonId()));
                m_next->addEnd(next);
            }
        }

        void setValue(JsonElement*val) {
            delete m_value;
            m_value = val;
        }

        // JsonProperty is reponsible for deleting the value.
        // use this to keep the value after the property is deleted
        void forgetValue() {
            m_value = NULL;
        }

        bool get(bool defaultValue);
        int get(int defaultValue);
        DRString get(const char *defaultValue);
        double get(double defaultValue);


    private:
        char* m_name;
        JsonElement* m_value;
        JsonProperty* m_next;
};

class JsonObject : public JsonElement {
    public:
        JsonObject(JsonRoot& root) : JsonElement(root,JSON_OBJECT) {
            m_logger->debug("create JsonObject ");
            m_firstProperty = NULL;
            mem.construct("JsonObject",this);
            set("jsonId",getJsonId());
        }
        virtual ~JsonObject() {
            delete m_firstProperty;
             mem.destruct("JsonObject",this);
       }
        
        virtual bool isObject() { return true;}
        virtual JsonObject* asObject() { return this;}

        virtual bool add(JsonElement* child) { 
            return false;
        }

        JsonProperty* add(JsonProperty* prop){
            if (m_firstProperty == NULL) {
                m_logger->info("set firstProperty of [%d]",getJsonId());
                m_firstProperty = prop;
            } else {
                m_logger->info("add property  [%d] (count=%d)",getJsonId(),getCount());
                JsonProperty * end = m_firstProperty;
                while(end->getNext() != NULL) {
                    end = end->getNext();
                }
                end->setNext(prop);
                m_logger->info("\tadded property  [%d] (count=%d)",getJsonId(),getCount());
                for(JsonProperty*p=m_firstProperty;p!=NULL;p=p->getNext()){
                    m_logger->info("\t\tprop [%d] %d",p->getJsonId(),p->getType());
                }                
            }
            return prop;
        }

        JsonProperty* set(const char *nameStart,size_t nameLen,JsonElement * value){
            JsonProperty* prop = new JsonProperty(m_root,nameStart,nameLen,value);
            add(prop);
            return prop;
        }

        
        JsonProperty* set(const char *name,JsonElement * value){
            m_logger->info("add property [%d] %s",getJsonId(),name,(value == NULL ? -1 : value->getType()));
            JsonProperty*prop = getProperty(name);
            if (prop != NULL) {
                m_logger->info("\tproperty exists.  replacing value for %s",name);
                prop->setValue(value);
            } else {
                m_logger->info("\tcreate new property %s",name);

                prop = new JsonProperty(m_root,name,value);
                add(prop);
                m_logger->info("\t\tcreated property %s",name);
              
            }
            return prop;
        }

        JsonProperty* set(const char *name,bool value);
        JsonProperty* set(const char *name,int value);
        JsonProperty* set(const char *name,const char *value);
        JsonProperty* set(const char *name,double value);

        bool get(const char *name,bool defaultValue);
        int get(const char *name,int defaultValue);
        DRString get(const char *name,const char *defaultValue);
        double get(const char *name,double defaultValue);
        JsonArray* getArray(const char * name);
        JsonObject* getChild(const char * name);

        JsonProperty * getProperty(const char * name) {
            m_logger->debug("get property %s",name);
            for(JsonProperty*prop=m_firstProperty;prop!=NULL;prop=prop->getNext()){
                m_logger->debug("\tcheck%s",prop->getName());
                if (strcmp(prop->getName(),name)==0) {
                    return prop;
                }
            }
            return NULL;
        }
        JsonProperty* getFirstProperty() { return m_firstProperty;}

        JsonElement * getPropertyValue(const char * name) {
            JsonProperty*e = m_firstProperty;
            while(e != NULL && strcmp(e->getName(),name) != 0) {
                e = e->getNext();
            }
            return e == NULL ? NULL : e->getValue();
        }

        int getCount() { return m_firstProperty == NULL ? 0 : m_firstProperty->getCount();}
        JsonElement* getAt(size_t idx) {
            return m_firstProperty == NULL ? NULL : m_firstProperty->getAt(idx);
        }
    protected:
        JsonProperty* m_firstProperty;
};



class JsonArrayItem : public JsonElement {
    public:
        JsonArrayItem(JsonRoot& root, JsonElement * value) :JsonElement(root,JSON_ARRAY_ITEM) {
            m_logger->debug("create JsonArray item for type %d",value->getType());

            m_value = value;
            m_next = NULL;
            mem.construct("JsonArrayItem",this);

        }

        virtual ~JsonArrayItem() {
            delete m_value;
            delete m_next;
            mem.destruct("JsonArrayItem",this);
        }


        int getCount() { return m_next == NULL ? 1 : 1+m_next->getCount();}
        
        JsonElement* getAt(size_t idx) {
            return idx==0 ? m_value : m_next == NULL ? NULL :m_next->getAt(idx-1);
        }

        JsonArrayItem* getNext() { return m_next;}
        JsonElement* getValue() { return m_value;}

        void add(JsonArrayItem *last) {
            if (m_next != NULL) {
                m_next->add(last);
            } else {
                m_next = last;
            }
        }


    protected:
        JsonElement* m_value;
        JsonArrayItem* m_next;
};

class JsonArray : public JsonElement {
    public:
        JsonArray(JsonRoot& root) : JsonElement(root,JSON_ARRAY) {
            m_logger->debug("create JsonArray ");
            m_firstItem = NULL;
            mem.construct("JsonArray",this);
        
        }
        virtual ~JsonArray() {
            delete m_firstItem;
            mem.destruct("JsonArray",this);
        
        }

        virtual bool isArray() { return true;}
        virtual JsonArray* asArray() { return this;}

        JsonArrayItem* addItem(JsonElement * value){
            JsonArrayItem* item = new JsonArrayItem(m_root,value);
            if (m_firstItem == NULL) {
                m_firstItem = item;
            } else {
                m_firstItem->add(item);
            }
            return item;
        }

        JsonArrayItem* add(const char * val);
        JsonArrayItem* add(int val);
        JsonArrayItem* add(double val);
        JsonArrayItem* add(bool val);

        int getCount() { return m_firstItem == NULL ? 0 : m_firstItem->getCount();}
        JsonElement* getAt(size_t idx) {
            return m_firstItem == NULL ? NULL : m_firstItem->getAt(idx);
        }
        JsonArrayItem* getFirstItem() {
            return m_firstItem;
        }

        void each(auto&& lambda) const {
            JsonArrayItem*item = m_firstItem;
            while(item != NULL) {
                JsonElement* value = item->getValue();
                lambda(value);
                item = item->getNext();
            }
        }
    protected: 
        JsonArrayItem * m_firstItem;
};

class JsonValue : public JsonElement {
    public:
        JsonValue(JsonRoot& root, JsonType type) : JsonElement(root,type) {
            mem.construct("JsonValue",this);
        
        }
        virtual ~JsonValue() {
            mem.destruct("JsonValue",this);
        
        }
};

class JsonString : public JsonValue {
    public:
        JsonString(JsonRoot& root, const char * value) : JsonValue(root,JSON_STRING) {
            size_t len = value == NULL ? 0 : strlen(value);
            m_value = root.allocString(value,len);
            m_logger->debug("create JsonString ~%s~ ",m_value ? m_value : "NULL");
            mem.construct("JsonString",this);
        }
        JsonString(JsonRoot& root, const char * value, size_t len) : JsonValue(root,JSON_STRING) {
            m_value = root.allocString(value,len);
            m_logger->debug("create JsonString ~%s~ ",m_value);
            mem.construct("JsonString",this);
        }

        virtual ~JsonString() { 
            getRoot()->freeString(m_value);
            mem.destruct("JsonString",this);
        }

        virtual bool  getDRStringValue(DRString& val) {
            val = getText();
            return val;
        }
        
        virtual bool getStringValue(char * value, size_t maxLen,const char* defaultValue=NULL) {
            strncpy(value,m_value,maxLen);
            return true;
        }

        const char * getText() { return m_value;}
    protected:
        char* m_value;
};


class JsonInt : public JsonValue {
    public:
        JsonInt(JsonRoot& root, int value) : JsonValue(root,JSON_INTEGER) {
            m_logger->debug("create JsonInt %d ",value);

            m_value = value;
            mem.construct("JsonInt",this);
        
        }

        virtual ~JsonInt() { 
            mem.destruct("JsonInt",this);
        }

        virtual bool getIntValue(int& value,int defaultValue) {
            value = m_value;
            return true;
        }
    protected:
        int m_value;
};


class JsonFloat : public  JsonValue {
    public:
        JsonFloat(JsonRoot& root, double value) : JsonValue(root,JSON_FLOAT) {
            m_logger->debug("create JsonFloat %f ",value);

            m_value = value;
            mem.construct("JsonFloat",this);
        
        }

        virtual ~JsonFloat() { 
            mem.destruct("JsonFloat",this);
        }

        virtual bool getFloatValue(double& value,double defaultValue) {
            value = m_value;
            return true;
        }
    protected:
        double m_value;
};

class JsonBool : public  JsonValue {
    public:
        JsonBool(JsonRoot& root, bool value) : JsonValue(root,JSON_BOOLEAN) {
            m_logger->debug("create JsonFloat %s ",value?"true":"false");

            m_value = value;
            mem.construct("JsonBool",this);
        
        }

        virtual ~JsonBool() { 
            mem.destruct("JsonBool",this);
        }

        virtual bool getBoolValue(bool& value,bool defaultValue) {
            value = m_value;
            return true;
        }
    protected:
        bool m_value;
};

class JsonVariable : public JsonValue {
    public:
        JsonVariable(JsonRoot& root, const char * value, size_t len):  JsonValue(root,JSON_VARIABLE) {
            m_value = root.allocString(value,len);
            m_logger->debug("create JsonVariable %s ",m_value);
            mem.construct("JsonVariable",this);
        

        }

        virtual ~JsonVariable() { delete m_value;
            mem.destruct("JsonVariable",this);
         }

         const char* getText() { return m_value;}
    protected:
        char * m_value;
};

class JsonNull : public JsonElement {
    public:
        JsonNull(JsonRoot& root) : JsonElement(root,JSON_NULL){

        }
};

class ParseGen {

};

class JsonGenerator : public ParseGen {
public:
    JsonGenerator(DRString& buffer) : m_buf(buffer) {
        m_buf = buffer;
        m_depth = 0;
        m_pos = 0;
        m_logger = &GeneratorLogger;
    }

    ~JsonGenerator() {
        
    }

    bool generate(JsonBase* element) {
        m_logger->debug("Generate JSON %d",element->getType());
        m_pos = 0;
        m_depth = 0; 
        m_logger->debug("clear buffer");
        
        m_buf.clear();
        m_logger->debug("\tcleared buffer");
        if (element == NULL) {
            m_logger->error("\telement is NULL");
            return false;
        }
        if (element->getType() == JSON_ROOT){
            m_logger->debug("write top element");
            writeElement(((JsonRoot*)element)->getTopElement());
        } else {
            m_logger->debug("write self");
            writeElement((JsonElement*)element);
        }
        return true;
    }

    void writeElement(JsonElement* element){
        if (element == NULL) {
            writeText("null");
            writeNewline();
            return;
        }
        int type = element->getType();
        m_logger->debug("write element type %d",type);
        switch(type) {
            case JSON_UNKNOWN:
                writeText("unknown element type");
                writeInt(type);
                writeNewline();
                break;
            case JSON_NULL:
                writeText("null");
                writeInt(type);
                writeNewline();
                break;
            case JSON_INTEGER:
                writeInteger((JsonInt*)element);
                break;
            case JSON_FLOAT:
                writeFloat((JsonFloat*)element);
                break;
            case JSON_STRING:
                writeString((JsonString*)element);
                break;
            case JSON_BOOLEAN:
                writeBool((JsonBool*)element);
                break;
            case JSON_VARIABLE:
                writeVariable((JsonVariable*)element);
                break;
            case JSON_OBJECT:
                writeObject((JsonObject*)element);
                break;
            case JSON_ARRAY:
                writeArray((JsonArray*)element);
                break;
            case JSON_ROOT:
                writeElement(((JsonRoot*)element)->getTopElement());
                break;
            case JSON_PROPERTY:
                writeProperty((JsonProperty*)element);
                break;
            case JSON_ARRAY_ITEM:
                writeArrayItem((JsonArrayItem*)element);
                break;
            default:
                writeText("unknown type: ");
                writeInt(type);
                writeNewline();
                break;
        }
    }

    void writeObject(JsonObject * object) {
        writeText("{");
        m_depth += 1;
        m_logger->info("write object [%d] count=%d",object->getJsonId(),object->getCount());
        for(JsonProperty*prop=object->getFirstProperty();prop!=NULL;prop=prop->getNext()){
            if (strcmp(prop->getName(),"jsonId")!=0) {
                m_logger->info("\tprop %s [%d]",(prop == NULL ? "no prop":prop->getName()),prop->getJsonId());
                writeNewline();

                writeProperty(prop);

                if (prop->getNext() != NULL) {
                    writeText(",");
                }
            }
        }
        m_logger->debug("\twrote properties");
        m_depth -= 1;
        writeNewline();
        writeText("}");
    }

    void writeProperty(JsonProperty * prop) {
        if (prop == NULL) {
            m_logger->error("writeProperty got null");
        }
        m_logger->debug("write property %s",prop->getName());
        writeString(prop->getName());
        m_logger->debug("\twrote name %s",prop->getName());
        
        writeText(": ");
        m_logger->debug("\twrote colon");
        JsonElement* val = prop->getValue();
        m_logger->debug("\twrite element %d",val);
        writeElement(val);
        m_logger->debug("\twrote value");
    }

    void writeArray(JsonArray* array) {
        writeText("[");
        m_depth += 1;
        for(JsonArrayItem* item=array->getFirstItem();item!= NULL;item=item->getNext()){
            writeNewline();
            writeArrayItem(item);
            if (item->getNext() != NULL) {
                writeText(",");
            }
        }
        m_depth -= 1;
        writeNewline();
        writeText("]");
    }

    void writeArrayItem(JsonArrayItem* item) {
        writeElement(item->getValue());
    }
  
    void writeInteger(JsonInt* element) {
        int i;
        if (element->getIntValue(i,0)) {
            writeInt(i);
        } else {
            writeText("null");
        }
    }
    
    void writeFloat(JsonFloat* element) {
        double f;
        if (element->getFloatValue(f,0)) {
            writeFloat(f);
        } else {
            writeText("null");
        }
    }
    
   
        
    void writeBool(JsonBool* element) {
        bool b;
        if (element->getBoolValue(b,false)) {
            writeText(b ? "true":"false");
        } else {
            writeText("null");
        }
    }
    
    void writeString(JsonString* element) {
        const char * txt = element->getText();
        if (txt == NULL) {        
            writeText("null");
            return;
        }
        writeText("\"");
        const char * end = strchr(txt,'\"');
        while(end != NULL) {
            size_t len = end-txt+1;
            char* pos = m_buf.increaseLength(len+1);
            memcpy(pos,txt,len);
            pos[len] = '\\';
            pos[len+1] = '\"';
            txt = end+1;
            end = strchr(txt,'\"');
        }
        writeText(txt);
        writeText("\"");
    }
    
        
    void writeVariable(JsonVariable* element) {
        writeText(element->getText());
    }
    
    void writeText(const char * text) {
        if (text == NULL) {
            return;
        }
        size_t len = strlen(text);
        if (len == 0) {
            return;
        }
        m_logger->debug("writeText %d %d ~%.15s~",len,m_buf.getLength(),text);
        char *pos = m_buf.increaseLength(len);
        memcpy(pos,text,len);
        pos[len] = 0;
        m_logger->debug("\tJSON: %d %.300s", m_buf.getLength(),m_buf.text());
    }

    void writeString(const char * text) {
        writeText("\"");
        writeText(text);
        writeText("\"");
    }

    void writeInt(int i) {
        snprintf(m_tmp,32,"%d",i);
        writeText(m_tmp);
    }

    void writeFloat(double f) {
        snprintf(m_tmp,32,"%f",f);
        writeText(m_tmp);
    }

    void writeNewline() {
        writeText("\n");
        writeTabs();
    }

    void writeTabs(){
        int cnt = m_depth;
        while(cnt-- > 0) {
            writeText("\t");
        }
    }

protected:
    char m_tmp[32];
    DRString& m_buf;    
    int m_depth;
    size_t m_pos;
    Logger * m_logger;
};



class TokenParser {
    public:
        TokenParser(const char  * data) {
            m_data = data;
            m_pos = data;
            m_errorMessage = NULL;
            m_token = TOK_START;
        }

        TokenType peek(){
            const char* pos = m_pos;            
            const char * tpos = m_tokPos;
            TokenType old = m_token;
            TokenType type = next();
            m_pos = pos;
            m_tokPos = tpos;
            m_token = old;
            return type;
        }

        TokenType next() {
            TokenType t = TOK_ERROR;
            const char * pos = m_pos;

            if (!skipWhite()) {
                t = TOK_EOD;
                return m_token;
            }
            char c = m_pos[0];
            m_tokPos = m_pos;
            m_pos+= 1;
            if (c == '['){
                m_token = TOK_ARRAY_START;
            } else if (c == ']'){
                m_token = TOK_ARRAY_END;
            } else if (c == '{'){
                m_token = TOK_OBJECT_START;
            } else if (c == '}'){
                m_token = TOK_OBJECT_END;
            }  else if (c == '"'){
                m_token = TOK_STRING;
            }  else if (isdigit(c)){
                const char * n = m_tokPos;
                while(*n !=0 && isdigit(*n)){
                    n++;
                }
                m_token = *n == '.' ? TOK_FLOAT : TOK_INT;
            }  else if (c == ':'){
                m_token = TOK_COLON;
            }  else if (c == ','){
                m_token = TOK_COMMA;
            }  else if (c == 'n' && strncmp(m_pos,"ull",3)==0){
                m_token = TOK_NULL;
                m_pos+=3;
            }  else if (c == 't' && strncmp(m_pos,"rue",3)==0){
                m_token = TOK_TRUE;
                m_pos+=3;
            }  else if (c == 'f' && strncmp(m_pos,"alse",4)==0){
                m_token = TOK_FALSE;
                m_pos+=4;
            }  else {

                ParserLogger.error("token error");
                m_token = TOK_ERROR;
            }
            return m_token;
        }

        bool skipWhite() {
            while(strchr(" \t\n\r",m_pos[0]) != NULL) {
                m_pos++;
            }
            if (m_pos[0] == 0) {
                m_token = TOK_EOD;
                return false;
            }
            return true;
        }

        bool nextInt(int& val) {
            val = 0;
            if (next() == TOK_INT) {
                val = atoi(m_tokPos);
                skipNumber();
                return true;
            }
            return false;
        }

        bool nextFloat(double& val) {
            val = 0;
            TokenType n = next();
            if (n == TOK_INT || n == TOK_FLOAT) {
                val = atof(m_tokPos);
                skipNumber();
                return true;
            }
            return false;
        }

        bool skipNumber() {

            while(*m_pos == '.' || isdigit(*m_pos)){
                m_pos++;
            }
            return true;
        }

        bool nextString(const char *& start, size_t& len) {

            skipWhite();
            start = NULL;
            len = 0;
            if (m_pos[0] != '"'){
                return false;
            }
            m_tokPos = m_pos;
            m_token = TOK_STRING;
            m_pos += 1;
            start = m_pos;
            while(m_pos[len] != '"' && m_pos[len] != 0){
                if (m_pos[len] == '\\'){
                    len += 2;
                } else {
                    len += 1;
                }
            }
            m_pos += len;
            if (m_pos[0] == '"') {
                m_pos+= 1;
                return true;
            }
            return false;
        }

        const char * getPos() { return m_pos;}
        const char * getTokPos() { return m_tokPos;}

        int getCurrentPos(){
            int p = m_pos - m_data;
            return p;
        }
        int getCharacterCount(){
            if (m_pos[0] == 0) {
                return m_pos-m_data;
            }
            int len = strlen(m_pos);
            return m_pos-m_data+len;
        }
        int getCurrentLine(){
            char * nl = strchr(m_data,'\n');
            int p = 1;
            while(nl > 0 && nl <= m_pos)  {
                nl = strchr(nl+1,'\n');
                p++;
            }
            return p;
        }

        int getLineCount(){
            const char * nl = strchr(m_data,'\n');
            int p = 1;
            while(nl > 0)  {
                nl = strchr(nl+1,'\n');
                p++;
            }
            return p;
        }

        int getLinePos(){
            const char * pos = m_pos;
            while(pos > m_data && pos[0] != '\n'){
                pos--;
            }
            return m_pos-pos;
        }
        int getLineCharacterCount(){
            const char * pos = m_pos;
            while(pos[0] != 0 && pos[0] != '\n'){
                pos++;
            }
            return getLinePos()+pos- m_pos;
        }

        DRString getCurrentLineText(){
            const char * start = m_pos;
            while(start > m_data && start[-1] != '\n'){
                start--;
            }
            const char * end = m_pos;
            while(end[0] != 0 && end[0] != '\n'){
                end++;
            }
            return DRString(start,(size_t)(end-start));
        }
    private: 
        const char * m_data;
        const char * m_pos;
        const char * m_tokPos;
        TokenType           m_token;
        const char *    m_errorMessage;
};

class JsonParser : public ParseGen {
public:
    JsonParser() {
        m_logger = &ParserLogger;
        m_errorMessage = NULL;
        m_hasError = false;
        m_root = NULL;
    }    

    ~JsonParser() { 
    }


    JsonRoot* read(const char * data) {
        m_logger->debug("parsing %s",data);
        
        m_errorMessage = NULL;
        m_hasError = false;
        JsonRoot * root = new JsonRoot();
        m_root = root;
        m_logger->debug("created root");

        if (data == NULL) {
            return root;
        }
        
        TokenParser tokParser(data);
        m_logger->debug("created TokenParser");
        JsonElement * json = parseNext(tokParser);
        if (json != NULL) {
            m_logger->debug("got top");
            root->setTopElement(json);
        } else {
            m_logger->debug("no top element found");

            m_logger->debug("\tpos %d/%d.  line%d/%d.  char %d/%d",
                tokParser.getCurrentPos(),
                tokParser.getCharacterCount(),
                tokParser.getCurrentLine(),
                tokParser.getLineCount(),
                tokParser.getLinePos(),
                tokParser.getLineCharacterCount()
                );
            
        }
        
        m_logger->debug("parse complete");
        m_root = NULL;
        return root;
    }

    JsonElement* parseNext(TokenParser&tok) {
        TokenType next = tok.peek();
        m_logger->debug("next token %d",next);

        JsonElement* elem = NULL;
        if (next == TOK_OBJECT_START) {
            elem = parseObject(tok);
        } else if (next == TOK_ARRAY_START) {
            elem = parseArray(tok);
        } else if (next == TOK_STRING) {
            elem = parseString(tok);
        }  else if (next == TOK_INT) {
            elem = parseInt(tok);
        }  else if (next == TOK_FLOAT) {
            elem = parseFloat(tok);
        } else if (next == TOK_NULL) {
            elem = new JsonNull(*m_root);
            skipToken(tok,TOK_NULL);
        } else if (next == TOK_TRUE) {
            elem = new JsonBool(*m_root,true);
            skipToken(tok,TOK_TRUE);
        } else if (next == TOK_FALSE) {
            elem = new JsonBool(*m_root,false);
            skipToken(tok,TOK_FALSE);
        }
        if (elem == NULL) {
            DRString errLine = tok.getCurrentLineText();
            m_logger->error("Parse error:");
            m_logger->error(errLine.get());
            DRString pos('-',tok.getLinePos());
            pos += "^";
            m_logger->error(pos.get());
        }
        return elem;
    }

    bool skipToken(TokenParser&tok, TokenType type) {
        TokenType next = tok.next();
        if (next != type) {
            m_hasError = true;
            m_logger->error("expected token type %d but got %d",(int)type,(int)next);
            m_logger->error("\ttoken pos %.20s",tok.getTokPos());
            m_logger->error("\ttokenparser pos %.20s",tok.getPos());
            return false;
        }
        return true;
    }

    
    bool skipOptional(TokenParser&tok, TokenType type) {
        TokenType next = tok.peek();
        if (next == type) {
            tok.next();
            return true;
        }
        return false;
    }

    JsonValue* parseString(TokenParser& tok) {
        const char * nameStart;
        size_t nameLen;
        if(tok.nextString(nameStart,nameLen)){
            if (nameLen>4 && strncmp(nameStart,"var(",4)==0){
                return new JsonVariable(*m_root,nameStart,nameLen);
            } else {
                return new JsonString(*m_root,nameStart,nameLen);
            }
        }
        return NULL;
    }
    JsonValue* parseInt(TokenParser& tok) {
        int val=0;
        if(tok.nextInt(val)){
            return new JsonInt(*m_root,val);
        }
        return NULL;
    }

    JsonValue* parseFloat(TokenParser& tok) {
        double val=0;
        if(tok.nextFloat(val)){
            return new JsonFloat(*m_root,val);
        }
        return NULL;
    }

    JsonObject* parseObject(TokenParser& tok) {
        m_logger->debug("parseObject");
        if (!skipToken(tok,TOK_OBJECT_START)) {
            m_logger->debug("\t{ not found");
            return NULL;
        }
        JsonObject* obj = new JsonObject(*m_root);
        const char * nameStart;
        size_t nameLen;
        m_logger->debug("\tread string");

        while(tok.nextString(nameStart,nameLen)){
            m_logger->debug("\tgot string");
            m_logger->debug("\t\t len=%d",nameLen);
            if (nameStart != NULL) {
                m_logger->debug("\t\tchar %c",nameStart[0]);
            } else {
                m_logger->debug("\t\t nameStart is null");
            }
            m_logger->debug("\ttokenparser pos ~%.20s",tok.getPos());

            if (!skipToken(tok,TOK_COLON)) {
                delete obj;
                return NULL;
            }
            m_logger->debug("got colon");
            JsonElement* val = parseNext(tok);
            if (val == NULL) {
                delete obj;
                return NULL;
            }
            m_logger->debug("got val");
            obj->set(nameStart,nameLen,val);
            skipOptional(tok,TOK_COMMA);
        }


        if (!skipToken(tok,TOK_OBJECT_END)) {
            delete obj;
            return NULL;
        }
        return obj;
    }

    JsonArray* parseArray(TokenParser& tok) {
        if (!skipToken(tok,TOK_ARRAY_START)) {
            return NULL;
        }
        JsonArray* arr = new JsonArray(*m_root);
        TokenType peek = tok.peek();
        if (peek == TOK_ARRAY_END) {
            // empty array
            m_logger->info("empty array");
            tok.next();
            return arr;
        }
        JsonElement * next = parseNext(tok);
        while(next != NULL){
            arr->addItem(next);
            skipOptional(tok,TOK_COMMA);
            peek = tok.peek();
            if (peek == TOK_ARRAY_END) {
                next = NULL;
            } else {
                next = parseNext(tok);
            }
        }
        if (!skipToken(tok,TOK_ARRAY_END)) {
            m_logger->error("Array end ] not found");
            delete arr;
            return NULL;
        }
        return arr;
    }

    bool hasError() { return m_hasError;}
    const char * errorMessage() { return m_errorMessage;}
    private:
        Logger * m_logger;
        const char * m_errorMessage;
        bool        m_hasError;
        JsonRoot* m_root;

};

JsonObject* JsonRoot::createObject(){
    if (m_value != NULL) {
        m_logger->error("creating JsonObject in root but top element exists");
        delete m_value;
    }
    m_value = new JsonObject(*this);
    return m_value->asObject();
}

JsonArray* JsonRoot::createArray(){
    if (m_value != NULL) {
        m_logger->error("creating JsonArray in root but top element exists");
        delete m_value;
    }
    m_value = new JsonObject(*this);
    return m_value->asArray();

}

JsonRoot::~JsonRoot() { 

    delete m_value; 
    mem.destruct("JsonRoot",this);
}

JsonObject* JsonElement::createObjectElement() {
    return new JsonObject(*getRoot());
}

JsonArray* JsonElement::createArrayElement() {
    return new JsonArray(*getRoot());
}

JsonProperty* JsonObject::set(const char *name,bool value){
    JsonBool* jval = new JsonBool(*getRoot(),value);
    return set(name,jval);

}

JsonProperty* JsonObject::set(const char *name,int value) {
    JsonInt* jval = new JsonInt(*getRoot(),value);
    return set(name,jval);
};

JsonProperty* JsonObject::set(const char *name,const char *value) {
    JsonString* jval = new JsonString(*getRoot(),value);
    return set(name,jval);
};

JsonProperty* JsonObject::set(const char *name,double value) {
    JsonFloat* jval = new JsonFloat(*getRoot(),value);
    return set(name,jval);
};

void MemLogger::construct(const char * type, JsonBase* object) {
    m_logger->info("++++construct: %s [%d]",type,object->getJsonId());
}

void MemLogger::destruct(const char * type, JsonBase* object) {
    m_logger->info("----destruct: %s [%d]",type,object->getJsonId());
}

void MemLogger::construct(const char * type, const char * name, JsonBase* object) {
    m_logger->info("++++construct: %s--%s",type,name);
}

void MemLogger::destruct(const char * type, const char * name,JsonBase* object) {
    m_logger->info("----destruct: %s--%s [%d]",type,name,object->getJsonId());
}

void MemLogger::allocString(const char * type, size_t len, JsonBase* object){
    m_logger->info("++++alloc string: %s, len=%d [%d]",type,len,object->getJsonId());
}

void MemLogger::freeString(const char * type,JsonBase* object){
    m_logger->info("----free string: %s [%d]",type,object->getJsonId());
}

JsonArrayItem* JsonArray::add(const char * val) {
    JsonString * value = new JsonString(*getRoot(),val);
    JsonArrayItem* item = new JsonArrayItem(m_root,value);
    addItem(item);
    return item;
}

JsonArrayItem* JsonArray::add(int val) {
    JsonInt * value = new JsonInt(*getRoot(),val);
    JsonArrayItem* item = new JsonArrayItem(m_root,value);
    addItem(item);
    return item;
}

JsonArrayItem* JsonArray::add(double val) {
    JsonFloat * value = new JsonFloat(*getRoot(),val);
    JsonArrayItem* item = new JsonArrayItem(m_root,value);
    addItem(item);
    return item;
}

JsonArrayItem* JsonArray::add(bool val) {
    JsonBool * value = new JsonBool(*getRoot(),val);
    JsonArrayItem* item = new JsonArrayItem(m_root,value);
    addItem(item);
    return item;
}

bool JsonObject::get(const char *name,bool defaultValue){
    m_logger->debug("get bool value for %s",name);

    bool val = defaultValue;
    JsonProperty*prop = getProperty(name);
    if (prop) {
        prop->get(defaultValue);
    }
    return val;
}
int JsonObject::get(const char *name,int defaultValue){
    m_logger->debug("get int value for %s",name);
    int val = defaultValue;
    JsonProperty*prop = getProperty(name);
    if (prop) {
        val = prop->get(defaultValue);
    } else {
        m_logger->debug("\tprop not found");
    }
    return val;
}

DRString JsonObject::get(const char *name,const char *defaultValue){
    m_logger->debug("get string value for %s",name);
    DRString val;
    JsonProperty*prop = getProperty(name);
    if (prop) {
        val = prop->get(defaultValue);
    } else {
        val = defaultValue;
    }
    return val;
}


double JsonObject::get(const char *name,double defaultValue){
    m_logger->debug("get float value for %s",name);

    double val = defaultValue;
    JsonProperty*prop = getProperty(name);
    if (prop) {
        val = prop->get(defaultValue);
    }
    return val;
}

JsonArray* JsonObject::getArray(const char * name){
    m_logger->debug("get array value for %s",name);

    JsonProperty * prop = getProperty(name);
    if (prop) {
        JsonElement* elem = prop->getValue();
        return elem ? elem->asArray() : NULL;
    }
    return NULL;
}

JsonObject* JsonObject::getChild(const char * name){
    JsonProperty * prop = getProperty(name);
    if (prop) {
        JsonElement* elem = prop->getValue();
        return elem ? elem->asObject() : NULL;
    }
    return NULL;
}

bool JsonProperty::get(bool defaultValue){
    bool val = defaultValue;
    if (m_value) {
        m_value->getBoolValue(val,defaultValue);
    }
    return val;
}

int JsonProperty::get(int defaultValue){
    int val = defaultValue;
    if (m_value) {
        m_value->getIntValue(val,defaultValue);
    }
    return val;
}

DRString JsonProperty::get(const char * defaultValue){
    DRString result;
    if (m_value) {
        if (!m_value->getDRStringValue(result)) {
            result = defaultValue;
        }
    } else {
        result = defaultValue;
    }
    return result;
}

double JsonProperty::get(double defaultValue){
    double val = defaultValue;
    if (m_value) {
        m_value->getFloatValue(val,defaultValue);
    }
    return val;
}

DRString JsonBase::toJsonString(){
    DRString dr;
    JsonGenerator gen(dr);
    gen.generate(this);
    return dr;
}

}
#endif
