#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/iom2560.h>
#include "timers.h"
#include "lcd.h"
#include "teclado.h"
// volatile avisa ao compilador que esta variavel pode mudar a qualquer momento
// ideal para interrup��es
volatile unsigned char flag_1ms = 0;
volatile unsigned char flag_2s = 0;
volatile unsigned int contador_serial = 0;
volatile unsigned long contador_timeout = 0;
volatile int dia=0, mes= 0, ano=0,h = 0, min = 0, seg = 0;
// inicializa�a� retirada de um laboratorio. 
// pre escala 64 
void timer0_init(void){
	TCCR0A = (1 << WGM01); // modo CTC
	TCCR0B = (1 << CS01) | (1 << CS00); // Define a preescala em 64 
	// calculo feito = (16MHz / (64 * 1000Hz)) - 1 = 249
	OCR0A = 249;
	//Habilita a interrupção por compara��o no canal a  Timer 0
	TIMSK0 |= (1 << OCIE0A);
}

void timer1_init(void){
	TCCR1B |= (1 << WGM12); // Modo CTC
	OCR1A = 249; // Interrupção a cada 1ms
	TIMSK1 |= (1 << OCIE1A);
	TCCR1B |= (1 << CS11) | (1 << CS10); // Prescaler 64
}

// interrupção do timer 0,
//  ta sendo usada na função de atraso abaixo
ISR(TIMER0_COMPA_vect){
	flag_1ms = 1;
}

ISR(TIMER1_COMPA_vect) {
	// Controle do Timeout de tentativa 
	contador_serial++;
	if (contador_serial >= 2000) {
		flag_2s = 1;
	}
	contador_timeout++;
	// logica para relogio , conferir se da certo.
	static int milis = 0;
	if (++milis >= 1000) {
		milis = 0;
		if (++seg >= 60) {
			seg = 0;
			if (++min >= 60) {
				min = 0;
				if (++h >= 24)
				 h = 0;
			}
		}
	}
}


//função que usa o timer 0 para gerar uma quantidade configurável de atraso,
//  usa o timer 0 entao é bom usar ela no caso de não estarmos usando ele
//  e caso o sor pedir para nao usar biblioteca feita como o delay.h
void atraso_ms(unsigned int milissegundos){
	
	for(unsigned int i =0; i<milissegundos; i++){
		flag_1ms = 0;
		while (!flag_1ms);
	}

}

unsigned char tempo_n_bloqueante(unsigned int tempo) {
    static unsigned int contador_tempo = 0;
	if (flag_1ms) {
		contador_tempo++;
	}
    
    if (contador_tempo >= tempo) {
        contador_tempo = 0;
        return 1;
    }
    return 0;
}

unsigned int contadores_tempo[2] = {0, 0};

unsigned char tempo_n_bloqueante_v2(unsigned int tempo, unsigned char id) {
    if (flag_1ms) {
        contadores_tempo[id]++;
    }
    if (contadores_tempo[id] >= tempo) {
        contadores_tempo[id] = 0;
        return 1;
    }
    return 0;
}

void mostrar_hora(){// // converte inteiro para char somando com '0' (ASCII 48), separa dezena (/10) e unidade (%10)
	lcd_caractere('0' + h/10);  // dezena
	lcd_caractere('0' + h%10); // unidade
	lcd_caractere(':');
	lcd_caractere('0'+ min/10);
	lcd_caractere('0'+ min%10); 
	lcd_caractere(':');
	lcd_caractere('0'+ seg/10);
	lcd_caractere('0'+ seg%10); 

}

void mostrar_data(){
    lcd_caractere('0'+ mes/10);
    lcd_caractere('0'+ mes%10);
    lcd_caractere(':');
    lcd_caractere('0'+ dia/10);
    lcd_caractere('0'+dia%10);
}


unsigned char alterar_mes(unsigned char tecla){
    static unsigned char pos = 0;
    static unsigned char valor = 0;
    
    if (tecla != 0 && tecla != '#') {
        lcd_posicionar(1, 0);
        valor = valor * 10 + (tecla - '0');
        pos++;
        lcd_caractere(tecla);
    }
    if (tecla == '#' || pos == 2) {
        if (pos > 0 && valor <= 12) {
            mes = valor;
        }
        valor = 0;
        pos = 0;
        return 1;
    }
    return 0;
}

unsigned char alterar_dia(unsigned char tecla){
    static unsigned char pos = 0;
    static unsigned char valor = 0;
    
    if (tecla != 0 && tecla != '#') {
        lcd_posicionar(1, 0);
        valor = valor * 10 + (tecla - '0');
        pos++;
        lcd_caractere(tecla);
    }
    if (tecla == '#' || pos == 2) {
        if (pos > 0 && valor <= 31) {
            dia = valor;
        }
        valor = 0;
        pos = 0;
        return 1;
    }
    return 0;
}

unsigned char alterar_hora(unsigned char tecla) {
    static unsigned char pos = 0;
    static unsigned char valor = 0;
    
    if (tecla != 0 && tecla != '#') {
        lcd_posicionar(1, 0);
        valor = valor * 10 + (tecla - '0');
        pos++;
        lcd_caractere(tecla);
    }
    if (tecla == '#' || pos == 2) {
        if (pos > 0 && valor <= 23) {
            h = valor;
        }
        valor = 0;
        pos = 0;
        return 1;
    }
    return 0;
}

unsigned char alterar_minuto(unsigned char tecla) {
    static unsigned char pos = 0;
    static unsigned char valor = 0;
    
    if (tecla != 0 && tecla != '#') {
        lcd_posicionar(1, 0);
        valor = valor * 10 + (tecla - '0');
        pos++;
        lcd_caractere(tecla);
    }
    if (tecla == '#' || pos == 2) {
        if (pos > 0 && valor <= 59) {
            min = valor;
        }
        valor = 0;
        pos = 0;
        return 1;
    }
    return 0;
}