#include <Arduino.h>

// Definição da porta LDR
#define LDR A0

// Periodo do dia
#define DIA true
#define NOITE false

// Definição da posição dos Leds para os VEICULOS
#define C_VERMELHO 2
#define C_AMARELO 4
#define C_VERDE 8

// Definição da posição dos Leds para os pedestres
#define P_VERMELHO 16
#define P_VERDE 32

// Definição botão dos pedestres
#define BOTAO 6

// Definição dos displays de 7 segmentos
#define COM_VEICULOS 5
#define COM_PEDESTRES 4

// Enum de estados
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
bool blink = false;

// Função timer do exemplo oferecido
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
  // A interrução irá incrementar/decrementar os contadores padrão,
  // do ldr, do display para veiculos e para pedestres a cada 8ms, ou seja
  // para contar 1s deverá haver 1125 incrementos para a variavel cont.
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

    // Caso a leitura do ldr for menor que 650
    // e quanto não houverem 3 detecções abaixo de 650, irá incrementar o numero de detecções
    // após 3 detecções e caso não seja periodo de DIA, dará se ao início o estado de ABERTO_PARA_VEICULOS
    // e acendendo os faróis correspondentes.
    if (ldr < 650 && ldr_state < 3)
    {

      ldr_state++;
      if (ldr_state >= 3 && periodo != DIA)
      {
        periodo = DIA;
        PORTC = (C_VERDE | P_VERMELHO);
        ldr_state = 0;
        cont = 0;
        state = ABERTO_PARA_VEICULOS;
      }
    }
    // Da mesma forma caso seja detectado acima de 650 e enquanto houver um número de contagens maior que 0,
    // irá decrementar até que se atinja 3 medições acima de 650 ou um número negativo de ldr_state.
    // Além disso só irá mudar para o período noturno caso o estado atual da máquina de estados esteja
    // em ABERTO_PARA_VEICULOS para não interromper a rotina de travessia dos pedestres.
    else if (ldr >= 650 && ldr_state >= 0 && state == ABERTO_PARA_VEICULOS)
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

  // Rotina do período de dia
  // É uma máquina de estados que irá ciclar entre os estados:
  // -ABERTO_PARA_VEICULOS
  // -SEMAFORO_AMARELO
  // -ABETO_PARA_PEDESTRES
  // -ATUALIZA_DISPLAY_VEICULOS
  // -ATUALIZA_DISPLAY_PEDESTRES
  // -ABERTO_PARA_VEICULOS
  if (periodo == DIA)
  {

    switch (state)
    {

      // Se botao dos pedestres for pressionado, irá entrar no próximo estado
      // SEMAFORO_AMARELO e zerar a contagem de cont.
    case ABERTO_PARA_VEICULOS:
      if (digitalRead(BOTAO))
      {
        state = SEMAFORO_AMARELO;
        cont = 0;
      }
      break;

      // Não fará nada até que tenha se passado 13*8 = 104ms
      // Após este tempo irá acender o farol amarelo para os veiculos
      // e manter o vermelho para os pedestres, atualizar para o próximo
      // estado para ABERTO_PARA_PEDESTRES e zerar o cont.
    case SEMAFORO_AMARELO:

      if (cont >= 13)
      {
        state = ABERTO_PARA_PEDESTRES;
        PORTC = (C_AMARELO | P_VERMELHO);
        cont = 0;
      }
      break;

      // Após 500*8=4000ms, isto é o led amarelo irá ficar aceso por 4s
      // e irá se apagar para fechar o farol para os veiculos e abrir para
      // os pedeestres. Logo em seguida irá definir os valores de cont_disp_v
      // para 1200*8=9600ms e cont_disp_p para 700*8=5600ms para iniciar a contagem
      // regressiva no display tanto para pedestre quanto para veiculos.
      // E irá prosseguir para o próximo estado para ATUALIZA_DISPLAY_VEICULOS
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

      // Irá definir o valor na saída do PORTB para visualizar o tempo
      // de aguardo dos veiculos, para podermos amostrar ao mesmo tempo
      // em ambos os displays, veiculos e pedestres, precisamos ativar e
      // desativar ambos sequencialmente causando a ilusãod e que estão
      // ligados simultaneamente. Um if cont >= 1 foi inserido para dar tempo
      // do display de 7seg atingir o brilho máximo, caso contrário será
      // muito rápido e o brilho muito fraco.
      // Além disso será verificado se a contagem de 9s chegou ao fim a
      // partir de cont_disp_v. Quando a condição for satisfeita, irá apagar o display
      // e mudar o estado dos farois para verde para veiculos e vermelho para pedestres
      // retornando ao estado inicial
    case ATUALIZA_DISPLAY_VEICULOS:
      PORTB = (8 * cont_disp_v) / 1000; // 8ms * 1200 = 9.60 s
      PORTD &= ~(1 << COM_VEICULOS);    // Ativa display VEICULOS

      if (cont >= 1)
      {
        PORTD |= (1 << COM_VEICULOS); // Desativa display VEICULOS
        state = ATUALIZA_DISPLAY_PEDESTRES;
        cont = 0;
      }

      // Quando a contagem dos veículos chegar a 5s o led verde irá desligar

      if (cont_disp_v <= 0)
      {
        PORTD |= (1 << COM_VEICULOS); // Trava o dígito no display VEICULOS
        PORTC = (C_VERDE | P_VERMELHO);
        state = ABERTO_PARA_VEICULOS;
        break;
      }
      else if (cont_disp_v <= 625)
      {
        PORTC &= ~(P_VERDE);
      }

      break;

      // Da mesma forma que o ATUALIZA_DISPLAY_VEICULOS, as operações
      // para ligar o display serão as mesmas com o detalhe de que
      // o do pedestre irá piscar por 4s quando chegar a 0.
      // Para isso quando a cont_disp_p for menor que 63*8=504ms
      // ele não irá acender o display e irá incrementar o cont_disp_p
      // para +1008ms quando cont_disp_p zerar.
      // A mesma tratativa para mudança de estado do ATUALIZA_DISPLAY_VEICULOS
      // foi implementada, após 8ms o display dos pedestres irá desligar
      // prosseguindo para o próximo estado ATUALIZA_DISPLAY_VEICULOS
    case ATUALIZA_DISPLAY_PEDESTRES:
      PORTB = (8 * cont_disp_p) / 1000; // 8ms * 650 = 5.60 s
      if (cont_disp_p > 126)            // Enquanto não for o último segundo o funcionamento é normal, de desliga e liga
      {
        PORTD &= ~(1 << COM_PEDESTRES); // Ativa display pedestre
      }
      else
      {

        if (cont_disp_p >= 63) // nos últimos segundos irá entrar na rotina de piscar o display e o led
        {
          PORTD &= ~(1 << COM_PEDESTRES); // Ativa display pedestre
          PORTC |= (P_VERMELHO);         // Ativa o led vermelho
        }
        else if (cont_disp_p <= 0)
        {
          cont_disp_p += 126;
        } else {
          PORTC &= ~(P_VERMELHO); // Desativa LED vermelho
        }
      }

      if (cont >= 1)
      {
        PORTD |= (1 << COM_PEDESTRES); // Trava o dígito no display pedestre

        state = ATUALIZA_DISPLAY_VEICULOS;
        cont = 0;
      }

      break;
    }
  }

  // No periodo de noturno, a cada 504ms o led amarelo irá acender e apagar
  // conforme a variável blink.
  else if (periodo == NOITE)
  {

    if (cont >= 63 && !blink)
    { // Verifica se a contage esta em valor limiar de 0.5s
      PORTC &= ~(C_AMARELO);
      cont = 0;
      blink = !blink;
    }
    else if (cont >= 63 && blink)
    { // Quando a contagem do pedestre zerar
      PORTC |= (C_AMARELO);
      cont = 0;
      blink = !blink; // Incrementa mais 1s para reiniciar o ciclo de blink de 0.5s ligado
                      // e 0.5s desligado
    }
  }
}