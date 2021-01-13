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
#include "port.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
//#include "uart0.h"
	

float adc_volt_coeff = ((float)(((float)2.91) / 4095) );			// Współczynnik korekcji wyniku, w stosunku do napięcia referencyjnego przetwornika
uint8_t wynik_ok = 0;
uint16_t temp;
uint16_t	wynik;
int recieveFlag = 0;
static int minDotCnt = 20;																			// zmienne do rozróżniania . od - na podstawie zliczonych impulsów
static int maxDotCnt = 40;
uint16_t mCnt = 0;																			// wewnętrzny licznik do zliczania wywołań ADC
uint16_t tempCnt = 0;
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
	
	// mCnt trzeba zamienić na tempCnt
		// jeśli wywołane, oznacza że nie przyszedł nowy znak od jakiegoś czasu, i koniec nadawnia jednej - lub .
	if (recieveFlag == 1){
	//	porównuje impulsy z mCnt i robi z tego "." lub "-"
		if ((mCnt >= 50) && (mCnt <= 80)){recSym[wordCnt] = '.';}
		else if((mCnt >= 150) && (mCnt <= 250)){recSym[wordCnt] = '-';}
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
			LCD1602_SetCursor(0,1);
			LCD1602_Print(recWord);
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




uint16_t status = 0;

void PORTB_IRQHandler(){
	
	
//		LCD1602_SetCursor(0,1);
//		LCD1602_Print("IRQ portB");	
	
		//wynik_ok=1;
		mCnt++;
		recieveFlag = 1;
		PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TIE_MASK;			 // wyłączenie licznika
		PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(BUS_CLOCK/2);	//czas między znakami
		PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK; 			// wystartowanie licznika
		PIT->CHANNEL[0].TFLG |= PIT_TFLG_TIF_MASK;	

		PORTB->PCR[1] |= PORT_PCR_IRQC_MASK;    // wykasowanie flagi
		//NVIC_EnableIRQ(PORTB_IRQn);

}

int main (void)
{
//	uint32_t i=0;
//	char display[]={0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20};
	LCD1602_Init();		 																		// Inicjalizacja wyświetlacza LCD
	LCD1602_Backlight(TRUE);
	LCD1602_Print("---");																	// Ekran kontrolny
	PIT_Init();																					  // Inicjalizacja licznika PIT0
	port_Init();																					// Inicjalizacja portów
	LCD1602_SetCursor(0,1);
	LCD1602_Print("setup");
	

	while(1)
	{
		LCD1602_SetCursor(0,1);
		sprintf(recWord,"%hu",status); // do kontroli
		LCD1602_Print(recWord);
		LCD1602_Print(" status");		
	
		sprintf(recWord,"%hu",mCnt); // do kontroli
		LCD1602_SetCursor(0,0);
		LCD1602_Print(recWord);
		LCD1602_Print(" licznik ");	
		//mCnt++;
		

				//mCnt++;

	if(wynik_ok){
		wynik_ok=0;
		//tempCnt = mCnt;

		LCD1602_SetCursor(0,1);
		LCD1602_Print("wynik ok flag");	
		
		//mCnt++;
		 
		

		

		//mCnt = 0;
		/*
		recieveFlag = 1;
		// załączenie licznika PIT, zresetowanie stanu, trzeba sprawdzić czy dobrze działa tak
		PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TIE_MASK;			// wyłączenie licznika
		PIT->CHANNEL[0].LDVAL = PIT_LDVAL_TSV(BUS_CLOCK);	//czas między znakami
		PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TIE_MASK; 			// wystartowanie licznika
		PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;	
		*/
	}

	}
}
