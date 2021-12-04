#ifndef PARSE_GEN_H
#define PARSE_GEN_H

#include "./logger.h";
#include "./buffer.h";

namespace DevRelief {
Logger* genLogger = new Logger("Gen",WARN_LEVEL);
Logger* parserLogger = new Logger("Parse",WARN_LEVEL);
char skipBuf[100];
char intValBuff[32];

enum ValueType {
    UNKNOWN=999,
    INTEGER=0,
    FLOAT=1,
    STRING=2,
    BOOLEAN=3,
    VARIABLE=4
};
#define PARSER_VALUE_MAX_LEN 20
class ParserValue {
    public:
        ParserValue() {
            type = UNKNOWN;

        }

        bool isInt() {
            return type == INTEGER;
        }

        bool isFloat() {
            return type == FLOAT;
        }

        bool isNumber() {
            return isInt() || isFloat();
        }

        bool isString() {
            return type == STRING;
        }
        bool isBoolean() {
            return type == BOOLEAN;
        }
        bool isVariable() {
            return type == VARIABLE;
        }

        ValueType type;
        union{
            int intValue;
            double floatValue;
            bool boolValue;
            char stringValue[PARSER_VALUE_MAX_LEN];
            char nameValue[PARSER_VALUE_MAX_LEN];
        };
};

class ParseGen {

};

class Generator : public ParseGen {
public:
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

class Parser : public ParseGen {
public:
    Parser( const char * data) {
        m_logger = parserLogger;

        m_data = data;
        m_start = 0;
        m_end = data ? strlen(data)-1 : 0;
        m_pos = 0;

    }    

    virtual bool readAll(DRBuffer& buf){
        if (m_start<0) {
            return false;
        }
        int len = m_end-m_start+1;
        char * data = (char*)buf.reserve(m_end-m_start+2);
        memcpy(data,m_data+m_start,m_end-m_start+1);
        data[m_end-m_start+1] = 0;
        buf.setLength(m_end-m_start+2);
        return true;
    } 
    
    virtual bool readIntValue(const char *name,int * value){
        m_logger->debug("read int %s",name);
        int16_t found = skipName(name);
        if (found >= 0) {
            int16_t pos = skipChars(": \t\n\r",found);
            (*value) = atoi(m_data+pos);
            m_logger->debug("\tgot int: %d from %.15s",*value,m_data+pos);
            return true;
        }
        return false;
    }

        
    virtual bool readFloatValue(const char *name,double * value){
        m_logger->debug("read int %s",name);
        int16_t found = skipName(name);
        if (found >= 0) {
            int16_t pos = skipChars(": \t\n\r",found);
            (*value) = atof(m_data+pos);
            m_logger->debug("\tgot float: %f from %.15s",*value,m_data+pos);
            return true;
        }
        return false;
    }

        
    virtual bool readValue(const char *name,ParserValue& val){
        m_logger->debug("read ParserValue %s",name);
        val.type = UNKNOWN;
        val.stringValue[0] = 0;
        int16_t found = skipName(name);
    
        if (found >= 0) {
            int16_t pos = skipChars(": \t\n\r",found);
            if (pos > m_end) {
                return false;
            }
            char ch = m_data[pos];
            if (ch == '"'){
                int16_t end = skipTo('"',pos+1);
                if (end == -1) {
                    return false;
                }
                pos += 1; // skip quote
                int len = end-pos;
                if (strncmp(m_data+pos,"var(",4)==0){
                    val.type = VARIABLE;
                    len -= 5;
                    if (len>PARSER_VALUE_MAX_LEN) {
                        m_logger->error("ParserValue variable name too long");
                        len = PARSER_VALUE_MAX_LEN;
                    }
                    memcpy(val.nameValue,m_data+pos+4,len);
                    val.nameValue[len]=0;
                    m_logger->debug("got VARIABLE type %d %d %.50s",pos,end,val.nameValue);
                }else {
                    val.type = STRING;
                    if (len>PARSER_VALUE_MAX_LEN) {
                        m_logger->error("ParserValue string too long");
                        len = PARSER_VALUE_MAX_LEN;
                    }
                    memcpy(val.stringValue,m_data+pos,len);
                    val.stringValue[len]=0;
                    m_logger->debug("got VARIABLE type %d %d %.50s",pos,end,val.stringValue);
                }
                return true;
            }
            if (ch>='0' && ch <='9'){
                val.type = INTEGER;
                for(int p=pos;p<10 && m_data[p]>='0' && m_data[p]<='9';p++) {
                    if (m_data[p] == '.'){
                        val.type = FLOAT;
                    }
                }
                val.floatValue = atof(m_data+pos);
                val.intValue = round(val.floatValue);
                if (val.intValue != val.floatValue) {
                    val.type = FLOAT;
                }
                m_logger->debug("got NUMBER type %d %d %.50s",val.intValue,pos,m_data+pos);
                return true;
            }
            if (strncmp(m_data+pos,"true",4)==0){
                val.type = BOOLEAN;
                val.boolValue = true;
                return true;
            }
            if (strncmp(m_data+pos,"false",5)==0){
                val.type = BOOLEAN;
                val.boolValue = false;
                return true;
            }
        }
        m_logger->debug("\tnot found");
        return false;
    }

