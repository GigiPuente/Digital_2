#include <Wire.h>
#include <Servo.h>
#include "Adafruit_TCS34725.h"

//--------------------- Pines ---------------------
const int pinSensor = 2;
const int pinServo = 9;

//--------------------- Servo ---------------------
Servo servo;

//--------------------- Sensor de Color ---------------------
Adafruit_TCS34725 tcs = Adafruit_TCS34725(
TCS34725_INTEGRATIONTIME_50MS,
TCS34725_GAIN_4X
);

//--------------------- Variables ---------------------
bool activado = false;

void setup() {

	Serial.begin(9600);

	pinMode(pinSensor, INPUT);

	servo.attach(pinServo);
	servo.write(0);

	// Inicializar sensor de color
	if (tcs.begin()) {
		Serial.println("Sensor TCS34725 encontrado.");
		} else {
		Serial.println("No se encontro el TCS34725.");
		while (1);
	}
}

void loop() {

	//------------------ Lectura del sensor de color ------------------

	uint16_t r, g, b, c;

	tcs.getRawData(&r, &g, &b, &c);

	Serial.print("R: ");
	Serial.print(r);

	Serial.print("  G: ");
	Serial.print(g);

	Serial.print("  B: ");
	Serial.print(b);

	Serial.print("  C: ");
	Serial.println(c);

	//------------------ Sensor de proximidad ------------------

	if (digitalRead(pinSensor) == HIGH && !activado) {

		Serial.println("Objeto detectado");

		delay(5000);

		servo.write(90);
		delay(1000);
		servo.write(0);

		activado = true;
	}

	if (digitalRead(pinSensor) == LOW) {
		activado = false;
	}

	delay(100);   // Lectura del sensor de color cada 100 ms
}