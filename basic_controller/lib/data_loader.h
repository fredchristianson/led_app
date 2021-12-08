#ifndef DR_DATA_LOADER_H
#define DR_DATA_LOADER_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"
#include "./data.h"
#include "./file_system.h"


namespace DevRelief {
const char * ERROR_NO_REQUEST = "Nothing has been loaded";

Logger DataLoaderLogger("DataLoader",DEBUG_LEVEL);



class DataLoader;

class LoadResult {
    public:
    LoadResult() {
        m_type = FILE_UNKNOWN_TYPE;
        m_json = NULL;
        m_success = false;
        m_error = ERROR_NO_REQUEST;
    }

    ~LoadResult() {
        delete m_json;
    }

    DRFileBuffer& getBuffer() { return m_buffer;}
    const char * getTest() {
        return m_buffer.text();
    }

    JsonElement* getJson() {
        return m_jsonRoot == NULL ? NULL : m_jsonRoot->getValue();
    }

    JsonRoot* getJsonRoot() {
        return m_jsonRoot;
    }

    void setJsonRoot(JsonRoot* json) {
        m_json = json;
    }

    bool isSuccess() { return m_success;}
    const char * error() { return m_error;}


private:
    friend DataLoader;
    DRFileBuffer m_buffer;    
    FileType m_type;
    JsonRoot* m_jsonRoot;
    JsonRoot* m_json;
    bool m_success;
    const char * m_error;
};


class DataLoader {
public:
    DataLoader()  {
        m_logger = &DataLoaderLogger;
    }
    virtual ~DataLoader() {
    }


    bool loadFile(const char * path,LoadResult & result) {
        result.m_type = m_fileSystem.getFileType(path);
        
        if (!m_fileSystem.read(path,result.getBuffer())){
            m_logger->error("cannot read file %s",path);
            result.m_success = false;
            result.m_error = "file read() failed";
        } else {

            if (result.m_type == FILE_JSON) {
                JsonParser parser;
                JsonRoot* root = parser.read(result.getBuffer().text());
                result.setJsonRoot(root);
                result.m_success = root != NULL;
                result.m_error = root == NULL ? "JSON parse failed" : NULL;
            } else {
                result.m_success = true;
                result.m_error = "File is not .json";
            }
       }
       return result.m_success;
    }

protected:


    Logger * m_logger;
private:
    static DRFileSystem m_fileSystem;

};


class ConfigDataLoader : public DataLoader {
    public:
        ConfigDataLoader() {
            
        }

        
        bool initialize(Config& config) {
            return true;
        }

        bool loadConfig(Config& config, const char * path = "/config.json"){
            LoadResult result;
            initialize(config);
            if (loadFile(path,result)){
                JsonElement* json = result.getJson();
                if (json == NULL) {
                    m_logger->error("Config json invalid");
                }
            } else {
                m_logger->error("Config json not found");
            }
            return result.isSuccess();
        }

    protected:
        void setDefaults() {
            
        }
    private:
        
};
};
#endif