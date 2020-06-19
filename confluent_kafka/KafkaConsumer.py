from confluent_kafka import Consumer


class KafkaConsumer:
    def __init__(self, brokers, group):
        config = dict()
        config['bootstrap.servers'] = brokers
        config['group.id'] = group
        config['auto.offset.reset'] = 'earliest'
        self.consumer = Consumer(config)

    def subscribe(self, topics):
        self.consumer.subscribe(topics=topics)

    def pull(self):
        while True:
            msg = self.consumer.poll(1.0)
            if msg is None:
                continue
            if msg.error():
                print("Consumer error: {}".format(msg.error()))
                continue
            print('Received message: {}'.format(msg.value().decode('utf-8')))

    def close(self):
        self.consumer.close()


if __name__ == "__main__":
    consumer = KafkaConsumer("192.168.0.105:9092", "test_group")
    consumer.subscribe(["test", "test2"])
    consumer.pull()
    consumer.close()