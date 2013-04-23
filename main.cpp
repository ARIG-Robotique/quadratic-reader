#include <Arduino.h>
#include <Wire.h>

#include "define.h"

// Prototype des fonctions
void setup();
void loop();
void resetEncodeursValues();
void sendEncodeursValues();
void readConfiguration();
void mouvementCadeaux();
void stopMoteur();
void sortirBras();
void rentrerBras();

// Fonction d'IRQ
void i2cReceive(int);
void i2cRequest();
void chaRead();
void chbRead();

// Compteurs pour l'encodeur
volatile EncodeursValues encodeurs;

// Booleen permettent de gérer la séquence des variables lors de l'envoi par I2C
volatile bool commut;

// Command reçu par l'I2C
volatile char i2cCommand;

int stepMouvementCdx = 0;

// ------------------------------------------------------- //
// ------------------------- MAIN ------------------------ //
// ------------------------------------------------------- //

// Point d'entrée du programme
int main(void) {
	// Initialisation du SDK Arduino. A réécrire si on veut customiser tout le bouzin.
	init();

	// Initialisation de l'application
	setup();

	while(true) {
		// Boucle infinie pour le fonctionnement.
		loop();
	}
}

// Method de configuration pour le fonctionnement du programme
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
	pinMode(ADD1, INPUT);
	pinMode(ADD2, INPUT);
	pinMode(CHA, INPUT);
	pinMode(CHB, INPUT);
	pinMode(IDX, INPUT);
	pinMode(IND_RENTRER, INPUT);
	pinMode(IND_SORTIE, INPUT);
	pinMode(MOT_BRAKE, OUTPUT);
	pinMode(MOT_DIR, OUTPUT);
	pinMode(MOT_PWM, OUTPUT);

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
	Serial.print(externalIntType, DEC);
	Serial.print(" ; CHB : ");
	Serial.print(withChb);
	Serial.println(")");
#endif

	// ------------------------ //
	// Configuration du bus I2C //
	// ------------------------ //
	int valAdd1 = digitalRead(ADD1);
	int valAdd2 = digitalRead(ADD2);
	int i2cAddress = BASE_ADD_I2C + (valAdd2 << 2) + (valAdd1 << 1);

	i2cCommand = 0;
	Wire.begin(i2cAddress);
	Wire.onReceive(i2cReceive);
	Wire.onRequest(i2cRequest);
#ifdef DEBUG_MODE
	Serial.print(" - I2C [OK] (Addresse : ");
	Serial.print(i2cAddress, HEX);
	Serial.println(")");
#endif

	// Initialisation des valeurs à 0
	resetEncodeursValues();

	// Configuration par défaut
	invert = false;

	// Arret du moteur
	stopMoteur();
}

// Méthode appelé encore et encore, tant que la carte reste alimenté.
void loop() {
	// Gestion des commande ne devant rien renvoyer.
	// /!\ Etre exhaustif sur les commandes car sinon le request ne pourra pas fonctionné si elle traité ici.
	if (i2cCommand == CMD_RESET || i2cCommand == CMD_BRAS_CDX || i2cCommand == CMD_SETUP) {
		switch (i2cCommand) {
			case CMD_RESET:
				resetEncodeursValues();
				break;
			case CMD_BRAS_CDX:
				stepMouvementCdx = MVT_SORTIR;
				break;
			case CMD_SETUP:
				readConfiguration();
				break;
		}

		i2cCommand = 0; // reset de la commande
	}

	// Gestion du mouvement du bras du cadeaux
	mouvementCadeaux();
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
		if (commut) {
			encodeurs.nbEncochesRealA += (valB == LOW) ? 1 : -1;
		} else {
			encodeurs.nbEncochesRealB += (valB == LOW) ? 1 : -1;
		}
	} else {
		// Front descendant de CHA
		if (commut) {
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
		if (commut) {
			encodeurs.nbEncochesRealA += (valA == HIGH) ? 1 : -1;
		} else {
			encodeurs.nbEncochesRealB += (valA == HIGH) ? 1 : -1;
		}
	} else {
		// Front descendant de CHB
		if (commut) {
			encodeurs.nbEncochesRealA += (valA == LOW) ? 1 : -1;
		} else {
			encodeurs.nbEncochesRealB += (valA == LOW) ? 1 : -1;
		}
	}
}

