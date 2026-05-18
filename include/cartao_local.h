#ifndef CARTAO_LOCAL_H
#define CARTAO_LOCAL_H

#include "senhas.h"

unsigned char cadastrar_cartao_local(cartao_local1 *cartoes, unsigned char num_cartoes);

unsigned char venda_local(cartao_local1 *cartoes, unsigned char num_cartoes);
#endif