    virtual bool readBoolValue(const char *name,bool * value, bool defaultValue=false){
        m_logger->debug("read bool %s",name);
        int16_t found = skipName(name);
        if (found >= 0) {
            int16_t pos = skipChars(": \t\n\r",found);
            (*value) = strncmp(m_data+pos,"true",4)==0;
            m_logger->debug("\tgot int: %d from %.15s",*value,m_data+pos);
            return true;
        }
        *value = defaultValue;
        return false;
    }

    virtual bool readStringValue(const char *name,char * value, int maxLen=-1) {
        value[0] = 0;
        int16_t found = skipName(name);
        if (found >= 0) {
            m_logger->debug("readStringValue %d ~%.50s~",found,m_data+found);
            found = skipChars(" \t\n\r:",found);
            m_logger->debug("\tskipped %d ~%.50s~",found,m_data+found);
            int16_t start = skipTo('"',found-1);
            int16_t comma = skipTo(',',found);
            if (comma > found && comma < start) {
                value[0] = 0;
                m_logger->debug("found comma before string %d %d",comma,start);
                return true;
            }
            int16_t end = skipTo('"',start+1);
            if (start > 0 && start < end && start <= m_end && end <= m_end) {
                start++;
                if (end <= m_end) {
                    int len = end-start;
                    if (maxLen>0 && len>=maxLen-1) {
                        len = maxLen-1;
                    }
                    memcpy(value,m_data+start,len);
                    *(value+end-start)= 0;
                    m_logger->debug("read string: %s",value);
                    return true;
                }
            }
        }
        m_logger->debug("no value found");
        return false;
    }

    virtual bool getArray(const char * name,Parser& array) {
        m_logger->never("getArray %s",name);
        int16_t found = skipName(name);
        if (found >= 0) {
            found = skipChars(": \t\n\r",found);
            m_logger->never("\tfound  %d ~~~~%.115s~~~~~",found,m_data+found);
            int16_t start = skipTo('[',found);
            int16_t end = skipTo(']',start+1);
            if (end == -1){
                end = m_end;
            }
            if (isValidPos(start,end)) {
                array.setData(m_data,start+1,end);
                if (m_logger->isDebug()){
                    DRBuffer buf(end-start);
                    memcpy(buf.data(),m_data+start+1,end-start);
                    *(buf.data()+end-start) = 0;
                    m_logger->debug("read array: %s",buf.data());
                }
                return true;
            } 
            m_logger->never("\tinvalid pos %d %d %d %d",start,end,m_start,m_end);

        }
        return false;
    }


    int16_t skipWhite(int16_t start = 0) {
        int16_t pos = start;
        char * white = " \t\n\r";
        return skipChars(white,start);
    }

    int16_t skipChars(const char * chars, int16_t start=0){
        int16_t pos = start;
        while(pos >= 0 && pos <= m_end && strchr(chars,m_data[pos]) != NULL) {
            pos += 1;
        }
        return pos;
    }

    int16_t skipPast(char ch, int16_t start = 0) {
        int16_t pos = skipTo(ch,start);
        if (pos >=0) {
            pos += 1;
        }
        return pos;
    }

    int16_t skipTo(char ch,int16_t start=0){
        m_logger->debug("skipTo %c.  %d-%d.  %.20s",ch,start,m_end,m_data+start);
        if (start <0 || start > m_end) {
            return -1;
        }
        int16_t pos = start;
        while(pos >=0 && pos <= m_end && m_data[pos] != ch){
            m_logger->debug(" %c at %.5s",ch, m_data+pos);
            if (m_data[pos] != ch){
                if (m_data[pos] == '\\'){
                    pos++;
                }
                if (m_data[pos] == '"') {
                    pos = skipTo('"',pos+1);
                    m_logger->debug("\tskipped string");
                }
                if (m_data[pos] == '[') {
                    pos = skipTo(']',pos+1);
                    m_logger->debug("\tskipped array");
                }
                if (m_data[pos] == '{') {
                    pos = skipTo('}',pos+1);
                    m_logger->debug("\tskipped object");
                }

                pos++;
            }
        }
        if (pos > m_end) {
            m_logger->debug("\tchar not found");
            return -1;
        }
        return pos;
    }

    int16_t skipName(const char * name,int16_t start=0){
        snprintf(skipBuf,100,"\"%s\"",name);
        int16_t pos = search(skipBuf,start);
        if (pos > 0) {
            pos += strlen(skipBuf);
        }
        return pos;
    }

