
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
#include "./util.h"

extern EspClass ESP;


namespace DevRelief {
    const char * VERSION ="2.0.0";
    DRFileBuffer statusBuffer;
    DRFileBuffer fileBuffer;


    class BasicControllerApplication : public Application {
    public: 
   
   
        BasicControllerApplication() {
            m_logger = new Logger("BasicControllerApplication",APP_LOGGER_LEVEL);

            m_logger->showMemory();
            if (!Tests::Run()) {
                return;
            }
            m_logger->showMemory();
            initialize();
        }



    protected:       
        void loop() {
            if (!m_initialized) {
                return;
            }
            m_httpServer->handleClient();
        }

        void initialize() {
            ConfigDataLoader configDataLoader;
            if (!configDataLoader.loadConfig(m_config)) {
                m_logger->error("Load failed.  Using default.");
                configDataLoader.initialize(m_config);
                configDataLoader.saveConfig(m_config);
            } else {
                m_logger->info("Loaded config.json");
            }
            m_logger->debug("set config instance");
            m_config.setInstance(&m_config);
            m_logger->debug("create httpserver");
            m_httpServer = new HttpServer();
            m_logger->debug("setup routes");
            setupRoutes();        
            m_logger->debug("begin http server");
            m_httpServer->begin();
            
            m_logger->debug("show build version");
            
            m_logger->debug("Running BasicControllerApplication configured: %s",m_config.getBuildVersion().text());
            m_initialized = true;
        }

        void setupRoutes() {
            m_httpServer->route("/",[this](Request* req, Response* resp){
                resp->send(200,"text/html","home not defined");
            });


            m_httpServer->routeBracesGet( "/api/config",[this](Request* req, Response* resp){
                m_logger->debug("get /api/config");
                ConfigDataLoader configDataLoader;
                SharedPtr<JsonRoot> jsonRoot = configDataLoader.toJson(m_config);
                JsonElement*json = jsonRoot->getTopElement();
                ApiResult api(json);
                DRString apiText;
                
                api.toText(apiText);
                const char * text = apiText.text();
                m_logger->debug("sending response");
                m_logger->debug("%s",text);
                
                resp->send(200,"text/json",text);
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
        HttpServer * m_httpServer;
        Config m_config;
        bool m_initialized;
    };

}
