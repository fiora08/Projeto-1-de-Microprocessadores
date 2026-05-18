
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
#include "estorno.h"



unsigned char usuario_autenticado = 0;

// Estados do menu principal
#define MENU_PRINCIPAL  0
#define MENU_VENDAS     1
#define MENU_ESTORNO    2
#define MENU_ADMIN      3



int main(void) {
	// Inicialização dos periféricos
	serial_inicializar(51); //-> formula para dar o 119200 com 16 mhz foi definido o 51 para a velocidade baud. 
	timer0_init();
	timer1_init();
	teclado_inicializar();
	lcd_inicializar();
	energia_inicializar();
	
	sei(); // Habilita interrup  es

	//inicialização dos usuários 
	usuario  novo_usuario[3];
	cartao_local1 cartao_local[5];

	strcpy(novo_usuario[0].nome ,"ZE ROBERTO");
	strcpy(novo_usuario[0].senha , "1254");
	novo_usuario[0].habilitado = 1;

	strcpy(novo_usuario[1].nome ,"HUMBERTO");
	strcpy(novo_usuario[1].senha , "2349");
	novo_usuario[1].habilitado = 1;

	strcpy(novo_usuario[2].nome ,"ADEMASTOR");
	strcpy(novo_usuario[2].senha , "0738");
	novo_usuario[2].habilitado = 1;


	unsigned char tecla=0;
	char buffer_lcd[17]; // 16 caracteres + '\0'
	unsigned char index = 0;
	//unsigned char usuario_autenticado;
	unsigned char modo_edicao = 0;
	static int ultimo_seg = -1;
	// junto com as outras variáveis, antes do while
	unsigned char menu_estado = MENU_PRINCIPAL;
	unsigned char menu_desenhado = 0;
    unsigned char num_cartoes = 0;
    char buffer[20];


while (1) {
        energia_gerenciar();
        maquina_login(novo_usuario, &usuario_autenticado);
        //verificar_parcelas();
        if(verificar_parcelas() == 1){
                   
                    menu_estado = MENU_PRINCIPAL;
                    menu_desenhado = 0;					
				}
        if (fora_do_ar_flag == 1) {
            PORTB |= (1 << LED_fora_do_ar);   // Liga o led
        } else {
            PORTB &= ~(1 << LED_fora_do_ar);  // Apaga o led
        }

        if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
            teclado_atualizar();
            tecla = teclado_obter_tecla();

            //func_test(); 
            maquina_protocolo();
            char* msg = protocolo_get_mensagem();

            // Só atualiza o buffer se tiver conteúdo novo
            if(strlen(msg) > 0) {
                if(strcmp(buffer,msg) != 0){
                    memset(buffer, '\0', sizeof(buffer));
                    strcpy(buffer, msg);
                }
            }
            if (usuario_autenticado!=2 && !novo_usuario[usuario_autenticado].habilitado) {
                if (!menu_desenhado) {
                    lcd_limpar();
                    lcd_escrever_string("OPERADOR BLOQ.");
                    menu_desenhado=1;
                    atraso_ms(500);
                    lcd_limpar();
                    sistema_ja_ligado = 1;
                    estado_atual = BLOQUEADO;
                
                }
            }
         //menu conjunto
            if (menu_estado == MENU_PRINCIPAL) {
                if (!menu_desenhado) {
                    lcd_limpar();
                    if (usuario_autenticado == 2) {
                        lcd_escrever_string("1-Vend 2-Est");
                        lcd_posicionar(1, 0);
                        lcd_escrever_string("3-Adm 4-CL 5- VL");
                    } else {
                        lcd_escrever_string("1-Vend 2-Est ");
                        lcd_posicionar(1, 0);
                        lcd_escrever_string("4- CL 5- VL");
                    }
                    menu_desenhado = 1;
                }

                if (tecla == '1') {
                    menu_estado = MENU_VENDAS;
                    serial_escrever("vendas ");
                    menu_desenhado = 0;
                    lcd_limpar();
					tecla = 0; // ← consome a tecla aqui, vendas() receberá 0
                }
                if (tecla == '2') {
                    menu_estado = MENU_ESTORNO;
                    serial_escrever("estorno ");
                    menu_desenhado = 0;
                    lcd_limpar();
                }
                //menu do admin
                if (tecla == '3' && usuario_autenticado == 2) {
                    tecla = 0;
                    menu_estado = MENU_ADMIN;
                    serial_escrever("admin");
                    menu_desenhado = 0;
                    lcd_limpar();
                }

                if (tecla == '4') {
                    num_cartoes = cadastrar_cartao_local(cartao_local, num_cartoes);
                    serial_escrever(cartao_local[num_cartoes-1].numero_cartao);
                    serial_escrever(" ");
                    serial_escrever(cartao_local[num_cartoes-1].senha_cartao);
                    serial_escrever(" saldo");
                    
                    menu_estado = MENU_PRINCIPAL;
                    menu_desenhado = 0;
}

                if(tecla == '5'){

                    venda_local(
                        cartao_local,
                        num_cartoes);
                        menu_estado = MENU_PRINCIPAL;
                }                
            }

            //venda
            if (menu_estado == MENU_VENDAS) {
                menu_desenhado = 1;
                // venda finalizada, volta ao menu
				if(vendas(tecla, buffer) == 1){
                    atraso_ms(1500);
                    menu_estado = MENU_PRINCIPAL;
                    menu_desenhado = 0;					
				}
            }
            

            
            //vai pro estorno
            if (menu_estado == MENU_ESTORNO) {
                 estorno_executar(novo_usuario);
                    menu_estado = MENU_PRINCIPAL;
                    menu_desenhado = 0;
                
            }

            //entra no menu admin
            if (menu_estado == MENU_ADMIN) {
                if (menu_admin(tecla, novo_usuario) == 1) {
                    menu_estado = MENU_PRINCIPAL;
                    menu_desenhado = 0;
                }
            }

            /*if(n_vendas[main_vd].tipo == parcelada)
            {
                if(n_vendas[main_vd].data < mes)
                {

                        verificar_parcelas();  
                }
            }*/

        } else {
            // sistema bloqueado ou desligado: reseta menu
            menu_estado = MENU_PRINCIPAL;
            menu_desenhado = 0;
        }

        flag_1ms = 0;
    }
    return 0;
}

