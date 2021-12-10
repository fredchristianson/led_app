#ifndef DR_UTIL_H
#define DR_UTIL_H

#include "./logger.h"

namespace DevRelief {

Logger UtilLogger("Util",WARN_LEVEL);
Logger StringLogger("DRString",WARN_LEVEL);

template<typename T>
class SharedPtr {
    public:
        SharedPtr(): data(NULL), m_refCount(NULL) {
            UtilLogger.debug("empty SharedPtr");
        };
        SharedPtr(T*val): data(val), m_refCount(NULL) {
            UtilLogger.debug("SharedPtr with direct data");
            incRefCount();
        };
        SharedPtr(const SharedPtr& other) :data(other.data),m_refCount(other.m_refCount) {
            UtilLogger.debug("SharedPtr with shared data");
            incRefCount();
        }

        ~SharedPtr() {
            freeData();
        }

        void freeData(){
            UtilLogger.debug("freeData");

            if (decRefCount()==0) {
                dump();
                UtilLogger.debug("\tno more refs");
                free(m_refCount);
                delete data;
                dump();
            } else {
                UtilLogger.debug("\tanother ref exists %d",*m_refCount);
            }
            m_refCount = NULL;
            data = NULL;
        }

        SharedPtr<T>& operator=(const SharedPtr<T>&other) {
            if (this != &other) {
                dump();
                UtilLogger.debug("SharePtr = other");
                freeData();
                UtilLogger.debug("\tother count %d",other.getRefCount());
                other.dump();
                m_refCount = other.m_refCount;
                incRefCount();
                data = other.data;
            }
            dump();
            return *this;
        }

        void set(T*t) {
            UtilLogger.debug("SharePtr::set() %s",(t ? "not null ":"null"));
            dump();
            freeData();
            if (t != NULL) {
                incRefCount();
                data = t;
            }
            dump();

        }

        SharedPtr<T>& operator=(T*t) {
            UtilLogger.debug("SharePtr::operator=(T*)");
            set(t);
            return *this;
        }

        T* operator->() { return data;}
        T& operator*() { return *data;}
        const T* operator->() const { return data;}

        const T* get() const  { return data;}
         T* get()  { return data;}
    private:
        int getRefCount() const { return m_refCount == NULL ? 0 : *m_refCount;}
        int incRefCount() {
            if (m_refCount == 0) {
                m_refCount = (int *) malloc(sizeof(int));
                *m_refCount = 1;
            } else {
                *m_refCount = (*m_refCount)+1;
            }
            return *m_refCount;
        }

        int decRefCount() {
            if (m_refCount == NULL) { return 0;}
            *m_refCount -= 1;
            return *m_refCount;
        }
        void dump() const {
            UtilLogger.debug("\t\t-----------------dump start---------%d",this);
            if (m_refCount == NULL) {
                UtilLogger.debug("\t\tno count");
            } else {
                UtilLogger.debug("\t\tcount %d",*m_refCount);
            }
            if (data == NULL) {
                UtilLogger.debug("\t\tno data");
            } else {
                UtilLogger.debug("\t\t%15s",(const char *)data);
            }
            UtilLogger.debug("\t\t-----------------dump end-------------");
        }
        T* data;
        int* m_refCount;
};

class DRStringData {
    public:
        DRStringData(size_t length) : m_length(0), m_maxLength(0), m_data(NULL) {
            StringLogger.debug("create DRStringData %d",m_data);
            ensureLength(length);
        }

        ~DRStringData() {
            delete m_data;
        }

        const char * get() const { return m_data;}
        char * ensureLength(size_t minLength){
            StringLogger.debug("\tensureLength %d  (%d)",minLength,m_maxLength);
            if (m_maxLength >= minLength) {
                StringLogger.debug("\tensureLength %d",minLength);
                return m_data;
            }

            size_t newLength = minLength+1;
            StringLogger.info("\tnew string length %d",newLength);

            char * newData = (char*) malloc(sizeof(char)*newLength);
            StringLogger.debug("\tzero mem");

            memset(newData,0,newLength);
            if (m_data != NULL){
                if (m_length > 0) {
                    StringLogger.debug("\tcopy old data");
                    memcpy(newData,m_data,m_length);
                }
                StringLogger.debug("\tfree old data");
                free(m_data);
            }
            m_data = newData;
            m_maxLength = newLength-1;
            StringLogger.debug("\tdone");
            return m_data;
        }

