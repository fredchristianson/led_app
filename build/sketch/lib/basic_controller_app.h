
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
            m_logger = new Logger("BasicControllerApplication",80);
            m_initialized = false;
            loadScriptOnLoop[0] =0;
            m_logger->showMemory();
            m_httpServer = new HttpServer();
            m_fileSystem = new DRFileSystem();
            loadScriptOnLoop[0] = 0;

            char * result = (char*)fileBuffer.reserve(2000);
            auto found = m_fileSystem->read("/config",fileBuffer);
            if (found) {
                m_config.read(fileBuffer.text());
            } else {
                m_config.read(DEFAULT_CONFIG);
            }
            m_executor.setConfig(m_config);

            if (m_fileSystem->read("/sysstatus",statusBuffer)) {
                m_fileSystem->write("/sysstatus","starting");
                m_logger->info("statusbuffer: %s",statusBuffer.text());
                if (strcmp(statusBuffer.text(),"script")==0) {
                    m_fileSystem->read("/run_script",fileBuffer);
                    m_logger->info("restart with script: %s",fileBuffer.text());
                    strncpy(loadScriptOnLoop,fileBuffer.text(),50);

                } else if (strcmp(statusBuffer.text(),"off")==0) {
                    m_logger->info("restart with execution off");
                }  else if (strcmp(statusBuffer.text(),"std")==0) {
                    m_logger->info("restart with std script");
                    readStdScript();
                } else {
                    m_logger->error("restart after boot failure: %s",statusBuffer.text());
                }
            } else {
                m_logger->error("status file not found");
            }

            
            m_httpServer->route("/",[this](Request* req, Response* resp){
                //this->getPage("STD_HOME",req,resp);
                resp->send(200,"text/html",STD_HOME);
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
                m_logger->always("set config",body);
                m_logger->always("set load script",m_config.startupScript);
              
                strncpy(loadScriptOnLoop,m_config.startupScript,20);
                m_fileSystem->write("/config",body);
                resp->send(200,"text/plain","posted /api/config");
                //this->apiRequest(req->pathArg(0),req,resp);
            });

            m_httpServer->routeBraces( "/api/std/{}",[this](Request* req, Response* resp){
                const char * name = req->pathArg(0).c_str();
                m_logger->never("std %s",name);
                const char * script = NULL;
                if (strcmp(name,"off")==0) {
                    script = STD_OFF;
                } else if (strcmp(name,"white")==0) {
                    script = STD_WHITE;
                } else if (strcmp(name,"color")==0) {
                    script = STD_COLOR;
                }

                if (script != NULL) {
                    ParameterVariableCommand* cmd = new ParameterVariableCommand();
                    scriptParser.setData(script);

                    for(int i=0;i<req->args();i++) {
                        m_logger->never("\t%s=%s",req->argName(i).c_str(),req->arg(i).c_str());
                        const char * name = req->argName(i).c_str();
                        const char * val = req->arg(i).c_str();
                        if (name[0] == 'p'){
                            addPattern(m_currentScript,name+1,val);
                        } else {
                            cmd->add(req->argName(i).c_str(),req->arg(i).c_str());
                        }
                    }
                    m_currentScript.clear();
                    m_currentScript.read(scriptParser,cmd);
                    /*
                    int test = 89;
                    if (cmd->getInt("hue",test)) {
                        m_logger->never("---- got hue:  %d",test);
                    } else {
                        m_logger->never("---- hue failed:  %d",test);
                    }

                    Command* c = m_currentScript.getFirstCommand();
                    while(c != NULL) {
                        if (c->hasVariables()){
                            if (cmd->getInt("hue",test)) {
                                m_logger->never("---- var cmd got hue:  %d",test);
                            } else {
                                m_logger->never("---- var cmd hue failed:  %d",test);
                            }   
                        }
                        c = c->getNext();
                    }
                    */
                    m_executor.setScript(&m_currentScript);
                    m_executor.start();
                    m_fileSystem->write("/sysstatus","std");

                    Generator gen(&fileBuffer);
                    gen.startObject();
                    gen.writeNameValue("name",name);
                    gen.writeName("args");
                    gen.startProperty();
                    gen.startArray();
                    for(int i=0;i<req->args();i++) {
                        gen.startProperty();
                        gen.startObject();
                        gen.writeNameValue("name",req->argName(i).c_str());
                        gen.writeNameValue("value",req->arg(i).c_str());
                        gen.endObject();
                        gen.endProperty();
                    }
                    gen.endArray();
                    gen.endProperty();
                    gen.endObject();

                    m_logger->never("script: %s",fileBuffer.text());
                    m_fileSystem->write("std_script",fileBuffer.text());
                    statusBuffer.setText("running");
                    resp->send(200,"text/plain",fileBuffer.text());
                } else {
                    resp->send(404,"text/plain","script not found");
                }
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


        void readStdScript(){
            if (m_fileSystem->read("std_script",fileBuffer)) {
                loadScriptOnLoop[0] = 0;
                m_currentScript.clear();
                m_logger->always("starting std script: %s",fileBuffer.text());
                scriptParser.setData(fileBuffer.text());
                scriptParser.readStringValue("name",tmpBuffer,20);
                const char * script = NULL;
                if (strcmp(tmpBuffer,"off")==0) {
                    script = STD_OFF;
                } else if (strcmp(tmpBuffer,"white")==0) {
                    script = STD_WHITE;
                } else if (strcmp(tmpBuffer,"color")==0) {
                    script = STD_COLOR;
                } else if (strcmp(tmpBuffer,"pattern")==0) {
                    script = STD_PATTERN;
                }
                m_logger->never("read std script %s",script);
                ArrayParser args;
                scriptParser.getArray("args",args);
                ObjectParser arg;
                
                ParameterVariableCommand* varCmd = new ParameterVariableCommand();

                
                char pname[20];
                char pval[20];
                while(args.nextObject(&arg)) {
                    if (arg.readStringValue("name",pname,20) &&
                        arg.readStringValue("value",pval,20)) {
                        if (pname[0] == 'p'){
                            addPattern(m_currentScript,pname+1,pval);
                        } else {
                            varCmd->add(pname,pval);
                            m_logger->always("param: %s=%s",pname,pval);
                        }
                    }
                }
                m_currentScript.clear();
                scriptParser.setData(script);
                m_currentScript.read(scriptParser,varCmd);
                m_executor.setScript(&m_currentScript);
                m_executor.start();
                m_fileSystem->write("/sysstatus","std");
                m_logger->always("started std script: %s",tmpBuffer);
            } else {
                m_fileSystem->write("/sysstatus","off");
                m_logger->error("std_script not found");
            }

        }
        
        void addPattern(Script& script, const char * pattern, const char * pval) {
            // to add HSL patterns in scripts
            m_logger->error("addPattern not implemented");
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
                m_fileSystem->write("/run_script",loadScriptOnLoop);
                m_fileSystem->write("/sysstatus","script");
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
            m_logger->debug("handle API %s",api);
            if (strcmp(api,"reboot") == 0) {
                resp->send(200,"text/json","{result:true,message:\"rebooting ... \"}");   
                delay(1000);
                ESP.restart();
            }
            //Serial.println("*** handle api ");
            //Serial.println(api);
         
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
