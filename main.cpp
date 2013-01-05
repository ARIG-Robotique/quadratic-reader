#include <Arduino.h>
#include <Wire.h>

#include "define.h"

// Prototype des fonctions
void setup();
void loop();
void resetEncodeursValues();
void sendEncodeursValues();

void i2cReceive(int);
void i2cRequest();
void chaRead();
void chbRead();

// Compteurs pour l'encodeur
volatile EncodeursValues encodeurs;
volatile bool commutCounter;

// Command reçu par l'I2C
volatile char i2cCommand;

// ------------------------------------------------------- //
// ------------------------- MAIN ------------------------ //
// ------------------------------------------------------- //

// Point d'entrée du programme
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

// Method de configuration pour le fonctionnement du programme
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
		Serial.print(withChb);
		Serial.println(")");
	}

	// Configuration du bus I2C
	int valAdd1 = digitalRead(ADD1);
	int valAdd2 = digitalRead(ADD2);
	int i2cAddress = BASE_ADD_I2C + (valAdd2 << 2) + (valAdd1 << 1);

	i2cCommand = 0;
	Wire.begin(i2cAddress);
	Wire.onReceive(i2cReceive);
	Wire.onRequest(i2cRequest);
	if (DEBUG_MODE == 1) {
		Serial.print(" - I2C [OK] (Addresse : ");
		Serial.print(i2cAddress);
		Serial.println(")");
	}

	resetEncodeursValues();
}

// Méthode appelé encore et encore, tant que la carte reste alimenté.
void loop() {
	// Gestion des commande ne devant rien renvoyé
	if (i2cCommand != 0) {
		switch (i2cCommand) {
			case CMD_RESET:
				resetEncodeursValues();
				break;
		}

		i2cCommand = 0; // reset de la commande
	}
}

// ------------------------------------------------------- //
// ------------ SOUS PROGRAMMES D'INTERRUPTION ----------- //
// ------------------------------------------------------- //

// Fonction d'intérruption pour le comptage sur les infos du canal A
void chaRead() {
	int valA = digitalRead(CHA);
	int valB = digitalRead(CHB);

	if (valA == HIGH) {
		// Front montant de CHA
		if (commutCounter) {
			encodeurs.nbEncochesRealA += (valB == LOW) ? 1 : -1;
		} else {
			encodeurs.nbEncochesRealB += (valB == LOW) ? 1 : -1;
		}
	} else {
		// Front descendant de CHA
		if (commutCounter) {
			encodeurs.nbEncochesRealA += (valB == HIGH) ? 1 : -1;
		} else {
			encodeurs.nbEncochesRealB += (valB == HIGH) ? 1 : -1;
		}
	}
}

// Fonction d'intérruption pour le comptage sur les infos du canal B
void chbRead() {
	int valA = digitalRead(CHA);
	int valB = digitalRead(CHB);

	if (valB == HIGH) {
		// Front montant de CHB
		if (commutCounter) {
			encodeurs.nbEncochesRealA += (valA == HIGH) ? 1 : -1;
		} else {
			encodeurs.nbEncochesRealB += (valA == HIGH) ? 1 : -1;
		}
	} else {
		// Front descendant de CHB
		if (commutCounter) {
			encodeurs.nbEncochesRealA += (valA == LOW) ? 1 : -1;
		} else {
			encodeurs.nbEncochesRealB += (valA == LOW) ? 1 : -1;
		}
	}
}

// Fonction de gestion de la réception des commande I2C
//
// /!\ Si ça merde optimiser ça avec une lecture hors du sous prog d'intérruption
//
void i2cReceive(int howMany) {
	while (Wire.available()) {
		// Lecture de la commande
		i2cCommand = Wire.read();
	}
}

// Fonction de traitement des envoi au maitre.
// La commande est sétter avant par le maitre.
void i2cRequest() {
	switch (i2cCommand) {
		case CMD_LECTURE :
			sendEncodeursValues();
			break;
		case CMD_VERSION :
			Wire.write(VERSION);
			break;
	}

	i2cCommand = 0; // Reset de la commande
}

// ------------------------------------------------------- //
// -------------------- BUSINESS METHODS ----------------- //
// ------------------------------------------------------- //

// Réinitialisation des valeurs de comptage
void resetEncodeursValues() {
	noInterrupts();
	commutCounter = false;
	encodeurs.nbEncochesRealA = 0;
	encodeurs.nbEncochesRealB = 0;
	interrupts();
}

// Gestion de l'envoi des valeurs de comptage.
// Gère également un roulement sur le compteur pour ne pas perdre de valeur lors de l'envoi
void sendEncodeursValues() {
	signed int value;
	if (commutCounter) {
		commutCounter = false;
		value = encodeurs.nbEncochesRealB;
		encodeurs.nbEncochesRealB = 0;
	} else {
		commutCounter = true;
		value = encodeurs.nbEncochesRealA;
		encodeurs.nbEncochesRealA = 0;
	}

	if (DEBUG_MODE == 1) {
		Serial.print("Valeur codeur : ");
		Serial.println(value);
	}

	// Envoi de la valeur sur 4 octets
	for (int cpt = 0 ; cpt < 4 ; cpt++) {
		Wire.write(value & 0xFF);
		value = value >> 8;
	}
}
