#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\drstring.h"
#ifndef DR_STRING_H
#define DR_STRING_H

#include "./logger.h"
#include "./shared_ptr.h"

namespace DevRelief {

Logger StringLogger("DRString",DRSTRING_LOGGER_LEVEL);
Logger* stringLogger = &StringLogger;

class DRStringData {
    public:
        DRStringData(size_t length) : m_length(0), m_maxLength(0), m_data(NULL) {
            stringLogger->debug("create DRStringData %d",m_data);
            ensureLength(length);
            m_length = length;
        }

        virtual ~DRStringData() {
            delete m_data;
        }

        virtual void destroy() { delete this;}

        const char * get() const {
            ((DRStringData*)this)->ensureLength(1);
            return m_data;
        }

        size_t updateLength() {
            if (m_data == NULL) {
                m_length = 0;
            } else {
                m_length = strlen(m_data);
            }
            return m_length;
        }
        char * ensureLength(size_t minLength){
            stringLogger->debug("\tensureLength %d  (%d)",minLength,m_maxLength);
            if (m_maxLength >= minLength) {
                stringLogger->debug("\tensureLength %d",minLength);
                return m_data;
            }

            size_t newLength = minLength+1;
            stringLogger->info("\tnew string length %d",newLength);

            char * newData = (char*) malloc(sizeof(char)*newLength);
            stringLogger->debug("\tzero mem");

            memset(newData,0,newLength);
            if (m_data != NULL){
                if (m_length > 0) {
                    stringLogger->debug("\tcopy old data");
                    memcpy(newData,m_data,m_length);
                }
                stringLogger->debug("\tfree old data");
                free(m_data);
            }
            m_data = newData;
            m_maxLength = newLength-1;
            stringLogger->debug("\tdone");
            return m_data;
        }

        char * setLength(size_t newLength) {
            ensureLength(newLength);
            stringLogger->debug("ensureLength done %d",newLength);
            m_length = newLength;
            if (m_data != NULL) {
                stringLogger->debug("add NULL terminate %d",m_data);
                m_data[newLength] = 0;
            }
            return m_data;
        }

        char * increaseLength(size_t additional) {
            // return pointer to new data
            stringLogger->debug("increase length %d + %d",m_length,additional);
            size_t actualLength = (m_data == NULL) ? 0 : strlen(m_data);
            size_t origLength = m_length;
            if (actualLength+additional <= m_maxLength) {
                // have enough space;
                m_length=actualLength + additional;
                stringLogger->debug("\tno increase %d %d %d %c",origLength, actualLength,additional,m_data[origLength]);
                stringLogger->debug("\t\tcurrent: ~%s~",m_data);
                return m_data+actualLength;

            }
            size_t addLength = additional;
            if (addLength < 32) {
                addLength = 32;
            } 
            if (addLength < m_length/2) {
                addLength = m_length/2;
            }
            
            ensureLength(m_length+addLength);
            m_length = actualLength+additional;
            stringLogger->debug("\tadded %d %d %d %c",origLength, actualLength,additional,m_data[origLength]);
            stringLogger->debug("\t\tcurrent: ~%s~",m_data);
            return m_data+actualLength;

        }

        char * data() { 
            stringLogger->debug("\tget DRStringBuffer data %d %d %s",m_length,m_maxLength,(m_data ? m_data : "<no data>"));
            return m_data;
        }
        const char * data() const { return m_data;}
        size_t getLength() const { return m_length;}
        size_t getMaxLength() const { return m_maxLength;}
    private:
        size_t m_length;
        size_t m_maxLength;
        char * m_data;
};

class DRString {
    public: 
        static DRString fromFloat(double val);
        DRString(const char * = NULL);
        DRString(const char *, size_t len);
        DRString(char c, size_t repeatCount);
        DRString(const DRString& other);
        ~DRString();

        const char* operator->() const { 
            stringLogger->debug("operator->");
            return m_data.get()->data();
        }

        operator const char*() const { 
            stringLogger->debug("cast(const char *)operator");
            return m_data.get()->data();
        }

        const char * get() const { 
            stringLogger->never("get()");
            if (m_data.get() == NULL){
                stringLogger->never("string is NULL");
                return "";
            }
            stringLogger->never("have  m_data 0x%04X",m_data.get());
            return m_data.get()->get();
        }

        DRString& append(const char * other);
        const char * operator+=(const char * other){return append(other);}
        const char * operator+=(const DRString& other) {return append(other.text());}

        void clear() {
            stringLogger->debug("clear buffer");
            if (m_data.get() == NULL) {
                stringLogger->debug("no m_data");
            }
            stringLogger->debug("setLength(0)");
            m_data.get()->setLength(0);
            stringLogger->debug("\tdone setLength(0)");
        }
        char * increaseLength(size_t charsNeeded) { 
            return m_data.get()->increaseLength(charsNeeded);
        }

        const char * text() const { return get();}
        size_t getLength() { return m_data.get()->getLength();}

        // trime whitespace + optional chars
        DRString& trimStart(const char * chars=NULL);
        DRString& trimEnd(const char * chars=NULL);
    protected:
        SharedPtr<DRStringData> m_data;

};

DRString DRString::fromFloat(double val){
    DRString result;
    char * buf = result.m_data->ensureLength(32);
    sprintf(buf,"%f",val);
    result.m_data->updateLength();
    return result;
}

DRString::DRString(const DRString& other) : m_data(other.m_data){
    stringLogger->debug("Create DRString from other");
    
}

DRString::DRString(const char * orig) {
    stringLogger->debug("Create DRString from const char *");
    size_t len = orig == NULL ? 0 : strlen(orig);
    m_data = new DRStringData(len);
    if (orig != NULL) { 
        stringLogger->debug("copy orig %.15s",orig);
        strncpy(m_data.get()->data(),orig,len);
        stringLogger->debug("copy done");
    }

}

DRString::DRString(const char * orig, size_t length) {
    stringLogger->debug("Create DRString from const char * and length");
    size_t len = orig == NULL ? 0 : length;
    m_data = new DRStringData(len);
    if (orig != NULL) { 
        strncpy(m_data.get()->data(),orig,len);
    }
}


DRString::DRString(char c, size_t repeatCount) {
    stringLogger->debug("Create DRString from char  and repeatCount");
    stringLogger->debug("\tchar: %c  count: %d",c,repeatCount);
    size_t len = repeatCount;
    m_data = new DRStringData(len);
    char * data = m_data.get()->data();
    for(int i=0;i<len;i++) {
        data[i] = c;
    }
    
}

DRString::~DRString() {
    stringLogger->debug("~DRString()");
    if (m_data.get() == NULL) {
        stringLogger->error("\tmissing m_data");
    } else {
        stringLogger->debug("\tlength=%d",m_data.get()->getLength());
        stringLogger->debug("\ttext=%s",m_data.get()->ensureLength(1));
    }
}

DRString& DRString::append(const char * other){
    if (other == NULL) { return *this;}
    
    size_t olen = strlen(other);
    if (olen == 0) {
        return *this;
    }
    char * extra = m_data.get()->increaseLength(olen);
    memcpy(extra,other,olen);
    return *this;
}


class DRFormattedString : public DRString{
    public:
        DRFormattedString(const char * format,...){
            va_list args;
            va_start (args,format);
            formatString(format,args);
        }

    private:
        void formatString(const char * format, va_list args) {
            char buf[2];
            int len = vsnprintf(buf,1,format,args)+1;
            m_data.get()->ensureLength(len+1);
            vsnprintf(m_data.get()->data(),len,format,args);
        }
};



}
#endif