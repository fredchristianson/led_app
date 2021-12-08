#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\file_system.h"
#ifndef DR_FILE_SYSTEM_H
#define DR_FILE_SYSTEM_H

#include <FS.h>
#include <LittleFS.h>
#include "./logger.h"
#include "./buffer.h"

#define MAX_PATH 100

namespace DevRelief {

typedef enum FileType {
    FILE_TEXT = 1,
    FILE_JSON = 2,

    FILE_UNKNOWN_TYPE=999
};

class DRPath {
public:
    DRPath() {};
    char tempPath[200];
    const char * concatTemp(const char *a,const char *b=NULL,const char *c=NULL) {
        strcpy(tempPath,a);
        if (b) {
            if (!endsWith(tempPath,"/") && b[0]!= '/') {
                strcat(tempPath,"/");
            }
            strcat(tempPath,b);
        }
        if (c) {
            strcat(tempPath,c);
        }
        return tempPath;
    };

    bool endsWith(const char * path,const char * end) {
        if (path == NULL || end == NULL) {
            return false;
        }
        size_t plen = strlen(path);
        size_t elen = strlen(end);
        if (elen > plen) {
            return false;
        }
        return strcmp(path+plen-elen,end)==0;
    }
};

class DRFileBuffer :  public DRBuffer {
public:
    DRFileBuffer(){
    }
    
};

bool drFileSystemInitialized=false;


class DRFileSystem {


public:
    DRFileSystem() {
        m_logger = new Logger("DRFileSystem",60);
        strcpy(m_path,"/");
        
        if (!drFileSystemInitialized) {
            LittleFS.begin();
            drFileSystemInitialized = true;
        }
        m_logger->debug("DRFileSystem open");
    }

    const char * getExtension(const char * name) {
        auto dot = strrchr(name,'.');
        return dot == NULL ? name : dot;
    }
    FileType getFileType(const char * name) {
        auto dot = strrchr(name,'.');
        if (dot == NULL || strcmp(dot,".txt")==0) {
            return FILE_TEXT;
        } else if (strcmp(dot,".json")==0) {
            return FILE_JSON;
        } else {
            return FILE_UNKNOWN_TYPE;
        }
    }

    const char * getFullPath(const char * path){
        if (path == NULL || path[0] == 0) {
            return m_root;
        }
        if (path[0] == '/') {
            return path;
        }
        strcpy(m_path,m_root);
        strcat(m_path,path);
    };

    bool exists(const char * path) {
        auto fullPath = getFullPath(path);
        return LittleFS.exists(fullPath);
    }

    File open(const char *  path) {
        auto fullPath = getFullPath(path);
        return LittleFS.open(fullPath,"r");
    }

    int listFiles(const char *  path, String* results, int maxResults) {
        m_logger->debug("listFiles: %s",path);
        Dir dir = LittleFS.openDir(path);
        int count = 0;
        while (dir.next()) {
            m_logger->debug("found file: %s",dir.fileName());
            results[count++] = dir.fileName();
        }
        return count;
    }

    File openFile(const char * path) {
        m_logger->debug("open file  %s",path);
        auto fullPath = getFullPath(path);
        m_logger->debug("full path %s",fullPath);
        File file = open(fullPath);
        return file;
    }

    void closeFile(File & file) {
        file.close();
    }

    uint16_t readChunk(const char *  path,size_t pos, size_t maxLength, DRFileBuffer& buffer ) {
        m_logger->debug("read from %s",path);
        auto fullPath = getFullPath(path);
        m_logger->debug("open %s",fullPath);
        File file = open(fullPath);
        if (!file.isFile()) {
            m_logger->warn("file not found %s",fullPath);
            return false;
        }
        size_t fileSize = file.size();
        m_logger->debug("file size %d",fileSize);
        size_t readSize = fileSize-pos;
        if (readSize > maxLength) {
            readSize = maxLength;
        }
        auto data = buffer.reserve(readSize+1);
        file.seek(pos,SeekSet);
        size_t readBytes = file.read(data,readSize);
        if (pos+readBytes==fileSize) {
            readBytes+=1;
            data[readBytes] = 0;
            buffer.setLength(readBytes);
        } else {
            buffer.setLength(readBytes);
        }
        file.close();
        m_logger->debug("read %d bytes.  buffer has %d bytes",readBytes, buffer.getLength());
        return readBytes;
    }

    bool read(const char * path, DRFileBuffer& buffer ) {
        m_logger->debug("read from %s",path);
        auto fullPath = getFullPath(path);
        File file = open(fullPath);
        if (!file.isFile()) {
            m_logger->warn("r file not found %s",fullPath);
            return false;
        }
        size_t size = file.size();
        auto data = buffer.reserve((long)size+1);
        size_t readBytes = file.read(data,size);
        data[size] = 0;
        file.close();
        buffer.setLength(size);
        m_logger->debug("read %d bytes.  buffer has %d bytes",readBytes, buffer.getLength());
        return true;
    }
    
    bool readBinary(const char * path, byte * data,size_t length ) {
        m_logger->debug("read from %s",path);
        auto fullPath = getFullPath(path);
        File file = open(fullPath);
        if (!file.isFile()) {
            m_logger->warn("rb file not found %s",fullPath);
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

    bool write(const char *  path, const char * data) {
        auto fullPath = getFullPath(path);
        File file = LittleFS.open(fullPath,"w");
        file.write(data,strlen(data));
        file.close();
        return true;
    }
    
    bool writeBinary(const char * path, const byte * data,size_t length) {
        auto fullPath = getFullPath(path);
        File file = LittleFS.open(fullPath,"w");
        file.write(data,length);
        file.close();
        return true;
    }

private:
    char m_root[MAX_PATH];    
    char m_path[MAX_PATH];    
    Logger* m_logger;
};
}
#endif