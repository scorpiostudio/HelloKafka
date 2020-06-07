#ifndef KAFKAPRODUCER_H
#define KAFKAPRODUCER_H

#pragma once
#include <string>
#include <iostream>
#include "rdkafkacpp.h"

class ProducerDeliveryReportCb : public RdKafka::DeliveryReportCb
{
public:
    void dr_cb(RdKafka::Message &message)
    {
        if (message.err())
            std::cerr << "Message delivery failed: " << message.errstr() << std::endl;
        else
            std::cerr << "Message delivered to topic " << message.topic_name()
                      << " [" << message.partition() << "] at offset "
                      << message.offset() << std::endl;
    }
};

class ProducerEventCb : public RdKafka::EventCb
{
public:
    void event_cb(RdKafka::Event &event)
    {
        switch(event.type())
        {
        case RdKafka::Event::EVENT_ERROR:
            std::cout << "RdKafka::Event::EVENT_ERROR: " << RdKafka::err2str(event.err()) << std::endl;
            break;
        case RdKafka::Event::EVENT_STATS:
            std::cout << "RdKafka::Event::EVENT_STATS: " << event.str() << std::endl;
            break;
        case RdKafka::Event::EVENT_LOG:
            std::cout << "RdKafka::Event::EVENT_LOG " << event.fac() << std::endl;
            break;
        case RdKafka::Event::EVENT_THROTTLE:
            std::cout << "RdKafka::Event::EVENT_THROTTLE " << event.broker_name() << std::endl;
            break;
        }
    }
};

class HashPartitionerCb : public RdKafka::PartitionerCb
{
public:
    int32_t partitioner_cb (const RdKafka::Topic *topic, const std::string *key,
                            int32_t partition_cnt, void *msg_opaque)
    {
        char msg[128] = {0};
        sprintf(msg, "HashPartitionerCb:[%s][%s][%d]", topic->name().c_str(),
                key->c_str(), partition_cnt);
        std::cout << msg << std::endl;
        return generate_hash(key->c_str(), key->size()) % partition_cnt;
    }
private:

    static inline unsigned int generate_hash(const char *str, size_t len)
    {
        unsigned int hash = 5381;
        for (size_t i = 0 ; i < len ; i++)
            hash = ((hash << 5) + hash) + str[i];
        return hash;
    }
};

class KafkaProducer
{
public:
    /**
     * @brief KafkaProducer
     * @param brokers
     * @param topic
     * @param partition
     */
    explicit KafkaProducer(const std::string& brokers, const std::string& topic,
                           int partition);
    /**
     * @brief push Message to Kafka
     * @param str, message data
     */
    void pushMessage(const std::string& str, const std::string& key);
    ~KafkaProducer();

protected:
    std::string m_brokers;//Broker列表，多个使用逗号分隔
    std::string m_topicStr;// Topic名称
    int m_partition;// 分区
    RdKafka::Conf* m_config;// Kafka Conf对象
    RdKafka::Conf* m_topicConfig;// Topic Conf对象
    RdKafka::Topic* m_topic;// Topic对象
    RdKafka::Producer* m_producer;// Producer对象
    RdKafka::DeliveryReportCb* m_dr_cb;
    RdKafka::EventCb* m_event_cb;
    RdKafka::PartitionerCb* m_partitioner_cb;
};

#endif // KAFKAPRODUCER_H
