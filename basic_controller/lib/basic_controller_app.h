
#include <ArduinoJson.h>
#include <math.h>
#include <time.h>

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

    class Command {
    public:
        char type;
        uint8_t startPercent;
        uint8_t endPercent;
        uint8_t startValue;
        uint8_t endValue;
        double zoom;
        double m_animationPercentPerSecond;
        Logger* m_logger;

    public:
        Command(char * def) {
           parse(def);
        }

        bool parse(char * def) {
            const char * delims = " ,\t\n\r;";
            char * tok = strtok(def,delims);
            
            this->type = tok[0];
            tok = strtok(NULL,delims);
            
            this->startPercent = atoi(tok);

            tok = strtok(NULL,delims);
            this->endPercent = atoi(tok);

            tok = strtok(NULL,delims);
            this->startValue = atoi(tok);

            tok = strtok(NULL,delims);
            this->endValue = atoi(tok);

            tok = strtok(NULL,delims);
            this->zoom = atof(tok);

            tok = strtok(NULL,delims);
            this->m_animationPercentPerSecond = atof(tok);
        }

    };

    struct HSVData {
        CHSV leds[300]; // using CHSV but storing HSL value
    };

    class BasicControllerApplication : public Application {
    public: 
        BasicControllerApplication() {
            m_pos = 0;
            m_logger = new Logger("BasicControllerApplication",100);
            m_logger->debug("Running BasicControllerApplication v0.1");
            m_httpServer = new HttpServer();
            m_fileSystem = new DRFileSystem();
            stripData.strip1Length = 150;
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
                //m_logger->debug("handling GET API  %s", req->uri().c_str());
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
                m_fileSystem->write("/lastsceen",sceneName.c_str());
                if (found) {
                    String sceneContents = fileBuffer.text();
                    loadScene(sceneName);
                    resp->send(200,"text/plain",sceneContents.c_str());
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
                loadScene(sceneName);
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
            m_strip1 = new DRLedStrip(1,STRIP1_LEDS);
            m_strip2 = new DRLedStrip(2,STRIP2_LEDS);
            m_ledCount = LED_COUNT;
            auto found = m_fileSystem->read("/lastsceen",fileBuffer);
            if (found) {
                loadScene(fileBuffer.text());
            }

        }

        void loadScene(String sceneName) {
            auto found = m_fileSystem->read("/scene/"+sceneName,fileBuffer);
            if (found) {
                m_logger->info("run scene: %s",sceneName.c_str());
                m_currentScene = fileBuffer.text();
                m_animationStartMillis = millis();
                m_lastAnimation = 0;
                m_hasAnimation = false;
                runScene(fileBuffer.text());
            } else {
                m_logger->info("scene not found: %s",sceneName.c_str());
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
            m_animationRan = false;
            m_httpServer->handleClient();
            if (!m_animationRan && m_currentScene.length() > 10) {
                runScene(m_currentScene);
            }
            DRLedStrip::show();
        }

        void runScene(String commandText) {
            const char * text = commandText.c_str();
            int start = 0;
            int end = commandText.indexOf('\n');
            m_strip1->clear();
            m_strip2->clear();
            for(int idx=0;idx<300;idx++){
                hsvData.leds[idx].hue = 0;
                hsvData.leds[idx].saturation = 0;
                hsvData.leds[idx].value = 0;
            }
            while(end>=0) {
                String cmd = commandText.substring(start,end);
                
                //m_logger->debug("run command "+cmd);
                runCommand(cmd,hsvData.leds);
                start = end + 1;
                //m_logger->debug("start/end  %d/%d",start,end);
                end = commandText.indexOf('\n',start);
               // m_logger->debug("next start/end  %d/%d",start,end);
            }

            int number;
            //m_logger->debug("set strip values");
            for(number=0;number<m_ledCount;number++) {
              CHSV& color = hsvData.leds[number];
             // m_logger->debug("HSV %d (%d,%d,%d)",number,color.hue,color.saturation,color.value);
              if (number < STRIP1_LEDS) {
                  m_strip1->setColor(number,HSLToRGB(color));
              } else {
                m_strip2->setColor(number-STRIP1_LEDS,HSLToRGB(color));
              }
            }
            DRLedStrip::show();
        }

        void runCommand(String command, CHSV* leds){
            if (command.length() < 10) {

                return;
            }
            char * text = (char*)fileBuffer.reserve(command.length());
            strcpy(text,command.c_str());
            Command cmd(text);
            int startIdx = round(m_ledCount*cmd.startPercent/100);
            int endIdx = round(m_ledCount*cmd.endPercent/100);
            int startValue = cmd.startValue;
            int endValue = cmd.endValue;
            int valueDiff = endValue-startValue;
            int steps = endIdx-startIdx+1;
            float stepDiff = 1.0*valueDiff/steps;
            long sceneMillis = millis();
            int ledOffset = 0;
            if (m_lastAnimation != 0 && cmd.m_animationPercentPerSecond != 0) {
                m_hasAnimation = true;
                float seconds = 1.0*(sceneMillis - m_animationStartMillis)/1000.0;
                long pixelOffset = 1.0*m_ledCount*(cmd.m_animationPercentPerSecond*seconds)/100.0;
                ledOffset = pixelOffset;
            }
            //m_logger->debug("set leds %d-%d.  %d-%d %d/%d/%f",startIdx,endIdx,startValue,endValue,valueDiff,steps,stepDiff);
            for(int i=startIdx;i<=endIdx;i++) {
                char value = round(cmd.startValue+i*stepDiff);
                int pos = (i+ledOffset)%m_ledCount;
                while (pos <0) {
                    pos = m_ledCount + pos;
                }
                if (cmd.type == 'h') {
                    leds[pos].hue = value;
                } else if (cmd.type == 's') {
                    leds[pos].saturation = value;
                } else if (cmd.type == 'l') {
                    leds[pos].value = value;
                }

            }
            m_animationRan = true;
            m_lastAnimation = sceneMillis;
        }

        // using CHSV, but the v is from an HSL color
        float HueToRGB(float v1, float v2, float vH) {
            if (vH < 0)
                vH += 1;

            if (vH > 1)
                vH -= 1;

            if ((6 * vH) < 1)
                return (v1 + (v2 - v1) * 6 * vH);

            if ((2 * vH) < 1)
                return v2;

            if ((3 * vH) < 2)
                return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

            return v1;
        }

        CRGB HSLToRGB(CHSV hslInHSV) {
           // m_logger->debug("hsl to rgb (%d,%d,%d)",(int)hslInHSV.h,(int)hslInHSV.s,(int)hslInHSV.v);
            unsigned char r = 0;
            unsigned char g = 0;
            unsigned char b = 0;

            unsigned char h = hslInHSV.hue;
            float s = 1.0*hslInHSV.s/100.0;
            float l = 1.0*hslInHSV.v/100.0;
            if (s == 0)
            {
                r = g = b = (unsigned char)(l * 255);
            }
            else
            {
                float v1, v2;
                float hue = (float)h / 255;

                v2 = (l < 0.5) ? (l * (1 + s)) : ((l + s) - (l * s));
                v1 = 2 * l - v2;

                r = (unsigned char)(255 * HueToRGB(v1, v2, hue + (1.0f / 3)));
                g = (unsigned char)(255 * HueToRGB(v1, v2, hue));
                b = (unsigned char)(255 * HueToRGB(v1, v2, hue - (1.0f / 3)));
            }

            CRGB rgb(r, g, b);
           // m_logger->debug("hsl (%d,%f,%f)->rgb(%d,%d,%d)",(int)h,s,l,rgb.r,rgb.g,rgb.b);
            return rgb;
        }
        
    private: 
        Logger * m_logger;
        DRFileSystem * m_fileSystem;
        HttpServer * m_httpServer;
        int m_ledCount = LED_COUNT;
        DRLedStrip * m_strip1;
        DRLedStrip * m_strip2;
        int m_pos;
        HSVData hsvData;
        StripData stripData;
        String m_currentScene;
        long m_animationStartMillis;
        long m_lastAnimation;
        bool m_animationRan;
        bool m_hasAnimation;
    };

}
