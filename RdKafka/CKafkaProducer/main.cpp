#include "CKafkaProducer.h"

const int N = 10000;


int main(int argc, char *argv[])
{
    CKafkaProducer producer;

    char topic[] = "test";
    char brokers[] = "192.168.0.105:9092";
    int partition = 0;

    int ret  = producer.init(topic, brokers, partition);
    if(ret != 0)
    {
        printf("Error: CKafkaProducer.init(): ret=%d;\n", ret);
        return 0;
    }
    for(int i = 0; i < N; i++)
    {
        char str_msg[64] = {0};
        sprintf(str_msg, "Hello Kafka%4d", i);
        ret = producer.sendMessage(str_msg, strlen(str_msg)); //向kafka服务器发送消息
        if(ret != 0)
        {
            printf("Error: CKafkaProducer.sendMessage(): ret=%d;\n", ret);
            return 0;
        }
        sleep(1);
    }
    return 0;
}









