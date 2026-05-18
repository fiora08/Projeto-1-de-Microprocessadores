#include <string.h>
#include "estorno.h"
#include "lcd.h"
#include "teclado.h"
#include "timers.h"
#include "protocolo.h"
#include "vendas.h"
#include "senhas.h"
#include "serial.h"
#include "maquina.h"
#include "estorno.h"
#include "energia.h"
#define OPERADOR_NULO 255


static void limpar_buffer_teclado() {
	for(int i=0; i<15; i++) {
		teclado_atualizar();
		teclado_obter_tecla();
	}
}


static void limpar_buffer_serial() {
	while (serial_disponivel()) {
		serial_ler();
	}
}

static unsigned char esperar_byte(char esperado) {
	while (!serial_disponivel());
	return (serial_ler() == esperado) ? 1 : 0;
}

static void descartar_bytes(unsigned char n) {
	for (unsigned char i = 0; i < n; i++) {
		while (!serial_disponivel());
		serial_ler();
	}
}

unsigned char estorno_executar(usuario *lista_usuarios) {

	if (estado_atual != DESBLOQUEADO) return 0;

	unsigned char tecla     = 0;
	int           idx_venda = -1;
	char          cartao_input[7];
	unsigned char indice_op = OPERADOR_NULO;
	
	limpar_buffer_teclado();
	
	lcd_limpar();
	lcd_escrever_string("SENHA OPERADOR:");
	lcd_posicionar(1, 0);

	while (1) {
		energia_gerenciar();
		teclado_atualizar();
		unsigned char resultado = mascara_autentica_senha(lista_usuarios, 4, &indice_op);

		if (indice_op != OPERADOR_NULO) {
			
			while (1) {
				energia_gerenciar();
				teclado_atualizar();
				if (teclado_obter_tecla() == 'D') break;
			}

			limpar_buffer_serial();
			
			if (enviar_login(indice_op) == 1) {
				if (!esperar_byte('S')) {
					lcd_limpar(); lcd_escrever_string("ERRO SERVIDOR"); atraso_ms(1500);
					indice_op = OPERADOR_NULO;
					limpar_buffer_teclado();
					lcd_limpar(); lcd_escrever_string("SENHA OPERADOR:"); lcd_posicionar(1, 0);
					continue;
				}
				if (!esperar_byte('L')) {
					lcd_limpar(); lcd_escrever_string("ERRO SERVIDOR"); atraso_ms(1500);
					
					indice_op = OPERADOR_NULO;
					limpar_buffer_teclado();
					
					lcd_limpar(); lcd_escrever_string("SENHA OPERADOR:"); lcd_posicionar(1, 0);
					continue;
				}

				while (!serial_disponivel());
				unsigned char n_nome = serial_ler();
				descartar_bytes(n_nome);
				break;
				
				} else {
				lcd_limpar(); lcd_escrever_string("FALHA COMUNICA."); atraso_ms(1500);
				indice_op = OPERADOR_NULO;
				limpar_buffer_teclado();
				lcd_limpar(); lcd_escrever_string("SENHA OPERADOR:"); lcd_posicionar(1, 0);
			}

			} else if (resultado == BLOQUEADO) {
			limpar_buffer_teclado();
			lcd_limpar();
			lcd_escrever_string("SENHA INVALIDA");
			atraso_ms(1500);
			estado_atual = DESBLOQUEADO;
			lcd_limpar();
			lcd_escrever_string("SENHA OPERADOR:");
			lcd_posicionar(1, 0);
		}
	}


	while (1) {
		unsigned int cod_lido = 0;

		lcd_limpar();
		lcd_escrever_string("COD. VENDA:");
		lcd_posicionar(1, 0);

		while (1) {
			energia_gerenciar();
			teclado_atualizar();
			tecla = teclado_obter_tecla();
			if (tecla == 'D') break;
			if (tecla >= '0' && tecla <= '9') {
				lcd_caractere(tecla);
				cod_lido = (cod_lido * 10) + (tecla - '0');
			}
		}

		idx_venda = -1;
		for (int i = 0; i < 5; i++) {
			if (n_vendas[i].codigo == cod_lido) {
				idx_venda = i;
				break;
			}
		}

		if (idx_venda != -1) break;

		lcd_limpar();
		lcd_escrever_string("NAO ENCONTRADA");
		atraso_ms(1500);
		limpar_buffer_teclado();
	}

	
	while (1) {
		lcd_limpar();
		lcd_escrever_string("1-DIGIT 2-INSER");
		unsigned char opcao = 0;

		while (1) {
			energia_gerenciar();
			teclado_atualizar();
			tecla = teclado_obter_tecla();
			if (tecla == '1' || tecla == '2') { opcao = tecla; break; }
		}

		if (opcao == '1') {
			unsigned char p = 0;
			lcd_limpar();
			lcd_escrever_string("NUM. CARTAO:");
			lcd_posicionar(1, 0);

			while (p < 6) {
				energia_gerenciar();
				teclado_atualizar();
				tecla = teclado_obter_tecla();
				if (tecla >= '0' && tecla <= '9') {
					cartao_input[p++] = tecla;
					lcd_caractere(tecla);
				}
			}
			cartao_input[6] = '\0';
			
			

			while (1) {
				energia_gerenciar();
				teclado_atualizar();
				if (teclado_obter_tecla() == 'D') break;
			}

			} else {
			lcd_limpar();
			lcd_escrever_string("AGUARDANDO PC..");
			
			limpar_buffer_serial();

			while (1) {
				if (!esperar_byte('S')) continue;
				if (!esperar_byte('M')) continue;
				break;
			}

			while (!serial_disponivel()); serial_ler();
			while (!serial_disponivel()); serial_ler();
			
			for (int i = 0; i < 6; i++) {
				while (!serial_disponivel());
				cartao_input[i] = serial_ler();
			}
			cartao_input[6] = '\0';

			
			serial_transmitir('M');
			serial_transmitir('M');
		}

		if (strcmp(cartao_input, (char *)n_vendas[idx_venda].num_cartao) == 0) {
			break;
		}

		lcd_limpar();
		lcd_escrever_string("DADOS INCORRETOS");
		atraso_ms(1500);
		limpar_buffer_teclado();
	}

	lcd_limpar();
	lcd_escrever_string("VALOR: ");
	lcd_escrever_string((char *)n_vendas[idx_venda].valor_venda);
	lcd_posicionar(1, 0);
	lcd_escrever_string("D P/ CONFIRMAR");

	while (1) {
		energia_gerenciar();
		teclado_atualizar();
		if (teclado_obter_tecla() == 'D') break;
	}

	
	limpar_buffer_serial();

	char ack = (char)enviar_estorno(
	n_vendas[idx_venda].bandeira,
	n_vendas[idx_venda].num_cartao,
	(char *)n_vendas[idx_venda].valor_venda
	);

	if (ack == 1) {
		while (!serial_disponivel()); char b1 = serial_ler();
		while (!serial_disponivel()); char b2 = serial_ler();
		while (!serial_disponivel()); char status = serial_ler();

		lcd_limpar();
		if (b1 == 'S' && b2 == 'E') {
			if (status == 'V') {
				lcd_escrever_string("ESTORNO OK");
				} else if (status == 'C') {
				lcd_escrever_string("ESTORNO NEGADO");
				} else {
				lcd_escrever_string("RESP. INVALIDA");
			}
			} else {
			lcd_escrever_string("RESP. INVALIDA");
		}
		} else {
		lcd_limpar();
		lcd_escrever_string("FALHA COMUNICA.");
	}

	atraso_ms(3000);
	limpar_buffer_teclado();
	return 1;
}