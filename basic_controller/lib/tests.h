#ifndef TESTS_H
#define TESTS_H

#define TESTS_ON 
#include "./parse_gen.h";
#include "./logger.h";
#include "./config.h";
#include "./data.h";
#include "./data_loader.h";
#include "./list.h";

namespace DevRelief {
    class TestResult {
        public:
            TestResult(Logger * logger) { 
                m_success = true;
                m_logger = logger;    
            }

            void fail(const char * msg = NULL) { 
                if (msg == NULL) {
                    msg = "assertEqual failed";
                }
                m_logger->error(msg);
                m_success = false;
            }
            
            bool isSuccess() { return m_success;}

            bool assertEqual(int a, int b,const char * msg = NULL) {
                if (a != b) {
                    fail();
                    return false;
                }
                return true;
            }
        private:
            Logger* m_logger;
            bool m_success;
    };

    class TestObject {
        public:
        static int nextId;
        TestObject(){
            id = TestObject::nextId++;
        }

        int id;
    };
    int TestObject::nextId=100;

    class Tests {
        typedef void (Tests::*TestFn)(TestResult &);
        public:
        static bool Run() {
#ifdef TESTS_ON
            Tests* tests = new Tests();
            bool result = tests->run();
            delete tests;
            return true; //result;
#endif            
        }

#ifdef TESTS_ON
        Tests() {

        }

        bool run() {
            m_logger = new Logger("Tests",DEBUG_LEVEL);
            m_logger->always("force logging to allocate static buffers in order to detect real memory leaks: %f %d %s %d",.123,4567,"abc",true);
            int startHeap = ESP.getFreeHeap();
            m_logger->info("Running testStringBuffer");
            m_logger->showMemory();
            bool success = true;
            //success = runTest("testStringBuffer",&Tests::testStringBuffer) && success;
            //success = runTest("testData",&Tests::testData) && success;
            //success = runTest("testConfigLoader",&Tests::testConfigLoader) && success;
            success = runTest("testList",&Tests::testList) && success;
            success = runTest("testPtrList",&Tests::testPtrList) && success;
            int endHeap = ESP.getFreeHeap();
  
            if (endHeap != startHeap) {
                m_logger->error("Leaked %d bytes of memory",startHeap-endHeap);
                success = false;
            }

            m_logger->info("tests complete: %s",(success ? "success": "fail"));

            delete m_logger;
  
            return success;
        }

        bool runTest(const char * name, TestFn test){
            TestResult result(m_logger);
            int mem = ESP.getFreeHeap();
            m_logger->info("Run test: %s",name);
            m_logger->indent();
            m_logger->showMemory("memory before test");
            (this->*test)(result);
            int endMem = ESP.getFreeHeap();
            m_logger->showMemory("memory after test");
            if (endMem != mem) {
                m_logger->error("Memory Leak: %d bytes",endMem-mem);
                result.fail();
            }
            m_logger->outdent();
            return result.isSuccess();
        }

        void testConfigLoader(TestResult& result) {
            Config config;
            ConfigDataLoader loader;
            loader.initialize(config);
            if (loader.loadConfig(config)){

            }
        }

        void testStringBuffer(TestResult& result) {
            m_logger->debug("test DRStringBuffer");
            DRStringBuffer buf;
            m_logger->debug("\tsplit");
            const char ** strs = buf.split("a/b/c/d","/");
            m_logger->debug("\tsplit done");
            while(*strs != NULL) {
                m_logger->debug("\tgot %s",*strs);
                strs++;
            }

            strs = buf.split("aqq/bqq:eec/zzzd","/:");
            m_logger->debug("\tsplit done");
            while(*strs != NULL) {
                m_logger->debug("\tgot %s",*strs);
                strs++;
            }
            m_logger->debug("\ttest done");
        }

        void testData(TestResult& result) {
            m_logger->debug("test ApiResult (Data object)");
            ApiResult api;
            api.addProperty("top",1);
            
            int i = api.getInt("top");
            m_logger->debug("top should be 1.  it is: %d",i);
            api.addProperty("a/b",2);
            i = api.getInt("a/b");
            m_logger->debug("a/b should be 2.  it is: %d",i);
            
            api.addProperty("a/c/d",3);
            i = api.getInt("a/c/d");
            m_logger->debug("a/c/d should be d.  it is: %d",i);
            api.addProperty("a/c/e",4);
            i = api.getInt("a/c/e");
            m_logger->debug("a/c/e should be 4.  it is: %d",i);

            api.addProperty("abc/def/xyz",5);
            i = api.getInt("abc/def/xyz");
            m_logger->debug("abc/def/xyz should be 6.  it is: %d",i);
            
            api.addProperty("true",true);
            api.addProperty("false",false);
           api.addProperty("float",1.234);
           

            api.addProperty("string","this is\" some text");

            bool bt = api.getBool("true");
            bool bf = api.getBool("false");
            double f  = api.getFloat("float");
            char sbuf[30];
            const char * t = api.getString("string",sbuf,30);
            m_logger->debug("true==%s,  false==%s, %f, %s",bt?"true":"false",bf?"true":"false",f,t);
            /*
            DRBuffer buf;
            JsonGenerator gen(buf);
            gen.generate(&api);
            m_logger->always("JSON:");
            m_logger->always(buf.text());
            */
        }   

        void testList(TestResult& result) {
            m_logger->debug("create LinkedList");
            LinkedList<int> list;
            m_logger->debug("add 1");
            list.add(1);
            list.add(3);
            m_logger->debug("insertAt 1,2");
            list.insertAt(1,2);
            list.add(4),
            m_logger->debug("get list size");
            result.assertEqual(4,list.size());
            m_logger->debug("get item 0");
            result.assertEqual(1,list.get(0));
            result.assertEqual(2,list.get(1));
            result.assertEqual(3,list.get(2));
            result.assertEqual(4,list.get(3));
            list.remove(1);
            result.assertEqual(1,list.get(0));
            result.assertEqual(3,list.get(1));
            result.assertEqual(4,list.get(2));
            
        } 

        void testPtrList(TestResult& result) {
            PtrList<TestObject*> list;
            list.add(new TestObject());
            list.add(new TestObject());
            list.insertAt(1,new TestObject());
            list.add(new TestObject()),
            result.assertEqual(4,list.size());
            list.remove(1);
            
        } 
        private:
            Logger* m_logger;        
#endif                   
    };
}
#endif
