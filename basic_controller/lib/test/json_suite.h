#ifndef JSON_TEST_H
#define JSON_TEST_H

#include "./test_suite.h"
#include "../script_data_loader.h"

#if RUN_TESTS == 1
namespace DevRelief
{
    const char *SIMPLE_SCRIPT = R"script(
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

    const char *VALUES_SCRIPT = R"script(
        {
            "commands": [
            {
                "type": "values",
                "x": 100,
                "yy": 200,
                "z": 300
            },
            {
                "type": "values",
                "x": "101"
                "y": "var(yy)",
            }
            ]
        }       
        )script";

    const char *POSITION_SCRIPT = R"script(
        {
            "commands": [
            {
                "type": "position",
                "unit": "pixel",
                "start": 5,
                "count": 10,
                "wrap": true,
                "reverse": true,
                "skip": 3,
                "animate": {"speed":20}
            },
            {
                "type": "rgb",
                "red": 255
            }
            ]
        }       
        )script";

    class TestStrip : public IHSLStrip
    {
    public:
        TestStrip(Logger *logger)
        {
            m_logger = logger;
        }

        virtual void setHue(int index, int16_t hue, HSLOperation op = REPLACE) {}
        virtual void setSaturation(int index, int16_t hue, HSLOperation op = REPLACE) {}
        virtual void setLightness(int index, int16_t hue, HSLOperation op = REPLACE) {}
        virtual void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE) {}
        virtual size_t getCount() { return 100; };
        virtual size_t getStart() { return 0; };
        virtual void clear() {}
        virtual void show() {}
    protected:
        Logger *m_logger;
    };

    class TestPositionStrip : public TestStrip {
        public:
            TestPositionStrip(Logger*logger,TestResult* result) : TestStrip(logger){
                m_result = result;
            }
            virtual void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE) {
                m_logger->debug("TestPositionStrip got index %d",index);
                m_result->assertBetween(index,5,14,"index in range 5-14");   
            }

        private:
            TestResult* m_result;

    };

    class TestValuesCommand : public ScriptCommand
    {
    public:
        TestValuesCommand(TestResult *result, Logger *logger): ScriptCommand("test")
        {
            m_logger = logger;
            m_result = result;
        }

        void doStep(ScriptState &state) override
        {
            m_logger->debug("TestValuesCommand step");
            m_result->assertEqual(state.getIntValue("x",0,0),101,"get x from closest values");
            m_result->assertEqual(state.getIntValue("y",0,0),200,"get y from inner var(yy)");
            m_result->assertEqual(state.getIntValue("z",0,0),300,"get z from 2nd values");
        }


    private:
        Logger *m_logger;
        TestResult *m_result;
    };

    class JsonTestSuite : public TestSuite
    {
    public:
        static TestSuite::TestFn jsonTests[];

        static bool Run(Logger *logger)
        {
#if RUN_JSON_TESTS
            JsonTestSuite test(logger);
            test.run();
            return test.isSuccess();
#else
            return false;
#endif
        }

        void run()
        {
            runTest("testJsonMemoryFree", [&](TestResult &r)
                    { testJsonMemory(r); });
            runTest("testJsonValue", [&](TestResult &r)
                    { testJsonValue(r); });
            runTest("testSimpleJson", [&](TestResult &r)
                    { testParseSimple(r); });
            runTest("testPosition", [&](TestResult &r)
                    { testPosition(r); });                    
        }

        JsonTestSuite(Logger *logger) : TestSuite("JSON Tests", logger,true)
        {
        }

    protected:
        void testJsonMemory(TestResult &result);
        void testJsonValue(TestResult &result);
        void testParseSimple(TestResult &result);
        void testPosition(TestResult &result);
    };

    void JsonTestSuite::testJsonMemory(TestResult &result)
    {
        m_logger->info("create JsonRoot");
        SharedPtr<JsonRoot> root = new JsonRoot();

        m_logger->info("create and populate object");
        JsonObject *obj = root->createObject();
        obj->set("int", 1);
        obj->set("string", "test");
        obj->set("float", 1.2);
        obj->set("bool", true);

        m_logger->info("create and populate array");

        JsonArray *arr = obj->createArray("test array");
        arr->add(true);
        arr->add(1);
        arr->add("test");
        arr->add(1.2);
    }

    void JsonTestSuite::testJsonValue(TestResult &result)
    {
        m_logger->info("Parse values JSON Script");
        m_logger->debug(VALUES_SCRIPT);
        ScriptDataLoader loader;
        JsonParser parser;
        m_logger->info("\tParse JSON");
        SharedPtr<JsonRoot> root = parser.read(VALUES_SCRIPT);
        m_logger->info("\tConvert to script");
        SharedPtr<Script> script = loader.jsonToScript(root.get());
        TestStrip strip(m_logger);
        ScriptState state;
        state.beginScript(script.get(),&strip);
        TestValuesCommand* cmd=new TestValuesCommand(&result, m_logger);
        script->add(cmd);
        script->step(state);
        m_logger->info("\tdone");
    }

    void JsonTestSuite::testParseSimple(TestResult &result)
    {
        m_logger->info("Parse simple JSON Script");
        m_logger->debug(SIMPLE_SCRIPT);
        ScriptDataLoader loader;
        JsonParser parser;
        m_logger->info("\tParse JSON");
        SharedPtr<JsonRoot> root = parser.read(SIMPLE_SCRIPT);
        m_logger->info("\tConvert to script");
        SharedPtr<Script> script = loader.jsonToScript(root.get());
        m_logger->info("\tdone");
    }

    void JsonTestSuite::testPosition(TestResult &result)
    {
        m_logger->info("Parse position JSON Script");
        m_logger->debug(POSITION_SCRIPT);
        ScriptDataLoader loader;
        JsonParser parser;
        SharedPtr<JsonRoot> root = parser.read(POSITION_SCRIPT);
        SharedPtr<Script> script = loader.jsonToScript(root.get());
        TestPositionStrip strip(m_logger,&result);
        ScriptState state;
        state.beginScript(script.get(),&strip);
        script->step(state);
        m_logger->info("\tdone");
    }

}
#endif

#endif