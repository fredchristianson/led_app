#ifndef DRSCRIPT_H
#define DRSCRIPT_H


#include "./parse_gen.h";
#include "./logger.h";
#include "./led_strip.h";
#include "./config.h";

namespace DevRelief {
    char tmpBuffer[100];
    ArrayParser commandArray;
    ObjectParser obj;
    ParserValue tempValue;

    double INVALID_DOUBLE = __FLT_MAX__; // 1000000000.0;
    long INVALID_LONG = LONG_MAX;
    class Script;
    class Command;
    class VariableCommand;

    enum PositionType {
        PIXEL,
        PERCENT
    };



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

        virtual void execute(IHSLStrip* strip,ExecutionState* state) {
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

        int getInt(ParserValue&val,int defaultValue,VariableCommand * variables);
        double getFloat(ParserValue&val,double defaultValue,VariableCommand * variables);
        const char * getString(ParserValue&val,char * resultValue,size_t maxLen, VariableCommand * variables);

    protected:
        virtual void doCommand(IHSLStrip* strip, ExecutionState* state)=0;
        Logger * m_logger;
        char    m_type[20];

     private:
        Command* next;
    };

    class VariableCommand : public Command {
    public:
        static VariableCommand * create(ObjectParser& parser, VariableCommand * prev) {
            VariableCommand * cmd = new VariableCommand(parser,prev);
            return cmd;
        }

        /* derived classes can manage variables.  not config object to init */
        VariableCommand(const char * subtype) : Command(subtype) {
            m_prev = NULL;
            variableParser.setData((const char *)data.reserve(1),0,data.getLength());
        }

        VariableCommand(ObjectParser& parser,VariableCommand * prev) : Command("variable") {
            m_prev = prev;
            parser.readAll(data);
            variableParser.setData((const char *)data.reserve(1),0,data.getLength());
            
        }

        bool getInt(const char * name, int& val) {
            if (!variableParser.readIntValue(name,&val)) {
                return m_prev != NULL &&  m_prev->getInt(name,val);
            }
            return true;
        }

        bool getFloat(const char * name, double& val) {
            if (!variableParser.readFloatValue(name,&val)) {
                return m_prev != NULL &&  m_prev->getFloat(name,val);
            }
            return true;
        }

        bool getString(const char * name, char * val, size_t maxLen) {
            if (!variableParser.readStringValue(name,val,maxLen)) {
                return m_prev != NULL && m_prev->getString(name,val,maxLen);
            }
            return true;
        }

    protected:
        virtual void doCommand(IHSLStrip* strip, ExecutionState * state) {
            state->setVariables(this);
        }

    private:
        VariableCommand * m_prev;
        DRBuffer data;
        ObjectParser variableParser;
    };

    /*
    StartCommand does nothing for now.  ensures we always have a VariableCommand before any other commands so they don't need to check NULL
    */
    class StartCommand : public VariableCommand {
        public:
            StartCommand() : VariableCommand("StartCommand") {

            }
    };

    enum GradientType {
        NO_GRADIENT=-1,
        PIXEL_GRADIENT=0,
        TIME_GRADIENT=1
    };

    class ValueGradient
    {
    public:
        ValueGradient()
        {
            m_logger = new Logger("ValueGradient",80);
        }

        bool read(ObjectParser &parser, VariableCommand *variables)
        {
            m_logger->debug("Read ValueGradient");
            if (parser.readValue("value_start", tempValue)){
                m_valueStart = ((Command *)variables)->getFloat(tempValue, 0, variables);
                parser.readValue("value_end", tempValue);
                m_valueEnd = ((Command *)variables)->getFloat(tempValue, 0, variables);
                parser.readBoolValue("unfold", &m_unfold, false);
            } else {
                parser.readValue("value", tempValue);
                m_valueStart = ((Command *)variables)->getFloat(tempValue, 0, variables);
                m_valueEnd = m_valueStart;
                m_unfold = false;
                return true;
            }

            m_speed = INVALID_DOUBLE;
            m_durationMsecs = INVALID_DOUBLE;
            m_type = PIXEL_GRADIENT;
            //m_logger = new Logger("Animator",100);
            if (parser.readValue("speed",tempValue)){
                m_speed = ((Command*)variables)->getFloat(tempValue,INVALID_DOUBLE,variables);
                if (m_speed != INVALID_DOUBLE) {
                    m_type = TIME_GRADIENT;
                }
            } else if (parser.readValue("duration",tempValue)){
                m_durationMsecs = ((Command*)variables)->getInt(tempValue,INVALID_LONG,variables);
                if (m_durationMsecs != INVALID_LONG) {
                    m_type = TIME_GRADIENT;
                }
            }
            m_logger->debug("\tValueGradient %f %f %d %d %f %d",
                m_valueStart,m_valueEnd,m_unfold,m_durationMsecs,m_speed,m_type);
            return true;
        }

