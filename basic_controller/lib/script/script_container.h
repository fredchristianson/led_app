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
            ScriptContainer(const char * type = "Container") : ScriptCommandBase(type)
            {
                m_logger->debug("ScriptContainer() create");
                m_position = NULL;
                m_strip  = NULL;
            }

            virtual ~ScriptContainer()
            {
                m_logger->debug("~ScriptContainer()");
            }

            void destroy() override { delete this; }

            IScriptState* getState() { return m_state;}

            void add(IScriptCommand* cmd) { 
                m_logger->never("\tadd command %s",cmd->getType());

                m_commands.add(cmd);
            }
 
            ScriptPosition* getPosition() override { 
                m_logger->always("container getPosition 0x%x",this);

                if (m_position) {
                    m_logger->always("\tposition %x",m_position);
                    return m_position;
                } else {
                    if (m_parentContainer) {
                        m_logger->always("\tget parent position %x",m_parentContainer);
                        return m_parentContainer->getPosition();
                    }
                    m_logger->always("\tno position");

                    return NULL;
                }
            }
        protected:
            virtual ScriptStatus doCommand(IScriptState *state) {
                m_logger->debug("ScriptContainer.execute");
                m_logger->debug("\t %s 0x%x",getType(),this);
                ScriptStatus status = SCRIPT_RUNNING;
                m_logger->debug("\tsetprev");
                state->setPreviousCommand(NULL);
                m_logger->debug("\tsetcontainer");
                m_parentContainer = state->setContainer(this);
                m_logger->debug("\tgetstrip");
                auto oldStrip = state->getStrip();
                if (m_position) {
                    state->setStrip(m_position);
                } else if (m_strip) {
                    state->setStrip(m_strip);
                }
                m_logger->debug("\truncommands");
                status = runCommands(state);
                m_logger->debug("\tset old strip");
                state->setStrip(oldStrip);
                state->setContainer(m_parentContainer);
                m_parentContainer = NULL;
                m_logger->never("\tdoneScriptCommandList.execute()");
                return status;
            }

            virtual ScriptStatus runCommands(IScriptState* state) {
                ScriptStatus status = SCRIPT_RUNNING;
                m_commands.each([&](IScriptCommand*cmd) {
                    m_logger->never("\tcommand 0x%04X - %s - %d",cmd,cmd->getType(),(int)status);
                    if (status == SCRIPT_RUNNING) {
                        status = cmd->execute(state);
                        state->setPreviousCommand(cmd);
                    } else {
                        m_logger->never("\tscript status %d",(int)status);

                    }
                });
                return status;
            }

            PtrList<IScriptCommand*> m_commands;
            
            PositionDomain m_positionDomain;

            IHSLStrip* m_strip;   
            IScriptCommand* m_parentContainer;        
    };

    class ScriptRootContainer : public ScriptContainer {
        public:
            ScriptRootContainer() : ScriptContainer("RootContainer") {

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
            ScriptSegmentContainer() : ScriptContainer("SegmentContainer") {

            }

            virtual ~ScriptSegmentContainer(){

            }

    };

    class TemplateInstance {
        public:
            TemplateInstance(IScriptCommand* parent,ScriptValueList& values){
                m_parent=parent;
                m_logger = &ScriptCommandLogger;;
                m_state = parent->getState()->createChild();
                values.each([&](NameValue* nv){
                    IScriptValue*val = new ScriptNumberValue(parent,nv->getValue(),0);
                    m_logger->debug("add ChildState %s=%s",nv->getName(),val->toString().get());
                    m_state->setValue(nv->getName(),val);
                });
            }

            ~TemplateInstance() {
                delete m_state;
            }

            ScriptStatus run(PtrList<IScriptCommand*>& commands){
                m_logger->debug("run instance");
                ScriptStatus status = SCRIPT_RUNNING;
                m_state->setPreviousCommand(m_parent);
                commands.each([&](IScriptCommand*cmd) {
                    m_logger->always("\tcommand 0x%04X - %s - %d",cmd,cmd->getType(),(int)status);
                    if (status == SCRIPT_RUNNING) {
                        status = cmd->execute(m_state);
                        m_state->setPreviousCommand(cmd);
                    } else {
                        m_logger->debug("\tscript status %d",(int)status);

                    }
                });
                m_state->setPreviousCommand(NULL);
                m_logger->debug("\tinstance done");
                return status;
            }

        protected:
            Logger* m_logger;
            IScriptCommand* m_parent;
            IScriptState* m_state;
    };

    class ScriptTemplate : public ScriptContainer {
        public:
            ScriptTemplate() : ScriptContainer("ScriptTemplate") {
                m_logger->never("create ScriptTemplate");
            }

            virtual ~ScriptTemplate(){

            }

            void addValue(const char * name, IScriptValue* value) override {
                m_logger->always("\tadd value %s=%s",name,value->toString().get());
                m_values.addValue(name,value);
            }

           virtual ScriptStatus runCommands(IScriptState* state) {
                int count = m_count ? m_count->getIntValue(this,0) : 0;
                int minCount = m_minCount ? m_minCount->getIntValue(this,0) : 0;
                int maxCount = m_maxCount ? m_maxCount->getIntValue(this,0) : 0;
                minCount = max(minCount,count);
                maxCount = max(maxCount,count);
                if (maxCount == 0) {
                    return SCRIPT_RUNNING;
                }
                m_logger->debug("run template");
                while(maxCount < m_instances.size()) {
                    m_logger->debug("\t remove instance");
                    m_instances.removeAt(0);
                }
                while(minCount > m_instances.size()) {
                    m_logger->debug("\t create instance with %d values",m_values.count());
                    TemplateInstance* inst = new TemplateInstance(this,m_values);
                    m_logger->debug("\t created");
                    m_instances.add(inst);
                    m_logger->debug("\t added");
                };

                ScriptStatus status = SCRIPT_RUNNING;
                m_instances.each([&](TemplateInstance*instance){
                    m_logger->debug("\t run instance 0x%x",instance);
                    instance->run(m_commands);
                    m_logger->debug("\t ran");
                });

                return status;
            }

            void setCount(IScriptValue*count) { m_count = count;}
            void setMinCount(IScriptValue*count) { m_minCount = count;}
            void setMaxCount(IScriptValue*count) { m_maxCount = count;}

        protected:
            IScriptValue* m_count;
            IScriptValue* m_minCount;
            IScriptValue* m_maxCount;
            ScriptValueList m_values;
            PtrList<TemplateInstance*> m_instances;

    };

}
#endif