#ifndef DRSCRIPT_H
#define DRSCRIPT_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";

namespace DevRelief {
    char tmpBuffer[100];
    ArrayParser commandArray;
    ObjectParser obj;

    class Script;
    class Command;
    class VariableCommand;
    class ExecutionState {
        public:
            ExecutionState(Config* config, Script* script, VariableCommand* variables) {
                m_config = config;
                m_script = script;
                m_variables = variables;
                m_startTime = millis();
                m_stepTime = m_startTime;
            }

            void setTime(long startTime, long stepTime){
                m_startTime = startTime;
                m_stepTime = stepTime;
            }

            long getStartTime() { return m_startTime;}
            long getStepTime() { return m_stepTime;}
            long getAnimationMsecs() { return m_stepTime-m_startTime;}

            Config *getConfig() { return m_config; }
            Script *getScript() { return m_script; }
            VariableCommand *getVariables() { return m_variables; }
            void setVariables(VariableCommand*cmd) { m_variables = cmd;}
        private:
            Config* m_config;
            Script* m_script;
            VariableCommand * m_variables;
            long m_startTime;
            long m_stepTime;
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

    Logger* commandLogger = new Logger("Command",80);

    class Command {
    public:
        Command(const char * type) {
            next = NULL;
            strncpy(m_type,type,20);
            m_logger = commandLogger;
            m_logger->info("create Command type: %s",m_type);
        }

        virtual ~Command() {
            m_logger->debug("delete command: %s",m_type);
            if (next) {

                delete next;
            }
        }

        const char * getType() { return m_type;}

