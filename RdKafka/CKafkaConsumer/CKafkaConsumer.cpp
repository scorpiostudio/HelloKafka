#include "CKafkaConsumer.h"

CKafkaConsumer::CKafkaConsumer()
{
    m_consumer_callback = NULL;
    m_consumer_callback_param = NULL;
    m_kafka_handle = NULL;
    m_kafka_topic = NULL;
    m_kafka_conf = NULL;
    m_kafka_topic_conf = NULL;
    m_kafka_topic_partition_list = NULL;
    m_kafka_queue = NULL;
}

CKafkaConsumer::~CKafkaConsumer()
{
    rd_kafka_flush(m_kafka_handle, 10 * 1000);
    rd_kafka_topic_partition_list_destroy(m_kafka_topic_partition_list);
    rd_kafka_queue_destroy(m_kafka_queue);
    rd_kafka_topic_destroy(m_kafka_topic);
    rd_kafka_destroy(m_kafka_handle);
}

int CKafkaConsumer::init(char *topic, char *brokers, char *partitions, char *groupId)
{
    m_kafka_conf = rd_kafka_conf_new();
    rd_kafka_conf_set_error_cb(m_kafka_conf, err_cb);
    rd_kafka_conf_set_throttle_cb(m_kafka_conf, throttle_cb);
    rd_kafka_conf_set_offset_commit_cb(m_kafka_conf, offset_commit_cb);
    rd_kafka_conf_set_stats_cb(m_kafka_conf, stats_cb);
    rd_kafka_conf_set_log_cb(m_kafka_conf, logger);

    rd_kafka_conf_res_t ret_conf = RD_KAFKA_CONF_OK;
    char errstr[512] = {0};
    //---------Consumer config-------------------
    ret_conf = rd_kafka_conf_set(m_kafka_conf, "queued.min.messages", "1000000",
                                 errstr, sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_conf_set() failed 1; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return ret_conf;
    }
    ret_conf = rd_kafka_conf_set(m_kafka_conf, "session.timeout.ms", "6000",
                                 errstr, sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_conf_set() failed 2; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return ret_conf;
    }
    ret_conf = rd_kafka_conf_set(m_kafka_conf, "group.id", groupId, errstr,
                                 sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_conf_set() failed 3; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return ret_conf;
    }
    //---------Kafka topic config-------------------
    m_kafka_topic_conf = rd_kafka_topic_conf_new();
    ret_conf = rd_kafka_topic_conf_set(m_kafka_topic_conf, "auto.offset.reset",
                                       "earliest", errstr, sizeof(errstr));
    if(ret_conf != RD_KAFKA_CONF_OK)
    {
        printf("Error: rd_kafka_topic_conf_set() failed 4; ret_conf=%d; errstr:%s\n",
               ret_conf, errstr);
        return ret_conf;
    }
    m_kafka_topic_partition_list = rd_kafka_topic_partition_list_new(1);
    int len = strlen(partitions);
    char *buffer = new char[len + 1];
    char* temp = buffer;
    sprintf(buffer, "%s", partitions); //partitions="0,1,2";
    while(*buffer != '\0')
    {
        char* s = strstr(buffer, ",");
        if(s != NULL)
        {
            *s = '\0';
            int partition = atoi(buffer);
            rd_kafka_topic_partition_list_add(m_kafka_topic_partition_list, topic, partition);
            buffer = s + 1;
        }
        else
        {
            break;
        }
    }
    delete [] temp;
    buffer = NULL;
    //---------Create Kafka handle-------------------
    m_kafka_handle = rd_kafka_new(RD_KAFKA_CONSUMER, m_kafka_conf, errstr, sizeof(errstr));
    if(m_kafka_handle == NULL)
    {
        printf("Error: Failed to create Kafka producer: %s\n", errstr);
        return -1;
    }
    rd_kafka_poll_set_consumer(m_kafka_handle); //Redirect rd_kafka_poll() to consumer_poll()
    //---------Add broker(s)-------------------
    if(brokers && rd_kafka_brokers_add(m_kafka_handle, brokers) < 1)
    {
        printf("Error: No valid brokers specified\n");
        return -1;
    }

    int partition = m_kafka_topic_partition_list->elems[0].partition;
    int partition_cnt = m_kafka_topic_partition_list->cnt;

    m_kafka_topic = rd_kafka_topic_new(m_kafka_handle, topic, m_kafka_topic_conf);
    m_kafka_queue = rd_kafka_queue_new(m_kafka_handle);
    return ret_conf;
}

void CKafkaConsumer::registerConsumerCall(consumer_callback consumer_cb, void* param_cb)
{
    m_consumer_callback = consumer_cb;
    m_consumer_callback_param = param_cb;
}

