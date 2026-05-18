#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/iom2560.h>
#include <util/delay.h>
#include <string.h>
#include <stdlib.h>
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
#include "admin.h"
#include "vendas.h"

 #define MAX_CARTOES 5
#define TAM_NUMERO 6
#define TAM_SENHA  6
#define SALDO_INICIAL 800


unsigned char cadastrar_cartao_local(cartao_local1 *cartoes,
                                     unsigned char num_cartoes){

    unsigned char p;
    unsigned char tecla;

    // verifica limite
    if(num_cartoes >= MAX_CARTOES){

        lcd_limpar();
        lcd_escrever_string("LIMITE CARTAO");
        atraso_ms(1500);

        return num_cartoes;
    }

    

    // NUMERO CARTAO
    
    lcd_limpar();
    lcd_escrever_string("N.CARTAO:");
    lcd_posicionar(1,0);

    p = 0;

    while(p < TAM_NUMERO){

        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if(tecla >= '0' && tecla <= '9'){

            cartoes[num_cartoes].numero_cartao[p] = tecla;

            lcd_caractere(tecla);

            p++;

            atraso_ms(200);
        }

        // cancela cadastro
        if(tecla == 'D'){

            lcd_limpar();
            lcd_escrever_string("CANCELADO");

            atraso_ms(1000);

            return num_cartoes;
        }
    }

    cartoes[num_cartoes].numero_cartao[p] = '\0';

    // =========================
    // SENHA
    // =========================

    lcd_limpar();
    lcd_escrever_string("SENHA:");
    lcd_posicionar(1,0);

    p = 0;

    while(p < TAM_SENHA){

        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if(tecla >= '0' && tecla <= '9'){
            cartoes[num_cartoes].senha_cartao[p] = tecla;
            lcd_caractere('*');
            p++;
            atraso_ms(200);
        }

        // cancela cadastro
        if(tecla == 'D'){

            lcd_limpar();
            lcd_escrever_string("CANCELADO");

            atraso_ms(1000);

            return num_cartoes;
        }
    }

    cartoes[num_cartoes].senha_cartao[p] = '\0';

    // saldo inicial
    cartoes[num_cartoes].saldo = SALDO_INICIAL;

    lcd_limpar();
    lcd_escrever_string("CADASTRADO");

    atraso_ms(1000);

    return num_cartoes + 1;
}






unsigned char venda_local(cartao_local1 *cartoes, unsigned char num_cartoes){

    char numero[10];
    char senha[8];

    unsigned char tecla;
    unsigned char p;
    int valor = 0;

    int indice = -1;

    // =========================
    // NUMERO CARTAO
    // =========================

    lcd_limpar();
    lcd_escrever_string("NUM CARTAO:");
    lcd_posicionar(1,0);

    p = 0;

    while(p < 6){

        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if(tecla >= '0' && tecla <= '9'){

            numero[p++] = tecla;

            lcd_caractere(tecla);

            atraso_ms(200);
        }

        if(tecla == 'D')
            return 0;
    }

    numero[p] = '\0';

    // procura cartao

    for(int i = 0; i < num_cartoes; i++){

        if(strcmp(numero, cartoes[i].numero_cartao) == 0){

            indice = i;
            break;
        }
    }

    if(indice == -1){

        lcd_limpar();
        lcd_escrever_string("NAO ENCONTRADO");

        atraso_ms(500);
        lcd_limpar();
        return 0;
    }


    // SENHA
    lcd_limpar();
    lcd_escrever_string("SENHA:");
    lcd_posicionar(1,0);

    p = 0;

    while(p < 6){

        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if(tecla >= '0' && tecla <= '9'){
            senha[p++] = tecla;
            lcd_caractere('*');
            atraso_ms(200);
        }

        if(tecla == 'D')
            return 0;
    }

    senha[p] = '\0';
    // verifica senha
    if(strcmp(senha, cartoes[indice].senha_cartao) != 0){
        lcd_limpar();
        lcd_escrever_string("SENHA INVALIDA");
        atraso_ms(1500);
        return 0;
    }

    // VALOR
    lcd_limpar();
    lcd_escrever_string("VALOR:");
    lcd_posicionar(1,0);

    while(1){

        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if(tecla >= '0' && tecla <= '9'){
            valor = (valor * 10) + (tecla - '0');
            lcd_caractere(tecla);
            atraso_ms(200);
        }
        if(tecla == 'D')
            break;
    }
    // CASHBACK
    if(cartoes[indice].cashback_disponivel){

        if(valor >= 20)
            valor -= 20;
        else
            valor = 0;
        cartoes[indice].cashback_disponivel = 0;
        lcd_limpar();
        lcd_escrever_string("CASHBACK -20");
        atraso_ms(1500);
    }

    // SALDO

    if(cartoes[indice].saldo < valor){
        lcd_limpar();
        lcd_escrever_string("SALDO INSUF.");
        atraso_ms(1500);
        return 0;
    }

    cartoes[indice].saldo -= valor;
    char buffer[10];

    itoa(cartoes[indice].saldo, buffer, 10);

    serial_escrever(buffer);


    // CONTADOR CASHBACK
    if(valor >= 50){
        cartoes[indice].contador_cashback++;
        if(cartoes[indice].contador_cashback >= 3){
            cartoes[indice].contador_cashback = 0;
            cartoes[indice].cashback_disponivel = 1;
            lcd_limpar();
            lcd_escrever_string("GANHOU CASHBK");
            atraso_ms(1500);
        }
    }

    lcd_limpar();
    lcd_escrever_string("VENDA APROV.");

    atraso_ms(1500);
    lcd_limpar();

    return 1;
}