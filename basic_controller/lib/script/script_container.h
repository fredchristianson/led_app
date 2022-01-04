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
                m_position = new ScriptPosition();
            
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
            IStripModifier* getStrip() override {
                if (m_position){
                    m_logger->always("ScriptContainer.getStrip has position 0x%04X",m_position);
                    return m_position;
                }
                m_logger->always("ScriptContainer.getStrip return container strip 0x%04X",getContainer());
                return getContainer()->getStrip();
            };

        protected:
            virtual ScriptStatus doCommand(ScriptState *state) {
                ScriptStatus status = SCRIPT_RUNNING;
                state->setPreviousCommand(NULL);
                m_commands.each([&](IScriptCommand*cmd) {
                    m_logger->always("\tcommand 0x%04X - %s - %d",cmd,cmd->getType(),(int)status);
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
            
            PositionDomain m_positionDomain;
    };

    class ScriptRootContainer : public ScriptContainer,  public IStripModifier {
        public:
            ScriptRootContainer() : ScriptContainer(this) {

            }

            virtual ~ScriptRootContainer(){

            }

            void destroy() override { delete this; }

            virtual PositionDomain* getAnimationPositionDomain() { 
                return m_position->getAnimationPositionDomain();
            }

            virtual void setPosition(int index) { 
                m_logger->debug("ScriptRootContainer.setPosition(%d)",index);
                m_position->setPosition(index);
            }

            void setStrip(IHSLStrip* strip) {
                if (m_position == NULL) {
                    m_position = new ScriptPosition();
                }
                m_strip = strip;
            }
            
            IHSLStrip * getHSLStrip() override {
                return this;
            }

            IStripModifier* getStrip() override {
                m_logger->always("ScriptRootContainer.getStrip 0x%04X",this);
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

            PositionUnit getPositionUnit() override { return m_position->getPositionUnit();}
        
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