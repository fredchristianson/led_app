#ifndef DR_BUFFER_H
#define DR_BUFFER_H

#include "./logger.h"

#define MAX_PATH 100

namespace DevRelief {


class DRBuffer {
public:
    DRBuffer(size_t length=0){
        m_data = NULL;
        m_maxLength = 0;
        m_length = length;
        reserve(m_length);
        m_logger = new Logger("DRBuffer",60);
    }

    ~DRBuffer() {
        m_logger->error("delete DRBuffer");
        if (m_data) {
            m_logger->error("delete DRBuffer data");
            if (m_data != NULL){
                free(m_data);
            }
        }
    }
    uint8_t* data() {
        m_logger->debug("return data.  length=%d, maxLength=%d",m_length,m_maxLength);
        m_logger->debug("first byte: %2",m_data[0]);
        return m_data;
    }

    const char * text() {
        reserve(1); // make sure there's room for an empty string
        m_data[m_length] = 0;
        return (const char *) m_data;
    }

    uint8_t* reserve(size_t length) {
        if (length > m_maxLength) {
            if (length < 128) {
                length = 128; // don't allocate small chunks
            }
            m_logger->info("increase buffer length. old length=%d, new length=%d",m_maxLength,length);
            uint8_t* newData = (uint8_t*)malloc(length+1);
            m_logger->info("allocated buffer");
            newData[0] = 2;
            m_logger->debug("set first value");
            m_logger->info("first byte: %d",newData[0]);
            if (m_data != NULL) {
                if (length > 0) {
                    memcpy(newData,m_data,length);
                }
                free(m_data);

            }
            m_data = newData;
            m_maxLength = length;
            m_logger->info("first byte of m_data: %d",m_data[0]);
        }
        return m_data;
    }

    void setLength(size_t length) {
        m_length = length;
        m_logger->info("set length.  length=%d, maxLength=%d",m_length,m_maxLength);
    }

    size_t getLength() {
        m_logger->info("get length.  length=%d, maxLength=%d",m_length,m_maxLength);
        return m_length;
    }
private:
    uint8_t * m_data;
    size_t m_maxLength;
    size_t m_length;
    Logger* m_logger;
};

}
#endif