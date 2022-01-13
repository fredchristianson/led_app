#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\ensure.h"
#ifndef ENSURE_H
#define ENSURE_H

#include "./logger.h"

namespace DevRelief {
#if ENSURE==1

Logger EnsureLogger("Ensure",ENSURE_LOGGER_LEVEL);
class Ensure {
    public:
    template<typename T>
    static bool notNull(T *&ptr,void* replace, char * msg=NULL,...) {
        if (ptr == NULL) {
            if (msg == NULL) { msg = "Ensure::notNull found NULL";}
            ptr = (T*)replace;
            va_list args;
            va_start(args,msg);
            Ensure::log(msg,args);
            return false;
        }   
        return true;
    }

    static void log(char* msg, va_list args){
        EnsureLogger.write(ERROR_LEVEL,msg,args);
    }
};

#else 
class Ensure {
    public:
    static bool notNull(void *ptr,char *msg=NULL,...) { return true;}
};
#endif // ENSURE==1
}
#endif // ENSURE_H