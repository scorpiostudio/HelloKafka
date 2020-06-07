#include "CKafkaProducer.h"

CKafkaProducer::CKafkaProducer()
{
    m_kafka_handle = NULL;
    m_kafka_topic = NULL;
    m_kafka_conf = NULL;
    m_kafka_topic_conf = NULL;
    m_kafka_topic_partition_list = NULL;
    m_partition = RD_KAFKA_PARTITION_UA;
}

CKafkaProducer::~CKafkaProducer()
{
    //wait for max 10 seconds
    rd_kafka_flush(m_kafka_handle, 10 * 1000);

    rd_kafka_topic_destroy(m_kafka_topic);
    rd_kafka_destroy(m_kafka_handle);
    rd_kafka_topic_partition_list_destroy(m_kafka_topic_partition_list);
}

int CKafkaProducer::init(const char *topic, const char *brokers, int partition)
{
    int ret = 0;
    rd_kafka_conf_res_t ret_conf = RD_KAFKA_CONF_OK;
    char errstr[512] = {0};

    m_kafka_conf = rd_kafka_conf_new();

    rd_kafka_conf_set_error_cb(m_kafka_conf, err_cb);
    rd_kafka_conf_set_throttle_cb(m_kafka_conf, throttle_cb);
    rd_kafka_conf_set_offset_commit_cb(m_kafka_conf, offset_commit_cb);
    rd_kafka_conf_set_stats_cb(m_kafka_conf, stats_cb);

    //---------Producer config-------------------
    ret_conf = rd_kafka_conf_set(m_kafka_conf, "queue.buffering.max.messages",
                                 "500000", errstr, sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_conf_set() failed 1; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return -1;
    }
    ret_conf = rd_kafka_conf_set(m_kafka_conf, "message.send.max.retries", "3",
                                 errstr, sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_conf_set() failed 2; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return -1;
    }
    ret_conf = rd_kafka_conf_set(m_kafka_conf, "retry.backoff.ms", "500", errstr,
                                 sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_conf_set() failed 3; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return -1;
    }
    //---------Kafka topic config-------------------
    m_kafka_topic_conf = rd_kafka_topic_conf_new();

    ret_conf = rd_kafka_topic_conf_set(m_kafka_topic_conf, "auto.offset.reset",
                                       "earliest", errstr, sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_conf_set() failed 4; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return -1;
    }
    m_kafka_topic_partition_list = rd_kafka_topic_partition_list_new(1);
    rd_kafka_topic_partition_list_add(m_kafka_topic_partition_list, topic,
                                      partition);
    m_partition = partition;
    //---------Create Kafka handle-------------------
    m_kafka_handle = rd_kafka_new(RD_KAFKA_PRODUCER, m_kafka_conf, errstr,
                                  sizeof(errstr));
    if(m_kafka_handle == NULL)
    {
        printf("Error: Failed to create Kafka producer: %s\n", errstr);
        return -1;
    }
    //---------Add broker(s)-------------------
    if(brokers && rd_kafka_brokers_add(m_kafka_handle, brokers) < 1)
    {
        printf("Error: No valid brokers specified\n");
        return -2;
    }
    m_kafka_topic = rd_kafka_topic_new(m_kafka_handle, topic, m_kafka_topic_conf);
    return ret;
}

int CKafkaProducer::sendMessage(char *str, int len)
{
    int ret = 0;
    if(str == NULL)
    {
        return -1;
    }
    if(len <= 0)
    {
        return -2;
    }
    char * topic = m_kafka_topic_partition_list->elems[0].topic;
    int partition = m_kafka_topic_partition_list->elems[0].partition;
    //------------向kafka服务器发送消息----------------
    ret = rd_kafka_produce(m_kafka_topic, partition, RD_KAFKA_MSG_F_COPY | RD_KAFKA_MSG_F_FREE, str, len, NULL, 0, NULL);

    if(ret == -1)
    {
        rd_kafka_resp_err_t err = rd_kafka_last_error();

        if(err == RD_KAFKA_RESP_ERR__UNKNOWN_PARTITION)
        {
            printf("Error: No such partition: %d\n", partition);
        }
        else
        {
            printf("Error: produce error: %s%s\n", rd_kafka_err2str(err),
                   err == RD_KAFKA_RESP_ERR__QUEUE_FULL ? " (backpressure)" : "");
        }

        rd_kafka_poll(m_kafka_handle, 1000); //Poll to handle delivery reports
    }
    else
    {
        rd_kafka_poll(m_kafka_handle, 0);
    }
    return ret;
}


void CKafkaProducer::err_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque)
{
    printf("ERROR CALLBACK: %s: %s: %s\n", rd_kafka_name(rk),
           rd_kafka_err2str((rd_kafka_resp_err_t)err), reason);
}

void CKafkaProducer::throttle_cb(rd_kafka_t *rk, const char *broker_name,
                                 int32_t broker_id, int throttle_time_ms, void *opaque)
{
    printf("THROTTLED %dms by %s (%d)\n", throttle_time_ms, broker_name, broker_id);
}

void CKafkaProducer::offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err,
                                      rd_kafka_topic_partition_list_t *offsets,
                                      void *opaque)
{
    int i;
    int verbosity = 1;
    if(err || verbosity >= 2)
    {
        printf("Offset commit of %d partition(s): %s\n", offsets->cnt, rd_kafka_err2str(err));
    }

    for(i = 0; i < offsets->cnt; i++)
    {
        rd_kafka_topic_partition_t * rktpar = &offsets->elems[i];
        if(rktpar->err || verbosity >= 2)
        {
            printf("%%  %s [%d] @ %d: %s\n", rktpar->topic, rktpar->partition,
                   rktpar->offset, rd_kafka_err2str(err));
        }
    }
}

int CKafkaProducer::stats_cb(rd_kafka_t *rk, char *json, size_t json_len,
                             void *opaque)
{
    printf("%s\n", json);
    return 0;
}