//=========================================================================
/*
usuario_autenticado = 0;
	//enviar_login(usuario_autenticado);
	//enviar_logoff(usuario_autenticado);

//LOOP P/ TESTAR A MAQUINA DE ESTADOS DE MONTAGEM DE FRAMES RECEBIDOS
while(1){
	energia_gerenciar();
	if (maquina_protocolo()==1) {
		serial_escrever(protocolo_get_mensagem());
		lcd_escrever_string(protocolo_get_mensagem());
		

	
	}
	flag_1ms = 0;
}
return 0;
}

*/
/*
while (1) {
    energia_gerenciar();
    maquina_login(novo_usuario, &usuario_autenticado);

    if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if (menu_estado == 0) {
            if (!menu_desenhado) {
                lcd_limpar();
                if (usuario_autenticado == 2) {
                    lcd_escrever_string("1-Vend 2-Est");
                    lcd_posicionar(1, 0);
                    lcd_escrever_string("3-Adm");
                } else {
                    lcd_escrever_string("1-Vend 2-Est");
                }
                menu_desenhado = 1;
            }
            if (tecla == '1') { menu_estado = 1; menu_desenhado = 0; }
            if (tecla == '2') { menu_estado = 2; menu_desenhado = 0; }
            if (tecla == '3' && usuario_autenticado == 2) { menu_estado = 4; menu_desenhado = 0; tecla = 0; }
        }

        if (menu_estado == 4) {
            if (menu_admin(tecla, novo_usuario) == 1) {
                menu_estado = 0;
                menu_desenhado = 0;
            }
        }
    }else {
		menu_desenhado =0;
		menu_estado = 0;
	}

    flag_1ms = 0;
}
return 0;
}
*/

//main com delays e sem menus de admin mostrando
/*	
while (1) {
    energia_gerenciar();
    maquina_login(novo_usuario, &usuario_autenticado);

    if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if (!menu_desenhado) {
            lcd_limpar();
            if (usuario_autenticado == 2) {
                lcd_escrever_string("1-Vend 2-Est");
                lcd_posicionar(1, 0);
                lcd_escrever_string("3-Adm");
            } else {
                lcd_escrever_string("1-Vend 2-Est");
            }
            menu_desenhado = 1;
        }
		if (tecla == '1') { menu_estado = 1; menu_desenhado = 0; }
if (tecla == '2') { menu_estado = 2; menu_desenhado = 0; }
if (tecla == '3' && usuario_autenticado == 2) { 
	menu_estado = 4;
	 menu_desenhado = 0; 
	tecla=0;}

		if (menu_estado == 4) {
    if (menu_admin(tecla, novo_usuario) == 1) {
        menu_estado = 0;
        menu_desenhado = 0;
    }
}

    }

    flag_1ms = 0;
}
return 0;
}
*/