int CKafkaConsumer::pullMessage()
{
    int ret = 0;
    char * topic = m_kafka_topic_partition_list->elems[0].topic;
    int partition = m_kafka_topic_partition_list->elems[0].partition;
    int partition_cnt = m_kafka_topic_partition_list->cnt;
    // RD_KAFKA_OFFSET_BEGINNING | RD_KAFKA_OFFSET_END | RD_KAFKA_OFFSET_STORED
    int64_t start_offset = RD_KAFKA_OFFSET_END;

    //------------从kafka服务器接收消息----------------
    for(int i = 0; i < partition_cnt; i++)
    {
        int partition = m_kafka_topic_partition_list->elems[i].partition;
        int r = rd_kafka_consume_start_queue(m_kafka_topic, partition, start_offset, m_kafka_queue);
        if(r == -1)
        {
            printf("Error: creating queue: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
            return -1;
        }
    }

    while(1)
    {
        int r = rd_kafka_consume_callback_queue(m_kafka_queue, 1000, msg_consume, this); //Queue mode
        if(r <= 0)
        {
            rd_kafka_poll(m_kafka_handle, 1000);
            continue;
        }
        rd_kafka_poll(m_kafka_handle, 0); //Poll to handle stats callbacks
    }
    //----------Stop consuming------------------------------
    for(int i = 0; i < partition_cnt; i++)
    {
        int r = rd_kafka_consume_stop(m_kafka_topic, (int32_t)i);
        if(r == -1)
        {
            printf("Error: in consume_stop: %s\n", rd_kafka_err2str(rd_kafka_last_error()));
        }
    }
    return ret;
}

void CKafkaConsumer::err_cb(rd_kafka_t *rk, int err, const char *reason, void *opaque)
{
    printf("ERROR CALLBACK: %s: %s: %s\n", rd_kafka_name(rk),
           rd_kafka_err2str((rd_kafka_resp_err_t)err), reason);
}

void CKafkaConsumer::throttle_cb(rd_kafka_t *rk, const char *broker_name,
                                 int32_t broker_id, int throttle_time_ms,
                                 void *opaque)
{
    printf("THROTTLED %dms by %s (%d)\n", throttle_time_ms, broker_name, broker_id);
}

void CKafkaConsumer::offset_commit_cb(rd_kafka_t *rk, rd_kafka_resp_err_t err,
                                      rd_kafka_topic_partition_list_t *offsets,
                                      void *opaque)
{
    int i;
    int verbosity = 1;

    if(err || verbosity >= 2)
    {
        printf("Offset commit of %d partition(s): %s\n", offsets->cnt,
               rd_kafka_err2str(err));
    }

    for(i = 0; i < offsets->cnt; i++)
    {
        rd_kafka_topic_partition_t * rktpar = &offsets->elems[i];

        if(rktpar->err || verbosity >= 2)
        {
            printf("%s [%d] @ %ld: %s\n", rktpar->topic, rktpar->partition,
                   rktpar->offset, rd_kafka_err2str(err));
        }
    }
}

int CKafkaConsumer::stats_cb(rd_kafka_t *rk, char *json, size_t json_len, void *opaque)
{
    printf("%s\n", json);
    return 0;
}

void CKafkaConsumer::logger(const rd_kafka_t *rk, int level, const char *fac, const char *buf)
{
    fprintf(stdout, "RDKAFKA-%i-%s: %s: %s\n", level, fac, rd_kafka_name(rk), buf);
}

void CKafkaConsumer::msg_consume(rd_kafka_message_t *rkmessage, void *opaque)
{
    CKafkaConsumer* consumer = (CKafkaConsumer *)opaque;
    if(consumer && consumer->m_consumer_callback)
    {
        consumer->m_consumer_callback(rkmessage, consumer->m_consumer_callback_param);
        return;
    }
    if(rkmessage->err)
    {
        if(rkmessage->err == RD_KAFKA_RESP_ERR__PARTITION_EOF)
        {
            printf("[INFO] Consumer reached end of %s [%d] message queue at offset %ld\n", rd_kafka_topic_name(rkmessage->rkt), rkmessage->partition, rkmessage->offset);
            return;
        }
        printf("Error: Consume error for topic \"%s\" [%d] offset %ld: %s\n", rkmessage->rkt ? rd_kafka_topic_name(rkmessage->rkt) : "", rkmessage->partition, rkmessage->offset, rd_kafka_message_errstr(rkmessage));

        return;
    }
    if(rkmessage->key_len)
    {
        printf("Key: %d: %s\n", (int)rkmessage->key_len, (char *)rkmessage->key);
    }
    printf("%d: %s\n", (int)rkmessage->len, (char *)rkmessage->payload);
}
