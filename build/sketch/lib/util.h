#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\util.h"
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

        static bool endsWith(const char *s, const char * end) {
            if (s == NULL || end == NULL) {
                return false;
            }
            int el = strlen(end);
            int sl = strlen(s);
            if (el > sl) {
                return false;
            }
            return strcmp(s+sl-el,end)==0;
        }
        static int toMsecs(const char *s) {
            int val = 0;
            if (s == NULL || !isdigit(*s)){
                return val;
            }
            if (Util::endsWith(s,"ms")){
                val = atoi(s);
            } else if (Util::endsWith(s,"s")){
                val = atoi(s)*1000;
            } else {
                val = atoi(s);
            }
            UtilLogger.always("toMsec %s ==> %d",s,val);
            return val;
        }

        // text in format "text1:int1,text2:int2,..."
        // for example "repeat:1,stretch:2,"clip:3" with text "repeat" returns 1
        static int mapText2Int(const char * text, const char * val, int defaultValue){
            if (text == NULL || val == NULL) {
                return defaultValue;
            }
            const char * pos = text;
            int len = strlen(val);
            while(*pos != 0 && (strncmp(pos,val,len) != 0 || pos[len] != ':')){
                pos += 1;
            }
            if (pos[len] == ':') {
                return atoi(pos+len+1);
            } else {
                return defaultValue;
            }

        }
};


}
#endif