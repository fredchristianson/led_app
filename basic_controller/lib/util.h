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
        SharedPtr(SharedPtr& other) :data(other.data),m_refCount(other.m_refCount) {
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
            UtilLogger.debug("SharePtr::set()");
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
        const T& operator->() const { return *data;}

        const T* get() const  { return data;}
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
                UtilLogger.debug("\t\t%15s",data);
            }
            UtilLogger.debug("\t\t-----------------dump end-------------");
        }
        T* data;
        int* m_refCount;
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
            return m_data.get();
        }
        operator const char*() const { 
            StringLogger.debug("cast(const char *)operator");
            return m_data.get();
        }
        const char * get() const { 
            StringLogger.debug("get()");
            return m_data.get();
        }

        const char * operator+=(const char * other);
    private:
        SharedPtr<const char> m_data;
        size_t m_length;
};


DRString::DRString(const DRString& other){
    StringLogger.debug("Create DRString from other");
    m_data = other.m_data;
    m_length = other.m_length;
}

DRString::DRString(const char * orig) {
    StringLogger.debug("Create DRString from const char *");
    if (orig == NULL) { 
        StringLogger.debug("\tNULL value");
        m_data = NULL;
        StringLogger.debug("\tset length");
        m_length = 0;
        StringLogger.debug("\tinit done");
    } else {
        StringLogger.debug("\ttext: %.15s",orig);
        size_t len = strlen(orig);
        char* copy = new char[len+1];
        strncpy(copy,orig,len);
        copy[len] = 0;
        m_length = len;
        StringLogger.debug("set m_data");
        m_data.set(copy);
        StringLogger.debug("\tset done");
    }
}

DRString::DRString(const char * orig, size_t length) {
    StringLogger.debug("Create DRString from const char * and length");
    if (orig == NULL) { 
        StringLogger.debug("\tNULL value");
        m_data = NULL;
        StringLogger.debug("\tset length");
        m_length = 0;
        StringLogger.debug("\tinit done");
    } else {
        StringLogger.debug("\ttext: %.15s",orig);
        size_t len = length;
        char* copy = new char[len+1];
        strncpy(copy,orig,len);
        copy[len] = 0;
        m_length = len;
        StringLogger.debug("set m_data");
        m_data.set(copy);
        StringLogger.debug("\tset done");
    }
}


DRString::DRString(char c, size_t repeatCount) {
    StringLogger.debug("Create DRString from char  and repeatCount");
    StringLogger.debug("\tchar: %c  count: %d",c,repeatCount);
    size_t len = repeatCount+1;;
    char* copy = new char[len+1];
    for(int i=0;i<len;i++) {
        copy[i] = c;
    }
    copy[len] = 0;
    m_length = len;
    StringLogger.debug("set m_data");
    m_data.set(copy);
    StringLogger.debug("\tset done");
    
}

DRString::~DRString() {
}

const char * DRString::operator+=(const char * other){
    if (other == NULL) { return m_data.get();}
    size_t olen = strlen(other);
    if (olen == 0) {
        return m_data.get();
    }
    size_t nlen = m_length + olen;
    char* copy = new char[nlen+1];
    memcpy(copy,m_data.get(),m_length);
    memcpy(copy+m_length,other,olen);
    copy[nlen] = 0;
    m_data = copy;
    m_length = nlen;
    return m_data.get();
}
};


class Util {
    public:

};
#endif