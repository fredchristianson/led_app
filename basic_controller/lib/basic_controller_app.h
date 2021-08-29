
#include <ArduinoJson.h>


#include "./application.h"
#include "./logger.h"
#include "./http_server.h"
#include "./file_system.h"
#include "./led_strip.h"

extern EspClass ESP;


namespace DevRelief {

    DRFileBuffer fileBuffer;
    struct StripData {
      uint16_t brightness;
      uint16_t strip1Length;
      uint16_t strip2Length;
      CRGB leds[3];
    };

    struct HSVData {
        CHSV leds[300];
    };

    class BasicControllerApplication : public Application {
    public: 
        BasicControllerApplication() {
            m_pos = 0;
            m_logger = new Logger("BasicControllerApplication",100);
            m_logger->debug("Running BasicControllerApplication v0.1");
            m_httpServer = new HttpServer();
            m_fileSystem = new DRFileSystem();
            stripData.strip1Length = 300;
            stripData.strip2Length = 123;

            m_httpServer->route("/",[this](Request* req, Response* resp){
               // //m_logger->debug("handling request / %s", req->uri().c_str());
                //resp->send(200,"text/html","working");
                this->getPage("index",req,resp);
            });

            m_httpServer->routeBraces("/{}.html",[this](Request* req, Response* resp){
               // //m_logger->debug("handling page / %s", req->uri().c_str());
                //resp->send(200,"text/html","working");
                this->getPage(req->pathArg(0),req,resp);
            });


            m_httpServer->routeBracesGet( "/api/scene/config",[this](Request* req, Response* resp){
                m_logger->debug("handling GET API  %s", req->uri().c_str());
                char * result = (char*)fileBuffer.reserve(2000);
                //String scenes = "\"abc\",\"def\"";
                int max = 20;
                String scenes[max];
                int fileCount = m_fileSystem->listFiles("/",scenes,max);
                int sceneCount = m_fileSystem->listFiles("/scene",scenes,max);
                String sceneNames="";
                for(int i=0;i<sceneCount;i++) {
                    if (i>0) {
                        sceneNames += ",";
                    }
                    sceneNames += "\""+scenes[i]+"\"";
                }
                sprintf(result,"{\"led_count\": %d,\"hostname\": \"%s\",\"ip_addr\": \"%s\", \"scenes\": [%s]}",LED_COUNT,HOSTNAME,WiFi.localIP().toString().c_str(),sceneNames.c_str());
                resp->send(200,"text/json",result);
                //this->apiRequest(req->pathArg(0),req,resp);
            });


            m_httpServer->routeBracesPost( "/api/scene/config",[this](Request* req, Response* resp){
                m_logger->debug("handling POST API  %s", req->uri().c_str());
                resp->send(200,"text/plain","post /api/scene/config");
                //this->apiRequest(req->pathArg(0),req,resp);
            });

            m_httpServer->routeBracesGet( "/api/scene/{}",[this](Request* req, Response* resp){
                m_logger->debug("get scene %s", req->pathArg(0).c_str());

                String msg = "get /api/scene/" + req->pathArg(0);
                String sceneName = req->pathArg(0);
                auto body = req->arg("plain");
                m_logger->debug("commands: " + body);
                auto found = m_fileSystem->read("/scene/"+sceneName,fileBuffer);
                if (found) {
                    resp->send(200,"text/plain",fileBuffer.text());
                } else {
                    String err = "cannot find scene "+sceneName;
                    resp->send(404,"text/plain",err);
                }
                //this->apiRequest(req->pathArg(0),req,resp);
            });


            m_httpServer->routeBracesPost( "/api/scene/{}",[this](Request* req, Response* resp){
                m_logger->debug("post scene %s", req->pathArg(0).c_str());
                String msg = "post /api/scene/" + req->pathArg(0);
                String sceneName = req->pathArg(0);
                auto body = req->arg("plain");
                m_logger->debug("commands: " + body);
                m_fileSystem->write("/scene/"+sceneName,body);
                this->runScene(body);
                resp->send(200,"text/plain",msg.c_str());
                //this->apiRequest(req->pathArg(0),req,resp);
            });


            m_httpServer->routeBraces("/api/{}",[this](Request* req, Response* resp){
                ////m_logger->debug("handling API  %s", req->uri().c_str());
                //resp->send(200,"text/html","working");
                this->apiRequest(req->pathArg(0),req,resp);
            });


            m_httpServer->routeNotFound([this](Request* req, Response* resp){
                //m_logger->debug("handling not found  %s", req->uri().c_str());
                this->notFound(req,resp);
            });
            m_httpServer->begin();
            m_strip1 = new DRLedStrip(1,150);
            m_strip2 = new DRLedStrip(2,150);

            auto found = m_fileSystem->readBinary("/stripData",(byte *)&stripData,sizeof(stripData));
            if (found) {
                m_logger->debug("found data");
                setLeds();
            }

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
            Serial.begin(115200);

           // //m_logger->debug("handle API %s",api.c_str());
            if (api == "restart") {
                //m_logger->warn("restart API called");
                resp->send(200,"text/json","{result:true,message:\"restarting\"}");
                ESP.restart();
            } else if (api == "config") {
                if (req->method() == HTTP_GET){
                    auto found = m_fileSystem->read("/config.json",fileBuffer);
                    String config = fileBuffer.text();
                    if (found){
                        resp->send(200,"application/json","{\"result\":true,\"data\":"+config+"}");    
                    } else {
                        resp->send(200,"application/json","{\"result\":false,\"data\":{}, \"message\": \"no configuration exists.\" }");    
                    }
                } else {
                    m_logger->debug("set config");
                    String config = req->arg("plain");
                    m_logger->debug("got config body");
                    m_logger->debug("body: " + config);
                    parseConfig(config);
                    m_logger->debug("parsed config body");
                    m_fileSystem->write("/config.json",config);
                    m_logger->debug("wrote config body");
                    resp->send(200,"application/json","{result:true}");
                }
                resp->send(200,"application/json","{result:true,message:\"config saved\"}");
            } else if (api == "colors") {
                String config = req->arg("plain");
                   // //m_logger->debug("body: " + config);
                    parseConfig(config);
                resp->send(200,"application/json","{result:true}");
            }  else if (api == "setcolors") {
               int start = req->arg("start").toInt();
                int count = req->arg("count").toInt();
                int r = req->arg("r").toInt();
                int g = req->arg("g").toInt();
                int b = req->arg("b").toInt();
                int c1 = req->arg("count1").toInt();
                int c2 = req->arg("count2").toInt();
                m_logger->debug("set colors %d %d %d %d %d %d %d",start,count,r,g,b,c1,c2);
                updateConfig(start,count,r,g,b,c1,c2);
               
                resp->send(200,"application/json","{result:true}");
            } else if (api == "echo") {
                char result[60];
                sprintf(result,"{result: %d}",millis());
                
                resp->send(200,"application/json",result);
            
            } else {
                resp->send(404,"application/json","{result:false}");
            }
        }