// Fonction de gestion de la réception des commandes I2C
//
// /!\ Si ça merde optimiser ça avec une lecture hors du sous prog d'intérruption
//
void i2cReceive(int howMany) {
	while (Wire.available()) {
		// Lecture de la commande
		i2cCommand = Wire.read();
	}
}

// Fonction de traitement des envois au maitre.
// La commande est setter avant par le maitre.
void i2cRequest() {
	// Si le maitre fait une demande d'info, c'est fait ici.
	switch (i2cCommand) {
		case CMD_LECTURE :
			// Envoi de la valeur sur 2 octets (int sur 2 byte en AVR 8bits)
			sendEncodeursValues();
			break;
		case CMD_VERSION :
			// Envoi de la version sur un octet
			Wire.write((char) VERSION);
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
	commut = false;
	encodeurs.nbEncochesRealA = 0;
	encodeurs.nbEncochesRealB = 0;

#ifdef DEBUG_MODE
	Serial.println("Initialisation des valeurs codeurs à 0.");
#endif

	interrupts();
}

// Gestion de l'envoi des valeurs de comptage.
// Gère également un roulement sur le compteur pour ne pas perdre de valeur lors de l'envoi
void sendEncodeursValues() {
	signed int value;
	if (commut) {
		commut = false;
		value = encodeurs.nbEncochesRealB;
		encodeurs.nbEncochesRealB = 0;
	} else {
		commut = true;
		value = encodeurs.nbEncochesRealA;
		encodeurs.nbEncochesRealA = 0;
	}

	// Application du coëficient si configuré
	if (invert) {
		value *= -1;
	}

#ifdef DEBUG_MODE
	Serial.print("Valeur codeur (invert : ");
	Serial.print(invert, HEX);
	Serial.print(") : ");
	Serial.println(value, DEC);
#endif

	// Envoi de la valeur sur 2 octets
	//
	// /!\ Envoi du MSB au LSB car la lecture décale a gauche
	//
	Wire.write(value >> 8);
	Wire.write(value & 0xFF);
}

/**
 * Fonction de gestion du mouvement du bras.
 * Est appelé a chaque boucle de l'arduino.
 */
void mouvementCadeaux() {
	if (stepMouvementCdx != MVT_STOP) {
		switch (stepMouvementCdx) {
			case MVT_SORTIR:
				sortirBras();
				if (digitalRead(IND_SORTIE) == HIGH) {
					stopMoteur();
					stepMouvementCdx = MVT_RENTRER;
				}
				break;
			case MVT_RENTRER:
				rentrerBras();
				if (digitalRead(IND_RENTRER) == HIGH) {
					stopMoteur();
					stepMouvementCdx = MVT_STOP;
				}
				break;
		}
	} else {
		stopMoteur();
	}
}

/**
 * Arret du moteur du bras
 */
void stopMoteur() {
	analogWrite(MOT_PWM, 0);
	digitalWrite(MOT_BRAKE, HIGH);
}

/**
 * Sortir le bras pour balancer un cadeau
 */
void sortirBras() {
	digitalWrite(MOT_BRAKE, LOW);
	digitalWrite(MOT_DIR, DIR_SORTIR);
	analogWrite(MOT_PWM, 64);
}

/**
 * Rentrer le bras pour ne pas accrocher le reste sur la table
 */
void rentrerBras() {
	digitalWrite(MOT_BRAKE, LOW);
	digitalWrite(MOT_DIR, DIR_RENTRER);
	analogWrite(MOT_PWM, 64);
}

/*
 * Cette fonction est en charge de lire les paramètres de configuration et de les enregistrer.
 */
void readConfiguration() {
#ifdef DEBUG_MODE
	Serial.print(" * Lecture des paramètres de configuration");
#endif

	// Lecture des infos I2C
	while (Wire.available()) {
		char param = Wire.read();
		switch (param) {
			case PARAM_INVERT :
				invert = Wire.read() == 1;
#ifdef DEBUG_MODE
	Serial.print("   - Configuration de l'invertion : ");
	Serial.println(invert, BIN);
#endif
				break;
		}
	}
}
