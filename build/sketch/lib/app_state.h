#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\app_state.h"
#ifndef APP_STATE_H
#define APP_STATE_H

#include "./parse_gen.h";
#include "./logger.h";
#include "./data.h";
#include "./list.h";
#include "./util.h";

namespace DevRelief {
    Logger AppStateLogger("AppState",APP_STATE_LOGGER_LEVEL);

    typedef enum ExecuteType  {
        EXECUTE_NONE=0,
        EXECUTE_API=1,
        EXECUTE_SCRIPT=2
    };

    class AppState {
        public:
 
            AppState() {
                m_logger = &AppStateLogger;
                m_executeType = EXECUTE_NONE;
                m_starting = true;
                m_running = false;
            }

            ~AppState() { }

            void setIsStarting(bool is) { 
                m_logger->debug("setIsStarting %d",is?1:0);
                m_starting = is;}
            void setIsRunning(bool is) { 
                m_logger->debug("setIsRunning %d",is?1:0);
                m_running = is;}
            void setExecuteType(ExecuteType type) { m_executeType = type;}
            void setExecuteValue(const char * val) { m_executeValue = val;}
            void setParameters(JsonObject* params) {
                JsonObject* old = m_root.getTopObject();
                copyParameters(old,params);
            }

            void setApi(const char * api, JsonObject*params) {
                setExecuteValue(api);
                // when setApi is called, the API is running and has started
                setIsRunning(true);
                setIsStarting(false);
                setExecuteType(EXECUTE_API);
                copyParameters(m_root.getTopObject(),params);
            }
             
            void setScript(const char * name, JsonObject*params) {
                setExecuteValue(name);
                setIsRunning(true);
                // set as starting until it has run long
                // enough to see it works (10 seconds?).
                // otherwise can get in a reboot-loop
                setIsStarting(true);
                setExecuteType(EXECUTE_SCRIPT);
                copyParameters(m_root.getTopObject(),params);
            }

            void copyParameters(JsonObject*  toObj,JsonObject*params=NULL) {
                m_logger->never("copyParameters");
                toObj->clear();
                m_logger->never("\tcleared");
                if (params == NULL){
                    m_logger->never("\tget current");
                    params = m_root.getTopObject();
                }
                if (params == NULL) {
                    m_logger->never("\tno params");

                    return;
                }

                m_logger->never("\teachProperty");

                params->eachProperty([&](const char * name, JsonElement*val){
                    m_logger->never("\tcopy %s",name);
                    toObj->set(name,val->getString());
                });
                m_logger->never("done");
            }

            JsonObject* getParameters() { return m_root.getTopObject();}
            const DRString& getExecuteValue()const { return m_executeValue;}
            bool isStarting() { return m_starting;}
            bool isRunning() { return m_running;}
            ExecuteType getType() { return m_executeType;}
    private:            
            DRString        m_executeValue;
            bool            m_starting;
            bool            m_running;
            ExecuteType     m_executeType;
            Logger *        m_logger;
            JsonRoot        m_root;
    };

}


#endif