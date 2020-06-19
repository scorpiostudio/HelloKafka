#include "KafkaConsumer.h"

KafkaConsumer::KafkaConsumer(const std::string& brokers, const std::string& groupID,
                             const std::vector<std::string>& topics, int partition)
{
    m_brokers = brokers;
    m_groupID = groupID;
    m_topicVector = topics;
    m_partition = partition;

    std::string errorStr;
    RdKafka::Conf::ConfResult errorCode;
    m_config = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);

    m_event_cb = new ConsumerEventCb;
    errorCode = m_config->set("event_cb", m_event_cb, errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed: " << errorStr << std::endl;
    }

    m_rebalance_cb = new ConsumerRebalanceCb;
    errorCode = m_config->set("rebalance_cb", m_rebalance_cb, errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed: " << errorStr << std::endl;
    }

    errorCode = m_config->set("enable.partition.eof", "false", errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed: " << errorStr << std::endl;
    }

    errorCode = m_config->set("group.id", m_groupID, errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed: " << errorStr << std::endl;
    }
    errorCode = m_config->set("bootstrap.servers", m_brokers, errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed: " << errorStr << std::endl;
    }
    errorCode = m_config->set("max.partition.fetch.bytes", "1024000", errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed: " << errorStr << std::endl;
    }

    m_topicConfig = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    // 获取最新的消息数据
    errorCode = m_topicConfig->set("auto.offset.reset", "earlist", errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Topic Conf set failed: " << errorStr << std::endl;
    }
    errorCode = m_config->set("default_topic_conf", m_topicConfig, errorStr);
    if(errorCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed: " << errorStr << std::endl;
    }
    m_consumer = RdKafka::KafkaConsumer::create(m_config, errorStr);
    if(m_consumer == NULL)
    {
        std::cout << "Create KafkaConsumer failed: " << errorStr << std::endl;
    }
    std::cout << "Created consumer " << m_consumer->name() << std::endl;
}

void msg_consume(RdKafka::Message* msg, void* opaque)
{
    switch (msg->err())
    {
    case RdKafka::ERR__TIMED_OUT:
        std::cerr << "Consumer error: " << msg->errstr() << std::endl;
        break;
    case RdKafka::ERR_NO_ERROR:
        std::cout << " Message in " << msg->topic_name() << " ["
                  << msg->partition() << "] at offset " << msg->offset()
                  << "key: " << msg->key() << " payload: "
                  << (char*)msg->payload() << std::endl;
        break;
    default:
        std::cerr << "Consumer error: " << msg->errstr() << std::endl;
        break;
    }
}

void KafkaConsumer::pullMessage()
{
    // 订阅Topic
    RdKafka::ErrorCode errorCode = m_consumer->subscribe(m_topicVector);
    if (errorCode != RdKafka::ERR_NO_ERROR)
    {
        std::cout << "subscribe failed: " << RdKafka::err2str(errorCode) << std::endl;
    }
    // 消费消息
    while(true)
    {
        RdKafka::Message *msg = m_consumer->consume(1000);
        msg_consume(msg, NULL);
        delete msg;
    }
}

KafkaConsumer::~KafkaConsumer()
{
    m_consumer->close();
    delete m_config;
    delete m_topicConfig;
    delete m_consumer;
    delete m_event_cb;
    delete m_rebalance_cb;

}
