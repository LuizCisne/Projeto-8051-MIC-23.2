/*
Autor: Luiz Augusto Feiten Cisne

Data da ultima alteracao: 29/11/2023

Descricao: projeto que usa um sensor reflexivo de movimento para tectetar a presenca de algum objeto 
caso nao tenha nem um objeto proximo o LCD exibe Far, caso tenha um objeto proximo sera exibido Near
e caso esse objeto fique mais proximo por mais de 10 segundos o LCD exibe Warning.

Obs: para gravar o codigo na memoria do 8051 foi preciso usar um arduino uno via comunicacao serial ISP,
caso queria apenas simular o funcionamento use o arm Keil.
*/


#include <reg51.h>

#define LCD P0
sbit IR = P3^0;
sbit LED = P1^1;

sbit RS = P2^0;  // Pino RS do LCD
sbit RW = P2^1;  // Pino RW do LCD
sbit E = P2^2;   // Pino E do LCD

// Variaveis para comunicacao serial (UART)
#define BAUD_RATE 9600
#define FREQ_OSC 16000000

volatile bit motionDetected = 0;
unsigned int nearTime = 0;

void timer1_init();
void uart_init();
void lcd_init();
void lcd_cmd(unsigned char command);
void lcd_data(unsigned char dado);
void lcd_string(char *str);
void delay_timer0(unsigned int milliseconds);

void main() {
    timer1_init();
    uart_init();
    lcd_init();

    lcd_cmd(0x80);  // Posiciona o cursor na primeira linha
    lcd_string("Status");

    while (1) {
        if (IR == 0) {  // Sensor ligado (deteccao)
            LED = 0;  // Liga o LED

            if (!motionDetected) {
                nearTime = 0;  // Reseta o contador quando algo e detectado
                lcd_cmd(0xC0);  // Posiciona o cursor na segunda linha
                lcd_string("Near       ");
            }

            LED = 1;  // Desliga o LED

            while (IR == 0) {
                motionDetected = 1;  // Sinaliza que um objeto esta sendo detectado
                delay_timer0(100);  // Atraso para evitar contagem rapida de tempo
                nearTime++;

                if (nearTime >= 4000) {  // Se passarem mais de 10 segundos (100 * 100 ms)
                    lcd_cmd(0xC0);  // Posiciona o cursor na segunda linha
                    lcd_string("Warning");
                }
            }
        }
        else {
            motionDetected = 0;  // Reseta a flag quando nao ha movimento
            nearTime = 0;  // Reseta o contador quando nada e detectado
            lcd_cmd(0xC0);  // Posiciona o cursor na segunda linha
            lcd_string("Far       ");
        }
    }
}

void timer1_init() {
    TMOD |= 0x20;  // Timer 1, modo 2 (8 bits auto reload)
    TH1 = 256 - (FREQ_OSC / (12 * 32 * BAUD_RATE));  // Configuracao do Timer para a taxa de transmissao
    TR1 = 1;  // Inicia o Timer 1
}

void uart_init() {
    TMOD &= 0x0F;  // Limpa os bits relacionados ao Timer 1
    TMOD |= 0x20;  // Timer 1, modo 2 (8 bits auto reload)
    SCON = 0x50;  // Modo 1, habilita a recepcao
    TH1 = 256 - (FREQ_OSC / (12 * 32 * BAUD_RATE));  // Configuracao do Timer para a taxa de transmissao
    TR1 = 1;  // Inicia o Timer 1
    EA = 1;  // Habilita as interrupcoes globais
    ES = 1;  // Habilita as interrupcoes serial
}

void lcd_init() {
    delay_timer0(15);  // Aguarda o tempo de inicializacao do LCD
    lcd_cmd(0x38);  // Inicializacao do LCD em modo 8 bits, 2 linhas, fonte 5x7
    lcd_cmd(0x0C);  // Exibe o cursor como sublinhado
    lcd_cmd(0x06);  // Incrementa o cursor apos a escrita
    lcd_cmd(0x01);  // Limpa o display
}

void lcd_cmd(unsigned char command) {
    RS = 0;       // Seleciona o modo de comando
    RW = 0;       // Define a operacao de escrita
    E = 1;        // Ativa o pino E
    LCD = command; // Envia o comando para o barramento de dados
    delay_timer0(5);  // Aguarda o tempo de execucao do comando
    E = 0;        // Desativa o pino E
}

void lcd_data(unsigned char dado) {
    RS = 1;       // Seleciona o modo de dados
    RW = 0;       // Define a operacao de escrita
    E = 1;        // Ativa o pino E
    LCD = dado;   // Envia os dados para o barramento
    delay_timer0(5);  // Aguarda o tempo de execucao do comando
    E = 0;        // Desativa o pino E
}

void lcd_string(char *str) {
    while (*str) {
        lcd_data(*str);  // Envia cada caractere da string para o LCD
        str++;
    }
}

void delay_timer0(unsigned int milliseconds) {
   
    // Calcular o valor necessario para o Timer 0
    // Formula: Valor = 65536 - (tempo * (Frequancia do cristal) / 12 / 1000)
    unsigned int countValue = 65536 - (milliseconds * (FREQ_OSC / 12) / 1000);

    // Configurar o Timer 0 em modo 1
    TMOD &= 0xF0;  // Limpar as configuracoes existentes
    TMOD |= 0x01;  // Modo 1 (temporizador de 16 bits com recarga automatica)

    // Configurar os registradores de contagem
    TH0 = countValue >> 8;
    TL0 = countValue & 0xFF;

    // Iniciar o Timer 0
    TR0 = 1;

    // Aguardar ate que o Timer 0 atinja o estouro
    while (!TF0);

    // Desligar o Timer 0
    TR0 = 0;
    TF0 = 0;  // Limpar a flag de estouro
}
