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
            ScriptContainer(IScriptCommand*container) : ScriptCommandBase(container,"Container")
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
            
            PositionDomain m_positionDomain;
    };

    class ScriptRootContainer : public ScriptContainer {
        public:
            ScriptRootContainer() : ScriptContainer(NULL) {

            }

            virtual ~ScriptRootContainer(){

            }

            void destroy() override { delete this; }

            virtual PositionDomain* getAnimationPositionDomain() { 
                return m_position->getAnimationPositionDomain();
            }

            virtual void setPosition(int index) { 
                m_logger->debug("ScriptRootContainer.setPosition(%d)",index);
                m_position->setPositionIndex(index);
            }

            void setStrip(IHSLStrip* strip) {
                if (m_position == NULL) {
                    m_position = new ScriptPosition();
                }
                m_strip = strip;
            }
            
            IHSLStrip * getStrip() override {
                return m_strip;
            }


        private: 
            IHSLStrip* m_strip;
    };

 
    class ScriptSegmentContainer : public ScriptContainer {
        public:
            ScriptSegmentContainer(IScriptCommand*container) : ScriptContainer(container) {

            }

            virtual ~ScriptSegmentContainer(){

            }

    };

}
#endif