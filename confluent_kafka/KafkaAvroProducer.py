from confluent_kafka import avro
from confluent_kafka.avro import AvroProducer


class Message:
    """
    Message struct
    """
    def __init__(self, key, value):
        self.key = {"name": "{}".format(key)}
        self.value = {"name": "{}".format(value)}


class KafkaAvroProducer:
    """
    Kafka Avro Producer Wrapper class
    """
    value_schema = ""
    key_schema = ""

    def __init__(self, brokers, schema_registry_url):
        config = dict()
        config['bootstrap.servers'] = brokers
        config['on_delivery'] = KafkaAvroProducer.delivery_report
        config['schema.registry.url'] = schema_registry_url
        self.avro_producer = AvroProducer(config=config,
                                          default_key_schema=KafkaAvroProducer.key_schema,
                                          default_value_schema=KafkaAvroProducer.value_schema)

    @classmethod
    def register_value_schema(cls, schema):
        cls.key_schema = avro.loads(schema)

    @classmethod
    def register_key_schema(cls, schema):
        cls.value_schema = avro.loads(schema)

    def send_message(self, topic, messages):
        for message in messages:
            self.avro_producer.produce(topic='test1', value=message.value, key=message.key)

        self.avro_producer.flush()

    @staticmethod
    def delivery_report(err, msg):
        """ Called once for each message produced to indicate delivery result.
            Triggered by poll() or flush(). """
        if err is not None:
            print('Message delivery failed: {}'.format(err))
        else:
            print('Message delivered to {} [{}]'.format(msg.topic(), msg.partition()))


if __name__ == "__main__":
    value_schema_str = """
    {
       "namespace": "kafka.test",
       "name": "value",
       "type": "record",
       "fields" : [
         {
           "name" : "name",
           "type" : "string"
         }
       ]
    }
    """
    key_schema_str = """
    {
       "namespace": "kafka.test",
       "name": "key",
       "type": "record",
       "fields" : [
         {
           "name" : "name",
           "type" : "string"
         }
       ]
    }
    """
    messages = list()
    for i in range(1000):
        messages.append(Message("key{}".format(i), "Hello Confluent Kafka{}".format(i)))
    KafkaAvroProducer.register_key_schema(key_schema_str)
    KafkaAvroProducer.register_value_schema(value_schema_str)
    schema_registry_url = 'http://127.0.0.1:8081'
    avroProducer = KafkaAvroProducer("192.168.0.105:9092", schema_registry_url)
    avroProducer.send_message("test1", messages=messages)
