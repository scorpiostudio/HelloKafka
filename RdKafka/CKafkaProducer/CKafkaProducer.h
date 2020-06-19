#ifndef CKAFKAPRODUCER_H
#define CKAFKAPRODUCER_H

#pragma once

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "rdkafka.h"


class CKafkaProducer
{
public:
    explicit CKafkaProducer();
    ~CKafkaProducer();

    int init(const char *topic, const char *brokers, int partition);
    /**
     * @brief 生产消息
     * @param str，消息数据
     * @param len，消息数长度
     * @return
     */
    int sendMessage(char *str, int len);

    static void err_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque);
    static void throttle_cb(rd_kafka_t *rk, const char *broker_name,
                            int32_t broker_id, int throttle_time_ms, void *opaque);
    static void offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err,
                                 rd_kafka_topic_partition_list_t *offsets,
                                 void *opaque);
    static int stats_cb(rd_kafka_t *rk, char *json, size_t json_len, void *opaque);

protected:
    rd_kafka_t* m_kafka_handle;  //Kafka生产者实例
    rd_kafka_topic_t* m_kafka_topic;   //Kafka Topic实例
    rd_kafka_conf_t* m_kafka_conf;    //kafka Config实例
    rd_kafka_topic_conf_t* m_kafka_topic_conf;// Kafka Topic配置实例
    rd_kafka_topic_partition_list_t* m_kafka_topic_partition_list;
    int m_partition;
};

#endif // CKAFKAPRODUCER_H
