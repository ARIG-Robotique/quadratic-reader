
#include <Arduino.h>
#include "define.h"


// Prototype
void setup();
void loop();

// Compteurs pour l'encodeur
volatile EncodeursValues encodeurs;

// Valeur de l'adresse pour l'I2C
int i2cAdress;

// Fonction principale, point d'entrée
int main(void) {
	// Initialisation du SDK Arduino. A réécrire si on veut customisé tout le bouzin.
	init();

	// Initialisation de l'application
	setup();

	while(true) {
		// Boucle ifinie pour le fonctionnement.
		loop();
	}
}

// Fonction d'intérruption pour le comptage
void chaRead() {
	int valA = digitalRead(CHA);
	if (valA == HIGH) {

	} else {

	}
}
void chbRead() {

}


// The setup() method runs once, when the sketch starts
void setup() {
	// Initialisation du port série en debug seulement (cf define.h)
	if (DEBUG_MODE == 1) {
		Serial.begin(115200);
		Serial.println(" == INITIALISATION CARTE CODEUR ==");
	}

	// Définition des broches IO
	pinMode(ADD1, INPUT);
	pinMode(ADD2, INPUT);
	pinMode(CHA, INPUT);
	pinMode(CHB, INPUT);
	pinMode(IDX, INPUT);

	if (DEBUG_MODE == 1) {
		Serial.println(" - IO [OK]");
	}

	// Définition des fonctions d'intérruption pour le comptage
	int externalIntType = -1;
	bool withChb = true;
	if (MULT_MODE_SEL == MULT_MODE_1X || MULT_MODE_SEL == MULT_MODE_2X) {
		externalIntType = RISING;
		withChb = (MULT_MODE_SEL == MULT_MODE_2X); // Pas de comptage sur CHB si mode 1x
	} else {
		externalIntType = CHANGE;
	}
	attachInterrupt(EXT_INT_CHA, chaRead, externalIntType);
	if (withChb == true) {
		attachInterrupt(EXT_INT_CHB, chbRead, externalIntType);
	}

	if (DEBUG_MODE == 1) {
		Serial.print(" - External INT [OK] (Mode : ");
		Serial.print(externalIntType);
		Serial.print(" ; CHB : ");
		Serial.println(withChb);
	}

	// Configuration du bus I2C
	int valAdd1 = digitalRead(ADD1);
	int valAdd2 = digitalRead(ADD2);

}

// the loop() method runs over and over again, as long as the Arduino has power
void loop() {
}
