
#ifndef DRHTTP_SERVER_H
#define DRHTTP_SERVER_H


#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <functional>
#include <memory>
#include <functional>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>
#include "./logger.h"
#include "./wifi.h"


namespace DevRelief {
typedef ESP8266WebServer Request;
typedef ESP8266WebServer Response;
using HttpHandler = std::function<void(Request*, Response*)> ;

class HttpServer {
    public:


        HttpServer() {
            m_logger = new Logger("HttpServer",60);
            m_logger->debug("HttpServer created");
            m_wifi = DRWiFi::get();

            m_server = new ESP8266WebServer(80);

            m_logger->info("Listening to port 80 on IP %s",WiFi.localIP().toString().c_str());
            /*
            if (MDNS.begin("LEDController")) {
                m_logger->info("MSND responder started");
            }
            */
           
        }

        void begin() {
            m_logger->info("HttpServer listening");
            m_server->begin();
        }

        void handleClient() {
            if (m_server->client()) {
                m_server->client().keepAlive();
            }
            m_server->handleClient();
        }

        void routeNotFound(HttpHandler httpHandler){
            m_logger->debug("routing Not Found handler");
            auto server = m_server;
            auto handler = httpHandler;
            m_server->onNotFound([this,handler,server](){
                if (m_server->method() == HTTP_OPTIONS){
                    this->m_logger->debug("send CORS for HTTP_OPTIONS");
                    this->cors(m_server);
                    m_server->send(204);
                    return;
                }
                this->cors(server);
                handler(server,server);
            });
        }

        void routeBraces(const char * uri, HttpHandler httpHandler){
            m_logger->debug("routing to Uri %s",uri);
            auto server = m_server;
            auto handler = httpHandler;
            m_server->on(UriBraces(uri),[this,handler,server](){
                this->cors(server);
                m_logger->debug("returning uri found");
                handler(server,server);
            });
        }

        void routeBracesGet(const char * uri, HttpHandler httpHandler){
            m_logger->debug("routing GET to Uri %s",uri);
            auto server = m_server;
            auto handler = httpHandler;
            m_server->on(UriBraces(uri),HTTP_GET,[this,handler,server](){
                this->cors(server);
                //m_logger->debug("uri found");
                handler(server,server);
            });
        }

        void routeBracesPost(const char * uri, HttpHandler httpHandler){
            m_logger->debug("routing POST to Uri %s",uri);
            auto server = m_server;
            auto handler = httpHandler;
            m_server->on(UriBraces(uri),HTTP_POST,[this,handler,server](){
                this->cors(server);
                m_logger->debug("uri found");
                handler(server,server);
            });
        }

        void route(const char * uri, HttpHandler httpHandler){
            m_logger->debug("routing %s",uri);
            auto server = m_server;
            auto handler = httpHandler;
            m_server->on(uri,[this,handler,server,uri](){
                this->cors(server);
                m_logger->debug("path found %s",uri);

                handler(server,server);
            });
        }
        void route(const char * uri, HTTPMethod method, HttpHandler httpHandler){
            m_logger->debug("routing %s",uri);
             auto server = m_server;
            auto handler = httpHandler;
            m_server->on(uri,method, [this,handler,server](){
                this->cors(server);
                handler(server,server);
            });
        }

        void send(const char * type, const char * value) {

            m_server->send(200,type,value);
        }

        void cors(ESP8266WebServer * server) {
            m_logger->debug("cors");
            server->sendHeader("Access-Control-Allow-Origin","*");
            server->sendHeader("Access-Control-Allow-Headers","Content-Type");
        }

    private:
        Logger * m_logger; 
        DRWiFi * m_wifi;   
        ESP8266WebServer * m_server;
    };
}
#endif 