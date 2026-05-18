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
#include "admin.h"
#include "venda_agendada.h"


unsigned char menu_admin(unsigned char tecla, usuario *usuarios) {
    static unsigned char mostrando_msg = 0;
    static unsigned char no_menu = 0;
    static unsigned char entrou_menu_hora = 0;
    static unsigned char entrou_menu_data = 0;
    static unsigned char entrou_menu_pendencias = 0;
    
    if (mostrando_msg) {
        if (tempo_n_bloqueante(1500)) {
            mostrando_msg = 0;
            no_menu = 0;
            lcd_limpar();
        }
        return 0;  
    }

    if (entrou_menu_hora) {
        static unsigned char modo_edicao = 1;
        static int ultimo_seg = -1;
        
        //teclado_atualizar();
        //tecla = teclado_obter_tecla();

            if (modo_edicao == 1) {
                if (alterar_hora(tecla)) {
                    modo_edicao = 2;
                    lcd_limpar();
                    tecla = 0; // zera aqui
                }
            }
           if (modo_edicao == 2 && tecla != '#') { // só processa minuto se não for '#'
                if (alterar_minuto(tecla)){ 
                    modo_edicao = 1;
                    entrou_menu_hora = 0;
                    no_menu = 0 ;
                }
            }

            if (seg != ultimo_seg) {
                ultimo_seg = seg;
                //no_menu = 0 ;
                lcd_posicionar(0, 0);
                mostrar_hora();
                lcd_posicionar(1, 0);
                mostrar_data();   
                 //entrou_menu_hora = 0;             
            }
            return 0;
    
    }

    if (entrou_menu_data) {
        static unsigned char modo_edicao = 1;
        static int ultimo_seg = -1;
        
        //teclado_atualizar();
        //tecla = teclado_obter_tecla();

            if (modo_edicao == 1) {
                if (alterar_dia(tecla)) {
                    modo_edicao = 2;
                    lcd_limpar();
                    tecla = 0; // zera aqui
                }
            }
            if (modo_edicao == 2 && tecla != '#') { // só processa minuto se não for '#'
                if (alterar_mes(tecla)){ 
                    modo_edicao = 1;
                    entrou_menu_data = 0;
                    no_menu = 0 ;
                }
            }

            if (seg != ultimo_seg) {
                ultimo_seg = seg;
                lcd_posicionar(0, 0);
                mostrar_data();
            }
            return 0;
    
    }

    if (!no_menu) {
        lcd_limpar();
        lcd_escrever_string("1-OP0 2-OP1 3-HR");
        lcd_posicionar(1, 0);
        lcd_escrever_string("4-PD 5-PD/ 6-DT");
        no_menu = 1;
    }
    switch (tecla) {
        case '1':
        usuarios[0].habilitado ^= 1;
        lcd_limpar();
            if (usuarios[0].habilitado) {
                
                lcd_escrever_string("OP0 HABILITADO");
                } else {
                lcd_escrever_string("OP0 DESABILITADO");
                }
                mostrando_msg = 1;
                break;
                
        case '2':
        usuarios[1].habilitado ^= 1;
        lcd_limpar();
        if (usuarios[1].habilitado) {
            
            lcd_escrever_string("OP1 HABILITADO");
            } else {
            lcd_escrever_string("OP1 DESABILITADO");
            }
            mostrando_msg = 1;
        break;

        case '3': 
        //alteração de hora
       entrou_menu_hora = 1;
       lcd_limpar();
       tecla=0;
       break;
        
    
        case '4': 
        //alteração de hora
       entrou_menu_pendencias = 1;
       lcd_limpar();
       tecla=0;
       //verificar_parcelas();
       listar_parcelas();
       break;

       case '6':
        //alteração de data
       entrou_menu_data = 1;
       lcd_limpar();
       tecla=0;
       break; 
        }
    if (tecla == '*') {
        no_menu = 0;
        entrou_menu_hora = 0;
        mostrando_msg = 0;
        return 1;
    }
    return 0;
}
    

unsigned char admin_verificar_pendencias(venda *n_vendas, unsigned char indice_out[]){
    unsigned char contas = 0;
    for(int i=0; i<5; i++){
        if(n_vendas[i].tipo_venda == VENDA_PARCELADA && n_vendas[i].num_parcelas > 0){
            indice_out[contas++] = i;
        }
    }
    for(int i=0; i<contas; i++){ //revisar após se lcd_caractere aqui nao ta travando o loop, ele costuma
        if(i == 2) lcd_posicionar(1, 0);
        lcd_caractere(indice_out[i] + '0');
        lcd_caractere(':');
        lcd_escrever_string((char*)n_vendas[indice_out[i]].valor_venda);
        lcd_caractere(' ');
        lcd_caractere(n_vendas[indice_out[i]].num_parcelas + '0');
        lcd_caractere('x');
        lcd_caractere(' ');
}
    return contas;
}