        void getPage(String page,Request * req,Response * resp){
            //m_logger->debug("get page %s",page.c_str());
           String path = "/"+page+".html";
            auto found = m_fileSystem->read(path, fileBuffer);        
            if (found) {
                auto mimeType = getMimeType(path);
                //m_logger->debug("sending %d characters",fileBuffer.getLength());
                uint8* data = fileBuffer.data();

                resp->send(200,mimeType.c_str(),fileBuffer.data(),fileBuffer.getLength());
            } else {
                //m_logger->error("URI not found: %s",req->uri().c_str());
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
                //m_logger->debug("sending %d characters",fileBuffer.getLength());
                uint8* data = fileBuffer.data();

                resp->send(200,mimeType.c_str(),fileBuffer.data(),fileBuffer.getLength());
            } else {
                //m_logger->error("URI not found: %s",req->uri().c_str());
                char buffer[200];
                sprintf(buffer,"not found: %s",req->uri().c_str());
                resp->send(200,"text/html",buffer);
            }

        }

        bool parseConfig(String & config){

           //m_logger->debug("Process config %d",millis());
            DynamicJsonDocument doc(14000);
            deserializeJson(doc,config);
            auto pins = doc["pins"];
            auto pin1 = pins[0];
            auto pin2 = pins[1];
            auto pin3 = pins[2];
            uint16_t brightness = doc["brightness"];
            if (brightness == 0) {
                m_logger->debug("got 0 brightness. setting to 50");
                brightness = 50;
            }
            //m_logger->debug("got pin 1");

            const JsonArray& leds = doc["leds"];
            //m_logger->debug("got leds %d",millis());
            uint16_t number = 0;
            String pin1Status = pin1["status"];
            String pin2Status = pin2["status"];
            String pin3Status = pin3["status"];
            //m_logger->debug("pin2 status %s %d",pin2Status.c_str(),(pin2Status.equals("off")));

            m_strip1->clear();
            m_strip2->clear();

            uint16_t pin1Count = (pin1Status == "off") ? 0 : pin1["count"];
            uint16_t pin2Count = (pin2Status.equals("off")) ? 0 : pin2["count"];
            uint16_t pin1End = pin1Count;
            uint16_t pin2End = pin1End + pin2Count;

            m_logger->debug("set leds %d %d %d %d",pin1Count,pin2Count,pin1End,pin2End);
           // m_strip1->setCount(pin1Count);
            //m_strip2->setCount(pin2Count);
            m_logger->debug("set brightness %d",brightness);
            m_strip1->setBrightness(brightness);
            m_strip2->setBrightness(brightness);

            for(number=0;number<pin1Count;number++) {
                int r = leds[number]["r"];
                int g = leds[number]["g"];
                int b = leds[number]["b"];
                m_strip1->setColor(number,CRGB(r,g,b));
            }
            for(number=pin1End;number<pin1End+pin2Count;number++) {
                int r = leds[number]["r"];
                int g = leds[number]["g"];
                int b = leds[number]["b"];
                m_strip2->setColor(number,CRGB(r,g,b));
            }
            m_logger->debug("done %d",number);
            DRLedStrip::show();
        }


