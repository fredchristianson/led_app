
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
#include "./standard.h"

extern EspClass ESP;


namespace DevRelief {
    const char * VERSION ="1.0.1";
    DRFileBuffer statusBuffer;
    char loadScriptOnLoop[50];
    DRFileBuffer fileBuffer;
    ObjectParser scriptParser;
    long lastLoopMessageTime;

    class BasicControllerApplication : public Application {
    public: 
   
   
        BasicControllerApplication() {
            m_logger = new Logger("BasicControllerApplication",100);
            m_initialized = false;
            loadScriptOnLoop[0] =0;
            m_logger->showMemory();
            m_httpServer = new HttpServer();
            m_fileSystem = new DRFileSystem();

            bool failed = true;
            if (m_fileSystem->read("/sysstatus",statusBuffer)) {
                if (strcmp(statusBuffer.text(),"running")==0) {
                    m_logger->error("restart after runtime failure: %s",statusBuffer.text());
                    failed = false;
                } else if (strcmp(statusBuffer.text(),"off")==0) {
                    m_logger->error("restart with execution off: %s",statusBuffer.text());
                    failed = false;
                } else {
                    m_logger->error("restart after boot failure: %s",statusBuffer.text());
                }
            } else {
                m_logger->error("status file not found");
            }
            statusBuffer.setText("starting");
            m_logger->debug("status %s",statusBuffer.reserve(1));
            m_logger->debug("status text %s",statusBuffer.text());
            m_fileSystem->write("/sysstatus","starting");
            char * result = (char*)fileBuffer.reserve(2000);
            auto found = m_fileSystem->read("/config",fileBuffer);
            if (found) {
                m_config.read(fileBuffer.text());
            } else {
                m_config.read(DEFAULT_CONFIG);
            }
            m_executor.setConfig(m_config);
            if (!failed) {
                if (m_config.startupScript[0] != 0) {
                    m_logger->info("loading startup script %s",m_config.startupScript);
                    strncpy(loadScriptOnLoop,m_config.startupScript,20);
                } else {
                    m_logger->warn("no startup script found");
                }
            } else {
                m_logger->error("not restarting because last restart failed");
            }

            m_httpServer->route("/",[this](Request* req, Response* resp){
                this->getPage("index",req,resp);
            });

            m_httpServer->routeBraces("/{}.html",[this](Request* req, Response* resp){
                this->getPage(req->pathArg(0).c_str(),req,resp);
            });


            m_httpServer->routeBracesGet( "/api/off",[this](Request* req, Response* resp){
                this->m_executor.turnOff();
                m_fileSystem->write("/sysstatus","off");
                resp->send(200,"text/json",fileBuffer.text());
            });

            m_httpServer->routeBracesGet( "/api/config",[this](Request* req, Response* resp){
                m_logger->debug("get /api/config");
                char * result = (char*)fileBuffer.reserve(2000);
                Generator gen(&fileBuffer);
                m_config.setAddr(WiFi.localIP().toString().c_str());
                m_logger->debug("IP addr is set");
                
                int max = 20;
                String scripts[max];
                int scriptCount = m_fileSystem->listFiles("/script",scripts,max);
                m_logger->debug("got script list");
                m_config.setScripts(scripts,scriptCount);
                m_logger->debug("set script list");
                m_config.write(gen);
                m_logger->debug("wrote config");
                resp->send(200,"text/json",fileBuffer.text());
 
            });


            m_httpServer->routeBracesPost( "/api/config",[this](Request* req, Response* resp){
                ////m_logger->debug("handling POST API  %s", req->uri().c_str());
                auto body = req->arg("plain").c_str();
                ObjectParser parser(body);
                m_config.read(parser);
                m_executor.setConfig(m_config);
                m_logger->debug("set load script",m_config.startupScript);
              
                strncpy(loadScriptOnLoop,m_config.startupScript,20);
                m_fileSystem->write("/config",body);
                resp->send(200,"text/plain","posted /api/config");
                //this->apiRequest(req->pathArg(0),req,resp);
            });

            m_httpServer->routeBraces( "/api/std/{}",[this](Request* req, Response* resp){
                m_logger->always("std %s",req->pathArg(0).c_str());
                resp->send(404,"text/plain","script not found");
            });

            m_httpServer->routeBracesGet( "/api/script/{}",[this](Request* req, Response* resp){
                if (m_fileSystem->read(path.concatTemp("/script/",req->pathArg(0).c_str()),fileBuffer)){
                    resp->send(200,"text/plain",fileBuffer.text());
                } else {
                    resp->send(404,"text/plain","script not found");
                }

            });


            m_httpServer->routeBracesPost( "/api/script/{}",[this](Request* req, Response* resp){
                //m_logger->debug("post scene %s", req->pathArg(0).c_str());
                const char * script = req->pathArg(0).c_str();
                const char * body = req->arg("plain").c_str();
                //m_logger->debug("commands: %s", body.c_str());
                m_fileSystem->write(path.concatTemp("/script/",script),body);
                m_logger->debug("set load script",script);
                
                strcpy(loadScriptOnLoop,script);
                //ObjectParser parser(body);
                //m_currentScript.read(parser);
                //m_executor.setScript(&m_currentScript);
                m_logger->debug("read script name %.15s",m_currentScript.name);
                resp->send(200,"text/plain",path.concatTemp("posted /api/script/",script));

            });

            m_httpServer->routeBracesGet( "/api/run/{}",[this](Request* req, Response* resp){
                if (m_fileSystem->read(path.concatTemp("/script/",req->pathArg(0).c_str()),fileBuffer)){
                    strcpy(loadScriptOnLoop,req->pathArg(0).c_str());


                    resp->send(200,"text/plain",fileBuffer.text());
                } else {
                    resp->send(404,"text/plain","script not found");
                }

            });


            m_httpServer->routeBracesGet("/api/{}",[this](Request* req, Response* resp){
                ////////m_logger->debug("handling API  %s", req->uri().c_str());
                //resp->send(200,"text/html","working");
                this->apiRequest(req->pathArg(0).c_str(),req,resp);
            });


            m_httpServer->routeNotFound([this](Request* req, Response* resp){
                //////m_logger->debug("handling not found  %s", req->uri().c_str());
                this->handleFile(req,resp);
            });

            m_httpServer->begin();
            
 
            m_logger->debug("Running BasicControllerApplication configured: %s",VERSION);
            m_initialized = true;
        }

        
        void loop() {
            if (!m_initialized) {
                return;
            }
            m_httpServer->handleClient();

            if (loadScriptOnLoop[0] != 0) {
                if (strcmp(statusBuffer.text(),"off")==0) {
                    loadScriptOnLoop[0]=0;
                    return;
                }
                m_logger->info("Loading script %s",loadScriptOnLoop);
                if (m_fileSystem->read(path.concatTemp("/script/",loadScriptOnLoop),fileBuffer)){
                    scriptParser.setData(fileBuffer.text());
                    m_currentScript.clear();
                    m_currentScript.read(scriptParser);
                    m_logger->debug("read script");
                    m_executor.setScript(&m_currentScript);
                    m_executor.start();
                    m_logger->debug("set executor");
                }
                loadScriptOnLoop[0] = 0;
                m_logger->debug("loop load done");
            } else if (m_executor.isRunning() && m_executor.isComplete()) {
                const char * next = m_executor.getNextScriptName();
                m_logger->info("next script: %s",next);
                if (next[0] != NULL && m_fileSystem->read(path.concatTemp("/script/",next),fileBuffer)){
                    m_logger->info("\t found next");
                    scriptParser.setData(fileBuffer.text());
                    m_currentScript.clear();
                    m_currentScript.read(scriptParser);
                    m_executor.setScript(&m_currentScript);
                    m_logger->info("\tnext script loaded");
                    m_logger->showMemory();
                } else {
                    m_executor.restart();
                }               
            } else {
                m_logger->periodic(DEBUG_LEVEL,5000,lastLoopMessageTime,"loop() %s",statusBuffer.text());
                m_executor.step();
                if (strcmp(statusBuffer.text(),"running")!=0){
                    statusBuffer.setText("running");
                    m_fileSystem->write("/sysstatus","running");
                }
            }
            
        }

        
        String getMimeType(String path) {
            if (path.endsWith(".css")){
                return "text/css";
            }
            if (path.endsWith(".js")){
                return "application/javascript";
            }
            
            if (path.endsWith(".html")){
                return "text/html";
            }
            return "text/plain";
        }

