#include "ramdb.h"
#include <esp_log.h>

Queue::Queue(){
    m_queue = new std::queue<Queue_T>;
    subscriber_count = 0;
}

Queue::~Queue(){
    delete m_queue;
}

void Queue::push(Queue_T item) {
    ESP_LOGI(""," pushing %s %s", item.first.c_str(), item.second.c_str());
    std::unique_lock<std::mutex> lock(m_mutex);
    m_queue->push(item);
    m_cond.notify_one();
}

Queue_T Queue::pop() {
    std::unique_lock<std::mutex> lock(m_mutex);

    m_cond.wait(lock,
                [this]() { return !m_queue->empty(); });

    Queue_T item = m_queue->front();
    m_queue->pop();
    ESP_LOGI(""," popping queue %s %s", item.first.c_str(), item.second.c_str());
    return item;
}

void Queue::subscribe() {
    std::unique_lock<std::mutex> lock(m_mutex);
    subscriber_count += 1;
}

uint Queue::unsubscribe() {
    std::unique_lock<std::mutex> lock(m_mutex);
    subscriber_count -= 1;
    return subscriber_count;
}

RAMDB::RAMDB() {
    m_db = new std::map<DBMap_T>;
    m_queues = new std::map<std::string, Queue*>;
    m_rwLock = xSemaphoreCreateCounting(1, 1);
    m_readCountLock = xSemaphoreCreateCounting(5, 1);
}

RAMDB::~RAMDB() {
    delete m_db;
    delete m_queues;
    vSemaphoreDelete(m_rwLock);
    vSemaphoreDelete(m_readCountLock);
}

Queue* RAMDB::subscribe(std::string tag) {
    ESP_LOGI(""," subscribing %s", tag.c_str());
    std::unique_lock<std::mutex> lock(m_queueMutex);
    auto res = m_queues->insert(std::make_pair(tag, new Queue()));
    res.first->second->subscribe();
    return res.first->second;
}

bool RAMDB::unsubscribe(std::string tag) {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    auto res = m_queues->find(tag);
    if (res != m_queues->end()) {
        uint count = res->second->unsubscribe();
        if (count == 0) {
            m_queues->erase(res);
        }
    }
}

bool RAMDB::send(std::string tag, Queue_T data) {
    ESP_LOGI(""," sending %s %s", data.first.c_str(), data.second.c_str());
    std::unique_lock<std::mutex> lock(m_queueMutex);
    auto it = m_queues->find(tag);
    if (it != m_queues->end()) {
        it->second->push(data);
        return true;
    }
    return false;
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

    // Acquire the writer lock
    if (xSemaphoreTake(m_rwLock, portMAX_DELAY)) {
        ESP_LOGI("", "Write Task is writing...");
        // Perform writing
        auto res = m_db->emplace(key, value);
        if (!res.second) {
            res.first->second = value;
        }

        // Release the writer lock
        xSemaphoreGive(m_rwLock);

        return true;
    }
    return false;
}

RAMDB RDB;