#line 1 "d:\\dev\\arduino\\led_app\\basic_controller\\lib\\script\\script_position.h"
#ifndef DRSCRIPT_POSITION_H
#define DRSCRIPT_POSITION_H

#include "../logger.h";
#include "../led_strip.h";
#include "./script_interface.h";
#include "./script_value.h";
#include "./script_state.h";
#include "./animation.h";

namespace DevRelief
{

    class ScriptPosition : public IStripModifier
    {
    public:
        ScriptPosition()
        {
            ScriptLogger.debug("create ScriptPosition()");
            m_startValue = NULL;
            m_endValue = NULL;
            m_countValue = NULL;
            m_offsetValue = NULL;
            m_skipValue = NULL;
            m_unit = POS_INHERIT;
            m_type = POS_ABSOLUTE;
            m_strip = NULL;
            m_wrapValue = NULL;
            m_reverseValue = NULL;
            m_physicalStrip = NULL;
            m_start = 0;
            m_count = 0;
            m_offset = 0;
            m_positionOffset = 0;
            m_wrap = true;
            m_reverse = false;
            m_logger = &ScriptLogger;
            m_operation = REPLACE;
        }

        ~ScriptPosition() {
            delete m_startValue;
            delete m_countValue;
            delete m_endValue;
            delete m_skipValue;
            delete m_wrapValue;
            delete m_reverseValue;
            delete m_offsetValue;
            delete m_physicalStrip;
        }

        void destroy() {
            delete this;
        }

        void setPositionType(PositionType t) { 
            m_type = t;
        }
        void updateValues(IScriptCommand* cmd, ScriptState* state, ICommandContainer* container){
            if (container == NULL){
                m_logger->error("ScriptPosition needs a container");
                return;
            }
            m_strip = container->getStrip();
            ScriptLogger.test("updateValues strip");
            if (m_wrapValue) {
                m_wrap = m_wrapValue->getBoolValue(cmd,true);
            } else {
                m_wrap = true;
            }
            if (m_reverseValue) {
                m_reverse = m_reverseValue->getBoolValue(cmd,true);
            } else {
                m_reverse = false;
            }
            if (m_skipValue) {
                int skip  = m_skipValue->getIntValue(cmd,1);
                m_skip = skip;
                ScriptLogger.test("skip %d %d.  %s",skip,m_skip,m_skipValue->toString().get());
            } else {
                m_skip = 1;
            }
            bool after = false;
            if (m_offsetValue) {
                m_logger->debug("have offset %s",m_offsetValue->toString().get());
                if (m_offsetValue->equals(cmd,"after")){
                    m_logger->debug("set position 'after' previous");
                    m_offset = 0;
                    m_positionOffset = 0;
                    m_start = 0;
                    IScriptCommand*prev = state->getPreviousCommand();
                    if (prev != NULL) {
                        IStripModifier* pos =  prev->getPosition();
                        m_logger->debug("previous pos %d %d",pos->getStart(),pos->getCount());
                        m_start = pos->getStart() + pos->getCount() + pos->getOffset() + pos->getPositionOffset();
                        after = true;
                    } else {
                        m_logger->debug("no previous command");
                    }
                } else {
                    int offset  = m_offsetValue->getIntValue(cmd,0);
                    m_offset = offset;
                }
            } else {
                m_offset = 0;
            }    
                 
            if (!after) {
                m_start = 0;
            }
            if (!after && m_startValue != NULL)
            {
                m_start = m_startValue->getIntValue(cmd,  m_start);
            }
            m_count = m_strip->getCount();
            if (m_countValue != NULL)
            {
                m_count = m_countValue->getIntValue(cmd,  m_count);
                m_end = m_start+m_count-1;
            }
            else if (m_endValue != NULL)
            {
                m_end = m_start + m_count-1;
                m_end = m_endValue->getIntValue(cmd, m_end);
                m_count = abs(m_end - m_start)+1;
            };     
            
            if (m_unit == POS_PERCENT) {
                double baseCount = m_strip->getCount();
                ScriptLogger.debug("PERCENT: base: %f. start: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",baseCount, m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));
                m_start = floor(m_start*baseCount/100.0);
                m_end = round(m_end*baseCount/100.0);
                m_count = round(m_count*baseCount/100.0);
                ScriptLogger.debug("\tadjusted: base: %f. start: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",baseCount, m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));
            }
            ScriptLogger.always("position: start %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s. offset=%d.  positionOffset=%d",m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"),m_offset,m_positionOffset);
   
        }
       
 

