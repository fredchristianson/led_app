#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\logger.h"
#ifndef LOGGER_H
#define LOGGER_H

#include <cstdarg>
#include <stdio.h>
#include <time.h>
extern EspClass ESP;

bool serialInitilized = false;
void initializeWriter() {
    if (!Serial) {
        serialInitilized = true;
        Serial.begin(115200);

        Serial.printf("\nSerial Logger Running\n--------------\n");
    }

}

enum LogLevel {
    DEBUG_LEVEL=100,
    INFO_LEVEL=80,
    WARN_LEVEL=60,
    ERROR_LEVEL=40,
    ALWAYS=1
};

#define MAX_MESSAGE_SIZE 1024
char messageBuffer[MAX_MESSAGE_SIZE+1];
String padding("                                   ");
const char * TABS = "\t\t\t\t\t\t";
int MAX_TAB_COUNT = 5;
char lastErrorMessage[100];
long lastErrorTime=0;
int loggerIndent;

class Logger {
public:
    Logger(const char * name, int level = 100) {
        initializeWriter();
        setModuleName(name);
        loggerIndent = 0;
        m_periodicTimer = 0;
        m_level = level;
        //this->always("create Logger %s",name);
    }

    virtual ~Logger() {
        //this->always("destroy Logger %s",m_name.c_str());
    }

    void setModuleName(const char * name) {
        m_name = name + padding;
        m_name = m_name.substring(0, padding.length());

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
        if (level > m_level) {
            return;
        }

        vsnprintf(messageBuffer,MAX_MESSAGE_SIZE,message,args);
        unsigned long now = millis()/1000;
        int hours = now/3600;
        now = now % 3600;
        int minutes = now/60;
        int seconds = now % 60;
        const char * tabs = loggerIndent<=0 ? "" : (TABS + MAX_TAB_COUNT-loggerIndent);
        Serial.printf("%s/%3d - %02d:%02d:%02d - %s: %s ",
            getLevelName(level),m_level,hours,minutes,seconds,m_name.c_str(),tabs);
        Serial.println(messageBuffer);
    }

    // void debug(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(100,message.c_str(),args);
    // }

    void debug(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(100,message,args);
    }



    // void info(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(80,message.c_str(),args);
    // }

    void info(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(80,message,args);
    }


    // void warn(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(40,message.c_str(),args);
    // }

    void warn(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(40,message,args);
    }


    // void error(String message,...) {
    //     va_list args;
    //     va_start(args,message);
    //     write(20,message.c_str(),args);
    // }
    
    void error(const char * message,...) {
        va_list args;
        va_start(args,message);
        write(20,message,args);
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
        }
        return "ERROR";
        
    }

    void showMemory(const char * label="Memory") {
        write("%s: stack=%d,  heap=%d",label,ESP.getFreeContStack(),ESP.getFreeHeap());

    }

private:
    String m_name;
    int m_level;
    long m_periodicTimer;

};

#endif