        double getValueAt(int index, int maxIndex, ExecutionState* state) {
            if (m_valueStart == m_valueEnd){
                return m_valueStart;
            }
            if ((index == 0 && m_type==PIXEL_GRADIENT) || m_type == NO_GRADIENT) {
                return m_valueStart;
            } else if (index >= maxIndex) {
                m_logger->warn("index too big %d %d",index,maxIndex);
                return m_valueEnd;
            }
            double value = m_valueStart;
            if (m_type == TIME_GRADIENT) {
                double pct = index*1.0/maxIndex;
                long diff = (m_valueEnd-m_valueStart);

                long runMsecs = state->getAnimationMsecs();
                long duration = m_durationMsecs;
                if (m_speed != INVALID_DOUBLE) {
                    duration = diff*1000.0/m_speed;
                } else if (m_durationMsecs != INVALID_DOUBLE){
                    duration = m_durationMsecs;
                } else {
                    duration = 1000.0;
                }
                double dpos = (runMsecs)%duration;
                double dpct = dpos/(1.0*duration);
                value = m_valueStart + dpct*diff;
                if (index == 0) {
                    m_logger->debug("time gradient value %d %d    %f    %f-%f  %f %f %d", index,maxIndex,value,m_valueStart,m_valueEnd,dpos,dpct,diff);
                }

            } else {
                if (index == 0 || maxIndex == 0) {
                    value = 0;
                } else {
                    double pct = index*1.0/maxIndex;
                    long diff = (m_valueEnd-m_valueStart+1);
                    double step = diff*pct;
                    value = m_valueStart + step;
                }
                m_logger->debug("pixel gradient value %d %d %f %f-%f",index,maxIndex, value,m_valueStart,m_valueEnd);
            }
            return value;
        }
    private:
        Logger * m_logger;
        float m_valueStart;
        float m_valueEnd;
        bool m_unfold;
        long m_durationMsecs;
        double m_speed;
        GradientType m_type;
    };

    class Position
    {
    public:
        Position()
        {
            m_logger = new Logger("Position",DEBUG_LEVEL);
        }

        bool read(ObjectParser &parser, VariableCommand *variables)
        {   
            m_logger->debug("parse Position object");
            parser.readStringValue("position_type", tmpBuffer, 20);
            if (strncasecmp(tmpBuffer, "pixel", 5) == 0)
            {
                m_logger->debug("\ttype pixel");
                m_positionType = PIXEL;
            }
            else
            {
                m_logger->debug("\ttype percent");
                m_positionType = PERCENT;
            }
            parser.readValue("start", tempValue);
            m_logger->debug("\tread start");
            m_start = ((Command *)variables)->getInt(tempValue, 0, variables);
            m_logger->debug("\tgot start %d",m_start);

            m_logger->debug("\tread count");
            if (!parser.readValue("count", tempValue))
            {
                m_logger->debug("\tread end");
                parser.readValue("end", tempValue);
                int end = ((Command *)variables)->getInt(tempValue, m_start, variables);
                m_count = end - m_start + 1;
                m_logger->debug("\tgot end %d %d %d",end,m_start,m_count);

            }
            else
            {
                m_count = ((Command *)variables)->getInt(tempValue, 0, variables);
                m_logger->debug("\tgot count %d",m_count);
            }
            return true;
        }

        int getStart(IHSLStrip *strip)
        {
            return convert(m_start, strip);
            return m_start;
        }

        int getCount(IHSLStrip *strip)
        {
            return convert(m_count, strip);
        }

        int getEnd(IHSLStrip *strip)
        {
            return convert(m_count + m_start, strip);
        }

        int convert(int val, IHSLStrip *strip)
        {
            if (m_positionType == PIXEL)
            {
                return val;
            }
            int count = strip->getCount();
            return (int)round((val * count) / 100.0);
        }

    private:
        Logger* m_logger;
        int m_start;
        int m_count;
        PositionType m_positionType;
    };

    class AnimatorCommand : public Command {
        public:
            AnimatorCommand(ObjectParser& parser, VariableCommand * variables, const char * type = "animator") : Command(type) {
                m_speed = INVALID_DOUBLE;
                m_durationMsecs = INVALID_DOUBLE;
                //m_logger = new Logger("Animator",100);
                if (parser.readValue("speed",tempValue)){
                    m_speed = getFloat(tempValue,INVALID_DOUBLE,variables);
                } else {
                    if (parser.readValue("duration",tempValue)){
                        m_durationMsecs = getInt(tempValue,INVALID_LONG,variables);
                    }
                }
                m_logger->debug("Animator speed=%f.  duration=%d",m_speed,m_durationMsecs);
            }

