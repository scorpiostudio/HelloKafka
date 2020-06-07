#include <iostream>
#include "KafkaProducer.h"
using namespace std;

int main()
{
    // 创建Producer
    KafkaProducer producer("192.168.0.105:9092", "test", 0);
    for(int i = 0; i < 10000; i++)
    {
        char msg[64] = {0};
        sprintf(msg, "%s%4d", "Hello RdKafka", i);
        // 生产消息
        char key[8] = {0};
        sprintf(key, "%d", i);
        producer.pushMessage(msg, key);
    }
    RdKafka::wait_destroyed(5000);
}





