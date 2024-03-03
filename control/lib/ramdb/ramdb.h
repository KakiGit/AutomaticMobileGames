#include <FreeRTOS.h>
#include <semphr.h>
#include <map>
#include "Arduino.h"
#include <queue>
#include <mutex>
#include <condition_variable>

#ifndef PSRAMDB_H
#define PSRAMDB_H

#define DBMap_T std::string, std::string
#define Queue_T std::pair<std::string, std::string>


class Queue {

    public:
        Queue();
        ~Queue();
        void push(Queue_T item);
        Queue_T pop();
        void subscribe();
        uint unsubscribe();

    private:
        std::queue<Queue_T>* m_queue;
        std::mutex m_mutex;
        std::condition_variable m_cond;
        uint subscriber_count;
};

class RAMDB {

    public:
        RAMDB();
        ~RAMDB();
        std::string read(const std::string& key);
        bool write(std::string key, std::string value);
        Queue* subscribe(std::string tag);
        bool unsubscribe(std::string tag);
        bool send(std::string tag, Queue_T data);

    private:
        std::map<std::string, Queue*>* m_queues;
        std::map<DBMap_T>* m_db;
        SemaphoreHandle_t m_rwLock;
        SemaphoreHandle_t m_readCountLock;
        std::mutex m_queueMutex;
        uint16_t m_readCount;

};

extern RAMDB RDB;
#endif // PSRAMDB_H