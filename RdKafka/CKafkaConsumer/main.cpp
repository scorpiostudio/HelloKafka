#include "CKafkaConsumer.h"

static void msg_consume(rd_kafka_message_t *rkmessage, void *opaque)
{
    printf("[MSG] %d: %s\n", (int)rkmessage->len, (char*)rkmessage->payload);
}

int main(int argc, char *argv[])
{
    char topic[] = "test";
    char brokers[] = "192.168.0.105:9092";
    char partitions[] = "0,1,2";
    char groupId[] = "testGroup";
    CKafkaConsumer consumer;
    consumer.init(topic, brokers, partitions, groupId);
    //注册消息回调函数，用户可以自定义此函数
    consumer_callback consumer_cb = msg_consume;
    void * param_cb = NULL; //param_cb=this;
    consumer.registerConsumerCall(consumer_cb, param_cb);

    int ret = consumer.pullMessage(); //从kafka服务器接收消息
    if(ret != 0)
    {
        printf("Error: CKafkaConsumer.pullMessage(): ret=%d;\n", ret);
    }
    return 0;
}




