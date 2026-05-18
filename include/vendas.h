#ifndef VENDAS_H
#define VENDAS_H

#define VENDA_VISTA 1
#define VENDA_PARCELADA 2
#define TIPO 3
#define VALOR 4
#define NUM_PARCELAS 5
#define OPCAO 6
#define BANDEIRA 7
#define NUM_CARTAO 8
#define SEN_CARTAO 9
#define DEBITO 10
#define CREDITO 11
#define ENVIAR 12
#define RESPOSTA 13
#define VENDA_QUITADA 13


typedef struct venda {
    unsigned int tipo_venda;
    unsigned char valor_venda[6];
    unsigned char num_parcelas;
    unsigned int opcao_venda;
    unsigned char bandeira;
    char num_cartao[7];
    unsigned char senha_cartao[7];
    unsigned int codigo;
    unsigned int data;
} venda;

extern venda n_vendas[5];
unsigned char vendas(unsigned char tecla, char buffer[]);
void func_test();
#endif