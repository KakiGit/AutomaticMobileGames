#include "ramdb.h"
#include <esp_log.h>

RAMDB::RAMDB() {
    ESP_LOGI("", "Trying to create PSRAMDB");
    psramInit();
    m_db = new std::map<DBMap_T>;
    ESP_LOGI("", "Total: %d. Used PSRAM: %d", ESP.getPsramSize(), ESP.getPsramSize() - ESP.getFreePsram());
    m_rwLock = xSemaphoreCreateCounting(1, 1);
    m_readCountLock = xSemaphoreCreateCounting(5, 1);
}

RAMDB::~RAMDB() {
    m_db->~map<DBMap_T>();
    free(m_db);
    vSemaphoreDelete(m_rwLock);
    vSemaphoreDelete(m_readCountLock);
}

std::string RAMDB::read(const std::string& key) {
    std::string res;
    // Acquire the read count lock
    if (xSemaphoreTake(m_readCountLock, portMAX_DELAY)) {
        m_readCount++;
        // If this is the first reader, acquire the reader-writer lock
        if (m_readCount == 1) {
            xSemaphoreTake(m_rwLock, portMAX_DELAY);
        }
        // Release the read count lock
        xSemaphoreGive(m_readCountLock);

        // Perform reading
        auto it = m_db->find(key);
        // If the key is found, print its corresponding value
        if (it != m_db->end()) {
            res = it->second;
        } else {
            ESP_LOGI("", "Key not found: %s", key);
        }
        ESP_LOGI("", "DB Read key: %s value: %s", key.c_str(), res.c_str());
        // Acquire the read count lock
        xSemaphoreTake(m_readCountLock, portMAX_DELAY);
        m_readCount--;
        // If this is the last reader, release the reader-writer lock
        if (m_readCount == 0) {
            xSemaphoreGive(m_rwLock);
        }
        // Release the read count lock
        xSemaphoreGive(m_readCountLock);
    }
    return res;
}

bool RAMDB::write(std::string key, std::string value) {
    size_t freePsram = ESP.getFreePsram();
    size_t usedPsram = ESP.getPsramSize() - ESP.getFreePsram();
    if (freePsram < 64) {
        ESP_LOGI("", "Cannot write into DB. PRSAM full. %d remaining.", freePsram);
        return false;
    }
    // Acquire the writer lock
    if (xSemaphoreTake(m_rwLock, portMAX_DELAY)) {
        ESP_LOGI("", "Write Task is writing...");
        // Perform writing
        auto res = m_db->emplace(key, value);
        if (!res.second) {
            res.first->second = value;
        }
        ESP_LOGI("", "PSRAM %d used. %d remaining.", usedPsram, freePsram);

        // Release the writer lock
        xSemaphoreGive(m_rwLock);

        return true;
    }
    return false;
}

RAMDB RDB;