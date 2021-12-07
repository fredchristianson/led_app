#ifndef DRWIFIF_H
#define DRWIFIF_H
#include "./logger.h"
#include <ESP8266WiFi.h>
#include "./config.h";

namespace DevRelief {
    const char* ssid = "c22-2.4"; //replace this with your WiFi network name
    const char* password = "Dolphin#22"; //replace this with your WiFi network password
    
    class DRWiFi {
    public:
        static DRWiFi* get() {
            if (DRWiFi::drWiFiSingleton == NULL) {
                DRWiFi::drWiFiSingleton = new DRWiFi();
                DRWiFi::drWiFiSingleton->initialize();
            }
            return DRWiFi::drWiFiSingleton;
        }
    protected:
        DRWiFi() {
            m_logger = new Logger("wifi",WARN_LEVEL);
            m_logger->debug("wifi created");
        }

        void initialize() {
            m_logger->info("WiFi initializing");
            WiFi.begin(ssid, password);
            
            while(WiFi.status() != WL_CONNECTED) {
                m_logger->info("waiting for wifi connection");
                delay(500);
            }
            WiFi.hostname(CONFIG_HOSTNAME);
            m_logger->info("WiFi connected %s",WiFi.localIP().toString().c_str());
        }

        bool isConnected() {
            return WiFi.status() == WL_CONNECTED;
        }

    private:
        static Logger * m_logger;    
        static DRWiFi*  drWiFiSingleton;
    };

    DRWiFi * DRWiFi::drWiFiSingleton;
    Logger*  DRWiFi::m_logger;
}


#endif 