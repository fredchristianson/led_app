#ifndef TESTS_H
#define TESTS_H


#include "../parse_gen.h";
#include "../logger.h";
#include "../config.h";
#include "../data.h";
#include "../data_loader.h";
#include "../list.h";

#include "./test_suite.h";
#include "./json_suite.h";
#include "./string_suite.h";

namespace DevRelief {

#if RUN_TESTS!=1
    class Tests {
        public:
        static bool Run() {
            return true;
        }


    };
#else    
  
    class Tests {
        public:
        static bool Run() {

            Tests* tests = new Tests();
            bool result = tests->run();
            delete tests;
            return true; //result;
        }

        Tests() {

        }

        bool run() {
            Logger::setTesting(true);
            m_logger = new Logger("Tests",TEST_LOGGER_LEVEL);
            m_logger->always("force logging to allocate static buffers in order to detect real memory leaks: %f %d %s %d",.123,4567,"abc",true);
            int startHeap = ESP.getFreeHeap();
            m_logger->showMemory();
            bool success = true;
            success = JsonTestSuite::Run(m_logger) && success;
            success = StringTestSuite::Run(m_logger) && success;
            //success = runTest("testSharedPtr",&Tests::testSharedPtr) && success;
            //success = runTest("testStringBuffer",&Tests::testStringBuffer) && success;
            //success = runTest("testDRString",&Tests::testDRString) && success;
            //success = runTest("testDRStringCopy",&Tests::testDRStringCopy) && success;
            //success = runTest("testData",&Tests::testData) && success;
            //success = runTest("testConfigLoader",&Tests::testConfigLoader) && success;
            //success = runTest("testList",&Tests::testList) && success;
            //success = runTest("testPtrList",&Tests::testPtrList) && success;

            m_logger->debug("test suites done");

            int endHeap = ESP.getFreeHeap();
            m_logger->debug("end heap %d",endHeap);
            if (endHeap != startHeap) {
                m_logger->error("Leaked %d bytes of memory",startHeap-endHeap);
                success = false;
            }

            m_logger->info("tests complete: %s",(success ? "success": "fail"));

            delete m_logger;
            Logger::setTesting(false);
            return success;
        }

