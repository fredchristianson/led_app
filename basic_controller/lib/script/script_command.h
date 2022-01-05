#ifndef DRSCRIPT_COMMAND_H
#define DRSCRIPT_COMMAND_H

#include "../parse_gen.h";
#include "../logger.h";
#include "../led_strip.h";
#include "../config.h";
#include "../ensure.h";
#include "../list.h";
#include "../led_strip.h";
#include "./script_interface.h";
#include "./animation.h";

namespace DevRelief
{
    
    class ScriptCommandBase : public IScriptCommand, IScriptValueProvider
    {
    public:
        ScriptCommandBase(const char *type)
        {
            m_logger = &ScriptCommandLogger;
            m_type = type;
            m_values = NULL;
            m_position = NULL;
            m_logger->debug("Create command %s",type);
        }

        virtual ~ScriptCommandBase()
        {
            delete m_values;
            delete m_position;
        }

        void destroy() override { delete this; }

        PositionUnit getPositionUnit() {
            ScriptPosition* pos = getPosition();
            return pos == NULL ? POS_PERCENT : pos->getPositionUnit();
        };


        void setPositionIndex(int index) override {
            ScriptPosition* pos = getPosition();
            if (pos) {
                pos->setPositionIndex(index);
            } else {
                m_logger->error("Command type %s needs a position",getType());
            }
        }

        ScriptPosition* getPosition() override { 
            m_logger->never("base getPosition 0x%x",this);
            if (m_position) {
                return m_position;
            } else {
                if (m_state->getContainer()) {
                    return m_state->getContainer()->getPosition();
                }
                return NULL;
            }
        }
        
        PositionDomain* getAnimationPositionDomain() override {
            ScriptPosition* strip = getPosition();
            m_logger->never("getAnimationPositionDomain from strip 0x%04X",strip);
            return strip->getAnimationPositionDomain();

        }

        ScriptStatus execute(ScriptState *state) override
        {
            m_logger->never("ScriptCommandBase.execute %s 0x%x",getType(),this);
            m_state = state;
             m_logger->never("get previous");
            m_previousCommand = state->getPreviousCommand();
            if (m_position) {
                m_logger->never("update position");
                m_position->updateValues(this,state);
            } else {
                m_logger->never("no position");
            }
            m_logger->never("setCurrentCommand");
            state->setCurrentCommand(this);
            m_logger->never("doCommand");
            ScriptStatus status = doCommand(state);
            m_logger->never("done %d",state);
            return status;
        }

        
        int getOffset() override { return getPosition()->getOffset();}
        

        /* ValueProvider methods */
        void addValue(const char *name, IScriptValue *value)
        {
            m_logger->debug("ScriptCommandBase.addValue %s 0x%04x",name,value);
            if (m_values == NULL) {
                m_logger->debug("\tcreate ScriptValueList");
                m_values = new ScriptValueList();
            }
            m_values->addValue(name, value);
            m_logger->debug("added NameValue");
        }


     

        /* IScriptValueProvider */
        bool hasValue(const char *name) override
        {
            return m_values == NULL ? false : m_values->hasValue(name) != NULL;
        }

        IScriptValue *getValue(const char *name) override
        {
            m_logger->never("getvalue %s",name);
            IScriptValue * stateValue = m_state->getValue(name);
            if (stateValue) {
                return stateValue;
            }

            IScriptValue* val =  m_values == NULL ? NULL : m_values->getValue(name);
            m_logger->never("\tnot found");
            if ((val == NULL||val->isRecursing()) && m_previousCommand != NULL) {
                m_logger->never("\tcheck previous");
                return m_previousCommand->getValue(name);
            }
            return val;
        }

        int getIntValue(const char * name,int defaultValue) {
            IScriptValue* sv = m_state->getValue(name);
            if (sv == NULL) {
                sv = getValue(name);
            }
            if (sv == NULL) { return defaultValue;}
            return sv->getIntValue(this,defaultValue);
        }

        const char *getType() override { return m_type; }

        void setScriptPosition(ScriptPosition* pos) {
            m_position = pos;
        }

        ScriptState* getState() { return m_state;}


    protected:
        virtual ScriptStatus doCommand(ScriptState *state) = 0;

        Logger *m_logger;
        ScriptPosition *m_position;
        IScriptCommand *m_previousCommand;
        ScriptValueList *m_values;
        DRString m_type;
        ScriptState* m_state;
    };

  
    class ScriptControlCommand : public ScriptCommandBase, IScriptControl
    {
    public:
        ScriptControlCommand() : ScriptCommandBase("ScriptControlCommand")
        {
            ScriptMemoryLogger.debug("ScriptControlCommand");
        }

        virtual ~ScriptControlCommand()
        {
            ScriptMemoryLogger.debug("~ScriptControlCommand()");
        }
    };

    class PositionCommand : public ScriptCommandBase
    {
    public:
        PositionCommand() : ScriptCommandBase("PositionCommand")
        {
        }

        ScriptStatus doCommand(ScriptState * state) override
        {
            // positioning is taken care of by base
            return SCRIPT_RUNNING;
        }

        
    };

    class ScriptValueCommand : public ScriptCommandBase
    {
    public:
        ScriptValueCommand(const char *type = "ScriptValueCommand") : ScriptCommandBase(type)
        {
            memLogger->debug("ScriptValueCommand()");
        }

        virtual ~ScriptValueCommand()
        {
            memLogger->debug("~ScriptValueCommand()");
        }

        ScriptStatus doCommand(ScriptState * state) override
        {
            // nothing to do for this type of command
            return SCRIPT_RUNNING;
        }

    };