            double getAnimationPosition(double low,double high){
                double diff = high-low;
                if (diff == 0) {
                    return low;
                }
                long runMsecs = executionState->getAnimationMsecs();
                long duration = m_durationMsecs;
                if (m_speed != INVALID_DOUBLE) {
                    duration = diff*1000/m_speed;
                } else if (m_durationMsecs != INVALID_DOUBLE){
                    duration = m_durationMsecs;
                } else {
                    return low;
                }
                double dpos = (runMsecs)%duration;
                double dpct = dpos/duration;
                double result = low + dpct*diff;
                m_logger->debug("animate %f-%f = %f.  duration=%d. dpos=%f.  pct=%f",low,high,result,duration,dpos,dpct);
                result = result < low ?  low : result;
                result = result > high ? high : result;
                return result;


            }

            ExecutionState * executionState;
        protected:
            long m_durationMsecs;
            double m_speed;
    };

    class PositionAnimator : public AnimatorCommand, public IHSLStrip {
        public:
            PositionAnimator(ObjectParser& parser, VariableCommand * variables) : AnimatorCommand(parser,variables, "PositionAnimator"){
                parser.readValue("direction",tempValue);
                m_forward = true;
                if (getString(tempValue,tmpBuffer,100,variables)){
                    m_forward = strncasecmp(tmpBuffer,"for",3) == 0;  // forword, foreword, forward, ...
                }
                m_logger->debug("PositionAnimator read position");
                m_position.read(parser,variables);
            }
            void execute(IHSLStrip* strip,ExecutionState* state) {
                m_logger->debug("execute PostionAnimator");
                baseStrip = strip;
                executionState = state;
                doCommand(strip,state);
                if (getNext() != NULL) {
                    getNext()->execute(this,state);
                }
            }

            void setHue(int index, int16_t hue, HSLOperation op) {
                m_logger->debug("setHue in PostionAnimator %d %d",index,animateIndex(index));
                if (baseStrip) {
                    baseStrip->setHue(animateIndex(index),hue,op);
                }
            };
            void setSaturation(int index, int16_t hue, HSLOperation op){
                if (baseStrip) {
                    baseStrip->setSaturation(animateIndex(index),hue,op);
                }
            };
            void setLightness(int index, int16_t hue, HSLOperation op){
                if (baseStrip) {
                    baseStrip->setLightness(animateIndex(index),hue,op);
                }

            };
            
            size_t getCount() { if (baseStrip) { return baseStrip->getCount();} else { return 0;}}

        protected:
           virtual void doCommand(IHSLStrip* strip, ExecutionState * state) {
               //not used
            }

            int animateIndex(int index) {
                int start = m_position.getStart(baseStrip);
                int end = m_position.getEnd(baseStrip);
                int count = m_position.getCount(baseStrip);
                if (index < start  || index > end ) {
                    m_logger->error("index out of range %d %d-%d",index,start,end);
                    return index;
                }
                double pos = getAnimationPosition(m_position.getStart(baseStrip),m_position.getEnd(baseStrip));
                if (index == 0) {
                    m_logger->debug("animation position is 0");
                }
                int idx=-1;
                if (m_forward) {
                    idx = start + ((int)(index+pos) % count);
                    if (idx <0 || idx>end) {
                        m_logger->error("forward out of bounds %d %d %f",index,idx,pos);
                    }
                } else {
                    idx = start + ((int)(index-pos+count) %count);
                    if (idx <0 || idx>=count+start) {
                        m_logger->error("backwards out of bounds %d %d %f",index,idx,pos);
                    }
                }

                return idx;
            }

        private:
            Position m_position;
            IHSLStrip * baseStrip;
            bool m_forward;
    };
    
    class HSLCommand : public Command {
    public:
       
        HSLCommand(const char * type, ObjectParser& parser, VariableCommand* variables) : Command(type) {
            m_logger->warn("read position");
            m_position.read(parser,variables);
            m_logger->warn("read value");
            m_value.read(parser,variables);
            m_logger->warn("read op");
            if (!parser.readStringValue("op",hslOpText,20)){
                strcpy(hslOpText,"replace");
            }
            hslOp = getHslOp(hslOpText);

            m_logger->warn("\tparsed HSLCommand %s",m_type);
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

        virtual void doCommand(IHSLStrip* strip, ExecutionState * state) {
            int first = m_position.getStart(strip);
            int count = m_position.getCount(strip);

            for(int idx=0;idx<count;idx++){
                int pos = first + idx;
                int val = m_value.getValueAt(idx,count,state);
                if (val >=0) {
                    setHSLComponent(strip,pos,val);
                }
            }
        }

        virtual void setHSLComponent(IHSLStrip* strip,int index, int value)=0;
        HSLOperation hslOp;
    private:
        Position m_position;
        ValueGradient m_value;
        char hslOpText[10];
    };

