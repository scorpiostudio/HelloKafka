from confluent_kafka import Producer


class KafkaProducer:
    def __init__(self, brokers):
        self.producer = Producer({'bootstrap.servers': brokers})

    def sendMessage(self, topic, payloads):
        for payload in payloads:
            # Trigger any available delivery report callbacks from previous produce() calls
            self.producer.poll(0)
            # Asynchronously produce a message, the delivery report callback
            # will be triggered from poll() above, or flush() below, when the message has
            # been successfully delivered or failed permanently.
            self.producer.produce(topic, payload.encode('utf-8'), callback=self.delivery_report)

        # Wait for any outstanding messages to be delivered and delivery report
        # callbacks to be triggered.
        self.producer.flush()

    @staticmethod
    def delivery_report(err, msg):
        """ Called once for each message produced to indicate delivery result.
            Triggered by poll() or flush(). """
        if err is not None:
            print('Message delivery failed: {}'.format(err))
        else:
            print('Message delivered to {} [{}]'.format(msg.topic(), msg.partition()))


if __name__ == "__main__":
    producer = KafkaProducer('192.168.0.105:9092')
    source_data = list()
    for x in range(100000):
        source_data.append("Hello kafka{}".format(x))
    producer.sendMessage('test', source_data)


