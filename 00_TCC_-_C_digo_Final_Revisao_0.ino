/*TCC - MEDIDOR DE CONSUMO DE ENERGIA ELÉTRICA INTELIGENTE */

/*TRABALHO DE CONCLUSÃO DO CURSO DE ENGENHARIA ELÉTRICA - ESTÁCIO */

/*
 * Autores:
 * ARTULINO JÚNIOR
 * RAFAEL SILVA
 */


/* INCLUI TODAS AS BIBLIOTECAS NECESSÁRIAS PARA FUNCIONAMENTO DO DISPOSITIVO */

/* BIBLIOTECA ELABORADA PELA OPEN ENERGY - REALIZA OS CÁLCULOS DE TENSÃO RMS, CORRENTE RMS, POTENCIA REAL E APARENTE */
  #include <EmonLib.h> 

/* BIBLIOTECA DE COMUNICAÇÃO VIA BARRAMENTO SPI  */
  #include <SPI.h>

/* BIBLIOTECA DE COMUNICAÇÃO VIA LoRa */
  #include <LoRa.h> 

//Define os pinos que serão usados pelo módulo transmissor LoRa
  #define SCK_LORA           18
  #define MISO_LORA          19
  #define MOSI_LORA          23
  #define RESET_PIN_LORA     14
  #define SS_PIN_LORA        5
  #define HIGH_GAIN_LORA     20  /* dBm */
  #define BAND               915E6  /* 915MHz de frequencia */

/* CRIA UMA INSTÂNCIA DO EMONLIB PARA CADA CONJUNTO DE SENSORES - TENSÃO + CORRENTE */
  EnergyMonitor emon1; /* SENSOR DE TENSÃO E CORRENTE FASE 1 */
  EnergyMonitor emon2; /* SENSOR DE TENSÃO E CORRENTE FASE 2 */


/* DEFINE OS PINOS ONDE ESTÃO CONECTADOS OS SENSORES DE TENSÃO E CORRENTE*/
  const int voltpin1 = 34;
  const int amppin1 = 32;

  const int voltpin2 = 25;
  const int amppin2 = 26;


/* DEFINE OS VALORES DE CALIBRAÇÃO PARA OS SENSORES DE CORRENTE E TENSÃO*/
  double voltCal = 150;
  double ampCal = 25.9;


/* CRIA UMA ESTRUTURA DE DADOS COM AS VARIÁVEIS QUE SERÃO ENVIADAS VIA LORA*/
/* typedefs */
typedef struct __attribute__((__packed__))  
{

    double volt1;
    double volt2;
    double amp1; 
    double amp2; 
    double WReal; 

}TDadosLora;


/* DECLARA AS FUNÇÕES DE INÍCIO DE COMUNICAÇÃO COM O CHIP E O ENVIO DOS DADOS VIA LoRa*/
/* prototypes */

  void envia_informacoes_lora(double V_Fase1,double V_Fase2,double I_Fase1, double I_Fase2);

  bool init_comunicacao_lora(void);


/* 
 * Função: envia por LoRa as informações de Tensão das Fases e Correntes lidas, assim como a Potência Real
 * Parâmetros: - Tensão na Fase 1 Lida
 *             - Tensão na Fase 2 Lida
 *             - Corrente na Fase 1 Lida
 *             - Corrente na Fase 2 Lida
 *             - Potência Total Calculada
 * Retorno: nenhum
 */
void envia_informacoes_lora(double V_Fase1,double V_Fase2,double I_Fase1, double I_Fase2)
{
    TDadosLora dados_lora;

    dados_lora.volt1= V_Fase1;
    dados_lora.volt2 = V_Fase2;
    dados_lora.amp1 = I_Fase1;
    dados_lora.amp2 = I_Fase2;
    dados_lora.WReal = (V_Fase1 * I_Fase1)+(V_Fase2 * I_Fase2);
    
    
    LoRa.beginPacket();
    LoRa.write((unsigned char *)&dados_lora, sizeof(TDadosLora));
    LoRa.endPacket();
}


/* Funcao: inicia comunicação com chip LoRa
 * Parametros: nenhum
 * Retorno: true: comunicacao ok
 *          false: falha na comunicacao
*/
bool init_comunicacao_lora(void)
{
    bool status_init = false;
    Serial.println("[LoRa Sender] Tentando iniciar comunicacao com o radio LoRa...");
    SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_PIN_LORA);
    LoRa.setPins(SS_PIN_LORA, RESET_PIN_LORA, LORA_DEFAULT_DIO0_PIN);
    
    if (!LoRa.begin(BAND)) 
    {
        Serial.println("[LoRa Sender] Comunicacao com o radio LoRa falhou. Nova tentativa em 1 segundo...");        
        delay(1000);
        status_init = false;
    }
    else
    {
        /* Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível) */ 
        LoRa.setTxPower(HIGH_GAIN_LORA); 
        LoRa.setSyncWord(0xF3);
        Serial.println("[LoRa Sender] Comunicacao com o radio LoRa ok");
        status_init = true;
    }

    return status_init;
}


void setup(void)
{

    Serial.begin(115200);
    
    emon1.voltage(voltpin1, voltCal, 1.7);  // voltage: input pin, calibration e phase shift.
    emon1.current(amppin1, ampCal); // Current: input pin, calibration.  
    emon2.voltage(voltpin2, voltCal, 1.7);  // voltage: input pin, calibration e phase shift.
    emon2.current(amppin2, ampCal); // Current: input pin, calibration.  

    /* Tenta, até obter sucesso, comunicacao com o chip LoRa */
    while(init_comunicacao_lora() == false);    
    
}

void loop(void){

    double V_Fase1, V_Fase2, I_Fase1, I_Fase2;

    emon1.calcVI(60,1000);
    emon1.calcIrms(1480);
    
    emon2.calcVI(60,1000);
    emon2.calcIrms(1480);
    
    V_Fase1 = emon1.Vrms;
    V_Fase2 = emon2.Vrms;
    I_Fase1 = emon1.Irms;
    I_Fase2 = emon2.Irms;
    
    envia_informacoes_lora(V_Fase1, V_Fase2, I_Fase1, I_Fase2);
    
    Serial.println("Tensões no Circuito");
    Serial.println("");
    Serial.println(V_Fase1);
    Serial.println(V_Fase2);
    Serial.println("");
    
    Serial.println("Correntes no Circuito");
    Serial.println("");
    Serial.println(I_Fase1);  
    Serial.println(I_Fase2);
    Serial.println(""); 
    

delay(15000);


}