/*
// main q esta totalmente sem delays, funciona o autenticar e le tecla
while (1) {
    energia_gerenciar();
    maquina_login(novo_usuario, &usuario_autenticado);

    if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
        teclado_atualizar();
        tecla = teclado_obter_tecla();
        if (tecla != 0) lcd_caractere(tecla);
    }

    flag_1ms = 0;
}
return 0;
}
*/

//=============================================================================

/*
while (1) {
    energia_gerenciar();
    maquina_login(novo_usuario, &usuario_autenticado);

    if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {

        if (fora_do_ar_flag == 1) PORTB |= (1 << LED_fora_do_ar);
        else                      PORTB &= ~(1 << LED_fora_do_ar);

        if (usuario_autenticado == 2 || novo_usuario[usuario_autenticado].habilitado) {
            if (menu_estado == 0) {
                if (!menu_desenhado) {
                    lcd_limpar();
                    if (usuario_autenticado == 2) {
                        lcd_escrever_string("1-Vend 2-Est");
                        lcd_posicionar(1, 0);
                        lcd_escrever_string("3-Adm");
                    } else {
                        lcd_escrever_string("1-Vend 2-Est");
                    }
                    menu_desenhado = 1;
                }
                teclado_atualizar();
                tecla = teclado_obter_tecla();
                if (tecla == '1') { menu_estado = 1; menu_desenhado = 0; }
                if (tecla == '2') { menu_estado = 2; menu_desenhado = 0; }
                if (tecla == '3' && usuario_autenticado == 2) { menu_estado = 4; menu_desenhado = 0; }
            }
            if (menu_estado == 4) {
                menu_admin(tecla, novo_usuario);
            }
        } else {
            if (!menu_desenhado) {
                lcd_limpar();
                lcd_escrever_string("OPERADOR BLOQ.");
                menu_desenhado = 1;
            }
        }
    }
    flag_1ms = 0;
}
return 0;
}

//=============================================================================

*/
//main que gera o login do operador e entra nas opções do admin
/*	
while (1) {
    energia_gerenciar();
    maquina_login(novo_usuario, &usuario_autenticado);
    if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
    teclado_atualizar();
    tecla = teclado_obter_tecla();

    // Monitoramento contínuo (sempre ativo)
    if (maquina_protocolo() == 1) {
        lcd_limpar();
        lcd_escrever_string(protocolo_get_mensagem());
    }
    if (fora_do_ar_flag == 1) PORTB |= (1 << LED_fora_do_ar);
    else                      PORTB &= ~(1 << LED_fora_do_ar);

    // Bifurcação de perfil
    if (usuario_autenticado == 2) {
        menu_admin(tecla, novo_usuario);
    } else if (novo_usuario[usuario_autenticado].habilitado) {
        // operador habilitado — menu principal virá aqui
        if (tecla != 0) lcd_caractere(tecla); // placeholder temporário
    } else {
        // operador desabilitado
        lcd_posicionar(1, 0);
        lcd_escrever_string("OPERADOR BLOQ.  ");
    }
}
    flag_1ms = 0;
}
return 0;
}
*/

//=============================================================================

/*
//main minima q estava funcionando só com o bug do teclado 3seg
while (1) {
    energia_gerenciar();
    maquina_login(novo_usuario, &usuario_autenticado);
    if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
        tecla = teclado_obter_tecla();
        if (maquina_protocolo() == 1) {
            lcd_limpar();
            lcd_escrever_string(protocolo_get_mensagem());
        }
        if (tecla != 0) lcd_caractere(tecla);
        if (fora_do_ar_flag == 1) PORTB |= (1<<LED_fora_do_ar);
        else PORTB &= ~(1<<LED_fora_do_ar);
    }
    flag_1ms = 0;
}
return 0;
}
*/

//=============================================================================

