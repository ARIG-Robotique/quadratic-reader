#include <Arduino.h>
#include <Wire.h>

#include "define.h"


// Prototype des fonctions
void setup();
void loop();
void resetEncodeursValues();
void sendEncodeursValues();

void i2cReceive(int);
void chaRead();
void chbRead();

// Compteurs pour l'encodeur
volatile EncodeursValues encodeurs;

// Command re�u par l'I2C
volatile char i2cCommand;

// ------------------------------------------------------- //
// ------------------------- MAIN ------------------------ //
// ------------------------------------------------------- //

// Point d'entr�e du programme
int main(void) {
	// Initialisation du SDK Arduino. A r��crire si on veut customis� tout le bouzin.
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
	// Initialisation du port s�rie en debug seulement (cf define.h)
	if (DEBUG_MODE == 1) {
		Serial.begin(115200);
		Serial.println(" == INITIALISATION CARTE CODEUR ==");
	}

	// D�finition des broches IO
	pinMode(ADD1, INPUT);
	pinMode(ADD2, INPUT);
	pinMode(CHA, INPUT);
	pinMode(CHB, INPUT);
	pinMode(IDX, INPUT);

	if (DEBUG_MODE == 1) {
		Serial.println(" - IO [OK]");
	}

	// D�finition des fonctions d'int�rruption pour le comptage
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
	if (DEBUG_MODE == 1) {
		Serial.print(" - I2C [OK] (Addresse : ");
		Serial.print(i2cAddress);
		Serial.println(")");
	}

	resetEncodeursValues();
}

// M�thode appel� encore et encore, tant que la carte reste aliment�.
void loop() {
	if (i2cCommand != 0) {
		i2cCommand = 0;

		switch (i2cCommand) {
			case CMD_RESET:
				resetEncodeursValues();
				break;

			case CMD_LECTURE:
				sendEncodeursValues();
				break;
			default:
				break;
		}
	}
}

// ------------------------------------------------------- //
// ------------ SOUS PROGRAMMES D'INTERRUPTION ----------- //
// ------------------------------------------------------- //

// Fonction d'int�rruption pour le comptage sur les infos du canal A
void chaRead() {
	int valA = digitalRead(CHA);
	int valB = digitalRead(CHB);

	if (valA == HIGH) {
		// Front montant de CHA

	} else {
		// Front descendant de CHA
	}
}

// Fonction d'int�rruption pour le comptage sur les infos du canal B
void chbRead() {
	int valA = digitalRead(CHA);
	int valB = digitalRead(CHB);

	if (valB == HIGH) {
		// Front montant de CHB

	} else {
		// Front descendant de CHB

	}
}

// Fonction de gestion de la r�ception des commande I2C
//
// /!\ Si �a merde optimiser �a avec une lecture hors du sous prog d'int�rruption
//
void i2cReceive(int howMany) {
	while (Wire.available()) {
		// Lecture de la commande
		i2cCommand = Wire.read();
	}
}

// ------------------------------------------------------- //
// -------------------- BUSINESS METHODS ----------------- //
// ------------------------------------------------------- //

// R�initialisation des valeurs de comptage
void resetEncodeursValues() {
	encodeurs.nbEncochesRealA = 0;
	encodeurs.nbEncochesRealB = 0;
}

// Gestion de l'envoi des valeurs de comptage.
// G�re �galement un roulement sur le compteur pour ne pas perdre de valeur lors de l'envoi
void sendEncodeursValues() {
	// TODO : Envoyer la valeur du codeur sur le bus I2C
}
