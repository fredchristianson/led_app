#ifndef DR_UTIL_H
#define DR_UTIL_H

#include "./logger.h"
#include "./drstring.h"

namespace DevRelief {

Logger UtilLogger("Util",WARN_LEVEL);


class Util {
    public:
        static bool startsWith(const char * var, const char * prefix) {
            if (var == NULL) { return false;}
            if (prefix == NULL || prefix[0] == 0) { return true;}
            return strncmp(var,prefix,strlen(prefix))==0;
        };

        static bool equal(const char * s1, const char * s2) {
            if (s1 == s2) { return true;}
            if ((s1 == NULL && s2 != NULL) || (s1 != NULL && s2 == NULL)) { return false;}
            return strcmp(s1,s2) == 0;
        }

        static bool isEmpty(const char * s) {
            return s == NULL || s[0] == 0;
        }
};


}
#endif