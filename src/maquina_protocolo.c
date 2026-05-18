#include "maquina_protocolo.h"
#include "serial.h"
#include <string.h>

static unsigned char estado_protocolo = IDLE;
static unsigned char tipo_mensagem = 0;
static unsigned char n;
static unsigned char contador_payload = 0;
static char mensagem[16];
unsigned char maquina_protocolo(){

    if(serial_disponivel()==1){
        tipo_mensagem = serial_ler();
        switch(estado_protocolo){
            
            case IDLE:
                if(tipo_mensagem == 'S'){
                    estado_protocolo =LENDO_TIPO;
                }

                break;
            case LENDO_TIPO:
                switch(tipo_mensagem){
                    case 'L':
                        strcpy(mensagem, "SL");
                        estado_protocolo = LENDO_N;
                        break;

                    case 'M':
                        //lendo cartao magnetico
                        strcpy(mensagem, "SM");
                        estado_protocolo = LENDO_N;
                        break;

                    case 'W':
                        //lendo cartao wireless
                        strcpy(mensagem, "SW");
                        estado_protocolo = LENDO_N;
                        break;
                    
                    case 'O':
                        //login ok, mensagem ja fixa
                        strcpy(mensagem, "SO"); 
                        estado_protocolo = IDLE;
                        return 1;
                        break;

                    case 'X':
                        //falha de login
                        strcpy(mensagem, "SX");
                        estado_protocolo = IDLE;
                        return 1;
                        break;

                    case 'V':
                        strcpy(mensagem, "SV");
                        //feita venda
                        estado_protocolo = LENDO_DADOS_FIXOS;
                        break;

                    case 'E':
                        strcpy(mensagem, "SE");
                        //feito estorno
                        estado_protocolo = LENDO_DADOS_FIXOS;
                        break;
                    
                    case 'P':
                        //feita parcelada
                        strcpy(mensagem, "SP");
                        estado_protocolo = LENDO_DADOS_FIXOS;
                        break;

                    case 'A':
                        
                        //feito agendamento
                        strcpy(mensagem, "SA");
                        estado_protocolo = LENDO_DADOS_FIXOS;
                        break;
                    
                }
                break;
            case LENDO_N:
                n = tipo_mensagem;
                //char mensagem[n+3];
                mensagem[2] = n;
                contador_payload=0;// reseta para nao dar erro de indice na primeira vez

                //trava de segurança para caso o pacote mande exemplo"SL0", dizendo que n tem payload
                if (n > 0) {
                    estado_protocolo = LENDO_PAYLOAD;
                } else {
                    mensagem[3] = '\0'; // Fecha a string vazia
                    estado_protocolo = IDLE;
                    return 1;
                }
                
                break;
            case LENDO_PAYLOAD:

                if (contador_payload < 13) {// esse if faz o controle para nao escrever fora do tamanho da mensagem
                    mensagem[3+contador_payload] = tipo_mensagem;
                    
                }
                contador_payload++;

                if (contador_payload>=n) {
                    if (contador_payload < 13) {
                        mensagem[3+contador_payload] = '\0';
                    }else {
                        mensagem[15]= '\0'; // no pior dos casos se acabar nao chegando em 15, coloca \0 no final para conseguir ler 
                    }
                    
                    estado_protocolo =IDLE;
                    return 1; // avisa que a mensagem ta montada

                }
                //mensagem[2]= '0'+n;//seria um jeito de mostrar n na tela mas nao ha necessidade
                break;

            case LENDO_DADOS_FIXOS:
                //tipo_mensagem = serial_ler();
                switch(tipo_mensagem){
                    case 'V':
                        mensagem[2] = 'V';
                        mensagem[3] = '\0';
                        estado_protocolo =IDLE;
                        return 1;
                        //venda com sucesso 
                        break;
                    case 'C': 
                        mensagem[2] = 'C';
                        mensagem[3] = '\0';
                        estado_protocolo =IDLE;
                        return 1;
                        //cartao com falha
                        break;
                    case 'P':
                        //
                        mensagem[2] = 'P';
                        mensagem[3] = '\0';
                        estado_protocolo =IDLE;
                        return 1;
                        break;
                    case 'S': 
                        //senha com falha
                        mensagem[2] = 'S';
                        mensagem[3] = '\0';
                        estado_protocolo =IDLE;
                        return 1;
                        break;
                    case 'I':
                        //saldo insufuciente
                        mensagem[2] = 'I';
                        mensagem[3] = '\0';
                        estado_protocolo =IDLE;
                        return 1;
                        break;
                    case 'N':
                        //pagamento nao localizado
                        mensagem[2] = 'N';
                        mensagem[3] = '\0';
                        estado_protocolo =IDLE;
                        return 1;
                        break;
                }  
                estado_protocolo = IDLE;
                break;

        }
    }
    
return 0;

}

char* protocolo_get_mensagem(void){
    return mensagem;
}