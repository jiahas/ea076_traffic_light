#include <Arduino.h>

// Definição LDR
#define LDR A0

// Periodo do dia
#define DIA true
#define NOITE false

// Definição dos Leds para os VEICULOS
#define C_VERMELHO 2
#define C_AMARELO 4
#define C_VERDE 8

#define LO_NIBBLE(b) ((b) & 0x0F)

// Definição dos Leds para os pedestres
#define P_VERMELHO 16
#define P_VERDE 32

// Definição botão dos pedestres
#define BOTAO 6

// Definição dos displays de 7 segmentos
#define COM_VEICULOS 5
#define COM_PEDESTRES 4

typedef enum state_tag
{
  ABERTO_PARA_VEICULOS,
  ABERTO_PARA_PEDESTRES,
  SEMAFORO_AMARELO,
  ATUALIZA_DISPLAY_VEICULOS,
  ATUALIZA_DISPLAY_PEDESTRES,

} state_type;

// Variaveis Globais
volatile unsigned int cont = 0;
int cont_ldr = 0;
int state = 0;
int cont_disp_v = 0;
int cont_disp_p = 0;
int ldr = 0;
int ldr_state = 0;
bool periodo = true;
int p_disp;
int v_disp;
bool blink = false;
// função timer

void configuracao_Timer0()
{
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Configuracao Temporizador 0 (8 bits) para gerar interrupcoes periodicas a cada x ms
  // no modo Clear Timer on Compare Match (CTC)
  // Frequência = 16e6 Hz
  // Prescaler = escolha um valor entre 1, 8, 64, 256 e 1024
  // Faixa = número de passos em OCR0A (contagem será de 0 até Faixa - 1)
  // Intervalo entre interrupcoes: (Prescaler/Frequência)*Faixa

  TCCR0A = 0;
  TCCR0B = 0;

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
ISR(TIMER0_COMPA_vect)
{
  // Insira aqui o código da rotina de serviço de interrupção disparada pelo temporizador
  cont++;
  cont_ldr++;
  cont_disp_v--;
  if (cont_disp_p >= 0)
  {
    cont_disp_p--;
  }
}

void setup()
{
  // Pinos de saida
  // Configurando port C como saída para controle dos leds
  DDRC = (C_VERMELHO | C_VERDE | C_AMARELO |
          P_VERMELHO | P_VERDE);
  // Definindo estado inicial com led vermelho dos pedestres desligado
  // e led verde dos veiculos ligado
  PORTC = (C_VERDE | P_VERMELHO);

  // Pinos de entrada
  pinMode(BOTAO, INPUT);
  pinMode(LDR, INPUT);

  // Configurando port D como controle do catodo comum do disp de 7 seg
  DDRD |= 0b00110000;  // define os pinos 4 e 5 do Arduino como saídas, outros pinos como entrada
  PORTD |= 0b00110000; // Determina estado inicial dos pinos (porta D) com nível lógico baixo

  // Configurando port B como saída do display de 7 seg
  DDRB = 0b00001111; // define os pinos 8 a 12 do Arduino como saídas, outros pinos como entrada
  PORTB = 0b0000000; // Determina estado inicial dos pinos (porta B) com nível lógico baixo

  // Interrupções
  cli();                 // desabilita as interrupções
  configuracao_Timer0(); // configura o temporizador
  sei();                 // habilita as interrupções
  Serial.begin(9600);
}

void loop()
{
  // Rotina de medição do LDR e definição do estado DIA e NOITE
  // ldr: armazena o a leitura analógica do pino do LDR
  // ldr_state: armazena o estado de quantas medições do ldr foram feitas
  //            o estado será contado como incremento ou decremento
  //            será inicializada em 0, enquanto for 0 será considerado NOITE
  //            quando ocorrerem 3 incrementos(detecção de luz) seguidos será definido como DIA
  //            da mesma forma, quando ocorrerem 3 decrementos seguidos até 0, será definido como noite
  if (cont_ldr > 126)
  {

    ldr = analogRead(LDR);

    // Caso a leitura do ldr for menor que 100
    //
    if (ldr < 650 && ldr_state < 3)
    {

      ldr_state++;
      if (ldr_state >= 3 && periodo != DIA)
      {
        periodo = DIA;
        PORTC = (C_VERDE | P_VERMELHO);
        ldr_state = 0;
        cont = 0;
        state = 0;
      }
    }
    else if (ldr >= 650 && ldr_state >= 0 && state == 0)
    {

      ldr_state--;
      if (ldr_state <= 0 && periodo != NOITE)
      {

        periodo = NOITE;
        PORTC = (C_AMARELO | P_VERMELHO);
        cont = 0;
        ldr_state = 0;
      }
    }
    cont_ldr = 0;
  }

  if (periodo == DIA)
  {

    switch (state)
    {

    case ABERTO_PARA_VEICULOS:
      if (digitalRead(BOTAO))
      {
        state = SEMAFORO_AMARELO;
        cont = 0;
      }
      break;

    case SEMAFORO_AMARELO:

      if (cont >= 13)
      {
        state = ABERTO_PARA_PEDESTRES;
        PORTC = (C_AMARELO | P_VERMELHO);
        cont = 0;
      }
      break;

    case ABERTO_PARA_PEDESTRES:

      if (cont >= 500)
      {
        // Inicio da contagem do display
        // VEICULOS em nove segundos (1200*8 = 9600ms)
        // Pedestre em cinco segundos (700*8 = 5600ms)
        state = ATUALIZA_DISPLAY_VEICULOS;
        PORTC = (C_VERMELHO | P_VERDE);
        cont_disp_v = 1200;
        cont_disp_p = 700;
        cont = 0;
      }
      break;

    case ATUALIZA_DISPLAY_VEICULOS:
      PORTB = (8*cont_disp_v)/1000;           // 8ms * 1150 = 9.60 s
      PORTD &= ~(1 << COM_VEICULOS); // Ativa display VEICULOS

      if (cont >= 1){
        PORTD |= (1 << COM_VEICULOS); // Trava o dígito no display VEICULOS
        state = ATUALIZA_DISPLAY_PEDESTRES;
        cont = 0;
      }

      if (cont_disp_v <= 0)
      {
        PORTD |= (1 << COM_VEICULOS); // Trava o dígito no display VEICULOS
        PORTC = (C_VERDE | P_VERMELHO);
        state = ABERTO_PARA_VEICULOS;
        break;
      }

      break;

    case ATUALIZA_DISPLAY_PEDESTRES:
      PORTB = (8*cont_disp_p)/1000; // 8ms * 650 = 5.60 s
      if (cont_disp_p >=63){
        PORTD &= ~(1 << COM_PEDESTRES); // Ativa display pedestre
      } else if (cont_disp_p <= 0){
        cont_disp_p += 126;
      }

      if (cont >= 1){
        PORTD |= (1 << COM_PEDESTRES); // Trava o dígito no display pedestre

        state = ATUALIZA_DISPLAY_VEICULOS;
        cont = 0;
      }

      break;
    }
  }
  else if (periodo == NOITE)
  {

    if (cont >= 63 && !blink)
    { // Verifica se a contage esta em valor limiar de 0.5s
      PORTC &= ~(C_AMARELO);
      Serial.println("apaga amrelo");
      cont = 0;
      blink = blink;
    }
    else if (cont >= 63 && blink)
    { // Quando a contagem do pedestre zerar
      PORTC |= (C_AMARELO);
      Serial.println("acende amarelo");
      cont = 0;
      blink = !blink; // Incrementa mais 1s para reiniciar o ciclo de blink de 0.5s ligado
                 // e 0.5s desligado
    }
  }
}