        char * setLength(size_t newLength) {
            ensureLength(newLength);
            StringLogger.debug("ensureLength done %d",newLength);
            m_length = newLength;
            if (m_data != NULL) {
                StringLogger.debug("add NULL terminate %d",m_data);
                m_data[newLength] = 0;
            }
            return m_data;
        }

        char * increaseLength(size_t additional) {
            // return pointer to new data
            StringLogger.debug("increase length %d + %d",m_length,additional);
            size_t actualLength = (m_data == NULL) ? 0 : strlen(m_data);
            size_t origLength = m_length;
            if (actualLength+additional <= m_maxLength) {
                // have enough space;
                m_length=actualLength + additional;
                StringLogger.debug("\tno increase %d %d %d %c",origLength, actualLength,additional,m_data[origLength]);
                StringLogger.debug("\t\tcurrent: ~%s~",m_data);
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
            StringLogger.debug("\tadded %d %d %d %c",origLength, actualLength,additional,m_data[origLength]);
            StringLogger.debug("\t\tcurrent: ~%s~",m_data);
            return m_data+actualLength;

        }

        char * data() { 
            StringLogger.debug("\tget DRStringBuffer data %d %d %s",m_length,m_maxLength,(m_data ? m_data : "<no data>"));
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
        DRString(const char * = NULL);
        DRString(const char *, size_t len);
        DRString(char c, size_t repeatCount);
        DRString(const DRString& other);
        ~DRString();

        const char* operator->() const { 
            StringLogger.debug("operator->");
            return m_data.get()->data();
        }

        operator const char*() const { 
            StringLogger.debug("cast(const char *)operator");
            return m_data.get()->data();
        }

        const char * get() const { 
            StringLogger.debug("get()");
            return m_data.get()->data();
        }

        const char * operator+=(const char * other);

        void clear() {
            StringLogger.debug("clear buffer");
            if (m_data.get() == NULL) {
                StringLogger.debug("no m_data");
            }
            StringLogger.debug("setLength(0)");
            m_data.get()->setLength(0);
            StringLogger.debug("\tdone setLength(0)");
        }
        char * increaseLength(size_t charsNeeded) { 
            return m_data.get()->increaseLength(charsNeeded);
        }

        const char * text() const { return m_data.get()->data();}
        size_t getLength() { return m_data.get()->getLength();}
    private:
        SharedPtr<DRStringData> m_data;

};


DRString::DRString(const DRString& other) : m_data(other.m_data){
    StringLogger.debug("Create DRString from other");
    
}

DRString::DRString(const char * orig) {
    StringLogger.debug("Create DRString from const char *");
    size_t len = orig == NULL ? 0 : strlen(orig);
    m_data = new DRStringData(len);
    if (orig != NULL) { 
        StringLogger.debug("copy orig %.15s",orig);
        strncpy(m_data.get()->data(),orig,len);
        StringLogger.debug("copy done");
    }
}

DRString::DRString(const char * orig, size_t length) {
    StringLogger.debug("Create DRString from const char * and length");
    size_t len = orig == NULL ? 0 : length;
    m_data = new DRStringData(len);
    if (orig != NULL) { 
        strncpy(m_data.get()->data(),orig,len);
    }
}


DRString::DRString(char c, size_t repeatCount) {
    StringLogger.debug("Create DRString from char  and repeatCount");
    StringLogger.debug("\tchar: %c  count: %d",c,repeatCount);
    size_t len = repeatCount;
    m_data = new DRStringData(len);
    char * data = m_data.get()->data();
    for(int i=0;i<len;i++) {
        data[i] = c;
    }
    
}

DRString::~DRString() {
}

const char * DRString::operator+=(const char * other){
    if (other == NULL) { return m_data.get()->data();}
    size_t olen = strlen(other);
    if (olen == 0) {
        return m_data.get()->data();
    }
    char * extra = m_data.get()->increaseLength(olen);
    memcpy(extra,other,olen);
    return m_data.get()->data();
}
};


class Util {
    public:

};
#endif