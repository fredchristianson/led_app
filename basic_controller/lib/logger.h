#ifndef LOGGER_H
#define LOGGER_H

#include <cstdarg>
#include <stdio.h>
#include <time.h>
#include "../env.h"

extern EspClass ESP;

namespace DevRelief {
enum LogLevel {
    DEBUG_LEVEL=100,
    INFO_LEVEL=80,
    WARN_LEVEL=60,
    ERROR_LEVEL=40,
    ALWAYS=1,
    TEST_LEVEL=-1,
    NEVER=-2
};

#if LOGGING_ON==1
bool serialInitilized = false;
void initializeWriter() {
    if (!Serial) {
        serialInitilized = true;
        Serial.begin(115200);

        Serial.printf("\nSerial Logger Running\n--------------\n");
        while(!Serial){
            // wait for Serial port to be ready
        }
    }

}



#define MAX_MESSAGE_SIZE 1024
char messageBuffer[MAX_MESSAGE_SIZE+1];
String padding("                                   ");
const char * TABS = "\t\t\t\t\t\t";
int MAX_TAB_COUNT = 5;
char lastErrorMessage[100];
long lastErrorTime=0;
int loggerIndent=0;
bool logTestingMessage = false;

class Logger {
public:
    Logger(const char * name, int level = 100) {
        initializeWriter();
        setModuleName(name);
        m_periodicTimer = 0;
        m_level = level;
        //this->always("create Logger %s",name);
    }

    static void setTesting(bool on) { logTestingMessage = on;}

    virtual ~Logger() {
        //this->always("destroy Logger %s",m_name.c_str());
    }

    void setModuleName(const char * name) {
        m_name = name;

    }

    void setLevel(int l) { m_level = l;}
    int getLevel() { return m_level;}
    bool isDebug() { return m_level==100;}

    void indent() {
        if (loggerIndent < MAX_TAB_COUNT) {
            loggerIndent++;
        }
    }

    void outdent() {
        if (loggerIndent>0) {
            loggerIndent--;
        }
    }
    void restart() {
        this->info("restarting serial");
        Serial.begin(115200);
        while(!Serial){
            ;//
        }
        this->info("serial restarted");
    }
    void write(int level, const char * message, va_list args ){
        if (level > m_level || level == NEVER) {
            return;
        }

        vsnprintf(messageBuffer,MAX_MESSAGE_SIZE,message,args);
        unsigned long now = millis()/1000;
        int hours = now/3600;
        now = now % 3600;
        int minutes = now/60;
        int seconds = now % 60;
        const char * tabs = loggerIndent<=0 ? "" : (TABS + MAX_TAB_COUNT-loggerIndent);
        Serial.printf("%6s/%3d - %02d:%02d:%02d - %20s: %s ",
            getLevelName(level),m_level,hours,minutes,seconds,m_name.c_str(),tabs);
        Serial.println(messageBuffer);
    }

    void write(int level, const char * message,...) {
        va_list args;
        va_start(args,message);
        write(level,message,args);
    }

    void test(const char * message,...) {
        if (!logTestingMessage) { return;}
        va_list args;
        va_start(args,message);
        write(TEST_LEVEL,message,args);
    }


    // void debug(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(100,message.c_str(),args);
    // }

    void debug(const char * message,...) {
#ifdef ENV_DEV
        va_list args;
        va_start(args,message);
        write(DEBUG_LEVEL,message,args);
#endif        
    }



    // void info(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(80,message.c_str(),args);
    // }

    void info(const char * message,...) {
#ifdef ENV_DEV
        va_list args;
        va_start(args,message);
        write(INFO_LEVEL,message,args);
#endif        
    }


    // void warn(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(40,message.c_str(),args);
    // }

    void warn(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(WARN_LEVEL,message,args);
    }


    // void error(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(20,message.c_str(),args);
    // }
    
    void error(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(ERROR_LEVEL,message,args);
    }

    // mainly for new messages during development.  don't need to turn debug level on and get old debug messages.
    // quick search-replace of "always"->"debug" when done
    void always(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(ALWAYS,message,args);
    }

     void never(const char * message,...) {
     }

    void errorNoRepeat(const char * message,...) {
        if (strncmp(message,lastErrorMessage,100)==0){
            return; //don't repeat the error message
        }
        if (millis()>lastErrorTime+500) {
            return; //don't show any errors too fast - even different message;
        }
        strncpy(lastErrorMessage,message,100);
        lastErrorTime = millis();
        va_list args;
        va_start(args,message);
        write(20,message,args);
    }
    void periodicNever(int level,long frequencyMS, const char * message,...){}
    void periodicNever(int level,long frequencyMS, long*, const char * message,...){}

    void periodic(int level,long frequencyMS, const char * message,...){
       long* lastTimer = &m_periodicTimer;
        
        if (millis() > (*lastTimer + frequencyMS))
        {
            *lastTimer = millis();
            va_list args;
            va_start(args, message);
            write(level, message, args);
            
        }
    }

    void periodic(int level,long frequencyMS, long * lastTimer,const char * message,...){
        if (lastTimer == NULL) {
            lastTimer = &m_periodicTimer;
        }
        if (millis() > (*lastTimer + frequencyMS))
        {
            *lastTimer = millis();
            va_list args;
            va_start(args, message);
            write(level, message, args);
            
        }
    }

    void write(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(0,message,args);
    }

    const char * getLevelName(int level) {
        if (level > 80) {
            return "DEBUG";
        }
        if (level > 60) {
            return "INFO ";
        }if (level > 40) {
            return "WARN ";
        }
        if (level ==  ALWAYS) {
            return "ALWAYS";
        } else if (level == TEST_LEVEL) {
            return "TEST";
        }
        return "ERROR";
        
    }

    void showMemory(const char * label="Memory") {
        write(INFO_LEVEL,"%s: stack=%d,  heap=%d",label,ESP.getFreeContStack(),ESP.getFreeHeap());

    }

private:
    String m_name;
    int m_level;
    long m_periodicTimer;

};
#else
    class Logger {
        public: 
        Logger(const char * name, int level = 100) {}
        void setModuleName(const char *) {}
        void setLevel(int l) {}
        int getLevel() { return 0;}
        void indent() {}
        void outdent() {}
        void restart() {}
        void write(int,const char*,va_list){}
        void write(int level,const char*,...){}
        void test(const char * message,...) {}

        void write(const char * message,...) {}
        void debug(const char *,...){}
        void info(const char *,...){}
        void warn(const char *,...){}
        void error(const char *,...){}
        void always(const char *,...){}
        void never(const char *,...){}
        void errorNoRepeat(const char * message,...) {}
        void periodic(int level,long frequencyMS, long * lastTimer,const char * message,...){}
        void periodic(int level,long frequencyMS,const char * message,...){}
        void periodicNever(int level,long frequencyMS, const char * message,...){}
        void periodicNever(int level,long frequencyMS, long*, const char * message,...){}
        const char * getLevelName(int level) {return NULL;}
        void showMemory(const char*ignore=NULL) {}
    };
#endif
}
#endif