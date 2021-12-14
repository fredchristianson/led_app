#ifndef JSON_TEST_H
#define JSON_TEST_H

#include "./test_suite.h"
#include "../script_data_loader.h"

#if RUN_TESTS==1
namespace DevRelief {
    const char * SIMPLE_SCRIPT=R"script(
            {
                "name": "simple",
                "commands": [
                {
                    "type": "rgb",
                    "red": 250
                }
                ]
            }        
        )script";

class JsonTestSuite : public TestSuite{
    public:
        static TestSuite::TestFn jsonTests[];

        static bool Run(Logger* logger) {
            #if RUN_JSON_TESTS
            JsonTestSuite test(logger);
            test.run();
            return test.isSuccess();
            #else
            return false;
            #endif
        }

        void run() {
            runTest("testJsonMemoryFree",[&](TestResult&r){testJsonMemory(r);});
            runTest("testJsonValue",[&](TestResult&r){testJsonValue(r);});
            runTest("testSimpleJson",[&](TestResult&r){testParseSimple(r);});
        }

        JsonTestSuite(Logger* logger) : TestSuite("JSON Tests",logger){

        }

    protected: 


    void testJsonMemory(TestResult& result);
    void testJsonValue(TestResult& result);
    void testParseSimple(TestResult& result);
};

void JsonTestSuite::testJsonMemory(TestResult& result) {
    m_logger->info("create JsonRoot");
    SharedPtr<JsonRoot> root = new JsonRoot();

    m_logger->info("create and populate object");
    JsonObject* obj = root->createObject();
    obj->set("int",1);
    obj->set("string","test");
    obj->set("float",1.2);
    obj->set("bool",true);

    m_logger->info("create and populate array");

    JsonArray* arr = obj->createArray("test array");
    arr->add(true);
    arr->add(1);
    arr->add("test");
    arr->add(1.2);
}

void JsonTestSuite::testJsonValue(TestResult& result) {
    // get int/float/string/bool from all types of nodes
}

void JsonTestSuite::testParseSimple(TestResult& result) {
    m_logger->info("Parse simple JSON Script");
    m_logger->debug(SIMPLE_SCRIPT);
   ScriptDataLoader loader;
   JsonParser parser;
   m_logger->info("\tParse JSON");
   SharedPtr<JsonRoot> root = parser.read(SIMPLE_SCRIPT);
   m_logger->info("\tConverto to script");
   SharedPtr<Script> script = loader.jsonToScript(root.get());
   m_logger->info("\tdone");
}


}
#endif 

#endif