#include "KafkaConsumer.h"

int main()
{
    std::string brokers = "192.168.0.105:9092";
    std::vector<std::string> topics;
    topics.push_back("test");
    topics.push_back("test2");
    std::string group = "testGroup";
    KafkaConsumer consumer(brokers, group, topics, RdKafka::Topic::OFFSET_BEGINNING);
    consumer.pullMessage();

    RdKafka::wait_destroyed(5000);
    return 0;
}
