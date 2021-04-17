#include "../lib/application.h"
#include "../lib/logger.h"
#include "../lib/http_server.h"
#include "../lib/file_system.h"
#include "../lib/led_strip.h"

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
                resp->send(200,"text/html","working");
            });

            m_httpServer->routeNotFound([this](Request* req, Response* resp){
                m_logger->debug("handling request  %s", req->uri().c_str());
                this->notFound(req,resp);
            });
            m_logger->info("BasicControllerApplication constructed");
            m_strip1 = new DRLedStrip(1,100);
            m_strip2 = new DRLedStrip(2,100);
            m_strip3 = new DRLedStrip(3,20);
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
            m_pos = (m_pos + 1) % 100;
            m_strip1->clear();
            m_strip2->clear();
            for(int i=0;i<20;i++) {
                m_strip1->setColor((m_pos+i) % 100,CRGB(255,0,0));
                m_strip2->setColor((m_pos+i) % 100,CRGB(0,255,0));
                m_strip3->setColor((m_pos+i) % 100,CRGB(0,0,255));
            }
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