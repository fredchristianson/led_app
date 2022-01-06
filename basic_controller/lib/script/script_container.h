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
            ScriptContainer() : ScriptCommandBase("Container")
            {
                m_logger = &ScriptContainerLogger;
                m_logger->debug("ScriptContainer() create");
                m_position = NULL;
                m_strip  = NULL;
            }

            virtual ~ScriptContainer()
            {
                m_logger->debug("~ScriptContainer()");
            }

            void destroy() override { delete this; }

            ScriptState* getState() { return m_state;}

            void add(IScriptCommand* cmd) { m_commands.add(cmd);}
 
            ScriptPosition* getPosition() override { 
                m_logger->never("container getPosition 0x%x",this);

                if (m_position) {
                    m_logger->never("\tposition %x",m_position);
                    return m_position;
                } else {
                    if (m_parentContainer) {
                        m_logger->never("\tget parent position %x",m_parentContainer);
                        return m_parentContainer->getPosition();
                    }
                    m_logger->never("\tno position");

                    return NULL;
                }
            }
        protected:
            virtual ScriptStatus doCommand(ScriptState *state) {
                m_logger->never("execute %s 0x%x",getType(),this);
                ScriptStatus status = SCRIPT_RUNNING;
                state->setPreviousCommand(NULL);
                m_parentContainer = state->setContainer(this);
                auto oldStrip = state->getStrip();
                if (m_position) {
                    state->setStrip(m_position);
                } else if (m_strip) {
                    state->setStrip(m_strip);
                }
                m_commands.each([&](IScriptCommand*cmd) {
                    m_logger->never("\tcommand 0x%04X - %s - %d",cmd,cmd->getType(),(int)status);
                    if (status == SCRIPT_RUNNING) {
                        status = cmd->execute(state);
                        state->setPreviousCommand(cmd);
                    } else {
                        m_logger->never("\tscript status %d",(int)status);

                    }
                });
                state->setStrip(oldStrip);
                state->setContainer(m_parentContainer);
                m_parentContainer = NULL;
                m_logger->never("\tdoneScriptCommandList.execute()");
                return status;
            }

            Logger *m_logger;
            LinkedList<IScriptCommand*> m_commands;
            
            PositionDomain m_positionDomain;

            IHSLStrip* m_strip;   
            IScriptCommand* m_parentContainer;        
    };

    class ScriptRootContainer : public ScriptContainer {
        public:
            ScriptRootContainer() : ScriptContainer() {

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
                    m_position->setWrap(new ScriptBoolValue(true));
                }
                m_strip = strip;
            }


    };

 
    class ScriptSegmentContainer : public ScriptContainer {
        public:
            ScriptSegmentContainer() : ScriptContainer() {

            }

            virtual ~ScriptSegmentContainer(){

            }

    };

}
#endif