// MAIN BASICA E FUNCIONAL ATÉ O MOMENTO
//PODEMOS CRIAR AS NOVAS VERSÕES A PARTIR DELA

/*while (1) {
    energia_gerenciar();
    if (energia_sistema_ativo() == LIGADO) {
        teclado_atualizar();

		
		if (sistema_ja_ligado == 0) {
			lcd_limpar();
			lcd_posicionar(0, 2);
			lcd_escrever_string("MicPay 2026");
			lcd_posicionar(1, 1);
			lcd_escrever_string("Sistema Ativo");
			sistema_ja_ligado = 1;
		}
		if (sistema_ja_ligado == 1 && tempo_n_bloqueante(1500)) {
			lcd_limpar();
			lcd_posicionar(0, 0);
			lcd_escrever_string("Bloqueado:");
			sistema_ja_ligado = 2;
			estado_atual = BLOQUEADO;
		}
		
		



        if (estado_atual == BLOQUEADO && sistema_ja_ligado ==2  ) {
            unsigned char usuario_autenticado;
            mascara_autentica_senha(novo_usuario, 4, &usuario_autenticado);
			inicia = 1;
        }
        if (estado_atual == DESBLOQUEADO) {
			vendas(inicia); 
			inicia = 0;
            tecla = teclado_obter_tecla();
            if (tecla != 0) {
                lcd_caractere(tecla);
            }
        }
    }
	flag_1ms=0;
}


return 0;


}*/









/*while (1) {
    energia_gerenciar();
    if (energia_sistema_ativo() == LIGADO) {
        teclado_atualizar();

		
		if (sistema_ja_ligado == 0) {
			lcd_limpar();
			lcd_posicionar(0, 2);
			lcd_escrever_string("MicPay 2026");
			lcd_posicionar(1, 1);
			lcd_escrever_string("Sistema Ativo");
			sistema_ja_ligado = 1;
		}
		if (sistema_ja_ligado == 1 && tempo_n_bloqueante(1500)) {
			lcd_limpar();
			lcd_posicionar(0, 0);
			lcd_escrever_string("Bloqueado:");
			sistema_ja_ligado = 2;
			estado_atual = BLOQUEADO;
		}
	
        if (estado_atual == BLOQUEADO && sistema_ja_ligado ==2  ) {
            mascara_autentica_senha(novo_usuario, 4, &usuario_autenticado);
			if (estado_atual == DESBLOQUEADO) {
				lcd_limpar();
        		lcd_posicionar(0, 0);
				lcd_escrever_string("bem vindo:");
				lcd_posicionar(1, 0);
       	 		lcd_escrever_string(novo_usuario[usuario_autenticado].nome);
				sistema_ja_ligado =3;
			}
        }
		if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 3 && tempo_n_bloqueante(1500)) {
   			 lcd_limpar();
   			 sistema_ja_ligado = 4;
}
        if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
            tecla = teclado_obter_tecla();
            if (tecla != 0) {
                lcd_caractere(tecla);
            }
        }
    }
	flag_1ms=0;
}


return 0;
}*/

//=============================================================================

/*
//main para testar a função de vendas
// força estado desbloqueado para testar sem autenticação
    estado_atual = DESBLOQUEADO;

    unsigned char inicio = 1;

    while (1) {
        teclado_atualizar();

        if (vendas(inicio)) {
            inicio = 1; // reseta para nova venda
            lcd_limpar();
            lcd_posicionar(0, 0);
            lcd_escrever_string("Venda OK!");
        } else {
            inicio = 0; // não redesenha tela a cada ciclo
        }

        flag_1ms = 0;
    }
    return 0;
}
*/


//=============================================================================


/* 
usuario_autenticado = 0;
	
	//enviar_login(usuario_autenticado);
	//enviar_logoff(usuario_autenticado);

LOOP P/ TESTAR A MAQUINA DE ESTADOS DE MONTAGEM DE FRAMES RECEBIDOS
while(1){
	energia_gerenciar();
	if (maquina_protocolo()==1) {
		serial_escrever(protocolo_get_mensagem());
		lcd_escrever_string(protocolo_get_mensagem());
		

	
	}
	flag_1ms = 0;
}
return 0;
}
*/

/*
while(1){
    if(serial_disponivel()){
        char c = serial_ler();
        serial_transmitir(c); // ecoa de volta o que recebeu
    }
}
return 0;
}*/


