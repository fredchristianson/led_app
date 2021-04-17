#ifndef LOGGER_H
#define LOGGER_H

#include <cstdarg>
#include <stdio.h>
#include <time.h>

bool serialInitilized = false;
void initializeWriter() {
    if (!serialInitilized) {
        serialInitilized = true;
        Serial.begin(115200);
        Serial.printf("\nLogger Running\n--------------\n");
    }

}

char messageBuffer[1024];
String padding("                                   ");

class Logger {
public:
    Logger(const char * name, int level = 100) {
        initializeWriter();
        m_name = name + padding;
        m_name = m_name.substring(0, padding.length());
        m_level = level;
    }

    void write(int level, String message, va_list args ){
        if (level > m_level) {
            return;
        }
        vsnprintf(messageBuffer,sizeof(messageBuffer),message.c_str(),args);
        unsigned long now = millis()/1000;
        int hours = now/3600;
        now = now % 3600;
        int minutes = now/60;
        int seconds = now % 60;
        Serial.printf("%s - %02d:%02d:%02d - %s: ",getLevelName(level).c_str(),hours,minutes,seconds,m_name.c_str());
        Serial.println(messageBuffer);
    }

    void debug(String message,...) {
        va_list args;
        va_start(args,message);
        write(100,message,args);
    }


    void info(String message,...) {
        va_list args;
        va_start(args,message);
        write(80,message,args);
    }


    void warn(String message,...) {
        va_list args;
        va_start(args,message);
        write(40,message,args);
    }


    void error(String message,...) {
        va_list args;
        va_start(args,message);
        write(20,message,args);
    }

    String getLevelName(int level) {
        if (level > 80) {
            return "DEBUG";
        }
        if (level > 60) {
            return "INFO ";
        }if (level > 40) {
            return "WARN ";
        }
        return "ERROR";
        
    }

private:
    String m_name;
    int m_level;
};

#endif