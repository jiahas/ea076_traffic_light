#include <Arduino.h>

// Definição LDR
#define LDR A0

// Definição dos Leds para os carros
#define C_VERMELHO A1
#define C_AMARELO A2
#define C_VERDE A3

// Definição dos Leds para os pedestres
#define P_VERMELHO A4
#define P_VERDE A5

// Definição botão dos pedestres
#define BOTAO 6

// Definição do decodificador de 7 segmentos CD4511
#define ENTRADA_2 9
#define ENTRADA_3 10
#define ENTRADA_4 11
#define ENTRADA_1 8

// Definição dos displays de 7 segmentos
#define COM_CARROS 5
#define COM_PEDESTRES 4

// Variaveis Globais
int contador = 9;



// put function declarations here:


void setup() {
// Pinos de saida
  pinMode(C_VERDE, OUTPUT);
  pinMode(C_AMARELO, OUTPUT);
  pinMode(C_VERMELHO, OUTPUT);
  pinMode(P_VERDE, OUTPUT);
  pinMode(P_VERMELHO, OUTPUT);

// Pinos de entrada
  pinMode(BOTAO, INPUT);
  pinMode(LDR, INPUT);

// Registrador CD4511
  DDRB = B00001111;
  DDRD = B01110000;
  PORTB = 0x00;
  PORTD = 0x00;

// Início
  digitalWrite(C_VERDE, HIGH);
  digitalWrite(P_VERMELHO, HIGH);

// Interrupções
cli(); // desabilita as interrupções
configuracao_Timer0(); // configura o temporizador
sei(); // habilita as interrupções
}

void loop() {
  
  if(digitalRead(BOTAO)){
    //interrupção 100ms
    digitalWrite(C_VERDE, LOW);
    digitalWrite(C_AMARELO, HIGH);
    //Delay de 4s

    digitalWrite(C_AMARELO, LOW);
    digitalWrite(C_VERMELHO, HIGH);
    digitalWrite(P_VERDE, HIGH);
    
    //Display carros
    while (contador <= 0){
      PORTB = contador << 2;
      //Espera um segundo (delay?)
      contador--;
      if(contador <= 5){
        PORTD = 0x08;
        //Delay 1s
        PORTD = 0x00;
        //Delay de 1s
      }
    }
    contador = 9;
    


  }
}


//função timer

void configuracao_Timer0(){
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Configuracao Temporizador 0 (8 bits) para gerar interrupcoes periodicas a cada x ms 
  // no modo Clear Timer on Compare Match (CTC)
  // Frequência = 16e6 Hz
  // Prescaler = escolha um valor entre 1, 8, 64, 256 e 1024
  // Faixa = número de passos em OCR0A (contagem será de 0 até Faixa - 1)
  // Intervalo entre interrupcoes: (Prescaler/Frequência)*Faixa 
  
  // OCR0A – Output Compare Register A
  OCR0A = valor da Faixa;

  // TCCR0A – Timer/Counter Control Register A
  // COM0A1 COM0A0 COM0B1 COM0B0 – – WGM01 WGM00
  // 0      0      0      0          1     0
  TCCR0A = 0x02;

  // TCCR0B – Timer/Counter Control Register B
  // FOC0A FOC0B – – WGM02 CS02 CS01 CS0
  // 0     0         0     *    *    *    ==> escolher valores de acordo com prescaler
  TCCR0B = 0xNN;

  // TIMSK0 – Timer/Counter Interrupt Mask Register
  // – – – – – OCIE0B OCIE0A TOIE0
  // – – – – – 0      1      0
  TIMSK0 = 0x02;
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

// Rotina de servico de interrupcao do temporizador
ISR(TIMER0_COMPA_vect){
  // Insira aqui o código da rotina de serviço de interrupção disparada pelo temporizador
}