     /*

        void testDRString(TestResult& result) {
            DRString s1;
            DRString s2("foo");
            m_logger->debug("call copy constructor");
            DRString s3(s2);
            result.assertEqual(s3.get(),s2.get(),"copy constructor");

            s1 = s3;
            result.assertEqual(s3.get(),s2.get(),"assignment to other DRString");
            s2 = "abc";
            result.assertEqual("abc",s2.get(),"assignment to const char *");
            
            m_logger->debug("set a= DRString");
            const char * a = s2;
            m_logger->debug("assignment done");
            result.assertEqual(a,s2.get(),"cast operator");

            DRString length;
            strcpy(length.increaseLength(4),"abcd");
            result.assertEqual("abcd",length,"increase length from 0");
            strcpy(length.increaseLength(20),"additional text");
            result.assertEqual("abcdadditional text",length,"increase length from 4");
            strcpy(length.increaseLength(5),"XYZ");
            result.assertEqual("abcdadditional textXYZ",length,"increase length from 24");
            m_logger->debug("length test value: %s",length.text());
        }

        void testDRStringCopy(TestResult& result) {
            DRString s1;
            DRString s2("foo");
            DRString s3(s2);
            result.assertEqual(s3.get(),s2.get(),"copy constructor");
            if (true) {
                DRString s4("abcd");
                s1 = s4;
            }
            result.assertEqual(s1.get(),"abcd","orig left scope");
           
        }


        void testSharedPtr(TestResult& result) {
            SharedPtr<TestObject> p1 = new TestObject(123);
            SharedPtr<TestObject> p2 = p1;
            SharedPtr<TestObject> p3 = p2;
            p1.freeData();
            p2.freeData();
            if (true) {
                SharedPtr<TestObject> p4 = p3;
                result.assertEqual(p4->value,123,"4th shared ptr");
            }
            result.assertEqual(p3->value,123,"3rd shared ptr");
            SharedPtr<TestObject> p5 = p3;
        }

        void testConfigLoader(TestResult& result) {
            m_logger->debug("create Config");
            Config config;

            m_logger->debug("test scripts");
            config.addScript("s1");
            config.addScript("s2");
            config.addScript("last script");
            result.assertEqual(config.getScripts().size(),3,"3 scripts added");
            result.assertEqual((const char*)(config.getScripts()[1]),"s2","index 1 script is 's2'");

            m_logger->debug("\ttest pins");
            config.addPin(2,150);
            config.addPin(3,75);
            config.addPin(4,75);
            result.assertEqual(config.getPin(1)->number,3,"pin number");
            result.assertEqual(config.getPin(1)->ledCount,75,"led count");
            result.assertEqual(config.getPin(1)->reverse,false,"reverse");
            result.assertEqual(config.getPinCount(),3,"pin count");

            config.setBrightness(10);
            config.setMaxBrightness(20);
            config.setHostname("config name");
            config.setAddr("1.2.3.4");

            ConfigDataLoader loader;
            result.assertTrue(loader.saveConfig(config,"/config.test.json"),"saveConfig");
          
            config.clearScripts();
            result.assertEqual(config.getScripts().size(),0,"scripts cleared");

            config.clearPins();
            result.assertEqual(config.getPins().size(),0,"pins cleared");
            

            result.assertTrue(loader.loadConfig(config,"/config.test.json"),"load config");

            result.assertEqual(config.getPin(1)->number,3,"pin number");
            result.assertEqual(config.getPin(1)->ledCount,75,"led count");
            result.assertEqual(config.getPin(1)->reverse,false,"reverse");
            result.assertEqual(config.getPinCount(),3,"pin count");
             result.assertEqual(config.getScripts().size(),3,"3 scripts added");

            result.assertEqual((const char*)(config.getScripts()[1]),"s2","index 1 script is 's2'");

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
            
            DRBuffer buf;
            JsonGenerator gen(buf);
            gen.generate(&api);
            m_logger->always("JSON:");
            m_logger->always(buf.text());
            
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
            result.assertEqual(1,list[0],"index 0");
            result.assertEqual(2,list.get(1));
            result.assertEqual(1,list[1],"index 1");
            result.assertEqual(3,list.get(2));
            result.assertEqual(4,list.get(3));
            list.removeAt(1);
            result.assertEqual(1,list.get(0));
            result.assertEqual(3,list.get(1));
            result.assertEqual(4,list.get(2));
            result.assertEqual(3,list.size(),"list size 3");
            list.add(5);
            list.add(5);
            list.insertAt(0,5);
            list.insertAt(1,5);
            list.insertAt(2,5);
            result.assertEqual(8,list.size(),"list size 8");
            list.removeAll(5);
            result.assertEqual(3,list.size(),"list size 3");
            
        } 

        void testPtrList(TestResult& result) {
            
            PtrList<TestObject*> list;
            TestObject* t0=new TestObject(0);
            TestObject* t1=new TestObject(1);
            TestObject* t2=new TestObject(2);
            TestObject* t3=new TestObject(3);
            TestObject* t4=new TestObject(4);
            TestObject* t5=new TestObject(5);
            TestObject* t6=new TestObject(6);
            TestObject* t7=new TestObject(7);

            list.add(t1);
            list.add(t2);
            list.add(t3);
            list.add(t4);

            result.assertEqual(list.get(0)->value,t1->value,"get(0)");
            result.assertEqual(list.get(1),t2,"get(1)");
            result.assertEqual(list.get(2),t3,"get(2)");
            result.assertEqual(list.get(3),t4,"get(3)");

            list.insertAt(0,t0);
            list.insertAt(3,t6);
            list.insertAt(10,t7);

            result.assertEqual(list.get(0),t0,"insert then get(0)");
            result.assertEqual(list.get(3),t6,"insert then get(3)");
            result.assertEqual(list.last(),t7,"insert then last()");


            list.insertAt(4,t5);
            result.assertEqual(list.firstIndexOf(t5),4,"firstIndexOf");

            list.removeAt(3);
            result.assertEqual(list.get(3),t5);

            list.removeFirst(t5);
            result.assertNotEqual(list.get(3),t5);

            list.clear();
            result.assertEqual(list.size(),0,"no items after clear()");            
/
            TestObject* tA = new TestObject();
            TestObject* tB = new TestObject();
            TestObject* tC = new TestObject();
            list.add(tA);
            list.add(tB);
            list.add(tC);
            result.assertEqual(list[0],tA,"index[0]");
            result.assertEqual(list[1],tB,"index[1]");
            result.assertEqual(list[2],tC,"index[2]");
            result.assertNull(list[3],"index at end of list");
            result.assertNull(list[-1],"negative index");
            
        } 

*/
        private:
            Logger* m_logger;        
    };
#endif                   
}
#endif
