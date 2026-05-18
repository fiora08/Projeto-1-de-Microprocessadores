// função estorno - a função dela é gerenciar toda a função estorno de forma local. 

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
#define OPERADOR_NULO 255 //  nenhum operador autenticado ainda 

// faz o descart de 15 leituras do teclado para limpar , é feito isso no teclado.c , é mais pra redundancia.
static void limpar_buffer_teclado() {
	for(int i=0; i<15; i++) {
		teclado_atualizar();
		teclado_obter_tecla();
	}
}

// faz o descarte de todos os bites usado na rx serial
static void limpar_buffer_serial() {
	while (serial_disponivel()) { // enquanto ouver bites no buffer
		serial_ler(); // le e descarta 
	}
}
// aguarda um byte chegar na serial e compara com o esperado
// retorna 1 se for igual e 0 se for diferente
static unsigned char esperar_byte(char esperado) {
	while (!serial_disponivel());  // ela trava até chegar um bite
	return (serial_ler() == esperado) ? 1 : 0; // faz a compararaçao e reotnra o resultado
}

// descarta bits da serial
// a função dela é pular o campo nome do operador na resposta sl do servidor.
static void descartar_bytes(unsigned char n) {
	for (unsigned char i = 0; i < n; i++) {
		while (!serial_disponivel());
		serial_ler();
	}
}