      bool updateConfig(int startColor, int colorCount, int r, int g, int b,int count1,int count2){
            
        stripData.brightness = 40;
        stripData.strip1Length = count1;
        stripData.strip2Length = count2;
              

            uint16_t pin1Count = count1;
            uint16_t pin2Count = count2;
            uint16_t pin1End = pin1Count;
            uint16_t pin2End = pin1End + pin2Count;

            m_logger->debug("set leds %d %d %d %d",pin1Count,pin2Count,pin1End,pin2End);
           // m_strip1->setCount(pin1Count);
            //m_strip2->setCount(pin2Count);
            m_strip1->setBrightness(40);
            m_strip2->setBrightness(40);

            int number;
            for(number=startColor;number<startColor+colorCount;number++) {
              stripData.leds[number] = CRGB(r,g,b);
              if (number < pin1End) {
                  m_strip1->setColor(number,CRGB(r,g,b));
              } else {
                m_strip2->setColor(number-pin1End,CRGB(r,g,b));
              }
            }
          

            m_logger->debug("done %d",number);
            DRLedStrip::show();
            m_fileSystem->writeBinary("/stripData",(const byte *)&stripData,sizeof(stripData));
            return true;
        }

        
      bool setLeds(){
        m_logger->debug("set leds %d %d %d",stripData.brightness,stripData.strip1Length,stripData.strip1Length);            
            m_strip1->setBrightness(stripData.brightness);
            m_strip2->setBrightness(stripData.brightness);

            int number;
            for(number=0;number<stripData.strip1Length+stripData.strip2Length;number++) {
              CRGB& color = stripData.leds[number];
              if (number < stripData.strip1Length) {
                  m_strip1->setColor(number,color);
              } else {
                m_strip2->setColor(number-stripData.strip1Length,color);
              }
            }
          

            DRLedStrip::show();
            return true;
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

        void runScene(String commandText) {
            const char * text = commandText.c_str();
            size_t start = 0;
            size_t end = commandText.indexOf('\n');
            m_strip1->clear();
            m_strip2->clear();
            for(int idx=0;idx<300;idx++){
                hsvData.leds[idx].hue = 0;
                hsvData.leds[idx].saturation = 0;
                hsvData.leds[idx].value = 0;
            }
            while(start != end) {
                String cmd = commandText.substring(start,end);
                
                m_logger->debug("run command "+cmd);
                runCommand(cmd,hsvData.leds);
                start = end + 1;
                end = commandText.indexOf('\n',start);
            }

                        int number;
            for(number=0;number<stripData.strip1Length+stripData.strip2Length;number++) {
              CHSV& color = hsvData.leds[number];
              if (number < stripData.strip1Length) {
                  m_strip1->setHSV(number,color);
              } else {
                m_strip2->setHSV(number-stripData.strip1Length,color);
              }
            }
            
        }

        void runCommand(String command, CHSV* leds){
            const char * text = command.c_str();
            char cmd = *text;
            size_t start = command.indexOf(',')+1;
            size_t end = command.indexOf(',',start);
            int vals[10];
            int vcount = 0;
            while(start>= 0 && end >= 0) {
                int val = atoi(command.substring(start,end).c_str());
                vals[vcount++] = val;
                start = end+1;
                end = command.indexOf(',',start);
            }
            for(int i=vals[1];i<vals[2];i++) {
                if (cmd == 'h') {
                    leds[i].hue = vals[3];
                } else if (cmd == 's') {
                    leds[i].saturation = vals[3];
                } else if (cmd == 'l') {
                    leds[i].value = vals[3];
                }

            }
        }

    private: 
        Logger * m_logger;
        DRFileSystem * m_fileSystem;
        HttpServer * m_httpServer;
        int m_count = 1;
        DRLedStrip * m_strip1;
        DRLedStrip * m_strip2;
        int m_pos;
        HSVData hsvData;
        StripData stripData;

    };

}
