#include "KafkaProducer.h"

KafkaProducer::KafkaProducer(const std::string& brokers, const std::string& topic, int partition)
{
    m_brokers = brokers;
    m_topicStr = topic;
    m_partition = partition;
    // 创建Kafka Conf对象
    m_config = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if(m_config == NULL)
    {
        std::cout << "Create RdKafka Conf failed." << std::endl;
    }
    // 创建Topic Conf对象
    m_topicConfig = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    if(m_topicConfig == NULL)
    {
        std::cout << "Create RdKafka Topic Conf failed." << std::endl;
    }
    // 设置Broker属性
    RdKafka::Conf::ConfResult errCode;
    m_dr_cb = new ProducerDeliveryReportCb;
    std::string errorStr;
    errCode = m_config->set("dr_cb", m_dr_cb, errorStr);
    if(errCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed:" << errorStr << std::endl;
    }
    m_event_cb = new ProducerEventCb;
    errCode = m_config->set("event_cb", m_event_cb, errorStr);
    if(errCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed:" << errorStr << std::endl;
    }

    m_partitioner_cb = new HashPartitionerCb;
    errCode = m_topicConfig->set("partitioner_cb", m_partitioner_cb, errorStr);
    if(errCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed:" << errorStr << std::endl;
    }

    errCode = m_config->set("statistics.interval.ms", "10000", errorStr);
    if(errCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed:" << errorStr << std::endl;
    }

    errCode = m_config->set("message.max.bytes", "10240000", errorStr);
    if(errCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed:" << errorStr << std::endl;
    }
    errCode = m_config->set("bootstrap.servers", m_brokers, errorStr);
    if(errCode != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set failed:" << errorStr << std::endl;
    }
    // 创建Producer
    m_producer = RdKafka::Producer::create(m_config, errorStr);
    if(m_producer == NULL)
    {
        std::cout << "Create Producer failed:" << errorStr << std::endl;
    }
    // 创建Topic对象
    m_topic = RdKafka::Topic::create(m_producer, m_topicStr, m_topicConfig, errorStr);
    if(m_topic == NULL)
    {
        std::cout << "Create Topic failed:" << errorStr << std::endl;
    }
}

void KafkaProducer::pushMessage(const std::string& str, const std::string& key)
{
    int32_t len = str.length();
    void* payload = const_cast<void*>(static_cast<const void*>(str.data()));
    RdKafka::ErrorCode errorCode = m_producer->produce(m_topic, RdKafka::Topic::PARTITION_UA,
                                   RdKafka::Producer::RK_MSG_COPY,
                                   payload, len, &key, NULL);
    m_producer->poll(0);
    if (errorCode != RdKafka::ERR_NO_ERROR)
    {
        std::cerr << "Produce failed: " << RdKafka::err2str(errorCode) << std::endl;
        if(errorCode ==  RdKafka::ERR__QUEUE_FULL)
        {
            m_producer->poll(1000);
        }
    }
}

KafkaProducer::~KafkaProducer()
{
    while (m_producer->outq_len() > 0)
    {
        std::cerr << "Waiting for " << m_producer->outq_len() << std::endl;
        m_producer->flush(5000);
    }
    delete m_config;
    delete m_topicConfig;
    delete m_topic;
    delete m_producer;
    delete m_dr_cb;
    delete m_event_cb;
    delete m_partitioner_cb;
}