unsigned char estorno_executar(usuario *lista_usuarios) {

	if (estado_atual != DESBLOQUEADO) return 0; // só executa se o sistema for autenticado

	unsigned char tecla     = 0; // tecla lida 
	int idx_venda = -1; // indice da venda em n_vendas[], -1 é não encontrada
	char cartao_input[7]; // numero do cartão digitado
	unsigned char indice_op = OPERADOR_NULO; // indice do operador 0,1,2
	
	limpar_buffer_teclado(); // garante que não ha teclas antigas no buffer
	
	lcd_limpar();
	lcd_escrever_string("SENHA OPERADOR:");
	lcd_posicionar(1, 0);
    // loop de autenticação fica neste while até senha e login no servidor.
	while (1) {
		energia_gerenciar();  // liga desliga
		if (fora_do_ar_flag == 1) PORTB |=  (1 << LED_fora_do_ar); // acende led sem comunicação
    			else  PORTB &= ~(1 << LED_fora_do_ar); // apaga o led se tiver comunicação
		teclado_atualizar(); // debounce 
		unsigned char resultado = mascara_autentica_senha(lista_usuarios, 4, &indice_op); // é uma função onde inplementa o login com os

		if (indice_op != OPERADOR_NULO) { // senha correta , operador logado ai segue em frente
			// aguarda o operador apertar o D
			while (1) { 
				energia_gerenciar();
				if (fora_do_ar_flag == 1) PORTB |=  (1 << LED_fora_do_ar);
    			else  PORTB &= ~(1 << LED_fora_do_ar);
				teclado_atualizar();
				if (teclado_obter_tecla() == 'D') break; // de confirmado ai sa do 1 loop
			}

			limpar_buffer_serial(); 
			
			if (enviar_login(indice_op) == 1) { // envio para o servidor ai aqui é onde ele espera a resposta se estiver certo o operador.
				if (!esperar_byte('S')) { // espera o primeiro bite do servidor
					lcd_limpar(); lcd_escrever_string("ERRO SERVIDOR"); atraso_ms(1500);
					indice_op = OPERADOR_NULO;
					limpar_buffer_teclado();
					lcd_limpar(); lcd_escrever_string("SENHA OPERADOR:"); lcd_posicionar(1, 0);
					continue;
				}
				if (!esperar_byte('L')) { // espera o segundo bite do servidor
					lcd_limpar(); lcd_escrever_string("ERRO SERVIDOR"); atraso_ms(1500);
					
					indice_op = OPERADOR_NULO; // se der erro ele reseta o operador
					limpar_buffer_teclado();
					
					lcd_limpar(); lcd_escrever_string("SENHA OPERADOR:"); lcd_posicionar(1, 0);
					continue;
					// volta ao inicio da autententicação até dar ok
				}

				while (!serial_disponivel()); // aqui salva o nome do operador 
				unsigned char n_nome = serial_ler();
				descartar_bytes(n_nome);
				break;
				
				} else { // serial enviar envia 3 vezes se der erro em 3 tentativas falha .
				lcd_limpar(); lcd_escrever_string("FALHA COMUNICA."); atraso_ms(1500);
				indice_op = OPERADOR_NULO;
				limpar_buffer_teclado();
				lcd_limpar(); lcd_escrever_string("SENHA OPERADOR:"); lcd_posicionar(1, 0);
			}

			} else if (resultado == BLOQUEADO) {  // ainda faz a proteção do usuario
			limpar_buffer_teclado();
			lcd_limpar();
			lcd_escrever_string("SENHA INVALIDA"); 
			atraso_ms(1500);
			estado_atual = DESBLOQUEADO; // mantem o sistema bloqueado
			lcd_limpar();
			lcd_escrever_string("SENHA OPERADOR:");
			lcd_posicionar(1, 0);
		}
		// se resultado for != bloqueado e indice operador == nulo
		// digitando menos de 4 digitos continua o loop.
	}

	// loop de busca da venda -> fica rodando até achar o digito do codigo da venda
	while (1) {
		unsigned int cod_lido = 0;

		lcd_limpar();
		lcd_escrever_string("COD. VENDA:");
		lcd_posicionar(1, 0);

		while (1) {
			energia_gerenciar();
			if (fora_do_ar_flag == 1) PORTB |=  (1 << LED_fora_do_ar);
    			else  PORTB &= ~(1 << LED_fora_do_ar);
			teclado_atualizar();
			tecla = teclado_obter_tecla(); // salva a tecla digitada 
			if (tecla == 'D') break;
			if (tecla >= '0' && tecla <= '9') {
				lcd_caractere(tecla); // mostra a tecla digitada ni lcd
 				cod_lido = (cod_lido * 10) + (tecla - '0'); // faz um acumulo -> não esta sendo utilizado totalmente. 
			}
		}

		idx_venda = -1; // reseta tudo antes da busca , coloca em menos 1 pois existe a posibilidade do codigo de venda ser "0";
		for (int i = 0; i < 5; i++) {
			if (n_vendas[i].codigo == cod_lido) { // comparando o codigo digitado com o salvo na "VENDAS.C"

				idx_venda = i; // se foi encontrado salva o indice.
				break;
			}
		}

 		if (idx_venda != -1) break; // venda encontrada ai sai do loop
		// codigo não encontrado ai segue aqui e refaz o loop até achar
		lcd_limpar(); 
		lcd_escrever_string("NAO ENCONTRADA");
		atraso_ms(1500);
		limpar_buffer_teclado();
	}

	
	while (1) {
		lcd_limpar();
		lcd_escrever_string("1-DIGIT 2-INSER"); // mostra as opções de colocar o cartão
		unsigned char opcao = 0;

		while (1) {
			energia_gerenciar();
			if (fora_do_ar_flag == 1) PORTB |=  (1 << LED_fora_do_ar);
    			else  PORTB &= ~(1 << LED_fora_do_ar);
			teclado_atualizar();
			tecla = teclado_obter_tecla();
			if (tecla == '1' || tecla == '2') { opcao = tecla; break; }
		} // opções validas 

		if (opcao == '1') {  // se for 1 ele vai digitar os 6 digitos no teclado
			unsigned char p = 0;
			lcd_limpar();
			lcd_escrever_string("NUM. CARTAO:");
			lcd_posicionar(1, 0);

			while (p < 6) {
				energia_gerenciar();
				if (fora_do_ar_flag == 1) PORTB |=  (1 << LED_fora_do_ar);
    			else  PORTB &= ~(1 << LED_fora_do_ar);
				teclado_atualizar();
				tecla = teclado_obter_tecla();
				if (tecla >= '0' && tecla <= '9') { // só aceita digitos evita o A,B e etc..
					cartao_input[p++] = tecla; // salva o digito 
					lcd_caractere(tecla);
				}
			}
			cartao_input[6] = '\0'; // termina a string do cartão.
			
			

			while (1) { // aguarda D para seguir em frente
				energia_gerenciar();
				if (fora_do_ar_flag == 1) PORTB |=  (1 << LED_fora_do_ar);
    			else  PORTB &= ~(1 << LED_fora_do_ar);
				teclado_atualizar();
				if (teclado_obter_tecla() == 'D') break;
			}

			} else {
				// opção == 2 inserir cartão magnetico
			lcd_limpar();
			lcd_escrever_string("AGUARDANDO PC..");
			
			limpar_buffer_serial(); // limpa o buffer da serial
			// aguarda os bits do servidor
			while (1) {
				if (!esperar_byte('S')) continue;
				if (!esperar_byte('M')) continue;
				break;
			}

		
			while (!serial_disponivel()) { energia_gerenciar(); } serial_ler();
			while (!serial_disponivel()) { energia_gerenciar(); } serial_ler();
			// for que faz a verificação do servidor e salva num vetor
			for (int i = 0; i < 6; i++) {
				while (!serial_disponivel()){energia_gerenciar();} // while (!serial_disponivel());
				cartao_input[i] = serial_ler(); // salva os dados da serial no vetor. (0 a 5)
			}
			cartao_input[6] = '\0'; // garante que o ultimo bit do vetor é um terminador

			// confirmação para o servidor que é cartão magnetico
			serial_transmitir('M');
			serial_transmitir('M');
		}
			// faz a comparação do cartão informado com o cartão salvo no VENDAS.C 
		if (strcmp(cartao_input, (char *)n_vendas[idx_venda].num_cartao) == 0) {
			break; // se confere , sai do loop e segue em diante
		}
			// se não der certo ele da esta mensagem e reinicia tudo denovo.
		lcd_limpar();
		lcd_escrever_string("DADOS INCORRETOS");
		atraso_ms(1500);
		limpar_buffer_teclado();
	}
	// aqui é a comparação com o valor da venda
	lcd_limpar();
	lcd_escrever_string("VALOR: ");
	lcd_escrever_string((char *)n_vendas[idx_venda].valor_venda);
	lcd_posicionar(1, 0);
	lcd_escrever_string("D P/ CONFIRMAR");

	while (1) {
		energia_gerenciar();
		if (fora_do_ar_flag == 1) PORTB |=  (1 << LED_fora_do_ar);
    			else  PORTB &= ~(1 << LED_fora_do_ar);
		teclado_atualizar();
		if (teclado_obter_tecla() == 'D') break;
	}

	
	limpar_buffer_serial();
	// função feita em protocolo.c para enviar ja organizado a string do estorno
	char ack = (char)enviar_estorno(
	n_vendas[idx_venda].bandeira,
	(char *)n_vendas[idx_venda].num_cartao, // n_vendas[idx_venda].num_cartao,
	(char *)n_vendas[idx_venda].valor_venda
	);

	if (fora_do_ar_flag == 1) PORTB |= (1 << LED_fora_do_ar); // teste para ligar o pino 50 do sem sinal

	if (ack == 1) { // servidor respondeu dentro das 3 tentativas 
		// aqui é a espera do ok do servidor e a alocação dos bites
		while (!serial_disponivel()) { energia_gerenciar(); }
		char b1 = serial_ler(); // 1 bite alocado
		while (!serial_disponivel()) { energia_gerenciar(); }
		char b2 = serial_ler(); 	// 2 bite alocado
		while (!serial_disponivel()) { energia_gerenciar(); }
		char status = serial_ler(); // bit de se deu certo ou não
		lcd_limpar();
		// aqui verifica a string do servidor
		if (b1 == 'S' && b2 == 'E') { 
			if (status == 'V') { // sucesso
				lcd_escrever_string("ESTORNO OK");
				} else if (status == 'C') { // erro
				lcd_escrever_string("ESTORNO NEGADO");
				} else { // não vale
				lcd_escrever_string("RESP. INVALIDA");
			}
			} else {
			lcd_escrever_string("RESP. INVALIDA");
		}
		} else { // se foi 3 vezes e não recebeu a resposta entra aqui em falha de comunicação
		lcd_limpar(); 
		lcd_escrever_string("FALHA COMUNICA."); 
	}
	// como saiu aqui entra o flag decladaro acima onde liga o led de falha de comunicação
	atraso_ms(3000);
	limpar_buffer_teclado();
	return 1; // estorno concluido sucesso ou falha e volta pro inicio do processo, sai do estorno.c 
}