        virtual void execute(HSLStrip* strip,ExecutionState* state) {
            m_logger->debug("execute command %s",m_type);
            doCommand(strip,state);
            if (next != NULL) {
                next->execute(strip,state);
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
        virtual void doCommand(HSLStrip* strip, ExecutionState* state)=0;
        Logger * m_logger;
        char    m_type[20];
    private:
        Command* next;
    };

    class VariableCommand : public Command {
    public:
        static VariableCommand * create(ObjectParser& parser) {
            VariableCommand * cmd = new VariableCommand(parser);
            return cmd;
        }

        VariableCommand(ObjectParser& parser) : Command("variable") {
            m_logger = new Logger("VariableCommand",100);
            parser.readAll(data);
            variableParser.setData((const char *)data.reserve(1),0,data.getLength());
            
        }

        bool getInt(const char * name, int& val) {
            if (!variableParser.readIntValue(name,&val)) {
                return m_prev != NULL &&  m_prev->getInt(name,val);
            }
            return true;
        }

        bool getString(const char * name, char * val) {
            if (!variableParser.readStringValue(name,val)) {
                return m_prev != NULL && m_prev->getString(name,val);
            }
            return true;
        }

    protected:
        virtual void doCommand(HSLStrip* strip, ExecutionState * state) {
            m_prev = state->getVariables();
            state->setVariables(this);
        }

    private:
        VariableCommand * m_prev;
        DRBuffer data;
        ObjectParser variableParser;
    };

    enum PositionType {
        PIXEL,
        PERCENT
    };
    
    class HSLCommand : public Command {
    public:
       
        HSLCommand(const char * type, ObjectParser& parser) : Command(type) {
            m_logger = new Logger("HSLCommand",80);
            //m_logger->debug("Parse HSLCommand");
            parser.readValue("start",m_start);
            parser.readValue("count",m_count);
            if (!parser.readValue("value_start",m_valueStart)){
                parser.readValue("value",m_valueStart);
            };


            if (!parser.readValue("value_end",m_valueEnd)) {
                m_logger->warn("value_end not found");
            }
            parser.readStringValue("position_type",tmpBuffer,20);
            if (strncmp(tmpBuffer,"pixel",5) == 0) {
                m_positionType = PIXEL;
            } else {
                m_positionType = PERCENT;
            }
            

            if (!parser.readStringValue("op",hslOpText,20)){
                strcpy(hslOpText,"replace");
            }
            hslOp = getHslOp(hslOpText);

            //m_logger->debug("\tparsed HSLCommand %s",m_type);
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
            return REPLACE;

        }

        virtual void doCommand(HSLStrip* strip, ExecutionState * state) {
            int first = getInt(m_start,0,state->getVariables());
            int count = getInt(m_count,-1,state->getVariables());
            int vstart = getInt(m_valueStart,0,state->getVariables());
            int vend = getInt(m_valueEnd,-1,state->getVariables());
            float step = 0;

            int value = vstart;
            //m_logger->debug("HSLValues %d %d %d %d",first,count,vstart,vend);
            if (m_positionType == PERCENT) {
                
                int ledCount = strip->getCount();
                first = first*ledCount/100;
                count = count*ledCount/100;
                m_logger->debug("\t to percents %d %d %d %d",first,count,vstart,vend);
            }

            if (vstart != vend && vend != -1) {
                step = 1.0*(vend-vstart)/count;
            }

            m_logger->debug("setValues %d %d %d %f - %d %d",first,count,value,step,vstart,vend);
            int ledCount = strip->getCount();
            for(int idx=0;idx<count;idx++){
                int pos = first + idx;
                int val = value+idx*step;
                //m_logger->debug("set HSL %d %d",pos,val);
                int animPos = pos;
                int move = ((state->getAnimationMsecs()/100));
                animPos = (animPos+move)%ledCount;
                setHSLComponent(strip,animPos,val);
            }
        }

        virtual void setHSLComponent(HSLStrip* strip,int index, int value)=0;
        HSLOperation hslOp;
    private:
        int getInt(ParserValue&val,int defaultValue,VariableCommand * variables){
            int result = defaultValue;
            if (val.type == INTEGER || val.type == FLOAT){
                //m_logger->debug("getInt for INTEGER ParserValue %d %d",val.intValue,val.type);
                result = val.intValue;
            } else if (val.type == VARIABLE && variables != NULL) {
                //m_logger->debug("getInt for variable ParserValue %s",val.nameValue);
                variables->getInt(val.nameValue,result);
            } else {
                m_logger->warn("unknown ParserValue type %d %f",val.type,val.floatValue);
            }
            //m_logger->debug("\tresult=%d",result);
            return result;
        }
        ParserValue m_start;
        ParserValue m_count;
        ParserValue m_valueStart;
        ParserValue m_valueEnd;
        PositionType m_positionType;
        char hslOpText[10];
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

            void run(HSLStrip * strip, ExecutionState* state) {
                strip->setBrightness(brightness);
                if (m_firstCommand != NULL) {
                    m_firstCommand->execute(strip,state);
                }

            }
            
            bool read(ObjectParser parser){
                m_logger->showMemory();

                if (m_firstCommand != NULL) {
                    m_logger->debug("clear commands");
                    delete m_firstCommand;
                    m_firstCommand = NULL;
                }
                size_t idx;
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
                //m_logger->debug("commands: %d %d %d %.50s --- %.50s",commandArray.getStart(),commandArray.getEnd(),commandArray.getPos(),commandArray.getData()+commandArray.getStart(),commandArray.getData()+commandArray.getEnd()-25);
                    
                m_logger->showMemory();
                while(commandArray.nextObject(&obj)){
                    m_logger->debug("object: %d %d %d ~~~%.*s~~~",obj.getStart(),obj.getEnd(),obj.getPos(),obj.getEnd()-obj.getStart(),obj.getData()+obj.getStart());

                    //DRBuffer tbuf;
                    //obj.readAll(tbuf);
                    //m_logger->debug("command: %s",tbuf.text());
                    obj.readStringValue("type",tmpBuffer);
                    Command * cmd = NULL;
                    if (strcmp(tmpBuffer,"variable")==0) {
                        cmd = VariableCommand::create(obj);
                    } else if (strcmp(tmpBuffer,"hsl")==0) {
                        cmd = createHSLCommand(obj);
                    } else {
                        m_logger->error("unknown command type: %s",tmpBuffer);
                    }
                    m_logger->info("add command %s",cmd->getType());
                    addCommand(cmd);
                    m_logger->info("\tadded command");
                }
                parser.readIntValue("brightness",&brightness);
                m_logger->debug("script read success %s",name);
                return true;
            }

            HSLCommand * createHSLCommand(ObjectParser& parser) {
                parser.readStringValue("component",tmpBuffer);
                HSLCommand * cmd = NULL;
                if (strcmp(tmpBuffer,"hue")==0) {
                    cmd = new HueCommand(parser);
                } else if (strcmp(tmpBuffer,"saturation")==0) {
                    cmd = new SaturationCommand(parser);
                } else if (strcmp(tmpBuffer,"lightness")==0) {
                    cmd = new LightnessCommand(parser);
                } else {
                    commandLogger->error("Unknown HSL component: %.20s",tmpBuffer);
                    return NULL;
                }
                m_logger->info("created command %s",cmd->getType());
                m_logger->showMemory();
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
                m_logger = new Logger("ScriptExecutor",80);
                m_startTime = micros();
                m_lastStepTime = m_startTime;
            }

            ~ScriptExecutor() {
                delete m_ledStrip;
            }

            void setConfig(Config& config) {
                m_startTime = millis();
                m_lastStepTime = m_startTime;
                m_config = &config;
                setupLeds();
            }

            void setScript(Script* script) {
                m_startTime = millis();
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
                /*
                long now = micros();
                if (((now-m_lastStepTime)/1000000)<5) {
                    return; // only step every 5 seconds for now;
                }
                */
                if (m_ledStrip == NULL) {
                    m_logger->error("no HSLStrip to run script");
                    return;
                }
               // m_logger->debug("step time %d %d %d",now,m_lastStepTime,(now-m_lastStepTime));
                long now = millis();
                m_ledStrip->clear();
                ExecutionState state(m_config,m_script,NULL);
                state.setTime(m_startTime,now);
                m_script->run(m_ledStrip,&state);
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