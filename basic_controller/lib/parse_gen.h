#ifndef PARSE_GEN_H
#define PARSE_GEN_H

#include "./logger.h";
#include "./buffer.h";

namespace DevRelief {
Logger* genLogger = new Logger("Gen",100);
Logger* parserLogger = new Logger("Parse",60);

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
    void writeNameValue(const char * name,const char* string) {
        writeName(name);
        write(string);
        write(",\n");
        m_logger->debug("add name/str: %s:%s : %.500s",name,string,m_buf->text());
    }


    void writeNameValue(const char * name,int val) {
        char strval[32];
        sprintf(strval,"%d",val);
        writeName(name);
        write(strval);
        write(",\n");
        m_logger->debug("add name/int: %.500s",m_buf->text());
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
        m_end = data ? strlen(data) : 0;
        m_pos = 0;

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
    virtual bool readStringValue(const char *name,char * value) {
        int16_t found = skipName(name);
        if (found >= 0) {
            int16_t start = skipTo('"',found);
            int16_t end = skipTo('"',start+1);
            if (start > 0 && start < end && start < m_end && end < m_end) {
                start++;
                if (end < m_end) {
                    memcpy(value,m_data+start,start-end);
                    *(value+end-start)= 0;
                    m_logger->debug("read string: %s",value);
                    return true;
                }
            }
        }
        return false;
    }

    virtual bool getArray(const char * name,Parser& array) {
        int16_t found = skipName(name);
        if (found >= 0) {
            int16_t start = skipTo('[',found);
            int16_t end = skipTo(']',start+1);
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
        while(pos >= 0 && pos < m_end && strchr(chars,m_data[pos]) != NULL) {
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
        if (start <0 || start >= m_end) {
            return -1;
        }
        int16_t pos = start;
        while(pos >=0 && pos < m_end && m_data[pos] != ch){
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
        char find[100];
        snprintf(find,100,"\"%s\"",name);
        int16_t pos = search(find,start);
        if (pos > 0) {
            pos += strlen(find);
        }
        return pos;
    }

    int16_t search(const char * what,int16_t from) {
        int16_t len = strlen(what);
        while(from < m_end && memcmp(what,m_data+from,len)!= 0) {
            from += 1;
        }
        if (from < m_end) {
            return from;
        }
        return -1;
    }

    bool isValidPos(int16_t start, int16_t end) {
        return start>=0 && start <= end && end <= m_end;
    }

    void setData(const char * data, int16_t start, int16_t end) {
        m_data = data+start;
        m_start = 0;
        m_end = end-start;
        m_pos = 0;
    }

    void trim(const char *  ignore) {
        m_logger->debug("trim %s from %.50s",ignore,m_data+m_start);
        trimStart(ignore);
        m_logger->debug("trimed start  %.50s",m_data+m_start);
        trimEnd(ignore);
        m_logger->debug("trimed end  %d-%d  %d",m_start,m_end,m_pos);
    }

    void trimStart(const char *  ignore) {
        while(m_start >= 0 && m_start < m_end && strchr(ignore,m_data[m_start]) != NULL){
            m_start += 1;
        }
        if (m_pos < m_start) {
            m_pos = m_start;
        }
    }


    void trimEnd(const char *  ignore) {
        while(m_end > 0 && m_start < m_end && strchr(ignore,m_data[m_end]) != NULL){
            m_end -= 1;
        }
        if (m_pos > m_end) {
            m_pos = m_end;
        }
    }

    virtual void setPos(int16_t pos) {
        m_pos = pos;
    }

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

    }   

    bool nextElement(Parser* parser) {
        m_logger->debug("next element %.20s",m_data+m_pos);
        if (m_pos == -1 || m_pos == m_end) {
            m_logger->debug("\tno next element");
            return false;
        }
        int16_t start = m_pos;
        int16_t comma = skipTo(',',m_pos);
        int16_t bracket = skipTo(']',m_pos);
        if (comma >=0 && comma < bracket) {
            m_pos = comma;
        } else if (bracket > 0) {
            m_pos = bracket;
        } else {
            parser->setData(m_data,start,start);
            return false;
        }
        m_logger->debug("got element %d-%d",start,m_pos);
        if (m_pos > 0 && m_pos >start && m_pos <= m_end) {
            parser->setData(m_data,start,m_pos);
            m_pos += 1;
            return true;
        } else {
            parser->setData(m_data,start,m_pos);
            return false;
        }
    }


    bool nextObject(Parser* parser) {
        m_logger->debug("next object %.20s",m_data+m_pos);
        if (nextElement(parser)) {
           parser->trim("{} \t\n\r,");
            return true;
        }
        m_logger->debug("\t not found");
        
        return false;
        /*
        if (m_pos == -1 || m_pos == m_end) {
            return false;
        }
        m_pos = skipTo('{',m_pos);
        int16_t start = m_pos;

        m_pos = skipTo('}',m_pos+1);
        if (m_pos > 0 && m_pos >start && m_pos < m_end) {
            parser->setData(m_data,start,m_pos);
            m_pos += 1;
            return true;
        } else {
            parser->setData(m_data,start,m_pos);
        }
        */
    }

    int16_t readInts(int * values,int16_t max=INT_MAX) {
        m_logger->error("readInts not implemented");
        return 0;
    }
};


}
#endif