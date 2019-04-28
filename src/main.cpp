#include <Arduino.h>
#include <Wire.h>

#include "define.h"

// Prototype des fonctions
void setup();
void resetEncodeursValues();
void sendEncodeursValues();
void heartBeat();

// Fonction d'IRQ
void i2cRequest();
void chaRead();
void chbRead();

// Buffer d'envoi des valeur codeurs
byte values[2];

// Compteurs pour l'encodeur
volatile signed int nbEncoches;

// Heartbeat variables
int heartTimePrec;
int heartTime;
boolean heart;

// ------------------------------------------------------- //
// ------------------------- MAIN ------------------------ //
// ------------------------------------------------------- //

/*
 * Methode de configuration pour le fonctionnement du programme
 */
void setup() {
	// ------------------------------------------------------------- //
	// Initialisation du port série en debug seulement (cf define.h) //
	// ------------------------------------------------------------- //
#ifdef DEBUG_MODE
	Serial.begin(115200);
	Serial.println(" == INITIALISATION CARTE CODEUR ==");
#endif

	// ------------------------- //
	// Définition des broches IO //
	// ------------------------- //
	pinMode(LED_BUILTIN, OUTPUT);
	pinMode(INVERT, INPUT);
	pinMode(ADD1, INPUT);
	pinMode(CHA, INPUT);
	pinMode(CHB, INPUT);
	pinMode(IDX, INPUT);

#ifdef DEBUG_MODE
	Serial.println(" - IO [OK]");
#endif

	// -------------------------------------------------------- //
	// Définition des fonctions d'intérruption pour le comptage //
	// -------------------------------------------------------- //
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

#ifdef DEBUG_MODE
	Serial.print(" - External INT [OK] (Mode : ");
	Serial.print(MULT_MODE_SEL, DEC);
	Serial.print("x ; Type : ");
    Serial.print(externalIntType == CHANGE ? "CHANGE" : "RISING");
    Serial.print(" ; CHB : ");
	Serial.print(withChb);
	Serial.println(")");
#endif

	// ------------------------ //
	// Configuration du bus I2C //
	// ------------------------ //
	int valAdd1 = (analogRead(ADD1) > 512) ? HIGH : LOW;
	int i2cAddress = BASE_ADD_I2C + (valAdd1 << 1);

	Wire.begin(i2cAddress);
	Wire.onRequest(i2cRequest);
#ifdef DEBUG_MODE
	Serial.print(" - I2C [OK] (Addresse : ");
	Serial.print(i2cAddress, HEX);
	Serial.println(")");
#endif

	invert = (analogRead(INVERT) > 512) ? true : false;
#ifdef DEBUG_MODE
	Serial.print(" - Invertion [OK] (Actif : ");
	Serial.print(invert);
	Serial.println(")");

	Serial.println(" -------------------- ");
	Serial.println(" r : Reset valeur codeur a 0 ");
	Serial.println(" l : Lecture de la valeur codeur");
#endif

	// Initialisation des valeurs à 0
	resetEncodeursValues();

	// Configuration par défaut
	heartTime = heartTimePrec = millis();
	heart = false;
}

/*
 * Fonction principale. Point d'entrée du programme
 */
int main(void) {
	// Initialisation du SDK Arduino. A réécrire si on veut customiser tout le bouzin.
	init();

	// Initialisation de l'application
	setup();

	while(true) {
		// Heart beat
		heartBeat();

#ifdef DEBUG_MODE
		if (Serial.available()) {
			int cmdSerial = Serial.read();
			switch (cmdSerial) {
			case CMD_RESET : resetEncodeursValues();break;
			case CMD_LECTURE : sendEncodeursValues();break;
			}
		}
#endif
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
		nbEncoches += (valB == LOW) ? 1 : -1;
	} else {
		// Front descendant de CHA
		nbEncoches += (valB == HIGH) ? 1 : -1;
	}
}

// Fonction d'intérruption pour le comptage sur les infos du canal B
void chbRead() {
	int valA = digitalRead(CHA);
	int valB = digitalRead(CHB);

	if (valB == HIGH) {
		// Front montant de CHB
		nbEncoches += (valA == HIGH) ? 1 : -1;
	} else {
		// Front descendant de CHB
		nbEncoches += (valA == LOW) ? 1 : -1;
	}
}

void i2cRequest() {
	sendEncodeursValues();
}

// ------------------------------------------------------- //
// -------------------- BUSINESS METHODS ----------------- //
// ------------------------------------------------------- //

/*
 * Méthode pour le battement de coeur
 */
void heartBeat() {
	heartTime = millis();
	if (heartTime - heartTimePrec > 1000) {
		heartTimePrec = heartTime;
		digitalWrite(LED_BUILTIN, (heart) ? HIGH : LOW);
		heart = !heart;
	}
}

// Réinitialisation des valeurs de comptage
void resetEncodeursValues() {
	nbEncoches = 0;

#ifdef DEBUG_MODE
	Serial.println("Initialisation des valeurs codeurs a 0.");
#endif
}

// Gestion de l'envoi des valeurs de comptage.
// Gère également un roulement sur le compteur pour ne pas perdre de valeur lors de l'envoi
void sendEncodeursValues() {
	signed int value = nbEncoches;
	nbEncoches = 0;

	// Application du coëficient si configuré
	if (invert) {
		value *= -1;
	}

#ifdef DEBUG_MODE
	Serial.print(" * Valeur codeur (invert : ");
	Serial.print(invert, HEX);
	Serial.print(") : ");
	Serial.print(value, DEC);
    Serial.print(" - Hex : 0x");
    Serial.print(value, HEX);
    Serial.print(" - Bin : 0b");
	Serial.println(value, BIN);
#endif

	// Envoi de la valeur sur 2 octets
	//
	// /!\ Envoi du MSB au LSB car la lecture décale a gauche
	values[0] = value >> 8;
	values[1] = value & 0xFF;
	Wire.write(values, 2);
}
