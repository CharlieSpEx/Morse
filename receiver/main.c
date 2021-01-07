/*--------------------------------------------------------------------------------------------------------
					Technika Mikroprocesorowa 2 - laboratorium
					Lab 5 - Ćwiczenie 4: wyzwalanie sprzętowe przetwornika A/C - tryb wyzwalania na żądanie (PIT0)
					autor: Mariusz Sokołowski
					wersja: 07.11.2020r.
---------------------------------------------------------------------------------------------------------*/
					
#include "MKL05Z4.h"
#include "ADC.h"
#include "pit.h"
#include "frdm_bsp.h"
#include "lcd1602.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "uart0.h"


float adc_volt_coeff = ((float)(((float)2.91) / 4095) );			// Współczynnik korekcji wyniku, w stosunku do napięcia referencyjnego przetwornika
uint8_t wynik_ok=0;
uint16_t temp;
uint16_t	wynik;
int recieveFlag = 0;
static int minDotCnt = 81;																			// zmienne do rozróżniania . od - na podstawie zliczonych impulsów
static int maxDotCnt = 0;
static uint16_t mCnt = 0;																			// wewnętrzny licznik do zliczania wywołań ADC
//static char recChar;
char recSym[] = {'0','0','0','0','0'};
static int wordCnt = 0;
char morseTab[] = {'5','H','4','S','-','V','3','I','-','F','-','U','-','-','2','E','-','L','-','R','-','-','-','A','-','P','-','W','-','J','1','6','B','-','D','-','X','-','N','-','C','-','K','-','Y','-','T','7','Z','-','G','-','Q','-','M','8','-','-','O','9','-','0'};
char recWord[] = {0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
char recChar;
void morseDecoder(char word[5]);	

static int index = 0;																					// index do wyświetlania na LCD i zapisywania w recWord
	
void PIT_IRQHandler()
{
		// jeśli wywołane, oznacza że nie przyszedł nowy znak od jakiegoś czasu, i koniec nadawnia jednej - lub .
	if (recieveFlag == 1){
	//	porównuje impulsy z mCnt i robi z tego "." lub "-"
		if ((mCnt >= minDotCnt) && (mCnt <= maxDotCnt)){recSym[wordCnt] = '.';}
		else if((mCnt >= 2*minDotCnt) && (mCnt <= 2*maxDotCnt)){recSym[wordCnt] = '-';}
		else {recSym[wordCnt] = '1';}												// błąd
		wordCnt++;
		mCnt = 0;
			LCD1602_SetCursor(0,0);
			LCD1602_Print("pit trigered case 1");
		
	// ustawienie licznika na przypuszczalne następne słowo
		PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TIE_MASK; // wyłączenie licznika
		PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(3*BUS_CLOCK);	// BUS_CLOCK - 1s /*czas między słowami - czas między znakami*/
		PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK; // wystartowanie licznika
		PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;	
		recieveFlag++;
		
	// jeśli będzie nadawany następny char morsa, to powyższy licznik nie dobiegnie końca (zostanie nadpisany przez ten w while(1)
	// i do recSym zostanie dodany następny char morsa.
	// jeśli dobiegnie do końca, oznacza to że odebraliśmy już całe słowo w dziedzinie morsa, i zostanie wykonany warunek poniżej 
	}
	else if(recieveFlag > 1){
	// zaczyna nowy symbol
		mCnt = 0;
		wordCnt = 0;
	morseDecoder(recSym);// wywołanie funkcji dekodującej
	recWord[16-index] = recChar;
	index++;
			LCD1602_SetCursor(0,0);
			LCD1602_Print("pit trigered case 2");
	//TO DO
	// wywalenie na lcd litery
	}
	else {/*nie powinno być wywołane*/};
			LCD1602_SetCursor(0,0);
			LCD1602_Print("pit trigered case 3");
	//resztki
	//PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;		// Skasuj flagę żądania przerwania
}


void morseDecoder(char word[5]){
	int i = 0;
	if (word[0] == '-') i += 47; else if (word[0] == '.')i += 16;
	if (word[1] == '-') i += 8; else if (word[1] == '.') i -= 8; else recChar =  morseTab[i-1];
	if (word[2] == '-') i += 4; else if (word[2] == '.') i -= 4; else recChar =  morseTab[i-1];
	if (word[3] == '-') i += 2; else if (word[3] == '.') i -= 2; else recChar =  morseTab[i-1];
	if (word[4] == '-') i += 1; else if (word[4] == '.') i -= 1; else recChar =  morseTab[i-1];
	recChar = morseTab[i-1];
}

void ADC0_IRQHandler()
{	
	temp = ADC0->R[0];		// Odczyt danej i skasowanie flagi COCO
	if(!wynik_ok)					// Sprawdź, czy wynik skonsumowany przez pętlę główną
	{
		wynik = temp;				// Wyślij nową daną do pętli głównej
		wynik_ok=1;

		sprintf(recWord,"%hu",wynik); // do kontroli
		LCD1602_SetCursor(0,1);
		LCD1602_Print(recWord);
		LCD1602_Print(" odczyt  ");
		sprintf(recWord,"%hu",mCnt); // do kontroli
		LCD1602_SetCursor(0,0);
		LCD1602_Print(recWord);
		LCD1602_Print(" licznik ");

			if (wynik >4000 ){//|| wynik <= NegCntrlVal){
			mCnt++;
			//wynik_ok=0;
			recieveFlag = 1;

			// załączenie licznika PIT, zresetowanie stanu, trzeba sprawdzić czy dobrze działa tak
			PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TIE_MASK;			// wyłączenie licznika
			PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(BUS_CLOCK);	//czas między znakami
			PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK; 			// wystartowanie licznika
			PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;	

			}
	}
	NVIC_EnableIRQ(ADC0_IRQn);
	ADC0->SC1[0] |= ADC_SC1_ADCH(12);		// Wyzwolenie programowe przetwornika ADC0 w kanale 12
}
int main (void)
{
	uint8_t	kal_error;
//	uint32_t i=0;
	uint16_t NegCntrlVal = 500; 																// wartość do której porównywany będzie odebrany sygnał, do ustalenia
	uint16_t PosCntrlVal = 4000;

//	char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
	LCD1602_Init();		 																		// Inicjalizacja wyświetlacza LCD
	LCD1602_Backlight(TRUE);
	LCD1602_Print("---");																	// Ekran kontrolny
	PIT_Init();																					  // Inicjalizacja licznika PIT0
	//UART0_Init();																					// Inicjalizacja portu szeregowego UART0

	kal_error=ADC_Init();																	// Inicjalizacja i kalibracja przetwornika A/C
	if(kal_error)
	{
		while(1);																						// Klaibracja się nie powiodła
	}
		ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(12);		// Pierwsze wyzwolenie przetwornika ADC0 w kanale 12 i odblokowanie przerwania

	//ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(4);		// Odblokowanie przerwania i wybranie kanału nr 4
	
																												// Inicjalizacja mikrofonu
																												// TO DO

	while(1)
	{
		//LCD1602_SetCursor(0,1);
		//LCD1602_Print("while loop starts");																	// Ekran kontrolny

		if(wynik_ok)
		{

			//mCnt++;
			//wynik = mCnt;
			/*
			sprintf(recWord,"%hu",wynik); // do kontroli
			LCD1602_SetCursor(0,1);
			LCD1602_Print(recWord);
			LCD1602_Print(" odczyt");
			sprintf(recWord,"%hu",mCnt); // do kontroli
			LCD1602_SetCursor(0,0);
			LCD1602_Print(recWord);
			*/
			wynik_ok=0;
		
			// DO NOT DELETE
/*			
			if (wynik >PosCntrlVal ){//|| wynik <= NegCntrlVal){
			mCnt++;
			wynik_ok=0;
			recieveFlag = 1;

			// załączenie licznika PIT, zresetowanie stanu, trzeba sprawdzić czy dobrze działa tak
			PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TIE_MASK;			// wyłączenie licznika
			PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(BUS_CLOCK);	//czas między znakami
			PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK; 			// wystartowanie licznika
			PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;	

			}
		
			*/
		}
		else if (!wynik_ok) {wynik_ok=1;}
		
	}
}
