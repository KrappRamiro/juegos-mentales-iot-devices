#include "Arduino.h"
#include "DFRobotDFPlayerMini.h"
#include "SoftwareSerial.h"
#include "secrets/radio_secrets.h"
#include "secrets/shared_secrets.h"
#include "utils/iot_utils.hpp"
#define SHADOW_GET_TOPIC "$aws/things/radio/shadow/get"
#define SHADOW_GET_ACCEPTED_TOPIC "$aws/things/radio/shadow/get/accepted"
#define SHADOW_UPDATE_TOPIC "$aws/things/radio/shadow/update"
#define SHADOW_UPDATE_ACCEPTED_TOPIC "$aws/things/radio/shadow/update/accepted"
#define SHADOW_UPDATE_DELTA_TOPIC "$aws/things/radio/shadow/update/delta"

bool should_reproduce_track = false;
int track_n = 0;

SoftwareSerial DFPlayer_serial(10, 11); // RX, TX
DFRobotDFPlayerMini myDFPlayer;

void print_DFPlayer_Detail(uint8_t type, int value)
{
	switch (type) {
	case TimeOut:
		Serial.println(F("Time Out!"));
		break;
	case WrongStack:
		Serial.println(F("Stack Wrong!"));
		break;
	case DFPlayerCardInserted:
		Serial.println(F("Card Inserted!"));
		break;
	case DFPlayerCardRemoved:
		Serial.println(F("Card Removed!"));
		break;
	case DFPlayerCardOnline:
		Serial.println(F("Card Online!"));
		break;
	case DFPlayerPlayFinished:
		Serial.print(F("Number:"));
		Serial.print(value);
		Serial.println(F(" Play Finished!"));
		break;
	case DFPlayerError:
		Serial.print(F("DFPlayerError:"));
		switch (value) {
		case Busy:
			Serial.println(F("Card not found"));
			break;
		case Sleeping:
			Serial.println(F("Sleeping"));
			break;
		case SerialWrongStack:
			Serial.println(F("Get Wrong Stack"));
			break;
		case CheckSumNotMatch:
			Serial.println(F("Check Sum Not Match"));
			break;
		case FileIndexOut:
			Serial.println(F("File Index Out of Bound"));
			break;
		case FileMismatch:
			Serial.println(F("Cannot Find File"));
			break;
		case Advertise:
			Serial.println(F("In Advertise"));
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}
void messageHandler(char* topic, byte* payload, unsigned int length)
{
	StaticJsonDocument<256> doc;
	JsonObject state_desired;
	deserializeJson(doc, payload); // Put the info from the payload into the JSON document
	Serial.printf("MESSAGE HANDLER: Topic: %s\n", topic);
	// ------------ RETRIEVING THE SHADOW DOCUMENT FROM AWS -------------//
	if (strcmp(topic, SHADOW_GET_ACCEPTED_TOPIC) == 0)
		state_desired = doc["state"]["desired"];
	else if (strcmp(topic, SHADOW_UPDATE_DELTA_TOPIC) == 0)
		state_desired = doc["state"];
	track_n = state_desired["track"];
	should_reproduce_track = true;
}
void report_state_to_shadow()
{
	StaticJsonDocument<256> doc;
	char jsonBuffer[256];
	JsonObject state_reported = doc["state"].createNestedObject("reported");
	state_reported["track"] = track_n;
	serializeJsonPretty(doc, jsonBuffer);
	Serial.println("Reporting the following to the shadow:");
	Serial.println(jsonBuffer);
	client.publish(SHADOW_UPDATE_TOPIC, jsonBuffer);
}
void setup()
{
	Serial.begin(115200);
	// ---------------------- START OF DFPLAYER CONFIG ------------------------- //
	DFPlayer_serial.begin(9600);
	Serial.println(F("DFRobot DFPlayer Mini Demo"));
	Serial.println(F("Initializing DFPlayer ... (May take 3~5 seconds)"));

	if (!myDFPlayer.begin(DFPlayer_serial)) { // Use softwareSerial to communicate with mp3.
		Serial.println(F("Unable to begin:"));
		Serial.println(F("1.Please recheck the connection!"));
		Serial.println(F("2.Please insert the SD card!"));
		while (true)
			;
	}
	Serial.println(F("DFPlayer Mini online."));
	// ---------------------- END OF DFPLAYER CONFIG ------------------------- //

	connectAWS(WIFI_SSID, WIFI_PASSWORD, THINGNAME, AWS_CERT_CA, AWS_CERT_CRT, AWS_CERT_PRIVATE, AWS_IOT_ENDPOINT); // Connect to AWS
	client.setCallback(messageHandler);
	StaticJsonDocument<8> doc;
	char jsonBuffer[8];
	serializeJson(doc, jsonBuffer);
	client.publish(SHADOW_GET_TOPIC, jsonBuffer);
}
void loop()
{
	now = time(nullptr); // The NTP server uses this, if you delete this, the connection to AWS no longer works
	if (!client.connected()) {
		reconnect(THINGNAME, "radio/status");
	}
	if (myDFPlayer.available()) {
		print_DFPlayer_Detail(myDFPlayer.readType(), myDFPlayer.read()); // Print the detail message from DFPlayer to handle different errors and states.
	}
	if (should_reproduce_track) {
		should_reproduce_track = false;
		myDFPlayer.play(track_n);
	}

	// Delay a little bit
	local_delay(100);
}
