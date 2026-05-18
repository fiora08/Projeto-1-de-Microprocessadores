#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/iom2560.h>
#include <util/delay.h>
#include <string.h>
#include "maquina.h"
#include "timers.h"
#include "serial.h"
#include "lcd.h"
#include "teclado.h"
#include "energia.h"
#include "interface.h"
#include "senhas.h"
#include "vendas.h"
#include "maquina_protocolo.h"
#include "protocolo.h"
#include "login.h"
unsigned char ind_parcelas[5];
unsigned char qtd_parcelas = 0;

/*void verificar_parcelas(void) {
    static unsigned char ja_tentou_12h = 0;
    static unsigned char ja_tentou_18h = 0;
    static unsigned char ja_tentou_22h = 0;
    // reseta as flags quando sai do horário
    if (h != 12) ja_tentou_12h = 0;
    if (h != 18) ja_tentou_18h = 0;
    if (h != 22) ja_tentou_22h = 0;

    if ((h == 12 && !ja_tentou_12h) || (h == 18 && !ja_tentou_18h) || (h == 22 && !ja_tentou_22h)) {

        for (int i = 0; i < 5; i++) {
            if (n_vendas[i].tipo_venda == VENDA_PARCELADA) {
                enviar_agendamento(n_vendas[i].bandeira, (char *)n_vendas[i].num_cartao, (char *)n_vendas[i].valor_venda );                
                ind_parcelas[qtd_parcelas] = i;
                qtd_parcelas++;               
                if (maquina_protocolo() == 1) {
                    char *msg = protocolo_get_mensagem();
                    if (msg[2] != 'P') {
                        //pedir pro tiago botar o campo pagamento_pendente na struct de vendas
                        //n_vendas[i].pagamento_pendente =1;
                        if(n_vendas[i].num_parcelas > 0) 
                            PORTB |= (1 << LED_pagamento_pendente);
                            if(protocolo_get_mensagem() == "SAP")
                                n_vendas[i].num_parcelas = n_vendas[i].num_parcelas - 1;
                }
            }
        }

        if (h == 12) ja_tentou_12h = 1;
        if (h == 18) ja_tentou_18h = 1;
        if (h == 22) ja_tentou_22h = 1;
    }
}

}

void listar_parcelas()
{
    for(int j = 0; j < qtd_parcelas; j++){
        unsigned char idx = ind_parcelas[j];
        lcd_limpar();
        lcd_escrever_string("Cod:");
        lcd_caractere('0'+n_vendas[idx].codigo);
        lcd_posicionar(1,0);
        lcd_escrever_string("Parc");
        lcd_caractere('0'+n_vendas[idx].num_parcelas);
        atraso_ms(2000);
    }
}*/

