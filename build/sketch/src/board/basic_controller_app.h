#include "../lib/application.h"
#include "../lib/logger.h"
#include "../lib/http_server.h"
#include "../lib/file_system.h"

namespace DevRelief {

    DRFileBuffer fileBuffer;

    class BasicControllerApplication : public Application {
    public: 
        BasicControllerApplication() {
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
        }

    private: 
        Logger * m_logger;
        DRFileSystem * m_fileSystem;
        HttpServer * m_httpServer;
        int m_count = 1;
    };

}