        void apiRequest(const char * api,Request * req,Response * resp) {

            ////m_logger->debug("handle API %s",api.c_str());
            Serial.println("*** handle api ");
            Serial.println(api);
            /*
            if (api == "restart") {
                ////m_logger->warn("restart API called");
                resp->send(200,"text/json","{result:true,message:\"restarting\"}");
                ESP.restart();
            } else if (api == "serial"){
                ////m_logger->debug("restart serial port");
                Serial.println("***");
                Serial.println(api.c_str());
                Serial.begin(115200);
                Serial.println("serial restarted");
                ////m_logger->debug("serial restarted");
                resp->send(200,"text/json","{result:true,message:\"serial restarted\"}");
            }  else if (api == "show"){
                ////m_logger->info("show leds");
                m_strip->show();
                resp->send(200,"text/json","{result:true,message:\"FastLED.show()\"}");
            } else {
                resp->send(200,"text/json","{result:false,message:\"unknown api \"}");
            }*/
             resp->send(200,"text/json","{result:false,message:\"unknown api \"}");
        }

        void getPage(const char * page,Request * req,Response * resp){
           ////m_logger->debug("get page %s",page.c_str());
           streamFile(resp,path.concatTemp("/",page,".html"));
        }

        void handleFile(Request *req , Response* resp) {
            const char * path = req->uri().c_str();
            streamFile(resp,path);
        }

