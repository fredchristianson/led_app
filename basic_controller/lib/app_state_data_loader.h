#ifndef APP_STATE_DATA_LOADER_H
#define APP_STATE_DATA_LOADER_H

#include "./logger.h"
#include "./parse_gen.h"
#include "./buffer.h"
#include "./data.h"
#include "./data_loader.h"
#include "./util.h"
#include "./app_state.h"


namespace DevRelief {

Logger StateLoaderLogger("AppStateDataLoader",APP_STATE_LOGGER_LEVEL);
const char * STATE_PATH_BASE="/state/";


class AppStateDataLoader : public DataLoader {
    public:
        AppStateDataLoader() {
            m_logger = &StateLoaderLogger;
        }

        DRString getPath(const char * name) {
            DRString path= STATE_PATH_BASE;
            path += name;
            path += ".json";
            m_logger->debug("AppStateDataLoader getPath(%s)==>%s",name,path.text());
            return path;
        }


        bool save(AppState& state){
            m_logger->debug("save AppState");
            auto paramJson = state.getParameters();
            m_logger->debug("got params");

            SharedPtr<JsonRoot> jsonRoot = toJson(state);
            
            bool success = writeJsonFile(getPath("state"),jsonRoot->getTopElement());
            m_logger->debug("\twrite %s",success?"success":"failed");
            return success;
        }

        bool load(AppState& state) {
            LoadResult result;
            if (loadFile(getPath("state"),result)) {
                JsonObject* obj = result.getJsonRoot()->asObject();
                if (obj != NULL) {
                    state.setExecuteType((ExecuteType)obj->get("type",EXECUTE_NONE));
                    state.setExecuteValue(obj->get("value",(const char *)0));
                    state.setIsRunning(obj->get("is-running",false));
                    state.setIsStarting(obj->get("is-starting",false));
                    state.setParameters(obj->getChild("parameters"));
                    auto paramJson = state.getParameters();
                    DRString json = paramJson? paramJson->toJsonString() : DRString();
                    m_logger->debug("Load AppState: %s %s %d %s %s",
                                state.isStarting()?"starting":"",
                                state.isRunning()?"running":"",
                                (int)state.getType(),
                                state.getExecuteValue().text(),
                                json.text());

                    return true;
                }
            }
            m_logger->debug("AppState not loaded");
            return false;
        }

        SharedPtr<JsonRoot> toJson(AppState& state) {
            m_logger->debug("toJson");
            SharedPtr<JsonRoot> jsonRoot = new JsonRoot();
            m_logger->debug("\tcreateObj");
            JsonObject* obj = jsonRoot->getTopObject();
            m_logger->debug("\tset type");
            obj->set("type",(int)state.getType());
            m_logger->debug("\tset running");
            obj->set("is-running",state.isRunning());
            m_logger->debug("\tset starting");
            obj->set("is-starting",state.isStarting());
            m_logger->debug("\tset value");
            obj->set("value",state.getExecuteValue().text());
            m_logger->debug("\tcreate params");
            JsonObject* params = obj->createObject("parameters");
            m_logger->debug("\tcopy params");
            state.copyParameters(params);
            m_logger->debug("\tobj: %s",obj->toJsonString().text());
            m_logger->debug("\treturn root: %s",jsonRoot->toJsonString().text());
            return jsonRoot;            
        }

      
    protected:

    private:
};
};
#endif