unsigned char verificar_parcelas() {
    static unsigned char ja_tentou_12h = 0;
    static unsigned char ja_tentou_18h = 0;
    static unsigned char ja_tentou_22h = 0;
    static int parc_quit = 0;
    static int state_send = 0;
    static int resposta = 1;

    if (h != 12) ja_tentou_12h = 0;
    if (h != 18) ja_tentou_18h = 0;
    if (h != 22) ja_tentou_22h = 0;

    if ((h == 12 && !ja_tentou_12h) ||
        (h == 18 && !ja_tentou_18h) ||
        (h == 22 && !ja_tentou_22h)) {
            //func_test(); 
        qtd_parcelas = 0; // reseta a lista antes de popular
        parc_quit = 0;
        //(strcmp(protocolo_get_mensagem(), "SAP") == 0)
        for (int i = 0; i < 5; i++) {
            //if (n_vendas[i].tipo_venda == VENDA_PARCELADA)
            serial_transmitir(n_vendas[i].codigo);
            if (n_vendas[i].tipo_venda == VENDA_PARCELADA && (mes-n_vendas[i].data == 1 || mes-n_vendas[i].data < 0)) {
                ind_parcelas[qtd_parcelas] = i;
                qtd_parcelas++;
                if(!state_send){
                    enviar_agendamento(n_vendas[i].bandeira,
                                   (char *)n_vendas[i].num_cartao,
                                   (char *)n_vendas[i].valor_venda);
                    resposta = 0;
                    state_send = 1;
                }
                
                while(!resposta){
                    if (maquina_protocolo() == 1) {
                    char *msg = protocolo_get_mensagem();
                    if (msg[2] != 'P') {
                        if (n_vendas[i].num_parcelas > 0 && msg[2] == 'N') {
                        PORTB |= (1 << LED_pagamento_pendente);
                        lcd_limpar();
        				lcd_posicionar(0, 1);
        				lcd_escrever_string("Pagamento nao");
        				lcd_posicionar(1, 3);
        				lcd_escrever_string("localizado!");
                        atraso_ms(500);
                        //lcd_limpar();
                        resposta = 1;
                        state_send = 0;
                        //return 1;
                        }else{
        					lcd_posicionar(0, 0);
        					lcd_escrever_string("Cartao com falha");
							lcd_posicionar(1, 3);
        					lcd_escrever_string("(invalida)");  
                        atraso_ms(500);
                        //lcd_limpar();
                            resposta = 1;
                            state_send = 0;
                            //return 1;
                        }                       
                    }else if((strcmp(protocolo_get_mensagem(), "SAP") == 0)){
                        n_vendas[i].data = mes; 
                        n_vendas[i].num_parcelas--;
                        if(n_vendas[i].num_parcelas == 0){
                            n_vendas[i].tipo_venda = VENDA_QUITADA; 
                        }
        				lcd_posicionar(0, 4);
        				lcd_escrever_string("Pagamento");
        				lcd_posicionar(1, 4);
        				lcd_escrever_string("efetivado");
                        atraso_ms(500);
                        //lcd_limpar();
                            resposta = 1;
                            state_send = 0;
                            //return 1;
                    }
                }
                }
                lcd_limpar();
            }
        }
        for (int j = 0; j < 5; j++){
            if(n_vendas[j].tipo_venda == VENDA_PARCELADA && n_vendas[j].num_parcelas != 0){
                 parc_quit = 1;
            }
        }
        if (parc_quit == 0){
            PORTB &= ~(1 << LED_pagamento_pendente);       
        }

        if (h == 12) ja_tentou_12h = 1;
        if (h == 18) ja_tentou_18h = 1;
        if (h == 22) ja_tentou_22h = 1;
    }
    return 1;
}
/*
void listar_parcelas(void) {
    if (qtd_parcelas == 0) {
        lcd_limpar();
        lcd_escrever_string("Sem parcelas");
        atraso_ms(500);
        return;
    }

    for (int j = 0; j < qtd_parcelas; j++) {
        unsigned char idx = ind_parcelas[j];
        char buf[8];

        lcd_limpar();
        lcd_escrever_string("Cod:");
        lcd_caractere(n_vendas[idx].codigo);

        lcd_posicionar(1, 0);
        lcd_escrever_string("Parc:");
        lcd_caractere(n_vendas[idx].num_parcelas);

        atraso_ms(500);
    }
    for (int j = 0; j < qtd_parcelas; j++) {
    unsigned char idx = ind_parcelas[j];
    char buf[8];

    lcd_limpar();
    lcd_escrever_string("Cod:");
    sprintf(buf, "%d", n_vendas[idx].valor_venda);
    lcd_escrever_string(buf);

    lcd_posicionar(1, 0);
    lcd_escrever_string("Parc:");
    sprintf(buf, "%d", n_vendas[idx].num_parcelas);
    lcd_escrever_string(buf);

    atraso_ms(500);
}
}*/

void listar_parcelas() {
    if (qtd_parcelas == 0) {
        lcd_limpar();
        lcd_escrever_string("Sem parcelas");
        atraso_ms(2000);
        return;
    }

    for (int j = 0; j < qtd_parcelas; j += 2) {
        char buf[17];
        lcd_limpar();

        lcd_posicionar(0, 0);
        sprintf(buf, "Cod:%-2d  Parc:%-2d",
                n_vendas[ind_parcelas[j]].codigo,
                n_vendas[ind_parcelas[j]].num_parcelas-'0');
        lcd_escrever_string(buf);

        if (j + 1 < qtd_parcelas) {
            lcd_posicionar(1, 0);
            sprintf(buf, "Cod:%-2d  Parc:%-2d",
                    n_vendas[ind_parcelas[j+1]].codigo,
                    n_vendas[ind_parcelas[j+1]].num_parcelas-'0');
            lcd_escrever_string(buf);
        }

        atraso_ms(3000);
    }

    lcd_limpar();
}