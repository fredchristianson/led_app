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

    class ScriptPosition : public IHSLStrip, IPositionable
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
            m_count = 100;
            m_unit = POS_PERCENT;
            m_offset = 0;
            m_wrap = true;
            m_position = 0;
            m_reverse = false;
            m_logger = &ScriptLogger;
            m_parentPosition = NULL;
        }

        ~ScriptPosition() {
            if (m_startValue) {m_startValue->destroy();}
            if (m_countValue) {m_countValue->destroy();}
            if (m_endValue) {m_endValue->destroy();}
            if (m_skipValue) {m_skipValue->destroy();}
            if (m_wrapValue) {m_wrapValue->destroy();}
            if (m_reverseValue) {m_reverseValue->destroy();}
            if (m_offsetValue) {m_offsetValue->destroy();}
            if (m_physicalStrip) {m_physicalStrip->destroy();}
        }



        void destroy() {
            delete this;
        }

        void setPositionType(PositionType t) { 
            m_type = t;
        }
        void updateValues(IScriptCommand* cmd, IScriptState* state){
            m_logger->never("update position 0x%x 0x%x",cmd,state);
            IScriptCommand* container = state->getContainer();
            m_logger->never("\tcontainer 0x%x",container);
            m_parentPosition =  container ? container->getPosition() : NULL;
            m_strip = state->getStrip();
            if (m_strip == NULL) {
                m_logger->error("position needs a strip");
                return;
            }
            int physicalCount = m_strip->getCount();
            int physicalStart = 0;
            if (m_physicalStrip != NULL) {
                int number = m_physicalStrip->getIntValue(cmd,0);
                physicalStart = getPhysicalStripOffset(number);
                physicalCount = getPhysicalStripLedCount(number);
            }
            ScriptLogger.never("updateValues strip");
            if (m_wrapValue) {
                m_wrap = m_wrapValue->getBoolValue(cmd,true);
            } else {
                m_wrap = false;
            }
            if (m_reverseValue) {
                m_reverse = m_reverseValue->getBoolValue(cmd,true);
            } else {
                m_reverse = false;
            }
            if (m_skipValue) {
                int skip  = m_skipValue->getIntValue(cmd,1);
                m_skip = skip;
                ScriptLogger.never("skip %d %d.  %s",skip,m_skip,m_skipValue->toString().get());
            } else {
                m_skip = 1;
            }
            m_offset= getStripPosition(cmd,state,m_offsetValue,0);
            m_start = getStripPosition(cmd,state,m_startValue,0);


            m_skip = getStripPosition(cmd,state,m_skipValue,0);
            if (m_endValue) {
                m_end = getStripPosition(cmd,state,m_endValue,0)+physicalStart;
                m_count = abs(m_start-m_end)+1;
            } else if (m_countValue){
                m_count =  getStripPosition(cmd,state,m_countValue,0);
                m_end = m_start+m_count-1;
            } else {
                if (m_unit == POS_PERCENT) {
                    m_end = 100;
                    m_count = 100;
                } else {
                    m_end = physicalStart+physicalCount-1;
                    m_count = abs(m_start-m_end)+1;
                }
            }

            if (m_startValue==NULL && m_endValue != NULL && m_countValue != NULL) {
                m_start = m_end-m_count+1;
            }
                       
            if (m_unit == POS_PERCENT) {
                double baseCount = physicalCount;
                ScriptLogger.never("PERCENT: base: %f. start: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",baseCount, m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));
                m_start = floor(m_start*baseCount/100.0);
                m_end = round(m_end*baseCount/100.0);
                m_count = round(m_count*baseCount/100.0+0.5);
                m_offset = round(m_offset*baseCount/100.0);
                m_skip = round(m_skip*baseCount/100.0);
                ScriptLogger.never("\tadjusted: base: %f. start: %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s.",baseCount, m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"));
            }
            m_start += physicalStart;
            m_end += physicalStart;
            ScriptLogger.never("position: start %d. count %d. end %d. skip %d. wrap: %s.  reverse: %s. offset=%d. physical: %d,%d ",m_start,m_count,m_end,m_skip,(m_wrap?"true":"false"),(m_reverse?"true":"false"),m_offset,physicalStart,physicalCount);
            //m_positionDomain.setMin(m_start);
            //m_positionDomain.setMax(m_end);
            //m_positionDomain.setPos(m_start);
            m_positionDomain.setMin(0);
            m_positionDomain.setMax(m_count);
            m_positionDomain.setPos(0);
        }
       
        int getStripPosition(IScriptCommand* cmd, IScriptState*state, IScriptValue* value,int defaultValue){
            m_logger->debug("getStripPosition");
            int rpos = defaultValue;
            if (value) {
                //m_logger->debug("have value %s",value->toString().get());
                IScriptCommand*prev = state->getPreviousCommand();
                if (prev&&value->equals(cmd,"after")){
                    ScriptPosition* pos =  prev->getPosition();
                    //m_logger->debug("previous pos %d %d",pos->getStart(),pos->getCount());
                    if (pos != NULL) {
                        rpos = pos->getStart() + pos->getCount() + pos->getOffset();
                    } else {
                        rpos = 0;
                    }
                } else if (prev&&value->equals(cmd,"before")){
                    ScriptPosition* pos =  prev->getPosition();
                    m_logger->debug("previous pos %d %d",pos->getStart(),pos->getCount());
                    if (pos == NULL) {
                        rpos = -1;
                    } else {
                        rpos = pos->getStart()-1;
                    }
                }  else if (value->equals(cmd,"center")){
                    IHSLStrip* pos =  m_strip;
                    m_logger->debug("center pos %d %d",pos->getStart(),pos->getCount());
                    rpos = pos->getStart()+round(pos->getCount()/2);
                } else {
                    rpos  = value->getIntValue(cmd,defaultValue);
                }
            }  else {
                m_logger->never("\tno value");
            }
            return rpos;
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

        int getPhysicalStripLedCount(int stripNumber){
            // 
            Config* cfg = Config::getInstance();
            int ledCount = 0;
            const PtrList<LedPin*>& pins = cfg->getPins();
            if (stripNumber<pins.size()) {
                ledCount = pins[stripNumber]->ledCount;
            }
            return ledCount;
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
        void setHue(int index, int16_t hue, HSLOperation op)
        {
            int orig = index;
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_logger->never("%d==>%d",orig,index);
            m_strip->setHue((index), hue, op);
        }
        void setSaturation(int index, int16_t saturation, HSLOperation op)
        {
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_strip->setSaturation((index), saturation, op);
        }
        void setLightness(int index, int16_t lightness, HSLOperation op)
        {
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,100,NULL,"Script position missing a previous strip");
                return;
            }
            m_logger->never("Position op %d",op);
            m_strip->setLightness((index), lightness, op);
        }
        void setRGB(int index, const CRGB &rgb, HSLOperation op)
        {
            int orig = index;
            if (m_strip == NULL || !translate(index)) {
                //ScriptLogger.periodic(ERROR_LEVEL,1000,NULL,"Script position missing a previous strip");
                return;
            }
          //  m_logger->never("\ttranslated RGB index %d===>%d",orig,index);
            m_strip->setRGB((index), rgb, op);
        }
        int getCount() { return m_count; }
        int getStart() { return m_start; }
        int getEnd() { return m_end; }
        bool translate(int& index)
        {
            int orig = index;
            // works in PIXEL units.  updateValues took care of % to pixel if needed
            int start = m_start;
            int count = m_count;
            int reverse = m_reverse;
            if (start>m_end) {
                start = m_end;
                reverse = !m_reverse;
            }
            if (count == 0) { return false;}
            if (m_skipValue) {
                m_logger->never("\tskip %d. index=%d",m_skip,index*m_skip);
                index = index * m_skip;
            } else {
                m_logger->never("\tno skip");
            }
            if (index < 0 && m_wrap) {
                    index = count + (index%count);
            }
            if (index >= count && m_wrap) {
                    index = index%count;
            }


            index = index + m_offset;
            if (m_start>m_end) {
                index = -index;

            }
            if (m_reverse) {
                index = m_end - index;
            } else {
                index = m_start+index;
            }
           // m_logger->never("%d-->%d",orig,index);
            return true;
        }

        void setPositionIndex(int index) override {
            // use original, not translated index
            m_logger->never("ScriptPosition.setPosition %d",index);

            m_positionDomain.setPos(index);
        }

        void setWrap(IScriptValue* wrap) { m_wrapValue = wrap; }
        void setReverse(IScriptValue* reverse) { m_reverseValue = reverse; }
        void setOffset(IScriptValue* offset) { m_offsetValue = offset; }
        void clear() { m_strip->clear();}
        void show() { m_strip->show();}

        PositionUnit getPositionUnit() override {
            if (m_unit == POS_INHERIT) {
                return m_parentPosition == NULL ? POS_PERCENT : m_parentPosition->getPositionUnit();
            }
            return m_unit;
        }

        int getOffset() override { return m_offset;}

        ScriptPosition* getPosition() override {
            return this;
        }
        PositionDomain* getAnimationPositionDomain() override {
            return &m_positionDomain;
        }


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
        ScriptPosition* m_parentPosition;

        int m_animationOffset;
        // evaluated variable values
        int m_start;
        int m_count;
        int m_end;
        bool m_wrap;
        bool m_reverse;
        int m_skip;
        int m_offset; // can be set in the JSON position definition.  useful for POS_RELATIVE?
        int m_position;
        PositionUnit m_unit;
        PositionType m_type;
        // m_strip is the strip this position writes to
        IHSLStrip *m_strip;

        Logger* m_logger;
        PositionDomain m_positionDomain;

    };

  
}
#endif
