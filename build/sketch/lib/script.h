#ifndef DRSCRIPT_H
#define DRSCRIPT_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";

namespace DevRelief {
    class Script;

    class ExecutionState {
        public:
            ExecutionState() {
            }

        private:
            Config* m_config;
            Script* m_script;
    };

    class Animator {
    public:
        Animator() {

        }
    };

    class ValueAnimator : public Animator {
    public:
        ValueAnimator() {

        }

        ~ValueAnimator() {
            
        }
    };


    class PositionAnimator : public Animator {
    public:
        PositionAnimator() {

        }

        ~PositionAnimator() {
            
        }
    };

    Logger* commandLogger = new Logger("Command",100);

    class Command {
    public:
        Command(const char * type) {
            next = NULL;
            strcpy(m_type,type);
            m_logger = commandLogger;
            m_logger->info("create Command type: %s",m_type);
        }

        virtual ~Command() {
            m_logger->debug("delete command: %s",m_type);
            if (next) {

                delete next;
            }
        }

        virtual void execute(HSLStrip* strip) {
            m_logger->debug("execute command %s",m_type);
            doCommand(strip);
            if (next != NULL) {
                next->execute(strip);
            }
        }

        void setNext(Command * newNext) {
            if (next) {
                delete next;
            }
            next = newNext;
        }

        Command* getNext() {
            return next;
        }

        void add(Command * last) {
            if (next == NULL) {
                next = last;
            } else {
                next->add(last);
            }
        }


    protected:
        virtual void doCommand(HSLStrip* strip)=0;
        Logger * m_logger;
        char    m_type[20];
    private:
        Command* next;
    };

    class StripCommand : public Command {
    public:
        static StripCommand * create(ObjectParser& parser) {
            StripCommand * cmd = new StripCommand();
            return cmd;
        }

        StripCommand() : Command("strip") {

        }

    protected:
        virtual void doCommand(HSLStrip* strip) {}

    private:

    };

    
    class HSLCommand : public Command {
    public:
       
        HSLCommand(const char * type, ObjectParser& parser) : Command(type) {
            if (!parser.readIntValue("start",&m_start)) {
                m_start = 0;
            }
            if (!parser.readIntValue("count",&m_count)) {
                m_count=-1;
            }

            if (parser.readIntValue("value_start",&m_valueStart)) {
                if (!parser.readIntValue("value_end",&m_valueEnd)) {
                    m_valueEnd = m_valueStart;    
                }
            } else if (parser.readIntValue("value",&m_valueStart)) {
                m_valueEnd = m_valueStart;
            } else {
                m_logger->error("HSLCommand is missing value or value_start value");
                m_valueStart = 0;
                m_valueEnd = 0;
            }
            if (!parser.readStringValue("op",hslOpText,20)){
                strcpy(hslOpText,"replace");
            }
            hslOp = getHslOp(hslOpText);
            m_logger->debug("HSL Operation %d",hslOp);
        }

    protected:
        HSLOperation getHslOp(const char * text) {
            if (strcasecmp(text,"add") == 0){
                return ADD;
            } else if (strcasecmp(text,"SUBTRACT") == 0){
                return SUBTRACT;
            } else if (strcasecmp(text,"AVERAGE") == 0){
                return AVERAGE;
            } else if (strcasecmp(text,"MIN") == 0){
                return MIN;
            } else if (strcasecmp(text,"MAX") == 0){
                return MAX;
            } else {
                return REPLACE;
            }
        }

        virtual void doCommand(HSLStrip* strip) {
            for(int idx=0;idx<=m_count;idx++){
                int val = m_valueStart;
                setHSLComponent(strip,idx,val);
            }
        }

        virtual void setHSLComponent(HSLStrip* strip,int index, int value)=0;
        HSLOperation hslOp;
    private:
        int m_start;
        int m_count;
        int m_valueStart;
        int m_valueEnd;
        char hslOpText[20];
    };

    class HueCommand : public HSLCommand {
    public:
        HueCommand(ObjectParser& parser) : HSLCommand("hue",parser) {

        }

    protected:
        virtual void setHSLComponent(HSLStrip* strip,int index, int value) {

            strip->setHue(index,value,hslOp);
        }
        
    };
    class SaturationCommand : public HSLCommand {
    public:
        SaturationCommand(ObjectParser& parser) : HSLCommand("saturation",parser) {

        }
    protected:
        virtual void setHSLComponent(HSLStrip* strip,int index, int value) {
            strip->setSaturation(index,value,hslOp);
        }        
    };
    class LightnessCommand : public HSLCommand {
    public:
        LightnessCommand(ObjectParser& parser) : HSLCommand("lightness",parser) {

        }
    protected:
        virtual void setHSLComponent(HSLStrip* strip,int index, int value) {
            strip->setLightness(index,value,hslOp);
        }
    };

    class Script {
        public:
            Script(){
                m_logger = new Logger("Script",100);
                strcpy(name,"Default");
                next[0] = 0;
                durationMsec = -1;
                brightness = -1;
                m_firstCommand = NULL;
            }

