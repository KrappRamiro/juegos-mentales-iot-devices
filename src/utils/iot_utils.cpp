#include "utils/iot_utils.hpp"
// The reason why net and client are both in the header and in the source: https://stackoverflow.com/questions/74729454/platformio-c-multiple-multiple-definition-of
WiFiClientSecure net = WiFiClientSecure();
PubSubClient client(net);
void connectAWS(const char* wifi_ssid, const char* wifi_password, const char* thingname, const char* aws_cert_ca, const char* aws_cert_crt, const char* aws_cert_private, const char* aws_cert_endpoint)
{
	// Connect to wifi
	WiFi.mode(WIFI_STA);
	WiFi.begin(wifi_ssid, wifi_password);
	Serial.println("Connecting to Wi-Fi");
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		Serial.print(".");
	}
	// Configure WiFiClientSecure to use the AWS IoT device credentials
#ifdef ESP32
	net.setCACert(aws_cert_ca);
	net.setCertificate(aws_cert_crt);
	net.setPrivateKey(aws_cert_private);
#elif defined(ESP8266)
	// See https://how2electronics.com/connecting-esp8266-to-amazon-aws-iot-core-using-mqtt/
	BearSSL::X509List cert(aws_cert_ca);
	BearSSL::X509List client_crt(aws_cert_crt);
	BearSSL::PrivateKey key(aws_cert_private);
#endif
	// Connect to the MQTT broker on the AWS endpoint we defined earlier
	client.setServer(aws_cert_endpoint, 8883);
	client.setBufferSize(2048); // See https://github.com/knolleary/pubsubclient/issues/485#issuecomment-435236670
	client.setKeepAlive(15);
	client.setSocketTimeout(15);
	Serial.println("Connecting to AWS IOT");
	while (!client.connect(thingname)) {
		Serial.print(".");
		delay(100);
	}
	if (!client.connected()) {
		Serial.println("AWS IoT Timeout!");
		return;
	}
	// Subscribe to the needed topics
	Serial.println("AWS IoT Connected!");
}

void reconnect(const char* thingname, const char* aws_iot_publish_topic)
{
	// Loop until we're reconnected
	while (!client.connected()) {
		Serial.print("Attempting MQTT reconnection...");
		// Attempt to connect
		if (client.connect(thingname, aws_iot_publish_topic, 0, true, "OFFLINE")) {
			Serial.println("connected");
			// Once connected, publish an announcement...
			client.publish(aws_iot_publish_topic, "ONLINE");
		} else {
			Serial.print("failed, rc=");
			Serial.print(client.state());
			Serial.println(" try again in 5 seconds");
			// Wait 5 seconds before retrying
			delay(5000);
		}
	}
}