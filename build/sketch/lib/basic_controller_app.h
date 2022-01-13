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
#include "./script/script.h"
#include "./script_executor.h"
#include "./standard.h"
#include "./data.h"
#include "./test/tests.h"
#include "./util.h"
#include "./script_data_loader.h"
#include "./app_state.h"
#include "./app_state_data_loader.h"

extern EspClass ESP;


namespace DevRelief {
    const char * VERSION ="2.0.0";
    DRFileBuffer statusBuffer;
    DRFileBuffer fileBuffer;


    class BasicControllerApplication : public Application {
    public: 
   
   
        BasicControllerApplication() {
            m_logger = new Logger("APP",APP_LOGGER_LEVEL);
            m_logger->showMemory();
            if (!Tests::Run()) {
                return;
            }
            m_logger->showMemory();
            initialize();
            resume();
        }

        // resume the api/script that was running when power was turned off
        const char * resume(bool forceStart=false, bool forceRun=false) {
            m_logger->debug("resume");

            m_executor.turnOff();
            AppStateDataLoader loader;
            m_logger->debug("\tload");
            if (loader.load(m_appState)){
                if (forceStart) {
                    m_appState.setIsStarting(false);
                }
                if (forceRun) {
                    m_appState.setIsRunning(true);
                }
                if (!m_appState.isStarting() && m_appState.isRunning() && m_appState.getType() != EXECUTE_NONE) {
                    m_logger->debug("\tset isStarting");
                    m_appState.setIsStarting(true);
                    m_logger->debug("\tsave");
                    loader.save(m_appState);
                    if (m_appState.getType() == EXECUTE_API) {
                        m_logger->debug("\texecute API %s",m_appState.getExecuteValue().text());
                        ApiResult result;
                        runApi(m_appState.getExecuteValue().text(),m_appState.getParameters()->asObject(),result);
                        m_logger->debug("\tAPI ran");
                    } else if (m_appState.getType() == EXECUTE_SCRIPT) {
                        m_logger->debug("\tscript state run");
                        JsonObject* params = m_appState.getParameters();
                        const char * name = m_appState.getExecuteValue();
                        ApiResult result;
                        runScript(name,params,result);                        
                    }
                } 
            }
            return m_appState.getExecuteValue().text();
        }


    protected:       
        void loop() {
            if (!m_initialized) {
                return;
            }
            if (m_appState.isStarting() && m_scriptStartTime+10*1000 < millis()) {
                m_appState.setIsStarting(false);
                AppStateDataLoader loader;
                loader.save(m_appState);
            }
            m_httpServer->handleClient();
            m_executor.step();
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
            m_executor.configChange(m_config);
            m_logger->debug("Running BasicControllerApplication configured: %s.  Built at %s %s",
                m_config.getBuildVersion().text(),
                m_config.getBuildDate().text(),
                m_config.getBuildTime().text());
            m_logger->showMemory();
            m_scriptStartTime = 0;
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
                
                auto body = req->arg("plain").c_str();
                ConfigDataLoader loader;
                loader.updateConfig(m_config,body);
                m_executor.configChange(m_config);

                ApiResult result(true);
                DRString apiText;
                result.toText(apiText);
                resume();
                resp->send(200,"text/json",apiText.text());
            });


            m_httpServer->routeBracesGet( "/api/script/{}",[this](Request* req, Response* resp){
                ScriptDataLoader loader;
                LoadResult load;
                m_logger->debug("load script");
                if (loader.loadScriptJson( req->pathArg(0).c_str(),load)){
                    ApiResult result(load.getJson());
                    result.send(req);
                }
                resp->send(404,"text/json","script not loaded");
            });

            m_httpServer->routeBracesGet( "/api/run/{}",[this](Request* req, Response* resp){
                m_logger->debug("run script");
                ApiResult result;
                JsonRoot* params = getParameters(req);
                const char * name = req->pathArg(0).c_str();
                runScript(name,params->getTopObject(),result);
                result.send(req);
            });


