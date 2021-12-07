#ifndef TESTS_H
#define TESTS_H

#define TESTS_ON 
#include "./parse_gen.h";
#include "./logger.h";

namespace DevRelief {
    class Tests {
        typedef bool (Tests::*TestFn)();
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
            int startHeap = ESP.getFreeHeap();
            m_logger->info("Running testStringBuffer");
            m_logger->showMemory();
            bool success = true;
            success = runTest("testStringBuffer",&Tests::testStringBuffer) && success;
            success = runTest("testData",&Tests::testData) && success;
     
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
            int mem = ESP.getFreeHeap();
            m_logger->info("Run test: %s",name);
            m_logger->indent();
            m_logger->showMemory("memory before test");
            bool success = (this->*test)();
            int endMem = ESP.getFreeHeap();
            m_logger->showMemory("memory after test");
            if (endMem != mem) {
                m_logger->error("Memory Leak: %d bytes",endMem-mem);
                success = false;
            }
            m_logger->outdent();
            return success;
        }

        bool testStringBuffer() {
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
            return true;
        }

        bool testData() {
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
            m_logger->debug("abc/def/xyz should be 5.  it is: %d",i);
            /*
            DRBuffer buf;
            JsonGenerator gen(buf);
            gen.generate(&api);
            m_logger->always("JSON:");
            m_logger->always(buf.text());
            */
            return true;
        }    
        private:
            Logger* m_logger;        
#endif                   
    };
}
#endif