
#include <math.h>
#include <time.h>

#include "./application.h"
#include "./logger.h"
#include "./http_server.h"
#include "./file_system.h"
#include "./led_strip.h"
#include "./parse_gen.h"
#include "./config.h"
#include "./script.h"
#include "./script_executor.h"
#include "./standard.h"
#include "./data.h"

extern EspClass ESP;


namespace DevRelief {
    const char * VERSION ="2.0.0";
    DRFileBuffer statusBuffer;
    DRFileBuffer fileBuffer;


    class BasicControllerApplication : public Application {
    public: 
   
   
        BasicControllerApplication() {
            m_logger = new Logger("BasicControllerApplication",DEBUG_LEVEL);

            m_logger->showMemory();
testStringBuffer();
            m_logger->showMemory();
testStringBuffer();
            m_logger->showMemory();
//testData();
            m_logger->showMemory();
            m_httpServer = new HttpServer();
            m_fileSystem = new DRFileSystem();
            setupRoutes();        
            m_httpServer->begin();
            
            m_executor = new ScriptExecutor();
            
            m_logger->debug("Running BasicControllerApplication configured: %s",VERSION);
            m_initialized = true;
        }

        void loop() {
            if (!m_initialized) {
                return;
            }
            m_httpServer->handleClient();
        }
    protected:       
        void setupRoutes() {
            m_httpServer->route("/",[this](Request* req, Response* resp){
                resp->send(200,"text/html","home not defined");
            });


            m_httpServer->routeBracesGet( "/api/config",[this](Request* req, Response* resp){
                m_logger->debug("get /api/config");
                resp->send(200,"text/json","not implemented");
            });


            m_httpServer->routeBracesPost( "/api/config",[this](Request* req, Response* resp){
                ////m_logger->debug("handling POST API  %s", req->uri().c_str());
                auto body = req->arg("plain").c_str();
                resp->send(200,"text/json","not implemented");
            });


            m_httpServer->routeBracesGet( "/api/script/{}",[this](Request* req, Response* resp){
                resp->send(200,"text/json","not implemented");
            });


            m_httpServer->routeBracesPost( "/api/script/{}",[this](Request* req, Response* resp){
                resp->send(200,"text/json","not implemented");
            });

            m_httpServer->routeBracesGet( "/api/run/{}",[this](Request* req, Response* resp){
                resp->send(200,"text/json","not implemented");

            });


            m_httpServer->routeBracesGet("/api/{}",[this](Request* req, Response* resp){

                this->apiRequest(req->pathArg(0).c_str(),req,resp);
            });

        }

    protected:
        void apiRequest(const char * api,Request * req,Response * resp) {
            m_logger->debug("handle API %s",api);
            int code=200;
            if (strcmp(api,"reboot") == 0) {
                code = 200;
                resp->send(200,"text/json","{result:true,message:\"rebooting ... \"}");   
                delay(1000);
                ESP.restart();
                return;
            }
            
            ApiResult result;
            if (strcmp(api,"off") == 0) {
                this->turnOff();
                //result.setText(R"j({"result":true,"message":"lights turned off"})j");
                result.setCode(200);
                result.setMessage("lights turned %s","off");
            } else if (strcmp(api,"on") == 0){
                this->turnOn();
                result.setCode(200);
                result.setMessage("lights turned %s","on");
            } else {
                code = 404;
                result.setCode(404);
                result.setMessage("failed");
            }
            resp->send(200,"text/json","{result:false,message:\"unknown api \"}");
        }
    
        void turnOn(){

        }

        void turnOff(){

        }

      

        void testParser() {
            JsonParser parser;
            m_logger->debug("read DEFAULT_CONFIG");
            m_logger->showMemory();
            JsonRoot* root = parser.read(DEFAULT_CONFIG);
            if (root == NULL) {
                m_logger->error("no JSON root");
                return;
            }
            m_logger->debug("got JsonRoot");
            JsonElement * elem = root->getValue();
            m_logger->debug("got JsonElement");
            if (elem == NULL) {
                m_logger->error("JSON root has no value");
            } else if (elem->getType() == JSON_OBJECT) {
                JsonObject*obj = (JsonObject*)elem;
                JsonProperty*prop = obj->getFirstProperty();
                while(prop != NULL) {
                    m_logger->debug("property %s is type %d",prop->getName(),prop->getValue()->getType());
                    prop = prop->getNext();
                }
                JsonElement* value = obj->getPropertyValue("name");
                m_logger->debug("got JSON value type %d",(int)(value?value->getType():-1));
                value = obj->getPropertyValue("brightness");
                m_logger->debug("got JSON brightness type %d",(int)(value?value->getType():-1));
                if (value != NULL) {
                    int val;
                    value->getIntValue(val,123);
                    m_logger->debug("\t%d",val);
                }
                value = obj->getPropertyValue("test");
                if (value != NULL && value->isArray()) {
                    m_logger->debug("got array");
                    JsonArray* arr = (JsonArray*)value;
                    size_t cnt = arr->getCount();
                    m_logger->debug("\tsize %d",cnt);
                    for(size_t idx=0;idx<cnt;idx++) {
                        JsonElement* item = arr->getAt(idx);
                        m_logger->debug("\titem %d type %d",idx,(int)item->getType());
                    }
                }
            } else if (elem->getType() == JSON_ARRAY) {
                m_logger->debug("got JSON array");
            } else {
                m_logger->debug("JSON error. type=%d",(int)elem->getType());
            }
            
            m_logger->debug("deleting root");
            m_logger->showMemory();

            delete root;
            m_logger->debug("deleted root");
            m_logger->showMemory();
            
        }

        void testStringBuffer() {
            m_logger->debug("test DRStringBuffer");
            DRStringBuffer buf;
            m_logger->debug("\tsplit");
            char ** strs = buf.split("a/b/c/d","/");
            m_logger->debug("\tsplit done");
            while(*strs != NULL) {
                m_logger->debug("\tgot %s",*strs);
                strs++;
            }
            m_logger->debug("\ttest done");
        }

        void testData() {
            ApiResult api;
            api.addProperty("top",1);
            int i = api.getInt("top");
            m_logger->debug("top should be 1.  it is: %d",i);
            api.addProperty("a/b",2);
            i = api.getInt("a/b");
            m_logger->debug("a/b should be 2.  it is: %d",i);
            api.addProperty("a/c/d",3);
            i = api.getInt("a/c/d");
            m_logger->debug("a/c/d should be d.  it is: %d",i);
            api.addProperty("a/c/e",4);
            i = api.getInt("a/c/e");
            m_logger->debug("a/c/e should be 4.  it is: %d",i);

            api.addProperty("abc/def/xyz",5);
            i = api.getInt("abc/def/xyz");
            m_logger->debug("abc/def/xyz should be 5.  it is: %d",i);
        }
    private: 
        Logger * m_logger;
        DRFileSystem * m_fileSystem;
        DRPath path;
        HttpServer * m_httpServer;
        Script* m_currentScript;
        Config* m_config;
        ScriptExecutor* m_executor;
        bool m_initialized;
    };

}