#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H
#include "./logger.h"
#include "./wifi.h"
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <functional>
#include <uri/UriBraces.h>
#include <uri/UriRegex.h>


namespace DevRelief {
typedef ESP8266WebServer Request;
typedef ESP8266WebServer Response;
typedef std::function<void(Request*, Response*)> HttpHandler;
    
class HttpServer {
    public:


        HttpServer() {
            m_logger = new Logger("HttpServer");
            m_logger->debug("HttpServer created");
            m_server = new ESP8266WebServer(80);
            m_wifi = DRWiFi::get();

            m_server = new ESP8266WebServer(80);
            m_logger->info("Listening to port 80 on IP %s",WiFi.localIP().toString().c_str());
            if (MDNS.begin("LEDController")) {
                m_logger->info("MSND responder started");
            }

            m_server->on(UriBraces("/xx{}"), [this]() {
                String user = m_server->pathArg(0);
                m_server->send(200, "text/plain", "one part: '" + user + "'");
            });

            m_server->on(UriBraces("/xxx{}/{}"), [this]() {
                String user = m_server->pathArg(0);
                String part2 = m_server->pathArg(0);
                m_server->send(200, "text/plain", "two parts: '" + user + "' "+part2);
            });

            m_server->on(UriBraces("/users/{}"), [this]() {
                String user = m_server->pathArg(0);
                m_server->send(200, "text/plain", "User: '" + user + "'");
            });

            m_server->on(UriRegex("^\\/users\\/([0-9]+)\\/devices\\/([0-9]+)$"), [this]() {
                String user = m_server->pathArg(0);
                String device = m_server->pathArg(1);
                m_server->send(200, "text/plain", "User: '" + user + "' and Device: '" + device + "'");
            });
        }

        void begin() {
            m_logger->info("HttpServer listening");
            m_server->begin();
        }

        void handleClient() {
            m_server->handleClient();
        }

        void routeNotFound(HttpHandler httpHandler){
            m_logger->debug("routing Not Found");
            auto server = m_server;
            auto handler = httpHandler;
            m_server->onNotFound([this,handler,server](){
                m_logger->debug("not found");
                handler(server,server);
            });
        }

        void routeUri(String uri, HttpHandler httpHandler){
            m_logger->debug("routing to Uri %s",uri.c_str());
            auto server = m_server;
            auto handler = httpHandler;
            m_server->on(UriBraces(uri.c_str()),[this,handler,server](){
                m_logger->debug("uri found");
                handler(server,server);
            });
        }

        void route(String uri, HttpHandler httpHandler){
            m_logger->debug("routing %s",uri.c_str());
            auto server = m_server;
            auto handler = httpHandler;
            m_server->on(uri,[this,handler,server](){
                m_logger->debug("path found");

                handler(server,server);
            });
        }
        void route(String &uri, HTTPMethod method, HttpHandler httpHandler){
            m_logger->debug("routing %s",uri.c_str());
             auto server = m_server;
            auto handler = httpHandler;
            m_server->on(uri,method, [handler,server](){
                handler(server,server);
            });
        }

        void send(String type, String value) {
            m_server->send(200,type,value);
        }

    private:
        Logger * m_logger; 
        DRWiFi * m_wifi;   
        ESP8266WebServer * m_server;
    };
}
#endif 