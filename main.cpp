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
		// Boucle infinie pour le fonctionnement.
		loop();
	}
}

// Method de configuration pour le fonctionnement du programme
void setup() {
	// ------------------------------------------------------------- //
	// Initialisation du port s�rie en debug seulement (cf define.h) //
	// ------------------------------------------------------------- //
#ifdef DEBUG_MODE
	Serial.begin(115200);
	Serial.println(" == INITIALISATION CARTE CODEUR ==");
#endif

	// ------------------------- //
	// D�finition des broches IO //
	// ------------------------- //
	pinMode(ADD1, INPUT);
	pinMode(ADD2, INPUT);
	pinMode(CHA, INPUT);
	pinMode(CHB, INPUT);
	pinMode(IDX, INPUT);

#ifdef DEBUG_MODE
	Serial.println(" - IO [OK]");
#endif

	// -------------------------------------------------------- //
	// D�finition des fonctions d'int�rruption pour le comptage //
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
	Serial.print(externalIntType);
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
	Serial.print(i2cAddress);
	Serial.println(")");
#endif

	// Initialisation des valeurs � 0
	resetEncodeursValues();
}

// M�thode appel� encore et encore, tant que la carte reste aliment�.
void loop() {
	// Gestion des commande ne devant rien renvoy�.
	// /!\ Etre exhaustif sur les commandes car sinon le request ne pourra pas fonctionn� si elle trait� ici.
	if (i2cCommand == CMD_RESET) {
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

// Fonction d'int�rruption pour le comptage sur les infos du canal A
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

// Fonction d'int�rruption pour le comptage sur les infos du canal B
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

// Fonction de gestion de la r�ception des commandes I2C
//
// /!\ Si �a merde optimiser �a avec une lecture hors du sous prog d'int�rruption
//
void i2cReceive(int howMany) {
	while (Wire.available()) {
		// Lecture de la commande
		i2cCommand = Wire.read();
	}
}

// Fonction de traitement des envoi au maitre.
// La commande est setter avant par le maitre.
void i2cRequest() {
	// Si le maitre fait une demande d'info, c'est fait ici.
	switch (i2cCommand) {
		case CMD_LECTURE :
			// Envoi de la valeur sur 4 octets
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

// R�initialisation des valeurs de comptage
void resetEncodeursValues() {
	noInterrupts();
	commutCounter = false;
	encodeurs.nbEncochesRealA = 0;
	encodeurs.nbEncochesRealB = 0;

#ifdef DEBUG_MODE
	Serial.println("Initialisation des valeurs codeurs � 0.");
#endif

	interrupts();
}

// Gestion de l'envoi des valeurs de comptage.
// G�re �galement un roulement sur le compteur pour ne pas perdre de valeur lors de l'envoi
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

#ifdef DEBUG_MODE
	Serial.print("Valeur codeur : ");
	Serial.println(value);
#endif

	// Envoi de la valeur sur 4 octets
	for (int cpt = 0 ; cpt < 4 ; cpt++) {
		Wire.write(value & 0xFF);
		value = value >> 8;
	}
}
