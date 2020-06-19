from confluent_kafka.admin import NewTopic, AdminClient


class KafkaManager:
    def __init__(self, broker):
        self.admin_client = AdminClient({'bootstrap.servers': broker})

    def create_topics(self, topics, num_partition):
        new_topics = [NewTopic(topic, num_partitions=num_partition, replication_factor=1) for topic in topics]
        fs = self.admin_client.create_topics(new_topics)
        # Wait for each operation to finish.
        for topic, f in fs.items():
            try:
                f.result()  # The result itself is None
                print("Topic {} created".format(topic))
            except Exception as e:
                print("Failed to create topic {}: {}".format(topic, e))

    def delete_topics(self, topics):
        fs = self.admin_client.delete_topics(topics=topics)
        for topic, f in fs.items():
            try:
                f.result()  # The result itself is None
                print("Topic {} deleted".format(topic))
            except Exception as e:
                print("Failed to delete topic {}: {}".format(topic, e))


if __name__ == "__main__":
    import time
    manager = KafkaManager("192.168.0.105:9092")
    manager.create_topics(["test3", "test4"], 1)
    time.sleep(3)
    manager.delete_topics(["test3", "test4"])

