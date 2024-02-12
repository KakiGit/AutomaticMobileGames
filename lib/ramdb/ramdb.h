#include <FreeRTOS.h>
#include <semphr.h>
#include <map>
#include "Arduino.h"

#ifndef PSRAMDB_H
#define PSRAMDB_H

#define DBMap_T std::string, std::string

class RAMDB {
    public:
        RAMDB();
        ~RAMDB();
        std::string read(const std::string& key);
        bool write(std::string key, std::string value);

    private:
        std::map<DBMap_T>* m_db;
        SemaphoreHandle_t m_rwLock;
        SemaphoreHandle_t m_readCountLock;
        uint16_t m_readCount;
};

extern RAMDB RDB;
#endif // PSRAMDB_H