            m_httpServer->routeBracesPost( "/api/script/{}",[this](Request* req, Response* resp){
                m_logger->debug("save script");
                m_executor.endScript();
                m_logger->debug("old script ended");
                auto body = req->arg("plain").c_str();
                m_logger->debug("\tgot new script:%s",body);
                auto name =req->pathArg(0).c_str();
                m_logger->debug("\tname: %s",name);
                if (name != NULL) {
                    ScriptDataLoader loader;
                    m_logger->debug("\twrite script");
                    Script* script = loader.writeScript(name,body);
                    m_logger->debug("\tgot script");
                    delete script;        
                    m_logger->debug("\tdeleted script");
    
                }
                m_logger->debug("\tget params");
                JsonRoot* params = getParameters(req);
                m_logger->debug("\tgot params");
                ApiResult result;
                m_logger->debug("\trun script");
                runScript(name,params->getTopObject(),result);
                m_logger->debug("\tsend result");
                result.send(req);
                m_logger->debug("\tdelete params");
                delete params;
            });


            m_httpServer->routeBracesDelete( "/api/script/{}",[this](Request* req, Response* resp){
                m_logger->debug("delete script");
                resp->send(200,"text/json","DELETE not implemented");
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
            m_logger->never("get parameters");
            SharedPtr<JsonRoot> paramJson = getParameters(req);

            JsonObject *params = paramJson->getTopObject();
            ApiResult result;
            if (runApi(api,params,result))
            {
                m_appState.setApi(api,params);
                AppStateDataLoader loader;
                loader.save(m_appState);
            }
            result.send(resp);
            //DRString apiText;

            //result.toText(apiText);
            //resp->send(200,"text/json",apiText.text());


        }

        bool runApi(const char * api, JsonObject* params, ApiResult& result){
            bool saveState=false;
            if (strcmp(api,"off") == 0) {
                m_executor.turnOff();
                saveState = true;
                result.setCode(200);
                result.setMessage("lights turned %s","off");
            } else if (strcmp(api,"on") == 0){
                int level = params->get("level",100);
                saveState = true;
                m_executor.white(level);
                result.setCode(200);
                result.setMessage("lights turned %s","on");
            } else if (strcmp(api,"resume") == 0){
                resume(true,true);
                result.setCode(200);
                result.setMessage("resumed last execution");
            } else if (strcmp(api,"color") == 0){
                m_executor.solid(params);
                saveState = true;
                result.setCode(200);
                result.setMessage("lights turned %s","on");
            } else if (strcmp(api,"mem") == 0){
               result.setCode(202);
            } else {
                result.setCode(404);
                result.setMessage("failed");
            }
            return saveState;
        }
    

        bool runScript(const char * name, JsonObject* params, ApiResult& result) {
            ScriptDataLoader loader;
            LoadResult load;
            m_logger->debug("load script %s",name);
            if (loader.loadScriptJson(name ,load)){
                m_logger->debug("\tloaded file");
                Script* script = loader.jsonToScript(load);
                m_logger->debug("\tloaded script");
                
                if (script == NULL) {
                    m_logger->debug("\tno script");

                    result.setCode(500);
                    DRFormattedString msg("script parse failed: %s",name);
                    result.setMessage(msg);
                } else {
                    m_logger->debug("\tm_executor.setScript");
                    m_executor.setScript(script,params);
                    m_scriptStartTime = millis();
                    m_logger->debug("\tset appState");
                    m_appState.setScript(name,params);
                    AppStateDataLoader loader;
                    m_logger->debug("\tsave appState");
                    loader.save(m_appState);
                    m_logger->debug("\tsaved");
                    return true;
                }
            } else {
                result.setCode(404);
                DRFormattedString msg("script not found: %s",name);
                return false;
            }
            m_logger->debug("\trunScript failed");
            return false;
        };

      
        JsonRoot* getParameters(Request*req){
            JsonRoot * root = new JsonRoot();
            JsonObject* obj = root->createObject();
            m_logger->never("\tloop parameters");
            for(int i=0;i<req->args();i++) {
                m_logger->debug("\t%s=%s",req->argName(i).c_str(),req->arg(i).c_str());
                obj->set(req->argName(i).c_str(),req->arg(i).c_str());
            }
            return root;
        }

    private: 
        Logger * m_logger;
        HttpServer * m_httpServer;
        Config m_config;
        AppState m_appState;
        ScriptExecutor m_executor;
        long m_scriptStartTime;
        bool m_initialized;
    };


}
