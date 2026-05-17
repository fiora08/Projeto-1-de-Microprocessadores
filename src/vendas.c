#ifndef F_CPU
#define F_CPU 16000000UL
#endif
#include <avr/io.h>
#include <avr/interrupt.h>
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

//  Variáveis internas
static unsigned char ind = 0;
static unsigned char start = 1;
static unsigned char ind_vds = 0;
static unsigned char ind_val = 0;
static unsigned char ind_num = 0;
static unsigned char ind_sen = 0;
static unsigned char inc_cod = 0;
static unsigned char cont_D = 0;
static unsigned char qual_venda;
static unsigned char estado_venda = TIPO;
//static  char buffer[20];
venda n_vendas[5];

unsigned char vendas(unsigned char tecla, char buffer[])
{
	
	if(estado_atual == DESBLOQUEADO)
	{
		teclado_atualizar();

		//strcpy(buffer, protocolo_get_mensagem());
		//tecla = teclado_obter_tecla();
		if(start)
		{
			lcd_posicionar(0, 0);
			lcd_escrever_string("1-Avist");
			lcd_posicionar(0, 8);
			lcd_escrever_string("2-Praz");
			lcd_posicionar(1, 0);
			lcd_escrever_string(">>");
			start = 0;
		}
		if(tecla != 0 && ((tecla - '0') <= 2) && estado_venda == TIPO)
		{
			qual_venda = tecla - '0';
			lcd_posicionar(1, 2);
			lcd_caractere(tecla);
		}else if(tecla == 'D' && estado_venda == TIPO)
			{
				if(qual_venda == VENDA_VISTA) n_vendas[ind_vds].codigo = inc_cod++;
				estado_venda = VALOR;
				lcd_limpar();
				//Ler o valor da compra
				lcd_posicionar(0, 0);
				lcd_escrever_string("Valor:");
			}

		switch(qual_venda)
		{
			case VENDA_VISTA:
				// Vai salvar o tipo da venda em um dos indices do struct vendas
				n_vendas[ind_vds].tipo_venda = VENDA_VISTA;
				
				if(tecla != 0 && estado_venda == VALOR)
				{				
					if(ind_val < 5 && tecla != 'D')
					{	
						n_vendas[ind_vds].valor_venda[ind_val] = tecla;
						//Imprimir valor na tela
						lcd_posicionar(0, (6+ind_val));
						lcd_caractere(tecla);
						ind_val++;
					} else if (tecla == 'D' && n_vendas[ind_vds].valor_venda[0] !=0) //n_vendas[ind_vds].valor_venda[0] !=0
						{
							// Caso o CONFIRMA - tecla D - for apertado sai do estado VALOR e finaliza o vetor de venda
							estado_venda = OPCAO; 
							n_vendas[ind_vds].valor_venda[ind_val] = '\0';
							ind_val = 0;
							lcd_limpar();
							//Ler a opção de compra
							lcd_posicionar(0, 0);
							lcd_escrever_string("1-Deb");
							lcd_posicionar(0, 7);
							lcd_escrever_string("2-Cred");
							lcd_posicionar(1, 0);
							lcd_escrever_string(">>");
						}
				}
				
				break;
			case VENDA_PARCELADA:
				// Vai salvar o tipo da venda em um dos indices do struct vendas
				n_vendas[ind_vds].tipo_venda = VENDA_PARCELADA; 

				if(tecla != 0 && estado_venda == VALOR)
				{
					if(ind_val < 5 && tecla != 'D')
					{
						n_vendas[ind_vds].valor_venda[ind_val] = tecla;
						//Imprimir valor na tela
						lcd_caractere(n_vendas[ind_vds].valor_venda[ind_val]);
						ind_val++;
					} else if (tecla == 'D' && n_vendas[ind_vds].valor_venda[0] != 0)
						{
							// Caso o CONFIRMA - tecla D - for apertado sai do estado VALOR e finaliza o vetor de venda
							estado_venda = NUM_PARCELAS; 
							n_vendas[ind_vds].valor_venda[ind_val] = '\0';
							ind_val = 0;
							lcd_limpar();
							//Ler a opção de compra
							lcd_posicionar(0, 0);
							lcd_escrever_string("Parcelas:");
						}
				}
				
				if(estado_venda == NUM_PARCELAS && tecla != 0 && ((tecla - '0') <= 3))
				{	
					lcd_posicionar(0, 9);
					lcd_caractere(tecla);
					n_vendas[ind_vds].num_parcelas = tecla;
					cont_D = 3;
				}else if(estado_venda == NUM_PARCELAS && tecla == 'D' && cont_D == 3)
					{
						estado_venda = OPCAO;
						lcd_limpar();
						//Ler a opção de compra
						lcd_posicionar(0, 0);
						lcd_escrever_string("Deb(1)");
						lcd_posicionar(0, 7);
						lcd_escrever_string("Cred(2)");
						lcd_posicionar(1, 0);
						lcd_escrever_string(">>");
					}

				break;
			default:
				break;
		}
			//ESTADO OPÇÃO DE VENDA (DEBITO OU CREDITO)
				if(estado_venda == OPCAO && ((tecla-'0') == 1))
				{	
					lcd_posicionar(1, 2);
					lcd_caractere(tecla);
					n_vendas[ind_vds].opcao_venda = DEBITO;
					cont_D = 1;
				}else if(estado_venda == OPCAO && ((tecla-'0') == 2))
					{	
						lcd_posicionar(1, 2);
						lcd_caractere(tecla);
						n_vendas[ind_vds].opcao_venda = CREDITO;
						cont_D = 1;
					}else if(estado_venda == OPCAO && tecla == 'D' && cont_D == 1)
						{
							estado_venda = BANDEIRA;
							lcd_limpar();
							//Ler a opção de compra
							lcd_posicionar(0, 0);
							lcd_escrever_string("Bandeira:");
						}

				if(estado_venda == BANDEIRA && tecla != 'D' && tecla != 0)
				{	
					lcd_posicionar(0, 9);
					lcd_caractere(tecla);
					n_vendas[ind_vds].bandeira = tecla;
					cont_D = 2;
				}else if(estado_venda == BANDEIRA && tecla == 'D' && cont_D == 2)
					{
						estado_venda = NUM_CARTAO;
						lcd_limpar();
						//Ler a opção de compra
						lcd_posicionar(0, 0);
						lcd_escrever_string("N.CARTAO:");
					}
				// NÚMERO DO CARTÃO
				if(estado_venda == NUM_CARTAO && ( tecla != 0 || strlen(buffer) > 1) ) 
				{		
					if(ind_num < 6 && strlen(buffer) == 16)
					{
						n_vendas[ind_vds].num_cartao[ind_num] = buffer[ind_num+4];
						lcd_caractere(n_vendas[ind_vds].num_cartao[ind_num]);	
						ind_num++;	
						if(ind_num == 6) serial_escrever("MW");					
					}else if(ind_num < 6 && strlen(buffer) == 10)
						{
							n_vendas[ind_vds].num_cartao[ind_num] = buffer[ind_num+4];
							lcd_caractere(n_vendas[ind_vds].num_cartao[ind_num]);	
							ind_num++;
							if(ind_num == 6) serial_escrever("MM");							
						}else if (ind_num < 6 && tecla != 'D' && tecla != 0) 
							{
								n_vendas[ind_vds].num_cartao[ind_num] = tecla;
								lcd_caractere(tecla);
								ind_num++;
							}else if (tecla == 'D' &&  ind_num == 6)
								{
									// Caso CONFIRMA - tecla D - for apertado sai do estado VALOR e finaliza o vetor numero do cartão
									estado_venda = SEN_CARTAO; 
									n_vendas[ind_vds].num_cartao[ind_num] = '\0';
									ind_num = 0;
									lcd_limpar();
									//Ler a opção de compra
									lcd_posicionar(0, 0);
									lcd_escrever_string("Senha:");	
								}
				}

				if(estado_venda == SEN_CARTAO && tecla != 0)
				{
					if (ind_sen < 6 && tecla != 'D') {
						n_vendas[ind_vds].senha_cartao[ind_sen] = tecla;
						lcd_caractere('*');
						ind_sen++;
					}else if(ind_sen == 6 && tecla == 'D')
						{
							// Caso o CONFIRMA - tecla D - for apertado sai do estado VALOR e finaliza o vetor numero do cartão
							estado_venda = ENVIAR; 
							n_vendas[ind_vds].senha_cartao[ind_sen] = '\0';
							ind_sen = 0;
							//qual_venda = 0;	
							//ind_vds++;
							//if(ind_vds > 4)	ind_vds = 0;
							lcd_limpar();
							//lcd_posicionar(0, 4);
							//lcd_escrever_string("Compra");
							//lcd_posicionar(1, 2);
							//lcd_escrever_string("Finalizada!");
						}
				}

				if(estado_venda == ENVIAR)
				{
					if(qual_venda == VENDA_VISTA)
					{
						enviar_venda(n_vendas[ind_vds].bandeira, n_vendas[ind_vds].num_cartao, n_vendas[ind_vds].senha_cartao, n_vendas[ind_vds].valor_venda);
					}else if (qual_venda == VENDA_PARCELADA)
						{
							enviar_venda_parcelada(n_vendas[ind_vds].bandeira, n_vendas[ind_vds].num_cartao, n_vendas[ind_vds].senha_cartao, n_vendas[ind_vds].num_parcelas, n_vendas[ind_vds].valor_venda);
						}
					qual_venda = 0;
					ind_vds++;
					if(ind_vds > 4)	ind_vds = 0;
					//memset(buffer, '\0', sizeof(buffer));
					estado_venda = RESPOSTA;
				}

				if(estado_venda == RESPOSTA)
				{
					if(buffer[0] == 'S' && buffer[1] == 'P' && buffer[2] == 'V'||
						buffer[0] == 'S' && buffer[1] == 'V' && buffer[2] == 'V') //strncmp(buffer, "SVV", 3) == 0 || strncmp(buffer, "SPV", 3) == 0
					{
						lcd_limpar();
						lcd_posicionar(0, 0);
        				lcd_escrever_string("Venda realizada");
						lcd_posicionar(1, 2);
        				lcd_escrever_string("com sucesso");
						estado_venda = TIPO;
						return 1;
					}else if(buffer[0] == 'S' && buffer[1] == 'V' && buffer[2] == 'C'|| 
						buffer[0] == 'S' && buffer[1] == 'P' && buffer[2] == 'C')
						{
							lcd_limpar();
        					lcd_posicionar(0, 0);
        					lcd_escrever_string("Cartao com falha");
							lcd_posicionar(1, 3);
        					lcd_escrever_string("(invalida)");	
							estado_venda = TIPO;
							return 1;						
						}else if(buffer[0] == 'S' && buffer[1] == 'V' && buffer[2] == 'S'||
								buffer[0] == 'S' && buffer[1] == 'P' && buffer[2] == 'S')
							{
								lcd_limpar();
        						lcd_posicionar(0, 0);
        						lcd_escrever_string("Senha com falha");
								lcd_posicionar(1, 3);
        						lcd_escrever_string("(invalida)");	
								estado_venda = SEN_CARTAO;
								//limpou_tela = 0;
								atraso_ms(500);
								lcd_limpar();
								lcd_posicionar(0, 0);
									lcd_escrever_string("Senha:");
														
							}else if(buffer[0] == 'S' && buffer[1] == 'V' && buffer[2] == 'I'||
									buffer[0] == 'S' && buffer[1] == 'P' && buffer[2] == 'I')
								{	
									lcd_limpar();
        							lcd_posicionar(0, 4);
        							lcd_escrever_string("Saldo");
									lcd_posicionar(1, 2);
        							lcd_escrever_string("Insuficiente");	
									estado_venda = TIPO; // ESTADO INICIAL
									return 1;						
								}
				}
				return 0;
	}
	return 0;
}
