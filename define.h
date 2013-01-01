/*
 * define.h
 *
 *  Created on: 1 janv. 2013
 *      Author: mythril
 */

#ifndef DEFINE_H_
#define DEFINE_H_

#include <Arduino.h>

// Mode debug : 1 => ON ; 0 => OFF
#define DEBUG_MODE 		1

// Definition du mode de comptage
// 1 : CHA front montant
// 2 : CHA et CHB front montant
// 4 : CHA et CHB change
#define MULT_MODE_1X	1
#define MULT_MODE_2X	2
#define MULT_MODE_4X	4
#define MULT_MODE_SEL	MULT_MODE_1X

// Addresse de base sur le bus I2C
#define BASE_ADD_I2C 	0xB0

// Bit permettant de d�terminer la partie variable de l'adresse I2C
#define ADD1			A7 // Analog 7
#define ADD2			A6 // Analog 6

// Broche permettant de r�cup�rer les informations du codeurs
#define CHA				2 // Digital 2
#define CHB				3 // Digital 3
#define IDX				4 // Digital 4

// Num�ro int�rruption externe
#define EXT_INT_CHA		0 // EXT Int 0
#define EXT_INT_CHB		1 // EXT Int 1

// Action depuis le bus I2C
#define RESET			'r'
#define LECTURE			'l'

// Structures pour la gestion des encodeurs
typedef struct {
	signed int nbEncochesRealA;
	signed int nbEncochesRealB;
} EncodeursValues;

extern volatile EncodeursValues encodeurs;

extern int i2cAddress;

#endif /* DEFINE_H_ */
