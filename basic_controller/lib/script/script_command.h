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
        }

        virtual ~ScriptCommandBase()
        {
            delete m_values;
            delete m_position;
        }

        void destroy() override { delete this; }


        ScriptStatus execute(ScriptState *state, IScriptCommand *previous) override
        {
            m_logger->never("execute command %s state=0x%04X  previous=0x%04X",getType(),state,previous);
            m_previousCommand = previous;
            ScriptStatus status = doCommand(state);
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


        /* IHSLStrip interface - pass to previous command */
        void setHue(int index, int16_t hue, HSLOperation op = REPLACE)
        {
            if (m_previousCommand)
            {
                m_previousCommand->setHue(modifyPosition(index), modifyHue(hue), modifyOperation(op));
            }
        };
        void setSaturation(int index, int16_t saturation, HSLOperation op = REPLACE)
        {
            if (m_previousCommand)
            {
                m_previousCommand->setSaturation(modifyPosition(index), modifySaturation(saturation), modifyOperation(op));
            }
        };
        void setLightness(int index, int16_t lightness, HSLOperation op = REPLACE)
        {
            if (m_previousCommand)
            {
                m_previousCommand->setLightness(modifyPosition(index), modifyLightness(lightness), modifyOperation(op));
            }
        };
        void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE)
        {
            if (m_previousCommand)
            {
                m_previousCommand->setRGB(modifyPosition(index), modifyRGB(rgb), modifyOperation(op));
            }
        };
        size_t getCount()
        {
            m_logger->never("getCount() for ScriptCommandBase");
            return m_position ? m_position->getCount() : m_previousCommand->getCount();
        };
        size_t getStart()
        {
            if (!Ensure::notNull(m_previousCommand, NULL, "ScriptCommandBase.getStart() needs m_previousCommand"))
            {
                return 0;
            }
            return m_position ? m_position->getStart() : m_previousCommand->getStart();
        };
        void clear()
        {
            if (m_previousCommand)
            {
                m_previousCommand->clear();
            }
        }
        void show()
        {
            if (m_previousCommand)
            {
                m_previousCommand->show();
            }
        }

        /* IScriptValueProvider */
        bool hasValue(const char *name) override
        {
            return m_values == NULL ? false : m_values->hasValue(name) != NULL;
        }

        IScriptValue *getValue(const char *name) override
        {
            IScriptValue* val =  m_values == NULL ? NULL : m_values->getValue(name);
            if (val == NULL && m_previousCommand != NULL) {
                return m_previousCommand->getValue(name);
            }
        }

        int getIntValue(const char * name,int defaultValue) {
            IScriptValue* sv = getValue(name);
            if (sv == NULL) { return defaultValue;}
            return sv->getIntValue(this,defaultValue);
        }

        const char *getType() override { return m_type; }

        void setScriptPosition(ScriptPosition* pos) {
            m_position = pos;
        }
    protected:
        virtual ScriptStatus doCommand(ScriptState *state) = 0;

        virtual int modifyPosition(int index) { return index; }
        virtual int16_t modifyHue(int16_t value) { return value; }
        virtual int16_t modifySaturation(int16_t value) { return value; }
        virtual int16_t modifyLightness(int16_t value) { return value; }
        virtual CRGB modifyRGB(CRGB rgb) { return rgb; }
        virtual HSLOperation modifyOperation(HSLOperation op) { return op; }

        Logger *m_logger;
        ScriptPosition *m_position;
        IScriptCommand *m_previousCommand;
        ScriptValueList *m_values;
        DRString m_type;
    };

    /* the first command in a CommandList.  It does not have a previous command like others */
    class ScriptStartCommand : public IScriptCommand
    {
        public:
            ScriptStartCommand() 
            {
                ScriptMemoryLogger.debug("ScriptStartCommand");
                m_strip = NULL;
                m_logger = &ScriptCommandLogger;
            }

            virtual ~ScriptStartCommand()
            {
                ScriptMemoryLogger.debug("~ScriptStartCommand()");
            }

            void destroy() override { delete this;}
            ScriptStatus execute(ScriptState *state, IScriptCommand *previous) override
            {
                return SCRIPT_RUNNING;
            }

            void setHue(int index, int16_t hue, HSLOperation op=REPLACE) {
                if (m_strip) {
                    m_strip->setHue(index,hue,op);
                }
            }
            void setSaturation(int index, int16_t saturation, HSLOperation op=REPLACE){
                if (m_strip) {
                    m_strip->setSaturation(index,saturation,op);
                }
            }
            void setLightness(int index, int16_t lightness, HSLOperation op=REPLACE){
                if (m_strip) {
                    m_strip->setLightness(index,lightness,op);
                }
            }
            void setRGB(int index, const CRGB& rgb, HSLOperation op=REPLACE){
                if (m_strip) {
                    m_strip->setRGB(index,rgb,op);
                }
            }
            size_t getCount(){
                m_logger->never("getCount() for ScriptStartCommand");
                if (m_strip) {
                    return m_strip->getCount();
                }
                m_logger->never("\t no strip");
                return 0;
            }
            size_t getStart(){
                if (m_strip) {
                    return m_strip->getStart();
                }
                return 0;
            }
            void clear(){
                if (m_strip) {
                    m_strip->clear();
                }
            }
            void show(){
                if (m_strip) {
                    m_strip->show();
                }
            }

            virtual IScriptValue* getValue(const char* name) { 
                return NULL;
            }

            const char *getType() override { return "ScriptStartCommand"; }
            void setStrip(IHSLStrip* strip) { m_strip = strip;}
        private: 
            IHSLStrip* m_strip;
            Logger* m_logger;

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

    class HSLCommand : public ScriptCommandBase
    {
    public:
        HSLCommand(IScriptValue *h = NULL, IScriptValue *s = NULL, IScriptValue *l = NULL) : ScriptCommandBase("HSLCommand")
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

        ScriptStatus doCommand(ScriptState* state) override
        {
            if (m_previousCommand == NULL) {
                m_logger->periodic(ERROR_LEVEL,1000,NULL,"HSLCommand.doCommand does not have a previousCommand");
            }
            auto *strip = m_previousCommand;
            int count = m_previousCommand->getCount();
            if (count == 0)
            {
                m_logger->periodic(0, 1000, NULL, "strip has 0 LEDS");
                return SCRIPT_ERROR;
            }
            for (int i = 0; i < count; i++)
            {
                state->setLedPosition(i,0,count);
               int h = m_hue ? m_hue->getIntValue(this,  -1) : -1;
                strip->setHue(i, h, REPLACE);
                int l = m_lightness ? m_lightness->getIntValue(this, -1) : -1;
                strip->setLightness(i, l, REPLACE);
                int s = m_saturation ? m_saturation->getIntValue(this, -1) : -1;
                strip->setSaturation(i, s, REPLACE);
            }
            return SCRIPT_RUNNING;
        }

    private:
        IScriptValue *m_hue;
        IScriptValue *m_saturation;
        IScriptValue *m_lightness;
    };

    class RGBCommand : public ScriptCommandBase
    {
    public:
        RGBCommand(IScriptValue *r = 0, IScriptValue *g = 0, IScriptValue *b = 0) : ScriptCommandBase("RGBCommand")
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

        ScriptStatus doCommand(ScriptState* state) override
        {
            m_logger->never("RGBCommand.doCommand()");

            if (m_previousCommand == NULL) {
                m_logger->periodic(ERROR_LEVEL,1000,NULL,"RGBCommand::doCommand requires previouscommand");
            }
            IHSLStrip *strip = m_previousCommand;
            m_logger->never("\tget count from strip");
            int count = strip->getCount();
            if (count == 0)
            {
                m_logger->periodic(0, 1000, NULL, "strip has 0 LEDS");
                return SCRIPT_ERROR;
            }
            for (int i = 0; i < count; i++)
            {
                state->setLedPosition(i,0,count-1);
                m_logger->never("RGB Led 0x%04, 0x%04, 0x%04", m_red, m_green, m_blue);
                int r = m_red ? m_red->getIntValue(this, 0) : 0;
                m_logger->never("\tred=%d", r);
                int g = m_green ? m_green->getIntValue(this, 0) : 0;
                m_logger->never("\tgreen=%d", g);
                int b = m_blue ? m_blue->getIntValue(this, 0) : 0;
                m_logger->never("\tblue=%d", b);
                CRGB crgb(r, g, b);
                strip->setRGB(i, crgb, REPLACE);
            }
            return SCRIPT_RUNNING;
        }



    private:
        IScriptValue *m_red;
        IScriptValue *m_blue;
        IScriptValue *m_green;
    };

    
    class ScriptCommandList {
        public:
            ScriptCommandList() {
                m_logger = &ScriptLogger;
                m_logger->debug("ScriptCommandList()");
                m_start = new ScriptStartCommand();
                add(m_start);
            }

            virtual ~ScriptCommandList() {
                m_logger->debug("~ScriptCommandList()");
                m_commands.each([&](IScriptCommand*cmd) {
                    cmd->destroy();
                });
            }

            ScriptStatus execute(ScriptState* state) {
                ScriptStatus status = SCRIPT_RUNNING;
                IScriptCommand* previous = NULL;
                m_logger->never("ScriptCommandList.execute()");
                m_start->setStrip(m_strip);
                m_commands.each([&](IScriptCommand*cmd) {
                    m_logger->never("\tcommand 0x%04X - %s",cmd,cmd->getType());
                    if (status == SCRIPT_RUNNING) {
                        status = cmd->execute(state,previous);
                    } else {
                        m_logger->never("\tscript status %d",(int)status);

                    }
                    previous = cmd;
                });
                m_logger->never("\tdoneScriptCommandList.execute()");
                
                return status;
            }

            void add(IScriptCommand* cmd) {
                m_commands.add(cmd);
            }

            void setStrip(IHSLStrip* strip) {
                m_strip = strip;
            }
        private:
            IHSLStrip* m_strip;
            ScriptStartCommand* m_start;
            LinkedList<IScriptCommand*> m_commands;
            Logger* m_logger;
    };


}
#endif