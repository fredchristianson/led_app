


#include "../lib/application.h"
#include "../lib/logger.h"
#include "../lib/http_server.h"
#include "../lib/file_system.h"
#include "../lib/led_strip.h"

extern EspClass ESP;


namespace DevRelief {

    DRFileBuffer fileBuffer;

    class BasicControllerApplication : public Application {
    public: 
        BasicControllerApplication() {
            m_pos = 0;
            m_logger = new Logger("BasicControllerApplication");
            m_httpServer = new HttpServer();
            m_fileSystem = new DRFileSystem();

            m_httpServer->route("/",[this](Request* req, Response* resp){
                m_logger->debug("handling request / %s", req->uri().c_str());
                //resp->send(200,"text/html","working");
                this->getPage("/index.html",req,resp);
            });


            m_httpServer->routeUri("/api/{}",[this](Request* req, Response* resp){
                m_logger->debug("handling API / %s", req->uri().c_str());
                //resp->send(200,"text/html","working");
                this->apiRequest(req->pathArg(0),req,resp);
            });


            m_httpServer->routeNotFound([this](Request* req, Response* resp){
                m_logger->debug("handling not found  %s", req->uri().c_str());
                this->notFound(req,resp);
            });
            m_httpServer->begin();
            m_strip1 = new DRLedStrip(1,150);
            m_strip2 = new DRLedStrip(2,150);
            m_strip3 = new DRLedStrip(3,150);
            setSolidColor(m_strip1,CRGB(255,0,0));
            setSolidColor(m_strip2,CRGB(0,255,0));            
            setSolidColor(m_strip3,CRGB(0,0,255));
        }

        void setSolidColor(DRLedStrip * strip, CRGB color) {
            strip->clear();
            for(int i=0;i<strip->getCount();i++) {
                strip->setColor(i,color);
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
            
        }

        void apiRequest(String api,Request * req,Response * resp) {
            m_logger->debug("handle API %s",api.c_str());
            if (api == "restart") {
                m_logger->warn("restart API called");
                resp->send(200,"text/json","{result:true,message:\"restarting\"}");
                ESP.restart();
            }
            resp->send(200,"text/json","{result:false}");
        }

        void getPage(String page,Request * req,Response * resp){
            m_logger->debug("get page %s",page.c_str());
           String path = req->uri();
            auto found = m_fileSystem->read(path, fileBuffer);        
            if (found) {
                auto mimeType = getMimeType(path);
                m_logger->debug("sending %d characters",fileBuffer.getLength());
                uint8* data = fileBuffer.data();

                resp->send(200,mimeType.c_str(),fileBuffer.data(),fileBuffer.getLength());
            } else {
                m_logger->error("URI not found: %s",req->uri().c_str());
                char buffer[200];
                sprintf(buffer,"URI not found: %s",req->uri().c_str());
                resp->send(200,"text/html",buffer);
            }
        }

        void notFound(Request *req , Response* resp) {
            String path = req->uri();
            auto found = m_fileSystem->read(path, fileBuffer);        
            if (found) {
                auto mimeType = getMimeType(path);
                m_logger->debug("sending %d characters",fileBuffer.getLength());
                uint8* data = fileBuffer.data();

                resp->send(200,mimeType.c_str(),fileBuffer.data(),fileBuffer.getLength());
            } else {
                m_logger->error("URI not found: %s",req->uri().c_str());
                char buffer[200];
                sprintf(buffer,"not found: %s",req->uri().c_str());
                resp->send(200,"text/html",buffer);
            }

        }

        void loop() {
            m_httpServer->handleClient();
/*             m_pos = (m_pos + 1) % 150;
            m_strip1->clear();
            m_strip2->clear();
            m_strip3->clear();
            for(int i=0;i<20;i++) {
                m_strip1->setColor((m_pos+i) % 150,CRGB(255,0,0));
                m_strip2->setColor((m_pos+i) % 150,CRGB(0,255,0));
                m_strip3->setColor((m_pos+i) % 150,CRGB(0,0,255));
            } */
            DRLedStrip::show();
        }

    private: 
        Logger * m_logger;
        DRFileSystem * m_fileSystem;
        HttpServer * m_httpServer;
        int m_count = 1;
        DRLedStrip * m_strip1;
        DRLedStrip * m_strip2;
        DRLedStrip * m_strip3;
        int m_pos;
    };

}