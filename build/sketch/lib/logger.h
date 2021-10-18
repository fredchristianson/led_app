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

#define MAX_MESSAGE_SIZE 1024
char messageBuffer[MAX_MESSAGE_SIZE+1];
String padding("                                   ");

class Logger {
public:
    Logger(const char * name, int level = 100) {
        initializeWriter();
        m_name = name + padding;
        m_name = m_name.substring(0, padding.length());
        m_level = level;
    }

    void setLevel(int l) { m_level = l;}
    bool isDebug() { return m_level==100;}
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
        Serial.printf("%s/%d - %02d:%02d:%02d - %s: ",getLevelName(level),m_level,hours,minutes,seconds,m_name.c_str());
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
        return "ERROR";
        
    }

    void showMemory() {
        write("Memory: stack=%d,  heap=%d",ESP.getFreeContStack(),ESP.getFreeHeap());

    }

private:
    String m_name;
    int m_level;
};

#endif