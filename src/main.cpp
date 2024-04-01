#include <Arduino.h>

// Definição LDR
#define LDR A0

// Definição dos Leds para os carros
#define C_VERMELHO 2
#define C_AMARELO 4
#define C_VERDE 8

// Definição dos Leds para os pedestres
#define P_VERMELHO 16
#define P_VERDE 32

// Definição botão dos pedestres
#define BOTAO 6

// Definição dos displays de 7 segmentos
#define COM_CARROS 5
#define COM_PEDESTRES 4

// Variaveis Globais
int cont = 0;
int state = 0;
int cont_disp_c = 0;
int cont_disp_p = 0;

// put function declarations here:


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
  OCR0A = 124;

  // TCCR0A – Timer/Counter Control Register A
  // COM0A1 COM0A0 COM0B1 COM0B0 – – WGM01 WGM00
  // 0      0      0      0          1     0
  TCCR0A = 0x02;

  // TCCR0B – Timer/Counter Control Register B
  // FOC0A FOC0B – – WGM02 CS02 CS01 CS0
  // 0     0         0     *    *    *    ==> escolher valores de acordo com prescaler
  TCCR0B = 0x05;

  // TIMSK0 – Timer/Counter Interrupt Mask Register
  // – – – – – OCIE0B OCIE0A TOIE0
  // – – – – – 0      1      0
  TIMSK0 = 0x02;
  
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////
}

// Rotina de servico de interrupcao do temporizador
ISR(TIMER0_COMPA_vect){
  // Insira aqui o código da rotina de serviço de interrupção disparada pelo temporizador
  cont++;
  cont_disp_c--;
  if (cont_disp_p >= 0){
    cont_disp_p--;
  }
}


void setup() {
// Pinos de saida
// Configurando port C como saída para controle dos leds
  DDRC = (C_VERMELHO|C_VERDE|C_AMARELO|
         P_VERMELHO|P_VERDE);
// Definindo estado inicial com led vermelho dos pedestres desligado
// e led verde dos veiculos ligado
  PORTC = (C_VERDE|P_VERMELHO);

// Pinos de entrada
  pinMode(BOTAO, INPUT);
  pinMode(LDR, INPUT);

// Configurando port D como controle do catodo comum do disp de 7 seg  
  DDRD = 0b00110000;  // define os pinos 4 e 5 do Arduino como saídas, outros pinos como entrada
  PORTD = 0b0110000; // Determina estado inicial dos pinos (porta D) com nível lógico baixo

// Configurando port B como saída do display de 7 seg
  DDRB = 0b00001111; // define os pinos 8 a 12 do Arduino como saídas, outros pinos como entrada
  PORTB = 0b0000000; // Determina estado inicial dos pinos (porta B) com nível lógico baixo
  

  // Interrupções
  cli(); // desabilita as interrupções
  configuracao_Timer0(); // configura o temporizador
  sei(); // habilita as interrupções
  Serial.begin(9600);

}

void loop() {

  if(digitalRead(BOTAO) && state == 0){
    Serial.print("Pedestre apertou botao\n");
    state = 1;
    cont = 0;
  }
         
  if(cont >= 13 && state==1){
    Serial.print("Farol amarelo aceso\n");
    state = 2;
    PORTC = (C_AMARELO|P_VERMELHO);
    cont = 0;
   }
  
  if(cont >= 500 && state==2){
    Serial.print("Passagem de pedestre\n");
    state = 3;
    // Inicio da contagem do displar
    // Carros em nove
    // Pedestre em cinco
    PORTC = (C_VERMELHO|P_VERDE);
    cont_disp_c = 1200;
    cont_disp_p = 700;
    cont = 0;
  }
  
  if(state == 3){

    if(cont >= 1125){
      // Contagem do pedestre
      Serial.print("Cruzamento aberto\n");
      PORTC = (C_VERDE|P_VERMELHO);
      state = 0;
    }
    // Seta display
    // Começa o decremento
    PORTB = int(8*cont_disp_c/1000); // 8ms * 1150 = 9.60 s
    PORTD &= ~(1<<COM_CARROS); // Ativa display carros
    PORTD |= (1<<COM_CARROS); // Trava o dígito no display carros
    
	  PORTB = int(8*cont_disp_p/1000); // 8ms * 650 = 5.60 s
    
    if(cont_disp_p >= 63){ // Verifica se a contage esta em valor limiar de 0.5s

      PORTD &= ~(1<<COM_PEDESTRES); // Ativa display pedestre
      PORTD |= (1<<COM_PEDESTRES); // Trava o dígito no display pedestre
      
    } else if(cont_disp_p <= 0){  // Quando a contagem do pedestre zerar
      cont_disp_p += 126; 		  // Incrementa mais 1s para reiniciar o ciclo de blink de 0.5s ligado
      							  // e 0.5s desligado
    }

    
  }


}