    class ScriptParameterCommand : public ScriptCommandBase, IScriptValueProvider
    {
        ScriptParameterCommand(const char *type = "ScriptParameterCommand") : ScriptCommandBase(type)
        {
            memLogger->debug("ScriptParameterCommand()");
        }

        virtual ~ScriptParameterCommand()
        {
            memLogger->debug("~ScriptParameterCommand()");
        }
    };

    class LEDCommand : public ScriptCommandBase
    {
    public:
        LEDCommand(const char * type) : ScriptCommandBase(type)
        {
            memLogger->debug("LEDCommand()");
            m_operation = REPLACE;
        }

        virtual ~LEDCommand()
        {
            memLogger->debug("~LEDCommand() ");
        }

        ScriptStatus doCommand(ScriptState* state) override
        {
            m_logger->never("LEDCommand");

            auto *position = getPosition();
            int count = position->getCount();
            if (count == 0)
            {
                m_logger->never(0, 1000, NULL, "strip has 0 LEDS");
                return SCRIPT_ERROR;
            }
            m_logger->never("LEDCommand count=%d",count);
            auto* positionDomain = position->getAnimationPositionDomain();
            m_logger->never("\tgot position domain %d",count);

            for (int i = 0; i < count; i++)
            {
                m_logger->never("\tLED %d",i);
                position->setPositionIndex(i); 
                m_logger->never("\tposition set %d",i);   
                updateLED(i,position);
                m_logger->never("\tupdated");
            }
            return SCRIPT_RUNNING;
        }

        void setOperation(const char *op) {
            if (Util::equal(op,"replace")){
                m_operation = REPLACE;
            } else if (Util::equal(op,"add")){
                m_operation = ADD;
            }else if (Util::equal(op,"subtract")){
                m_operation = SUBTRACT;
            }else if (Util::equal(op,"average")){
                m_operation = AVERAGE;
            }else if (Util::equal(op,"min")){
                m_operation = MIN;
            }else if (Util::equal(op,"max")){
                m_operation = MAX;
            }
            m_operation = REPLACE;
        }

    protected:
        virtual void updateLED(int index, IHSLStrip* strip)=0;
        HSLOperation m_operation;
    private:
        IScriptValue *m_hue;
        IScriptValue *m_saturation;
        IScriptValue *m_lightness;
    };

   class HSLCommand : public LEDCommand
    {
    public:
        HSLCommand(IScriptValue *h = NULL, IScriptValue *s = NULL, IScriptValue *l = NULL) : LEDCommand("HSLCommand")
        {
            memLogger->debug("HSLCommand()");
            m_hue = h;
            m_saturation = s;
            m_lightness = l;
        }

        virtual ~HSLCommand()
        {
            memLogger->debug("~HSLCommand() start");
            delete m_hue;
            delete m_saturation;
            delete m_lightness;
            memLogger->debug("~HSLCommand() start");
        }

        void setHue(IScriptValue *hue) { m_hue = hue; }
        IScriptValue *getHue(IScriptValue *hue) { return m_hue; }
        void setLightness(IScriptValue *lightness) { m_lightness = lightness; }
        IScriptValue *getLightness(IScriptValue *lightness) { return m_lightness; }
        void setSaturation(IScriptValue *saturation) { m_saturation = saturation; }
        IScriptValue *getSaturation(IScriptValue *saturation) { return m_saturation; }

        void updateLED(int index,  IHSLStrip* strip) override {
            if (m_hue){
                int h = m_hue->getIntValue(this,  -1);
                strip->setHue(index, h, m_operation);
            }
            if (m_lightness) {
                int l = m_lightness ? m_lightness->getIntValue(this, -1) : -1;
                strip->setLightness(index, l, m_operation);
            }
            if (m_saturation){
                int s = m_saturation ? m_saturation->getIntValue(this, -1) : -1;
                strip->setSaturation(index, s, m_operation);
            }

        }

    private:
        IScriptValue *m_hue;
        IScriptValue *m_saturation;
        IScriptValue *m_lightness;
    };

    class RGBCommand : public LEDCommand
    {
    public:
        RGBCommand(IScriptCommand* container,IScriptValue *r = 0, IScriptValue *g = 0, IScriptValue *b = 0) : LEDCommand("RGBCommand")
        {
            memLogger->debug("RGBCommand()");
            m_red = r;
            m_green = g;
            m_blue = b;
        }

        virtual ~RGBCommand()
        {
            memLogger->debug("~RGBCommand() start");
            delete m_red;
            delete m_green;
            delete m_blue;
            memLogger->debug("~RGBCommand() done");
        }

        void setRed(IScriptValue *red) { m_red = red; }
        IScriptValue *getRed(IScriptValue *red) { return m_red; }
        void setGreen(IScriptValue *green) { m_green = green; }
        IScriptValue *getGreen(IScriptValue *green) { return m_green; }
        void setBlue(IScriptValue *blue) { m_blue = blue; }
        IScriptValue *getBlue(IScriptValue *blue) { return m_blue; }

        void updateLED(int index, IHSLStrip* strip) override {
                m_logger->never("RGB Led %d (0x%04, 0x%04, 0x%04)",index, m_red, m_green, m_blue);
                int r = m_red ? m_red->getIntValue(this, 0) : 0;
                m_logger->never("\tred=%d", r);
                int g = m_green ? m_green->getIntValue(this, 0) : 0;
                m_logger->never("\tgreen=%d", g);
                int b = m_blue ? m_blue->getIntValue(this, 0) : 0;
                m_logger->never("\tblue=%d", b);
                CRGB crgb(r, g, b);
                strip->setRGB(index, crgb, m_operation);
        }

    private:
        IScriptValue *m_red;
        IScriptValue *m_blue;
        IScriptValue *m_green;
    };

    
 

}
#endif