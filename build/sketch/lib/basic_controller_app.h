#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\basic_controller_app.h"

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
#include "./tests.h"

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
            if (!Tests::Run()) {
                return;
            }
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
