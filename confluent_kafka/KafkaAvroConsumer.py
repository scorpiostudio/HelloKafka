from confluent_kafka.avro import AvroConsumer
from confluent_kafka.avro.serializer import SerializerError


class KafkaAvroConsumer:
    def __init__(self, brokers, group, schema_registry_url):
        self.avro_consumer = AvroConsumer({
            'bootstrap.servers': brokers,
            'group.id': group,
            'auto.offset.reset': 'earliest',
            'schema.registry.url': schema_registry_url})

    def subscribe(self, topics):
        self.avro_consumer.subscribe(topics=topics)

    def pull_message(self):
        while True:
            try:
                msg = self.avro_consumer.poll(2)
            except SerializerError as e:
                print("Message deserialization failed for {}: {}".format(msg, e))
                break
            if msg is None:
                continue
            if msg.error():
                print("AvroConsumer error: {}".format(msg.error()))
                continue
            print(msg.key(), ": ", msg.value())

    def close(self):
        self.avro_consumer.close()


if __name__ == "__main__":
    schema_registry_url = "http://127.0.0.1:8081"
    avroConsumer = KafkaAvroConsumer("192.168.0.105:9092", "test_group", schema_registry_url)
    avroConsumer.subscribe(["test1", "test2"])
    avroConsumer.pull_message()
    avroConsumer.close()
