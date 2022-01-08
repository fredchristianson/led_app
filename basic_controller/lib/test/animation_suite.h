#ifndef ANIMATION_TEST_H
#define ANIMATION_TEST_H

#include "./test_suite.h"
#include "../script/script_command.h"
#include "../script/script_value.h"
#include "../script/animation.h"

#if RUN_TESTS==1
namespace DevRelief {

class TestAnimationCommand : public ScriptCommandBase{
    public:
        TestAnimationCommand() : ScriptCommandBase("TestAnimationCommand") {}
        ScriptStatus doCommand(IScriptState* state) { return SCRIPT_RUNNING;}
};

class AnimationTestSuite : public TestSuite{
    public:

        static bool Run(Logger* logger) {
            AnimationTestSuite test(logger);
            test.run();
            return test.isSuccess();
        }

        void run() {
            runTest("testDurationValue",[&](TestResult&r){testDurationValue(r);});
            runTest("testRangeDurationValue",[&](TestResult&r){testRangeDurationValue(r);});
            runTest("testRangeDurationValueClone",[&](TestResult&r){testRangeDurationValueClone(r);});
        }

        AnimationTestSuite(Logger* logger) : TestSuite("Animation Tests",logger){

        }

    protected: 


    void testDurationValue(TestResult& result);
    void testRangeDurationValue(TestResult& result);
    void testRangeDurationValueClone(TestResult& result);
};

void AnimationTestSuite::testDurationValue(TestResult& result) {
    TestAnimationCommand cmd;
    m_logger->debug("Running testDurationValue tests");
    DurationValueAnimator* dur = new DurationValueAnimator(new ScriptNumberValue(1000));
    delete dur;
}

void AnimationTestSuite::testRangeDurationValue(TestResult& result) {
    TestAnimationCommand cmd;
    m_logger->debug("Running testRangeDurationValue tests");
    IScriptValue* start = new ScriptNumberValue(0);
    IScriptValue* end = new ScriptNumberValue(100);
    DurationValueAnimator* dur = new DurationValueAnimator(new ScriptNumberValue(1000));
    ScriptRangeValue range(start,end,dur);
}

void AnimationTestSuite::testRangeDurationValueClone(TestResult& result) {
    TestAnimationCommand cmd;
    m_logger->debug("Running testRangeDurationValueClone tests");
    IScriptValue* start = new ScriptNumberValue(0);
    IScriptValue* end = new ScriptNumberValue(100);
    DurationValueAnimator* dur = new DurationValueAnimator(new ScriptNumberValue(1000));
    ScriptRangeValue range(start,end,dur);
    IScriptValue* clone = range.eval(&cmd);
    clone->destroy();
}


}
#endif 

#endif