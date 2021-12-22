#ifndef DRSCRIPT_CONTAINER_H
#define DRSCRIPT_CONTAINER_H

#include "../parse_gen.h";
#include "../logger.h";
#include "../led_strip.h";
#include "../config.h";
#include "../ensure.h";
#include "../list.h";
#include "../led_strip.h";
#include "./script_interface.h";
#include "./script.h";
#include "./animation.h";

namespace DevRelief
{
    Logger ScriptContainerLogger("Container",SCRIPT_CONTAINER_LOGGER_LEVEL);

    class ScriptContainer : public ScriptCommandBase, public ICommandContainer
    {
        public:
            ScriptContainer(ICommandContainer*container) : ScriptCommandBase(container,"Container")
            {
                m_logger = &ScriptContainerLogger;
                m_logger->debug("ScriptContainer() create");
            
            }

            virtual ~ScriptContainer()
            {
                m_logger->debug("~ScriptContainer()");
            }

            void destroy() override { delete this; }

            ScriptState* getState() { return m_state;}

            void add(IScriptCommand* cmd) { m_commands.add(cmd);}
            IHSLStrip * getHSLStrip() override {

            }
            IStripModifier* getStrip(){
                if (m_position){
                    return m_position;
                }
                return getContainer()->getStrip();
            };

        protected:
            virtual ScriptStatus doCommand(ScriptState *state) {
                ScriptStatus status = SCRIPT_RUNNING;
                state->setPreviousCommand(NULL);
                m_commands.each([&](IScriptCommand*cmd) {
                    m_logger->never("\tcommand 0x%04X - %s - %d",cmd,cmd->getType(),(int)status);
                    if (status == SCRIPT_RUNNING) {
                        status = cmd->execute(state);
                        state->setPreviousCommand(cmd);
                    } else {
                        m_logger->never("\tscript status %d",(int)status);

                    }
                });
                m_logger->never("\tdoneScriptCommandList.execute()");
                return status;
            }

            Logger *m_logger;
            LinkedList<IScriptCommand*> m_commands;
    };

    class ScriptRootContainer : public ScriptContainer,  IStripModifier {
        public:
            ScriptRootContainer() : ScriptContainer(NULL) {

            }

            virtual ~ScriptRootContainer(){

            }

            void destroy() override { delete this; }

            void setStrip(IHSLStrip* strip) {
                m_strip = strip;
            }
            
            IHSLStrip * getHSLStrip() override {
                return this;
            }

            IStripModifier* getStrip(){
                return this;
            };

            void setHue(int index, int16_t hue, HSLOperation op=REPLACE){
                m_strip->setHue(index,hue,op);
            }
            void setSaturation(int index, int16_t saturation, HSLOperation op=REPLACE){
                m_strip->setSaturation(index,saturation,op);
            }
            void setLightness(int index, int16_t lightness, HSLOperation op=REPLACE){
                m_strip->setLightness(index,lightness,op);
            }
            void setRGB(int index, const CRGB& rgb, HSLOperation op=REPLACE){
                m_strip->setRGB(index,rgb,op);
            }
            int getCount() override {
                return m_strip->getCount();
            }
            int getStart()override{
                return m_strip->getStart();
            }
            int getEnd()override{
                return m_strip->getStart()+m_strip->getCount()-1;
            }
            int getOffset()override{
                return 0;
            }
            void clear()override{
                m_strip->clear();
            }
            void show()override{
                m_strip->show();
            }

            PositionUnit getPositionUnit() override { return POS_PERCENT;}
        
        private: 
            IHSLStrip* m_strip;
    };

 
    class ScriptSegmentContainer : public ScriptContainer {
        public:
            ScriptSegmentContainer(ICommandContainer*container) : ScriptContainer(container) {

            }

            virtual ~ScriptSegmentContainer(){

            }

    };

}
#endif