//LOOP PARA TESTAR FUNÇÃO DE SETAR HORA
//AO LIGAR ELA COMEÇA A CONTAR AS HORAS DE 00:00:00
// O LOOP APÓS LIGAR COM d POR 4S E PRESSIONAR "A"
// A FUNÇÃO DE ALTERAR HORA -> ESPERA UM NUMERO PARA HORA E CONFIRMA COM #
//DAÍ VAI PRA ALTERAR MINUTO -> ESPERA UM NUMERO E CONFIRMA COM #
// EM A,BAS CASO PRESSIONE UM NUMERO DE DUAS CASAS ELE CONFIRMA AUTOMATICO
//SE PRESSIONAR UMA HORA OU MINUTO NAO VALIDO TIPO 90, ELA RESETA O LOOP
/*	
while (1) {
    energia_gerenciar();
    if (energia_sistema_ativo() == LIGADO) {
        teclado_atualizar();
        tecla = teclado_obter_tecla();

        if (tecla == 'A') {
            modo_edicao = 1;
            tecla = 0;
        }

        if (modo_edicao == 1) {
    if (alterar_hora(tecla)) {
        modo_edicao = 2;
        tecla = 0; // zera aqui
    }
}
if (modo_edicao == 2 && tecla != '#') { // só processa minuto se não for '#'
    if (alterar_minuto(tecla)) modo_edicao = 0;
}

        if (seg != ultimo_seg) {
            ultimo_seg = seg;
            lcd_posicionar(0, 0);
            mostrar_hora();
        }
    }
    flag_1ms = 0;
}
    return 0;
}
*/

/*
//MAIN TESTADA E FUNCIONAL DIA 14/05 às 2:14

while (1) {
    energia_gerenciar();
    if (energia_sistema_ativo() == LIGADO) {
        teclado_atualizar();

		
		if (sistema_ja_ligado == 0) {
			lcd_limpar();
			lcd_posicionar(0, 2);
			lcd_escrever_string("MicPay 2026");
			lcd_posicionar(1, 1);
			lcd_escrever_string("Sistema Ativo");
			sistema_ja_ligado = 1;
		}
		if (sistema_ja_ligado == 1 && tempo_n_bloqueante(1500)) {
			lcd_limpar();
			lcd_posicionar(0, 0);
			lcd_escrever_string("Bloqueado:");
			sistema_ja_ligado = 2;
			estado_atual = BLOQUEADO;
		}
	
        if (estado_atual == BLOQUEADO && sistema_ja_ligado ==2  ) {
            mascara_autentica_senha(novo_usuario, 4, &usuario_autenticado);
			if (estado_atual == DESBLOQUEADO) {
				lcd_limpar();
        		lcd_posicionar(0, 0);
				lcd_escrever_string("bem vindo:");
				lcd_posicionar(1, 0);
       	 		lcd_escrever_string(novo_usuario[usuario_autenticado].nome);
				sistema_ja_ligado =3;
			}
		if (estado_atual == ERRO) {
			lcd_limpar();
			lcd_escrever_string("SENHA INVALIDA");
			estado_atual = BLOQUEADO;
			sistema_ja_ligado = 1;
    }
        }

		if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 3 && tempo_n_bloqueante(1500)) {
   			 lcd_limpar();
			 enviar_login(usuario_autenticado);
   			 sistema_ja_ligado = 4;
}
// até esse bloco aqui ele só liga mostra a telinha inicial e autentica usuário
//as flags sistema ja ligado são só pra fazer um passo a passo
        if (estado_atual == DESBLOQUEADO && sistema_ja_ligado == 4) {
			
            tecla = teclado_obter_tecla();
			if(maquina_protocolo()== 1){
				char *msg = protocolo_get_mensagem();
				lcd_limpar();
				lcd_escrever_string(msg);
			}
			//a partir daqui da para adicionar coisas novas para testar!!
            if (tecla != 0) {
                lcd_caractere(tecla);// aqui lcd esta mostrando a tecla que foi lida
				//vendas(1);
            }

			if (fora_do_ar_flag ==1) {
				PORTB |= (1<<LED_fora_do_ar);
			
			}else {
				PORTB &= ~(1<< LED_fora_do_ar);
			}
        }
    }
	flag_1ms=0;
}
return 0;
}



*/
