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
        ScriptCommandBase( ICommandContainer* container,const char *type)
        {
            m_logger = &ScriptCommandLogger;
            m_type = type;
            m_values = NULL;
            m_position = NULL;
            m_container = container;
            m_logger->debug("Create command %s with container 0x%04X",type,container);
        }

        virtual ~ScriptCommandBase()
        {
            delete m_values;
            delete m_position;
        }

        void destroy() override { delete this; }

        ICommandContainer* getContainer() { return m_container;}

        IStripModifier* getStrip() override {
            return m_container->getStrip();
        }

        IStripModifier* getPosition() override {
            if (m_position) {
                m_logger->always("have position 0x%04X",m_position);
                return m_position;
            } else if (m_container != NULL) {
                m_logger->always("get position from container 0x%04X",m_previousCommand);
                return m_container->getPosition();
            } else {
                m_logger->always("no position");
                return NULL;
            }
        }

        void setPosition(int index) override {
            IStripModifier* pos = getPosition();
            if (pos) {
                pos->setPosition(index);
            } else {
                m_logger->error("Command type %s needs a position",getType());
            }
        }

        
        PositionDomain* getAnimationPositionDomain() override {
            IStripModifier* strip = getPosition();
            m_logger->debug("getAnimationPositionDomain from strip 0x%04X",strip);
            return strip->getAnimationPositionDomain();

        }


        IHSLStrip * getHSLStrip() override { return m_container->getHSLStrip();}

        ScriptStatus execute(ScriptState *state) override
        {
            m_logger->always("ScriptCommandBase.execute");
            m_logger->debug("execute command %s state=0x%04X",getType(),state);
            m_state = state;
             m_logger->debug("get previous");
            m_previousCommand = state->getPreviousCommand();
            if (m_position) {
                m_logger->debug("update position");
                m_position->updateValues(this,state,m_container);
            } else {
                m_logger->debug("no position");
            }
            m_logger->debug("setCurrentCommand");
            state->setCurrentCommand(this);
            m_logger->debug("doCommand");
            ScriptStatus status = doCommand(state);
            m_logger->debug("done %d",state);
            return status;
        }

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
        ICommandContainer* m_container;
    };

  
    class ScriptControlCommand : public ScriptCommandBase, IScriptControl
    {
    public:
        ScriptControlCommand(ICommandContainer* container) : ScriptCommandBase(container,"ScriptControlCommand")
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
        PositionCommand(ICommandContainer* container) : ScriptCommandBase(container,"PositionCommand")
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
        ScriptValueCommand(ICommandContainer* container,const char *type = "ScriptValueCommand") : ScriptCommandBase(container,type)
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
        ScriptParameterCommand(ICommandContainer* container,const char *type = "ScriptParameterCommand") : ScriptCommandBase(container,type)
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
        LEDCommand(ICommandContainer* container,const char * type) : ScriptCommandBase(container,type)
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
            m_logger->always("LEDCommand");

            auto *strip = getPosition();
            if (strip == NULL) {
                strip = getStrip();
                m_logger->always("got strip %04x",strip);
            } else {
                m_logger->always("got position %04x",strip);
            }
            int count = strip->getCount();
            if (count == 0)
            {
                m_logger->always(0, 1000, NULL, "strip has 0 LEDS");
                return SCRIPT_ERROR;
            }
            m_logger->always("LEDCommand count=%d",count);
            auto* positionDomain = strip->getAnimationPositionDomain();
            m_logger->always("\tgot position domain %d",count);
            

            for (int i = 0; i < count; i++)
            {
                strip->setPosition(i);    
                updateLED(i,strip);
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
        HSLCommand(ICommandContainer* container,IScriptValue *h = NULL, IScriptValue *s = NULL, IScriptValue *l = NULL) : LEDCommand(container,"HSLCommand")
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

        void updateLED(int index, IHSLStrip* strip) override {
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
        RGBCommand(ICommandContainer* container,IScriptValue *r = 0, IScriptValue *g = 0, IScriptValue *b = 0) : LEDCommand(container,"RGBCommand")
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