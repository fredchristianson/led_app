#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\shared_ptr.h"
#ifndef DR_SHARED_PTR_H
#define DR_SHARED_PTR_H

#include "./logger.h"

namespace DevRelief {

Logger SharedPtrLogger("SharedPtr",WARN_LEVEL);
Logger* sharedPtrLogger = &SharedPtrLogger;

template<typename T>
class SharedPtr {
    public:
        SharedPtr(): data(NULL), m_refCount(NULL) {
            sharedPtrLogger->debug("empty SharedPtr");
        };
        SharedPtr(T*val): data(val), m_refCount(NULL) {
            sharedPtrLogger->debug("SharedPtr with direct data");
            incRefCount();
        };
        SharedPtr(const SharedPtr& other) :data(other.data),m_refCount(other.m_refCount) {
            sharedPtrLogger->debug("SharedPtr with shared data");
            incRefCount();
        }

        ~SharedPtr() {
            freeData();
        }

        void freeData(){
            sharedPtrLogger->debug("freeData");

            if (decRefCount()==0) {
                dump();
                sharedPtrLogger->debug("\tno more refs");
                free(m_refCount);
                delete data;
                dump();
            } else {
                sharedPtrLogger->debug("\tanother ref exists %d",*m_refCount);
            }
            m_refCount = NULL;
            data = NULL;
        }

        SharedPtr<T>& operator=(const SharedPtr<T>&other) {
            if (this != &other) {
                dump();
                sharedPtrLogger->debug("SharePtr = other");
                freeData();
                sharedPtrLogger->debug("\tother count %d",other.getRefCount());
                other.dump();
                m_refCount = other.m_refCount;
                incRefCount();
                data = other.data;
            }
            dump();
            return *this;
        }

        void set(T*t) {
            sharedPtrLogger->debug("SharePtr::set() %s",(t ? "not null ":"null"));
            dump();
            freeData();
            if (t != NULL) {
                incRefCount();
                data = t;
            }
            dump();

        }

        SharedPtr<T>& operator=(T*t) {
            sharedPtrLogger->debug("SharePtr::operator=(T*)");
            set(t);
            return *this;
        }

        T* operator->() { return data;}
        T& operator*() { return *data;}
        const T* operator->() const { return data;}

        const T* get() const  {
            sharedPtrLogger->never("get() 0x%04X",data);
            return data;
        }
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
            sharedPtrLogger->debug("\t\t-----------------dump start---------%d",this);
            if (m_refCount == NULL) {
                sharedPtrLogger->debug("\t\tno count");
            } else {
                sharedPtrLogger->debug("\t\tcount %d",*m_refCount);
            }
            if (data == NULL) {
                sharedPtrLogger->debug("\t\tno data");
            } else {
                sharedPtrLogger->debug("\t\t%15s",(const char *)data);
            }
            sharedPtrLogger->debug("\t\t-----------------dump end-------------");
        }
        T* data;
        int* m_refCount;
};


}
#endif