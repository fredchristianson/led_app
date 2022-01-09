#ifndef SCRIPT_LOADER_TEST_H
#define SCRIPT_LOADER_TEST_H

#include "./test_suite.h"
#include "../script_data_loader.h"
#include "../script/script_command.h"
#include "../script/script_value.h"
#include "../script/animation.h"

#if RUN_TESTS==1
namespace DevRelief {

const char *LOAD_SIMPLE_SCRIPT = R"script(
        {
            "name": "simple",
            "commands": [
            {
                "type": "rgb",
                "red": 250
            },            {
                "type": "rgb",
                "red": 250
            },            {
                "type": "rgb",
                "red": 250
            }
            ]
        }        
    )script";    


class ScriptLoaderTestSuite : public TestSuite{
    public:

        static bool Run(Logger* logger) {
            ScriptLoaderTestSuite test(logger);
            test.run();
            return test.isSuccess();
        }

        void run() {
            runTest("testScriptCommandMemLeak",[&](TestResult&r){memLeakScriptCommand(r);});
            runTest("testScriptLoaderMemLeak",[&](TestResult&r){memLeak(r);});
        }

        ScriptLoaderTestSuite(Logger* logger) : TestSuite("ScriptLoader Tests",logger){
        }

    protected: 


    void memLeakScriptCommand(TestResult& result);
    void memLeak(TestResult& result);
};

void ScriptLoaderTestSuite::memLeakScriptCommand(TestResult& result) {
    auto script = new Script();
    auto root = script->getContainer();
    auto* cmd = new RGBCommand(new ScriptNumberValue(0));
    root->add(cmd);
    script->destroy();
}

void ScriptLoaderTestSuite::memLeak(TestResult& result) {
    ScriptDataLoader loader;
    LoadResult load;
    m_logger->debug("load script");
    m_logger->showMemory();
    JsonRoot* root = loader.parse(LOAD_SIMPLE_SCRIPT);
    m_logger->debug("parsed");
    m_logger->showMemory();
    m_logger->debug("jsonToScript");
    Script* script = loader.jsonToScript(root);
    m_logger->debug("got script %x",script);
    m_logger->showMemory();
    m_logger->debug("destroy script");
    if (script) {script->destroy();}
    m_logger->showMemory();
    m_logger->debug("destroy json root");
    delete root;
    m_logger->showMemory();

}




}
#endif 

#endif