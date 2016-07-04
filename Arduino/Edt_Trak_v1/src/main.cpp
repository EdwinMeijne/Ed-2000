/*
Edt-Trak

Using PlatformIO
*/
#define VERSION "v2"

#include "Arduino.h"
#include "SPI.h"
#include "Ethernet.h"
#include "EthernetUdp.h"
#include "OSCBundle.h"
#include "Time.h"
#include "Statemachine.h"

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };
IPAddress ipLocal(192, 168, 0, 120);
IPAddress ipBroadcaster(192, 168, 0, 103);
IPAddress ipMulticast(239, 255, 255, 250);

String oscPrefix = "/Trak1/";
String oscGameTrakName[] = { "left", "right" };

bool hasSerial = false;

int portLocal = 8000;
int portBroadcaster = 9000;
uint16_t portMulticast = 12345;

EthernetUDP Udp;

// AI: LEFT { X, Y, Z } RIGHT { X, Y, Z }
int gameTrakPinConfig[2][3] = { { 0,0,0 },{ 0,0,0 } };
// DI: PEDAL
int foodPedalPinConfig = 7;

void setup() {
	Statemachine.begin(13, LOW);
}

void loop() {
	Statemachine.loop();

	if (Statemachine.isBegin()) {
		Time.begin();
		pinMode(foodPedalPinConfig, INPUT_PULLUP);

		Serial.begin(9600);

		if (Serial) {
			hasSerial = true;
		}
		
		Serial.print("Edt-Trak ");
		Serial.println(VERSION);

		Serial.println("Starting Ethernet..");
		Ethernet.begin(mac, ipLocal);
		Serial.println("Started Ethernet.");

		Serial.print("IP: ");
		for (byte thisByte = 0; thisByte < 4; thisByte++) {
			Serial.print(Ethernet.localIP()[thisByte], DEC);
			Serial.print(".");
		}
		Serial.println();

		Serial.println("Starting UDP..");
		Udp.beginMulticast(ipMulticast, portMulticast);
		Serial.println("Started UDP.");

		Statemachine.ready();
	}
	else {
		while (Statemachine.isRun()) {
			Time.loop();

			for (int i = 0; i < 2; i++) {
				char url[48];

				strcpy(url, oscPrefix.c_str());
				strcat(url, oscGameTrakName[i].c_str());

				OSCMessage message = OSCMessage(url);

				for (int j = 0; j < 3; j++) {
					message.add<float>((float)analogRead(gameTrakPinConfig[i][j] / 1023.0));
				}

				Udp.beginPacket(ipBroadcaster, portBroadcaster);
				message.send(Udp);
				message.empty();
				Udp.endPacket();
			}

			// restart when Serial has been detected
			if (!hasSerial && Serial) {
				Statemachine.restart();
			}
			// unset hasSerial
			if (hasSerial && !Serial) {
				hasSerial = false;
			}
		}
	}
}