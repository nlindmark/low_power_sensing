const char* mqtt_server = "lindmark.synology.me";
char client_name[50];

void setup_mqtt() {

  snprintf (client_name, 50, "%ld", system_get_chip_id());
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(10);
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

}

bool reconnect() {
  // Loop until we're reconnected

  int i = 10;
  while (!client.connected() && i > 0) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(client_name)) {
      Serial.println(client_name);
      Serial.println(" is connected");
      // Once connected, publish an announcement...
      // client.publish("outTopic", "hello world");
      // ... and resubscribe
      client.subscribe("outTopic");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
    i--;
  }

  return (i == 0) ? false : true;
}