    int16_t search(const char * what,int16_t from) {
        int16_t len = strlen(what);
        while(from <= m_end && memcmp(what,m_data+from,len)!= 0) {
            from += 1;
        }
        if (from <= m_end) {
            return from;
        }
        return -1;
    }

    bool isValidPos(int16_t start, int16_t end) {
        return start>=0 && start <= end && end <= m_end;
    }

    void setData(const char * data) {
        setData(data,0,strlen(data));
    }

    void setData(const char * data, int16_t start, int16_t end) {
        m_data = data+start;
        m_start = 0;
        m_end = end-start;
        m_pos = 0;
    }

    void trim(const char *  ignore) {
        m_logger->never("trim ~~%s~~ from ~~%.50s~~",ignore,m_data+m_start);
        trimStart(ignore);
        m_logger->never("trimed start  ~~%.50s~~  %d",m_data+m_start,(int)m_data[m_start]);
        trimEnd(ignore);
        m_logger->never("trimed end  %d-%d  %d",m_start,m_end,m_pos);
    }

    void trimStart(const char *  ignore) {
        while(m_start >= 0 && m_start <= m_end && strchr(ignore,m_data[m_start]) != NULL){
            m_logger->never("strim %d '%c'",m_start,m_data[m_start]);
            m_start += 1;
        }
        if (m_pos < m_start) {
            m_pos = m_start;
        }
    }


    void trimEnd(const char *  ignore) {
        while(m_end > 0 && m_start <= m_end && strchr(ignore,m_data[m_end-1]) != NULL){
            m_logger->debug("etrim %d '%c'",m_end,m_data[m_end]);
            m_end -= 1;
        }
        m_logger->debug("etrim stopp '%c'",m_data[m_end-1]);
        if (m_pos > m_end) {
            m_pos = m_end;
        }
    }

    virtual void setPos(int16_t pos) {
        m_pos = pos;
    }

    void dump(Logger*logger) {
        if (logger != NULL) {
            logger->never("Parser start=%d end=%d pos=%d ~~~%.*s~~~",m_start,m_end,m_pos,getLen(),m_data+m_start);
        }
    }

    const char * getData() { return m_data;}
    int16_t getStart() { return m_start;}
    int16_t getEnd() { return m_end;}
    int16_t getPos() { return m_pos;}
    int16_t getLen() { int16_t l = m_end-m_start+1;  return l<0 ? 0 : l;};

protected:
    const char * m_data;
    int16_t m_start;     
    int16_t m_end;
    int16_t m_pos;

    Logger * m_logger;
};

class ObjectParser : public Parser{
public:
    ObjectParser(const char * data=NULL) : Parser(data) {

    }   
};

class ArrayParser : public Parser{
public:
    ArrayParser(const char * data=NULL) : Parser(data) {
        m_logger = parserLogger;// new Logger("ArrayParser",40);
    }   

    bool nextElement(Parser* parser) {
        m_logger->never("next element %.20s",m_data+m_pos);
        dump(m_logger);
        if (m_pos == -1 || m_pos >= m_end) {
            m_logger->never("\tno next element");
            return false;
        }
        int16_t start = skipWhite(m_pos);
        
        int16_t comma = skipTo(',',m_pos);
        int16_t bracket = skipTo(']',m_pos);
            m_logger->never("comma and bracket%d %d",comma, bracket);
        if (comma == -1 && bracket == -1) {
            comma = m_end;
            m_pos = m_end;
        } else if (comma >=0 && (bracket == -1 || comma < bracket)) {
            m_logger->never("found comma %d ~~~%.40s~~~",comma,m_data+comma-1);
            m_pos = comma-1;
        } else if (bracket > 0) {
            m_logger->never("found bracket %d",comma);
            m_pos = bracket-1;
        } else {
            parser->setData(m_data,start,start);
            return false;
        }
        m_logger->never("got element %d-%d ~~~%.*s~~~",start,m_pos,m_pos-start,m_data+start);
        if (m_pos > 0 && m_pos >start && m_pos <= m_end) {
            parser->setData(m_data,start,m_pos);
            m_pos += 2;
            return true;
        } else {
            parser->setData(m_data,start,m_pos);
            return false;
        }
    }


    bool nextObject(Parser* parser) {
        m_logger->never("next object %.120s",m_data+m_pos);
        if (nextElement(parser)) {
           parser->trim(" \t\n\r,{}[]");
           char first = m_data[m_start];
           char last = m_data[m_end];
           m_logger->never("first/last %c/%c   %d - %d",first,last,m_start,m_end);
           if ((first == '{' && last == '}') || (first == '[' && last == ']')) {
               m_start += 1;
               m_end -= 1;
               m_pos = m_pos < m_start ? m_start : m_pos;
               m_pos = m_pos > m_end ? m_end : m_pos;
           }
            return true;
        }
        m_logger->never("\t not found");
        
        return false;

    }

    int16_t readInts(int * values,int16_t max=20) {
        m_logger->error("readInts not implemented");
        return 0;
    }
};


}
#endif