#include <msp430.h>
#include "pins.h"

/**
TO-DO:	Redirecionar a entrada do TimerA0 para o pino P1.2 (Canal 1)
		Ler o botão por interrupção
		Verificar a interrupção do modo de captura (TAIV talvez não seja o registro certo)
		Utilizar a flag CCI/SCII para identificar os flancos de subida ou decida do relógio
		That's all folks
*/


/**
 * Trigger: P2.5 - Output
 * Echo: P1.2  - Input
 * */
inline void delay_mili(unsigned short int t_mili) {
    //Seta timer: Clock = ACLK dividido por 9, modo de subida.
    TA2CTL = (TASSEL__ACLK | MC__UP | ID__8);
    //Divide o valor do clock por 4
    TA2EX0 = TAIDEX_4;
    //Seta modo de saida para SET
    TA2CCTL0 = OUTMOD_1;
    //Limiar da contagem
    TA2CCR0 = t_mili;
    // Reseta o timer A1 e inicia a contagem
    SET_REG(TA2CTL, TACLR);
    // Enquanto o tempo for menor aguarda o delay
    while(!READ_REG(TA2CCTL0,CCIFG));
}

void delay_micro(unsigned short int delay_microsec) {
    //Coloca o limiar de contagem
    TA0CCR0 = (delay_microsec);
    //Coloca o OUTMOD para SET
    TA0CCTL0 = OUTMOD_1;
    //Inicia o timer com o SMCLK | modo de subida | zerado (CLR)
    TA0CTL = (TASSEL__SMCLK | MC__UP | TACLR);
    while(!READ_REG(TA0CCTL0,CCIFG));
}

void config_pins() {
    setPin(P2_5, OUTPUT); //Trigger
    setPin(P1_2, INPUT);  //Echo
    setPin(P1_0, OUTPUT); //LED Vermelho
    setPin(P4_7, OUTPUT); //LED Verde
    setPin(P2_1, INPUT);  //Botão S1
}

void config_timerA0() {
    TA0CCTL1 = (CM_3 | CAP | CCIE);
    TA0CTL = (TASSEL__SMCLK | MC_UP | TAIE | TACLR);
}

unsigned char has_interrupted = 0;
unsigned int timer;

void main(void) {
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	
	config_module();
	__enable_interrupt();

	while(1){
	    while(!readPin(P2_1)) {
	        writePin(P1_0, LOW);  // Limpa os outputs da última medição
	        writePin(P4_7, LOW);
	        writePin(P2_5, HIGH); // Escreve pulso no trigger
	        delay_micro(10);      // Delay de 10 micro
	        writePin(P2_5, LOW);  // Limpa o pulso do trigger
	        config_timerA0();     // Configura o timer pro capturar as interrupções em echo
	        delay_mili(100);      // Debounce
	    }
	    delay_mili(100);    //Debounce

	    if(timer != 0 && !has_interrupted){     //valor do timer já foi definido (Modo CAP já gerou as duas interrupções)
	        if(timer <= 0.0016667) { //< 20 cm 
	            writePin(P1_0, HIGH);
	        }
	        else if(timer >= 0.0016667 && timer <= 0.0023529) { //  20 cm << x << 40 cm
	            writePin(P1_0, HIGH);
	            writePin(P4_7, HIGH);
	        }
	        else { // >> 40 cm
	            writePin(4_7, HIGH);
	        }
	        timer = 0; //zera o valor do timer
	    }
	}
}


//Rotina de interrupções do Timer A
#pragma vector = TA_IV_VECTOR
__interrupt void TAISR(){
    switch(TAIV){
    case 0x02:
        if(!has_interrupted){
            timer = TA0CCR1;
            has_interrupted = 1;
        }
        else {
            timer = TA0CCR1 - timer;
            has_interrupted = 0;
        }

        break;
    case 0x04:
        break;
    case 0x06:
        break;
    case 0x08:
        break;
    case 0x0A:
        break;
    case 0x0C:
        break;
    case 0x0E:
        break;
    }
}