            ~Script() {
                if (m_firstCommand == NULL) {
                    delete m_firstCommand;
                    m_firstCommand = NULL;
                }
            }

            void run(HSLStrip * strip) {
                strip->setBrightness(brightness);
                if (m_firstCommand != NULL) {
                    m_firstCommand->execute(strip);
                }

            }
            
            bool read(ObjectParser parser){
                if (m_firstCommand != NULL) {
                    m_logger->debug("clear commands");
                    delete m_firstCommand;
                    m_firstCommand = NULL;
                }
                size_t idx;
                ArrayParser commandArray;
                ObjectParser obj;
                if (!parser.readStringValue("name",name)) {
                    strcpy(name,"Default");
                }
                if (!parser.readStringValue("next",next)) {
                    next[0] = 0;
                }
                if (!parser.readIntValue("duration",&durationMsec)) {
                    durationMsec = -1;
                }
                if (!parser.readIntValue("duration",&brightness)) {
                    brightness = -1;
                }

                parser.getArray("commands",commandArray);
                
                char tmp[50];
                while(commandArray.nextObject(&obj)){
                    //obj.readAll(tmp);
                    //m_logger->debug("command: %s",tmp.text());
                    obj.readStringValue("type",tmp);
                    Command * cmd = NULL;
                    if (strcmp(tmp,"strip")==0) {
                        cmd = StripCommand::create(obj);
                    } else if (strcmp(tmp,"hsl")==0) {
                        cmd = createHSLCommand(obj);
                    } else {
                        m_logger->error("unknown command type: %s",tmp);
                    }
                    addCommand(cmd);
                }
                parser.readIntValue("brightness",&brightness);
                m_logger->debug("script read success %s",name);
                return true;
            }

            HSLCommand * createHSLCommand(ObjectParser& parser) {
                char component[20];
                parser.readStringValue("component",component);
                HSLCommand * cmd = NULL;
                if (strcmp(component,"hue")==0) {
                    cmd = new HueCommand(parser);
                } else if (strcmp(component,"saturation")==0) {
                    cmd = new SaturationCommand(parser);
                } else if (strcmp(component,"lightness")==0) {
                    cmd = new LightnessCommand(parser);
                } else {
                    commandLogger->error("Unknown HSL component: %.20s",component);
                }

                return cmd;
            }

            void addCommand(Command* cmd) {
                if (cmd == NULL) {
                    return;
                }
                if (m_firstCommand == NULL) {
                    m_firstCommand = cmd;
                } else {
                    m_firstCommand->add(cmd);
                }
            }

            Command* getFirstCommand() { return m_firstCommand;}

            char        name[100];
            char        next[100];
            int         durationMsec;
            int         brightness; // -1 to use Config value
            Logger *    m_logger;
        private:
            Command *   m_firstCommand;

    };

    class ScriptExecutor {
        public:
            ScriptExecutor() {
                m_logger = new Logger("ScriptExecutor",100);
                m_startTime = micros();
                m_lastStepTime = m_startTime;
            }

            ~ScriptExecutor() {
                delete m_ledStrip;
            }

            void setConfig(Config& config) {
                m_startTime = micros();
                m_lastStepTime = m_startTime;
                m_config = &config;
                setupLeds();
            }

            void setScript(Script* script) {
                m_startTime = micros();
                m_lastStepTime = m_startTime;
                if (script != NULL) {
                    m_logger->debug("no script set");
                } else {
                    m_logger->debug("script set to %s",script->name);
                }
                m_script = script;
            }

            void step() {
                
                if (m_script == NULL || m_script->getFirstCommand() == NULL) {
                    //m_logger->debug("no script");
                    return;
                }
                long now = micros();
                if (((now-m_lastStepTime)/1000000)<5) {
                    return; // only step every 5 seconds for now;
                }
                if (m_ledStrip == NULL) {
                    m_logger->error("no HSLStrip to run script");
                    return;
                }
                m_logger->debug("step time %d %d %d",now,m_lastStepTime,(now-m_lastStepTime));

                m_ledStrip->clear();
                m_script->run(m_ledStrip);
                m_ledStrip->show();
                m_logger->debug("shown");
                m_lastStepTime = micros();
            }

        private:
            void setupLeds() {
                if (m_ledStrip) {
                    delete m_ledStrip;
                }
                CompoundLedStrip*  compound = new CompoundLedStrip();
                for(int index=0;index<m_config->getStripCount();index++){
                    int pin = m_config->getPin(index);
                    DRLedStrip * real = new PhyisicalLedStrip(abs(pin),m_config->getLedCount(index));
                    if (!m_config->isReversed(index)) {
                        compound->add(real);
                    } else {
                        auto* reverse = new ReverseStrip(real);
                        compound->add(reverse);
                    }
                }
                m_ledStrip = new HSLStrip(compound);
                m_logger->info("created HSLStrip");
            }

        private: 
            long m_startTime;
            long m_lastStepTime;
            int m_brightness;
            Config* m_config;
            Script* m_script;
            HSLStrip* m_ledStrip;
            Logger * m_logger;
    };
}
#endif