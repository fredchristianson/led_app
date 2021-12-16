#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\test\\string_suite.h"
#ifndef STRING_TEST_H
#define STRING_TEST_H

#include "./test_suite.h"

#if RUN_TESTS==1
namespace DevRelief {

class StringTestSuite : public TestSuite{
    public:
        static StringTestSuite::TestFn jsonTests[];

        static bool Run(Logger* logger) {
            #if RUN_STRING_TESTS
            StringTestSuite test(logger);
            test.run();
            return test.isSuccess();
            #else
            return true;
            #endif
        }

        void run() {
            runTest("testReturnDRStringValue",[&](TestResult&r){testReturnDRStringValue(r);});
        }

        StringTestSuite(Logger* logger) : TestSuite("JSON Tests",logger){

        }

    protected: 


    void testReturnDRStringValue(TestResult& result);
};

void StringTestSuite::testReturnDRStringValue(TestResult& result) {
    m_logger->debug("Running JSON Value tests");
}


}
#endif 

#endif