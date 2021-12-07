#ifndef PARSE_GEN_H
#define PARSE_GEN_H

#include "./logger.h";
#include "./buffer.h";



namespace DevRelief {

char skipBuf[100];
char intValBuff[32];

class MemLogger {
    public:
    MemLogger() {
        m_logger = new Logger("Memory",ERROR_LEVEL);

    }

    void construct(const char * type, void * object) {
        m_logger->info("++++construct: %s",type);
    }

    void destruct(const char * type, void * object) {
        m_logger->info("----destruct: %s",type);
    }

    void construct(const char * type, const char * name, void * object) {
        m_logger->info("++++construct: %s--%s",type,name);
    }

    void destruct(const char * type, const char * name,void * object) {
        m_logger->info("----destruct: %s--%s",type,name);
    }

    void allocString(const char * type, size_t len, void * object){
        m_logger->info("++++alloc string: %s, len=%d",type,len);
    }

    void freeString(const char * type,void * object){
        m_logger->info("----free string: %s",type);
    }

    private:
    Logger* m_logger;

};

MemLogger mem;

class PGLogger: public Logger {
    public:
    PGLogger(const char * module, int level) : Logger(module,level) {
        strcpy(skipBuf,"Logger ");
        strcat(skipBuf,module);
        mem.construct(skipBuf,this);
    }
};

Logger* genLogger = new PGLogger("Gen",INFO_LEVEL);
Logger* parserLogger = new PGLogger("Parse",INFO_LEVEL);

typedef enum TokenType {
    TOK_EOD=1000,
    TOK_ERROR=1001,
    TOK_START=1002,
    TOK_OBJECT_START=1,
    TOK_OBJECT_END=2,
    TOK_ARRAY_START=3,
    TOK_ARRAY_END=4,
    TOK_STRING=5,
    TOK_INT=6,
    TOK_FLOAT=7,
    TOK_COLON=8,
    TOK_COMMA=9
};

typedef enum JsonType {
    JSON_UNKNOWN=999,
    JSON_INTEGER=100,
    JSON_FLOAT=101,
    JSON_STRING=102,
    JSON_BOOLEAN=103,
    JSON_VARIABLE=104,
    JSON_OBJECT=5,
    JSON_ARRAY=6,
    JSON_ROOT=7,
    JSON_PROPERTY=8,
    JSON_ARRAY_ITEM=9,
    JSON_BOOL
};

class JsonRoot;
class JsonElement;
class JsonArray;
class JsonObject;

class JsonBase {
    public:
        JsonBase() {
            mem.construct("JsonBase",this);
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

        virtual int getStringValue(char * value, size_t maxLen,const char* defaultValue=NULL) {
            if (defaultValue != NULL) {
                strncpy(value,defaultValue,maxLen);
            } else {
                value[0] = 0;
            }
            return false;
        }
};

class JsonRoot : public JsonBase {
    public:
        JsonRoot() : JsonBase() {
            m_value = NULL;
            m_logger = new PGLogger("JsonRoot",WARN_LEVEL);
            mem.construct("JsonRoot",this);
        }
        virtual ~JsonRoot();
        virtual JsonType getType() { return JSON_ROOT;}
        char * allocString(const char * val, size_t len) {
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

        JsonElement* getValue(){
            return m_value;
        }

        virtual Logger* getLogger() { return m_logger;}
    protected:
        JsonElement * m_value;
        Logger* m_logger;
};

class JsonElement : public JsonBase {
    public:
        JsonElement(JsonRoot& root,JsonType t) :m_root(root) {
            m_type = t;
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

        JsonRoot& m_root;
    protected:
        Logger * m_logger;
        JsonType m_type;
};



class JsonProperty : public JsonElement {
    public:
        JsonProperty(JsonRoot& root, const char * name,size_t nameLength,JsonElement* value, JsonProperty* next) : JsonElement(root,JSON_PROPERTY) {
            m_name = root.allocString(name,nameLength);
            m_value = value;
            m_next = next;
            m_logger->debug("JsonProperty for type %d",m_value->getType());
            mem.construct("JsonProperty",name,this);
        }
        
