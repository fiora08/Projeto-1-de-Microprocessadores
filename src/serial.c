#ifndef F_CPU
#define F_CPU 16000000UL // Define a velocidade do microprocessador
#endif
#define BAUD 19200 // define a velocidade do protocolo
#define MYUBRR F_CPU/16/BAUD-1 // calculo do registrador de velocidade

#include "serial.h"
#include "timers.h"
#include <avr/io.h>
#include <avr/interrupt.h>

volatile char buffer_rx[TAM_BUFFER]; // buffer para armazenar variaveis na opera��o
volatile unsigned char ler_ptr = 0, esc_ptr = 0; // ponteiros para controlar a fila de leitura e escrita
volatile char erro_com_flag = 0; // indica apos 3 tentativas 
volatile char fora_do_ar_flag = 0; // indica o erro de 2 minutos

void serial_inicializar(unsigned int valor_ubrr) {
	UBRR0H = (unsigned char)(valor_ubrr >> 8);  // Ajusta a parte alta da velocidade 
	UBRR0L = (unsigned char)valor_ubrr; // ajusta a parte baixa da velocidade 
	
	UCSR0B |= (1 << TXEN0) | (1 << RXEN0) | (1 << RXCIE0); // liga a transmiss�o rx e tx  e o alarme de interrup��o 
	UCSR0C = (1 << UCSZ01) | (1 << UCSZ00) | (1 << UPM01); //  define os 8 bits  de dados  e ativa a paridade par 
}

ISR(USART0_RX_vect) {
	char dado = UDR0; // captura o byte que acabou de chegar no rx 
	unsigned char prox = (esc_ptr + 1) % TAM_BUFFER;  // calcula o proximo na fila. 
	if (prox != ler_ptr) { // se o buffer n�o estiver cheio 
		buffer_rx[esc_ptr] = dado; // salva o dado 
		esc_ptr = prox; // move o ponteiro de escrita
	}
	// aqui zera o flag do alarme e o contador do alarme .
	contador_timeout = 0; 
	fora_do_ar_flag = 0;
}

unsigned char serial_disponivel(void) {
	return ler_ptr != esc_ptr; // retorna verdadeiro se tiver letras no buffer para ser lidos 
}

char serial_ler(void) {
	char c = buffer_rx[ler_ptr];  // pega o caracter mais antigo na fila 
	ler_ptr = (ler_ptr + 1) % TAM_BUFFER; // move o ponteiro de leitura  
	return c;
}

void serial_transmitir(unsigned char dado) {
	while (!(UCSR0A & (1 << UDRE0))); // s� envia o dado  quando o canal tx estiver livre 
	UDR0 = dado; // envia o byte 
}

void serial_escrever(const char* texto) {
	while (*texto) serial_transmitir(*texto++); // percorre a frase enviando letra por letra
}   


void serial_enviar_triplo(const char* comando, char *sucesso) {
	*sucesso = 0; // inicia assumindo falha 
	contador_timeout = 0;
	for (int i = 0; i < 3; i++) { // loop das 3 tentativas 
		serial_escrever(comando);  // envia um comando , estorno , venda , etc...
		
		contador_serial = 0; // zera o cronometro do timer 1
		flag_2s = 0;         // reseta a flag de tempo esgotado
		
		while (!flag_2s) {  // enquanto n�o passar os 2 segundos 
			if (serial_disponivel()) { // se o servidor responder 
				*sucesso = 1; // deu certo 
				erro_com_flag = 0; // limpa o erro 
				fora_do_ar_flag = 0;
				return; // sai da fun��o 
				}
		if(contador_timeout>=5000){
			fora_do_ar_flag = 1;
			erro_com_flag = 1;
			return;
		}
			 
			}

		} 
	fora_do_ar_flag = 1;
	erro_com_flag = 1; // se saiu do loop de 3 tentativas servidor falhou
	}