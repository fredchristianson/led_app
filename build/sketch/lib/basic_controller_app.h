
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
        int16_t startPercent;
        int16_t endPercent;
        int16_t startValue;
        int16_t endValue;
        double zoom;
        double m_animationPercentPerSecond;
        Logger* m_logger;

    public:
        Command(char * def) {
            m_logger = new Logger("Command",100);
           if (parse(def)) {
            m_logger->info("success");
           } else {
               m_logger->warn("error in command");
           }
        }


        bool parse(char * def) {
            m_logger->debug("parse command");
            m_logger->debug(def);
            const char * delims = " ,\t\n\r;";
            char * tok = strtok(def,delims);
            if (tok == NULL) {
                return false;
            }
            this->type = tok[0];
            tok = strtok(NULL,delims);
            if (tok == NULL) {
                return false;
            }
            m_logger->debug("tok %s",tok);
            
            this->startPercent = atoi(tok);

            tok = strtok(NULL,delims);
            if (tok == NULL) {
                return false;
            }
            m_logger->debug("tok %s",tok);
            this->endPercent = atoi(tok);

            tok = strtok(NULL,delims);
            if (tok == NULL) {
                return false;
            }
            m_logger->debug("tok %s",tok);
            this->startValue = atoi(tok);

            tok = strtok(NULL,delims);
            if (tok == NULL) {
                return false;
            }
            m_logger->debug("tok %s",tok);
            this->endValue = atoi(tok);

            tok = strtok(NULL,delims);
            if (tok == NULL) {
                return false;
            }
            m_logger->debug("tok %s",tok);
            this->zoom = atof(tok);

            tok = strtok(NULL,delims);
            if (tok == NULL) {
                return false;
            }
            m_logger->debug("tok %s",tok);
            this->m_animationPercentPerSecond = atof(tok);
            return true;
        }

    };

    struct HSLData {
        CHSV leds[300]; // using CHSV but storing HSL value
    };

    class BasicControllerApplication : public Application {
    public: 
        BasicControllerApplication() {
            m_pos = 0;
            m_logger = new Logger("BasicControllerApplication",100);
            m_httpServer = new HttpServer();
            m_fileSystem = new DRFileSystem();
            stripData.strip1Length = STRIP1_LEDS;
            stripData.strip2Length = 0;
            fileBuffer.reserve(25000);

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
                Serial.println("***get scene");
                

                String msg = "get /api/scene/" + req->pathArg(0);
                String sceneName = req->pathArg(0);
                auto body = req->arg("plain");
                m_logger->debug("commands: " + body);
                auto found = m_fileSystem->read("/scene/"+sceneName,fileBuffer);
                m_fileSystem->write("/lastscene",sceneName.c_str());
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


            m_httpServer->routeBracesGet("/api/{}",[this](Request* req, Response* resp){
                ////m_logger->debug("handling API  %s", req->uri().c_str());
                //resp->send(200,"text/html","working");
                this->apiRequest(req->pathArg(0),req,resp);
            });


            m_httpServer->routeNotFound([this](Request* req, Response* resp){
                //m_logger->debug("handling not found  %s", req->uri().c_str());
                this->notFound(req,resp);
            });
            m_httpServer->begin();
            //m_logger->restart();
            m_strip1 = new DRLedStrip(1,STRIP1_LEDS);
            m_logger->debug("created strip1");
            //m_strip2 = new DRLedStrip(2,STRIP2_LEDS);
            m_ledCount = LED_COUNT;
            auto found = m_fileSystem->read("/lastscene",fileBuffer);
            if (found) {
                loadScene(fileBuffer.text());
            }

            m_logger->debug("Running BasicControllerApplication configured: v0.5");

        }

        
        void loop() {
            this->m_animationRan = false;
            m_httpServer->handleClient();
            
            long now = millis();
            long diff = (now - this->m_lastAnimation);
            if (!this->m_animationRan && diff > 300 && this->m_currentScene.length() > 10) {
                this->runScene(this->m_currentScene);
                DRLedStrip::show();
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
                
                DRLedStrip::show();
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
            return "text/plain";
        }

        void apiRequest(String api,Request * req,Response * resp) {

            m_logger->debug("handle API %s",api.c_str());
            Serial.println("*** handle api ");
            Serial.println(api.c_str());
            if (api == "restart") {
                m_logger->warn("restart API called");
                resp->send(200,"text/json","{result:true,message:\"restarting\"}");
                ESP.restart();
            } else if (api == "serial"){
                m_logger->debug("restart serial port");
                Serial.println("***");
                Serial.println(api.c_str());
                Serial.begin(115200);
                Serial.println("serial restarted");
                m_logger->debug("serial restarted");
                resp->send(200,"text/json","{result:true,message:\"serial restarted\"}");
            }  else if (api == "show"){
                m_logger->info("show leds");
                FastLED.show();
                resp->send(200,"text/json","{result:true,message:\"FastLED.show()\"}");
            } else {
                resp->send(200,"text/json","{result:false,message:\"unknown api \"}");
            }
        }

        void getPage(String page,Request * req,Response * resp){
           m_logger->debug("get page %s",page.c_str());
           String path = "/"+page+".html";
           streamFile(resp,path);
        }

        void notFound(Request *req , Response* resp) {
            String path = req->uri();
            streamFile(resp,path);
        }

        void streamFile(Response * resp,String & path) {
            m_logger->debug("stream file %s",path.c_str());
            File file = m_fileSystem->openFile(path);
            if (file.isFile()) {
                auto mimeType = getMimeType(path);
                resp->streamFile(file,mimeType);
                m_fileSystem->closeFile(file);
            } else {
                m_logger->debug("file not found");
                resp->send(404,"text/html","file not found ");
            }
          
        }

       
        void runScene(String commandText) {

            const char * text = commandText.c_str();
            m_logger->debug("execute commands");
            m_logger->debug(text);
            int start = 0;
            int end = commandText.indexOf('\n');
            m_setLedCount = m_ledCount;
            m_strip1->clear();
            //m_strip2->clear();
            for(int idx=0;idx<300;idx++){
                hslData.leds[idx].hue = 0;
                hslData.leds[idx].saturation = 0;
                hslData.leds[idx].value = 0;
            }
            while(end>=0) {
                char cmd[100];
                memcpy(cmd,text+start,end-start);// = commandText.substring(start,end);
                cmd[end-start] = 0;
                m_logger->debug("run command %s",cmd);
                runCommand(cmd,hslData.leds);
                m_logger->debug("command done");
                start = end + 1;
                //m_logger->debug("start/end  %d/%d",start,end);
                end = commandText.indexOf('\n',start);
               // m_logger->debug("next start/end  %d/%d",start,end);
            }

            int number;
            m_logger->debug("set strip values %d",m_setLedCount);
            for(number=0;number<m_ledCount && number < this->m_setLedCount;number++) {
              CHSV& color = hslData.leds[number];
             // m_logger->debug("HSV %d (%d,%d,%d)",number,color.hue,color.saturation,color.value);
              if (number < STRIP1_LEDS) {
                  auto rgb = HSLToRGB(color);
                  if (number< 10) {
                      m_logger->debug("R,G,B=(%d,%d,%d)",rgb.r,rgb.g,rgb.b);
                  }
                  m_strip1->setColor(number,rgb);
              } else {
               // m_strip2->setColor(number-STRIP1_LEDS,HSLToRGB(color));
              }
            }
        }

        void runCommand(char * text, CHSV* leds){
            Command cmd(text);
            m_logger->debug("parsed command %s",text);
            if (cmd.type == 'z')if (cmd.type == 'z') {
                    m_logger->debug("limit led count %d",cmd.startPercent);
                    this->m_setLedCount = cmd.startPercent;
                    return;
            }
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
            m_logger->debug("set leds %d-%d.  %d-%d %d/%d/%f",startIdx,endIdx,startValue,endValue,valueDiff,steps,stepDiff);
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
                } else {
                    m_logger->error("unknown command type %c",cmd.type);
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
            //m_logger->debug("hsl to rgb (%d,%d,%d)",(int)hslInHSV.h,(int)hslInHSV.s,(int)hslInHSV.v);
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
            //m_logger->debug("hsl (%d,%f,%f)->rgb(%d,%d,%d)",(int)h,s,l,rgb.r,rgb.g,rgb.b);
            return rgb;
        }
        
    private: 
        Logger * m_logger;
        DRFileSystem * m_fileSystem;
        HttpServer * m_httpServer;
        int m_ledCount = LED_COUNT;
        DRLedStrip * m_strip1;
        //DRLedStrip * m_strip2;
        int m_pos;
        HSLData hslData;
        StripData stripData;
        String m_currentScene;
        long m_animationStartMillis;
        long m_lastAnimation;
        bool m_animationRan;
        bool m_hasAnimation;
        long m_setLedCount;
    };

}