        JsonProperty(JsonRoot& root, const char * name,JsonElement* value, JsonProperty* next) : JsonElement(root,JSON_PROPERTY) {
            m_logger->always("construct property %d %s",name,name);
            size_t nameLength = strlen(name)+1;
            m_logger->always("\tallocate %d chars string %s ",nameLength,name);
            m_name = root.allocString(name,nameLength);
            m_logger->always("\talloced %s into %s",name,m_name);
            m_value = value;
            m_next = next;
            m_logger->always("\tconstrcted JsonProperty %s for type %d",m_name,m_value->getType());
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


        void setValue(JsonElement*val) {
            delete m_value;
            m_value = val;
        }

        // JsonProperty is reponsible for deleting the value.
        // use this to keep the value after the property is deleted
        void forgetValue() {
            m_value = NULL;
        }
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

        JsonProperty* add(const char *nameStart,size_t nameLen,JsonElement * value){
            JsonProperty* prop = new JsonProperty(m_root,nameStart,nameLen,value,m_firstProperty);
            m_firstProperty = prop;
            return prop;
        }

        
        JsonProperty* add(const char *name,JsonElement * value){
            m_logger->always("add property %s",name);
            JsonProperty*prop = getProperty(name);
            if (prop != NULL) {
                m_logger->always("\tproperty exists.  replacing value for %s",name);
                prop->setValue(value);
            } else {
                m_logger->always("\tcreate new property %d %s",name,name);

                prop = new JsonProperty(m_root,name,value,m_firstProperty);
                m_logger->always("\t\tcreated property %s",name);
                m_firstProperty = prop;
            }
            return prop;
        }

        JsonProperty* add(const char *name,bool value);
        JsonProperty* add(const char *name,int value);
        JsonProperty* add(const char *name,const char *value);
        JsonProperty* add(const char *name,double);

        JsonProperty * getProperty(const char * name) {
            m_logger->always("get property %s",name);
            for(JsonProperty*prop=m_firstProperty;prop!=NULL;prop=prop->getNext()){
                m_logger->always("\tcheck%s",prop->getName());
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
        JsonArrayItem(JsonRoot& root, JsonElement * value,JsonArrayItem* next) :JsonElement(root,JSON_ARRAY_ITEM) {
            m_logger->debug("create JsonArray item for type %d",value->getType());

            m_value = value;
            m_next = next;
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
            JsonArrayItem* item = new JsonArrayItem(m_root,value,m_firstItem);
            m_firstItem = item;
            return item;
        }

        int getCount() { return m_firstItem == NULL ? 0 : m_firstItem->getCount();}
        JsonElement* getAt(size_t idx) {
            return m_firstItem == NULL ? NULL : m_firstItem->getAt(idx);
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
            size_t len = strlen(value);
            m_value = root.allocString(value,len);
            m_logger->debug("create JsonString ~%s~ ",m_value);
            mem.construct("JsonString",this);
        }
        JsonString(JsonRoot& root, const char * value, size_t len) : JsonValue(root,JSON_STRING) {
            m_value = root.allocString(value,len);
            m_logger->debug("create JsonString ~%s~ ",m_value);
            mem.construct("JsonString",this);
        }

        virtual ~JsonString() { getRoot()->freeString(m_value);
            mem.destruct("JsonString",this);
        }
        
        virtual int getStringValue(char * value, size_t maxLen,const char* defaultValue=NULL) {
            strncpy(value,m_value,maxLen);
            return true;
        }
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
        JsonBool(JsonRoot& root, bool value) : JsonValue(root,JSON_BOOL) {
            m_logger->debug("create JsonFloat %s ",value?"true":"false");

            m_value = value;
            mem.construct("JsonBool",this);
        
        }

        virtual ~JsonBool() { 
            mem.destruct("JsonBool",this);
        }

        virtual bool getBoolValue(double& value,bool defaultValue) {
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
    protected:
        char * m_value;
};

class ParseGen {

};

class Generator : public ParseGen {
private:
    Generator(DRBuffer* buffer) {
        m_buf = buffer;
        depth = 0;
        pos = 0;
        m_buf->reserve(1)[0] = 0;  
        m_logger = genLogger;
    }

    void startObject() {
        write("{\n",true);
        depth += 1;
        m_logger->debug("start obj: %.20s",m_buf->text());
    }
    void endObject() {
        char *  data = (char *) m_buf->text();
        int16_t comma = pos;

        while(comma > 0 && data[comma] != ',') {
            data[comma] = ' ';
            comma -= 1;
        }
        if (comma > 0) {
            data[comma] = '\n';
        }
        depth -= 1;
        write("}",true);
        m_logger->debug("end obj: %.500s",m_buf->text());
    }

    
    void startArray() {
        write("[\n",true);
        depth += 1;
        m_logger->debug("start array: %.500s",m_buf->text());
    }
    void endArray() {
        depth -= 1;
        char *  data = (char *) m_buf->text();
        int16_t comma = pos;

        while(comma > 0 && data[comma] != ',') {
            data[comma] = ' ';
            comma -= 1;
        }
        if (comma > 0) {
            data[comma] = '\n';
        }
        write("]",true);
        m_logger->debug("end array: %.500s",m_buf->text());
    }

    void startProperty() {

    }

    void endProperty() {
        write(",\n");
    }

    void writeName(const char * name) {
        write("\"",true);
        write(name);
        write("\": ");
    }

    void writeArrayValue(const char * val) {
        write("\"",true);
        write(val);
        write("\", ");
    }

    void writeNameValue(const char * name,const char* string) {
        writeName(name);
        write("\"");
        write(string);
        write("\",\n");
        m_logger->debug("add name/str: %s:%s : %.500s",name,string,m_buf->text());
    }



    void writeNameValue(const char * name,int val) {
        sprintf(intValBuff,"%d",val);
        writeName(name);
        write(intValBuff);
        write(",\n");
    }

    
    void writeNameValue(const char * name,bool val) {
        writeName(name);
        write(val ? "true":"false");
        write(",\n");
    }

    void write(const char * data,bool indent=false) {
        if (indent) {
            uint8_t * spaces = m_buf->reserve(pos+depth*2)+pos;
            memset(spaces,' ',depth*2);
            pos += depth*2;
        }
        size_t len = strlen(data);
        uint8_t  * to =  m_buf->reserve(pos+len+1)+pos;
        memcpy(to,data,len);
        pos += len;
        m_buf->setLength(pos);
        to[len] = '~';
    }

protected:
    DRBuffer* m_buf;    
    int depth;
    size_t pos;
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
            }  else {
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
        m_logger = new PGLogger("Parser",WARN_LEVEL);
        m_errorMessage = NULL;
        m_hasError = false;
        m_root = NULL;
    }    

    ~JsonParser() { 
        delete m_logger;
    }


    JsonRoot* read(const char * data) {
        m_logger->debug("parsing %.50s...",data);
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
            root->add(json);
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
            obj->add(nameStart,nameLen,val);
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

JsonRoot::~JsonRoot() { 
    if (m_value == NULL) {
        m_logger->warn("deleting JsonRoot without m_value");
    }
    delete m_value; 
    delete m_logger; 
    mem.destruct("JsonRoot",this);
}


JsonProperty* JsonObject::add(const char *name,bool value){
    JsonBool* jval = new JsonBool(*getRoot(),value);
    return add(name,jval);

}

JsonProperty* JsonObject::add(const char *name,int value) {
    JsonInt* jval = new JsonInt(*getRoot(),value);
    return add(name,jval);
};

JsonProperty* JsonObject::add(const char *name,const char *value) {
    JsonString* jval = new JsonString(*getRoot(),value);
    return add(name,jval);
};

JsonProperty* JsonObject::add(const char *name,double value) {
    JsonFloat* jval = new JsonFloat(*getRoot(),value);
    return add(name,jval);
};


}
#endif