        int getPhysicalStripOffset(int stripNumber){
            // 
            Config* cfg = Config::getInstance();
            int offset = 0;
            const PtrList<LedPin*>& pins = cfg->getPins();
            for(int i=0;i<stripNumber && i< pins.size();i++){
                offset += pins[i]->ledCount;
            }
            return offset;
        }

        // IHSLStrip *getBase()
        // {
        //     return m_strip;
        // }
        void setStripNumber(IScriptValue *val) { m_physicalStrip = val; }

        void setStartValue(IScriptValue *val) { m_startValue = val; }
        void setCountValue(IScriptValue *val) { m_countValue = val; }
        void setEndValue(IScriptValue *val) { m_endValue = val; }
        void setSkipValue(IScriptValue *val) {
            if (val != NULL) {
                delete m_skipValue;
                m_logger->never("got skip value %s",val->toString().get());
                m_skipValue = val; 
            } else {
                m_logger->never("skip is NULL");
            }
        }
        void setUnit(PositionUnit unit) { m_unit = unit; }
        void setHue(int index, int16_t hue, HSLOperation op = REPLACE)
        {
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_strip->setHue((index), hue, op);
        }
        void setSaturation(int index, int16_t saturation, HSLOperation op = REPLACE)
        {
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_strip->setSaturation((index), saturation, op);
        }
        void setLightness(int index, int16_t lightness, HSLOperation op = REPLACE)
        {
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_strip->setLightness((index), lightness, op);
        }
        void setRGB(int index, const CRGB &rgb, HSLOperation op = REPLACE)
        {
            int orig = index;
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,1000,NULL,"Script position missing a previous strip");
                return;
            }
          //  m_logger->never("\ttranslated RGB index %d===>%d",orig,index);
            m_strip->setRGB((index), rgb, op);
        }
        size_t getCount() { return m_count; }
        size_t getStart() { return m_start; }
        bool translate(int& index)
        {
            // never works in PIXEL units.  updateValues took care of % to pixel if needed

            if (m_count == 0) { return false;}
            if (m_skipValue) {
                m_logger->never("\tskip %d. index=%d",m_skip,index*m_skip);
                index = index * m_skip;
            } else {
                m_logger->never("\tno skip");
            }
            if (index < 0) {
                if (m_wrap) {
                    index = m_count + (index%m_count);
                } else {
                    m_logger->never("clip");
                    return false;
                }
            }
            if (index >= m_count) {
                if (m_wrap) {
                    index = index%m_count;
                } else {
                    m_logger->never("clip");
                    return false;
                }
            }


            if (m_start>m_end) {
                index = -index;

            }
            index = m_start+index;
            index = index + m_offset + m_positionOffset;
            if (m_reverse) {
                index = m_end - index;
            }
            return true;
        }

        void setWrap(IScriptValue* wrap) { m_wrapValue = wrap; }
        void setReverse(IScriptValue* reverse) { m_reverseValue = reverse; }
        void setOffset(IScriptValue* offset) { m_offsetValue = offset; }
        void clear() { m_strip->clear();}
        void show() { m_strip->show();}

        PositionUnit getPositionUnit() override {
            if (m_unit == POS_INHERIT) {
                return m_strip == NULL ? POS_PERCENT : m_strip->getPositionUnit();
            }
            return m_unit;
        }

        int getOffset() override { return m_offset;}
        int getPositionOffset() override { return m_positionOffset;}

    private:
        // values that may be variables
        IScriptValue* m_wrapValue;
        IScriptValue* m_reverseValue;
        IScriptValue *m_startValue;
        IScriptValue *m_endValue;
        IScriptValue *m_countValue;
        IScriptValue *m_skipValue;
        IScriptValue *m_offsetValue;
        IScriptValue *m_physicalStrip;

        int m_animationOffset;
        // evaluated variable values
        int m_start;
        int m_count;
        int m_end;
        bool m_wrap;
        bool m_reverse;
        int m_skip;
        int m_offset; // can be set in the JSON position definition.  useful for POS_RELATIVE?
        PositionUnit m_unit;
        PositionType m_type;
        // m_strip is the strip this position writes to
        IStripModifier *m_strip;
        int m_positionOffset;  // offset withing m_strip (usually based on m_positionStrip)

        HSLOperation m_operation;
        Logger* m_logger;
    };

  
}
#endif