    class HueCommand : public HSLCommand {
    public:
        HueCommand(ObjectParser& parser, VariableCommand* variables) : HSLCommand("hue",parser,variables) {

        }

    protected:
        virtual void setHSLComponent(IHSLStrip* strip,int index, int value) {

            strip->setHue(index,value,hslOp);
        }
        
    };
    class SaturationCommand : public HSLCommand {
    public:
        SaturationCommand(ObjectParser& parser, VariableCommand* variables) : HSLCommand("saturation",parser,variables) {

        }
    protected:
        virtual void setHSLComponent(IHSLStrip* strip,int index, int value) {
            strip->setSaturation(index,value,hslOp);
        }        
    };
    class LightnessCommand : public HSLCommand {
    public:
        LightnessCommand(ObjectParser& parser, VariableCommand* variables) : HSLCommand("lightness",parser,variables) {

        }
    protected:
        virtual void setHSLComponent(IHSLStrip* strip,int index, int value) {
            
            strip->setLightness(index,value,hslOp);
        }
    };

    class Script {
        public:
            Script(){
                m_logger = new Logger("Script",10);
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
                VariableCommand * vars = new StartCommand();
                while(commandArray.nextObject(&obj)){
                    m_logger->debug("object: %d %d %d ~~~%.*s~~~",obj.getStart(),obj.getEnd(),obj.getPos(),obj.getEnd()-obj.getStart(),obj.getData()+obj.getStart());

                    //DRBuffer tbuf;
                    //obj.readAll(tbuf);
                    //m_logger->debug("command: %s",tbuf.text());
                    obj.readStringValue("type",tmpBuffer);
                    Command * cmd = NULL;
                    if (strcmp(tmpBuffer,"variable")==0) {
                        vars = VariableCommand::create(obj,vars);
                        cmd = vars;
                    } else if (strcmp(tmpBuffer,"hsl")==0) {
                        cmd = createHSLCommand(obj,vars);
                    } else if (strcmp(tmpBuffer,"position_animator")==0) {
                        cmd = new PositionAnimator(obj,vars);
                    } else {
                        m_logger->error("unknown command type: %s",tmpBuffer);
                    }
                    if (cmd != NULL) {
                        m_logger->info("add command %s",cmd->getType());
                        addCommand(cmd);
                        m_logger->info("\tadded command");
                    }
                }
                parser.readIntValue("brightness",&brightness);
                m_logger->debug("script read success %s",name);
                return true;
            }

            HSLCommand * createHSLCommand(ObjectParser& parser, VariableCommand*variables) {
                parser.readStringValue("component",tmpBuffer);
                HSLCommand * cmd = NULL;
                if (strcmp(tmpBuffer,"hue")==0) {
                    cmd = new HueCommand(parser,variables);
                } else if (strcmp(tmpBuffer,"saturation")==0) {
                    cmd = new SaturationCommand(parser,variables);
                } else if (strcmp(tmpBuffer,"lightness")==0) {
                    cmd = new LightnessCommand(parser,variables);
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
                long now = millis();
                if (m_ledStrip == NULL) {
                    m_logger->errorNoRepeat("no HSLStrip to run script");
                    return;
                }
               // m_logger->debug("step time %d %d %d",now,m_lastStepTime,(now-m_lastStepTime));
                m_ledStrip->clear();
                ExecutionState state(m_config,m_script,NULL);
                state.setTime(m_startTime,now);
                m_script->run(m_ledStrip,&state);
                m_ledStrip->show();
                m_logger->debug("shown");
                m_lastStepTime = now;
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
                        m_logger->info("create forward strip %d",index);
                        compound->add(real);
                    } else {
                        m_logger->info("create reverse strip %d",index);
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


     int Command::getInt(ParserValue&val,int defaultValue,VariableCommand * variables){
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
        double Command::getFloat(ParserValue&val,double defaultValue,VariableCommand * variables){
            double result = defaultValue;
            if (val.type == INTEGER || val.type == FLOAT){
                //m_logger->debug("getInt for INTEGER ParserValue %d %d",val.intValue,val.type);
                result = val.floatValue;
            } else if (val.type == VARIABLE && variables != NULL) {
                //m_logger->debug("getInt for variable ParserValue %s",val.nameValue);
                variables->getFloat(val.nameValue,result);
            } else {
                m_logger->warn("unknown ParserValue type %d %f",val.type,val.floatValue);
            }
            //m_logger->debug("\tresult=%d",result);
            return result;
        }

        const char * Command::getString(ParserValue&val,char * resultValue,size_t maxLen, VariableCommand * variables){
            if (val.type == STRING) {
                strncpy(resultValue,val.stringValue,maxLen);
                return resultValue;
            } else if (val.type == VARIABLE) {
                if (variables->getString(val.nameValue,resultValue,maxLen)) {
                    return resultValue;
                }
            }
            return NULL;

        }
}
#endif