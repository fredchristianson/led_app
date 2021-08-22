#ifndef DR_FILE_SYSTEM_H
#define DR_FILE_SYSTEM_H

#include <FS.h>
#include <LittleFS.h>
#include "./logger.h"

namespace DevRelief {

class DRFileBuffer {
public:
    DRFileBuffer(){
        m_data = NULL;
        m_maxLength = 0;
        m_length = 0;
        m_logger = new Logger("DRFileBuffer",40);
    }
    uint8_t* data() {
        m_logger->debug("return data.  length=%d, maxLength=%d",m_length,m_maxLength);
        m_logger->debug("first byte: %2",m_data[0]);
        return m_data;
    }

    String text() {
        String text;
        text.concat((char*)m_data,m_length);
        return text;
    }

    uint8_t* reserve(size_t length) {
        if (length > m_maxLength) {
            m_logger->debug("increase buffer length. old length=%d, new length=%d",m_maxLength,length);
            auto newData = new uint8_t[length];
            m_logger->debug("allocated buffer");
            newData[0] = 2;
            m_logger->debug("set first value");
            m_logger->debug("first byte: %d",newData[0]);
            if (m_data != NULL) {
                if (length > 0) {
                    memcpy(newData,m_data,length);
                }
                delete m_data;

            }
            m_data = newData;
            m_maxLength = length;
            m_logger->debug("first byte of m_data: %d",m_data[0]);
        }
        return m_data;
    }

    void setLength(size_t length) {
        m_length = length;
        m_logger->debug("set length.  length=%d, maxLength=%d",m_length,m_maxLength);
    }

    size_t getLength() {
        m_logger->debug("get length.  length=%d, maxLength=%d",m_length,m_maxLength);
        return m_length;
    }
private:
    uint8_t * m_data;
    size_t m_maxLength;
    size_t m_length;
    Logger* m_logger;
};

bool drFileSystemInitialized=false;

class DRFileSystem {

public:
    DRFileSystem() {
        m_logger = new Logger("DRFileSystem",40);
        m_root = "/";
        if (!drFileSystemInitialized) {
            LittleFS.begin();
            drFileSystemInitialized = true;
        }
        m_logger->debug("DRFileSystem open");
    }

    String getFullPath(String path){
        if (path.startsWith("/")) {
            return path;
        }
        return m_root + path;
    };

    bool exists(String path) {
        auto fullPath = getFullPath(path);
        return LittleFS.exists(fullPath);
    }

    File open(String path) {
        auto fullPath = getFullPath(path);
        return LittleFS.open(fullPath,"r");
    }

    bool read(String path, DRFileBuffer& buffer ) {
        m_logger->debug("read from %s",path.c_str());
        auto fullPath = getFullPath(path);
        File file = open(fullPath);
        if (!file.isFile()) {
            m_logger->error("file not found %s",fullPath.c_str());
            return false;
        }
        size_t size = file.size();
        auto data = buffer.reserve((long)size);
        size_t readBytes = file.read(data,size);
        file.close();
        buffer.setLength(size);
        m_logger->debug("read %d bytes.  buffer has %d bytes",readBytes, buffer.getLength());
        return true;
    }
    
    bool readBinary(String path, byte * data,size_t length ) {
        m_logger->debug("read from %s",path.c_str());
        auto fullPath = getFullPath(path);
        File file = open(fullPath);
        if (!file.isFile()) {
            m_logger->error("file not found %s",fullPath.c_str());
            return false;
        }
        size_t size = file.size();
        if (size != length) {
            m_logger->debug("invalid length %d %d",size,length);
            return false;
        }
        size_t readBytes = file.read(data,size);
        file.close();
        m_logger->debug("read %d bytes.",readBytes);
        return true;
    }

    bool write(String path, const String& data) {
        auto fullPath = getFullPath(path);
        File file = LittleFS.open(fullPath,"w");
        file.write(data.c_str(),data.length());
        file.close();
        return true;
    }
    
    bool writeBinary(String path, const byte * data,size_t length) {
        auto fullPath = getFullPath(path);
        File file = LittleFS.open(fullPath,"w");
        file.write(data,length);
        file.close();
        return true;
    }

private:
    String m_root;    
    Logger* m_logger;
};
}
#endif