        void streamFile(Response * resp,const char * path) {
            ////m_logger->debug("stream file %s",path.c_str());
            File file = m_fileSystem->openFile(path);
            if (file.isFile()) {
                auto mimeType = getMimeType(path);
                resp->streamFile(file,mimeType);
                m_fileSystem->closeFile(file);
            } else {
                ////m_logger->debug("file not found");
                resp->send(404,"text/html","file not found ");
            }
          
        }

       /*
        void runScene(String commandText) {

            const char * text = commandText.c_str();
            //m_logger->warn("execute commands");
            //m_logger->warn(text);
            int start = 0;
            int end = commandText.indexOf('\n');
            m_setLedCount = m_ledCount;
            m_strip->clear(); 
            
            for(int idx=0;idx<300;idx++){
                hslData.leds[idx].hue = 0;
                hslData.leds[idx].saturation = 0;
                hslData.leds[idx].lightness = 0;
            }
            while(end>=0) {
                char cmd[100];
                memcpy(cmd,text+start,end-start);// = commandText.substring(start,end);
                cmd[end-start] = 0;
                //m_logger->debug("run command %s",cmd);
                runCommand(cmd,hslData.leds);
                //m_logger->debug("command done");
                start = end + 1;
                //////m_logger->debug("start/end  %d/%d",start,end);
                end = commandText.indexOf('\n',start);
               // ////m_logger->debug("next start/end  %d/%d",start,end);
            }

            int number;
            //m_logger->debug("set strip values %d",m_setLedCount);
            for(number=0;number<m_ledCount && number < this->m_setLedCount;number++) {
              CHSL& color = hslData.leds[number];
              // ////m_logger->debug("HSV %d (%d,%d,%d)",number,color.hue,color.saturation,color.value);
              auto rgb = HSLToRGB(color);
              m_strip->setColor(number,rgb);
            }
        }
*/
        
    private: 
        Logger * m_logger;
        DRFileSystem * m_fileSystem;
        DRPath path;
        HttpServer * m_httpServer;
        Script m_currentScript;
        Config m_config;
        ScriptExecutor m_executor;
        bool m_initialized;
    };

}
