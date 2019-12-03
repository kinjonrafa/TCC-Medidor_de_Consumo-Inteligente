/*TCC - MEDIDOR DE CONSUMO DE ENERGIA ELÉTRICA INTELIGENTE */

/*TRABALHO DE CONCLUSÃO DO CURSO DE ENGENHARIA ELÉTRICA - ESTÁCIO */

/*
 * Autores:
 * ARTULINO JÚNIOR
 * RAFAEL SILVA
 */


/* INCLUI AS BIBLIOTECAS NECESSÁRIAS PARA FUNCIONAMENTO DO DISPOSITIVO */
  #include <SPI.h>
  #include <LoRa.h>
  #include <WiFi.h>
  #include "ThingSpeak.h"


/*DEFINE OS PINOS REFERENTES AO MÓDULO DE COMUNICAÇÃO LoRa - DEFINE O GANHO E A FAIXA DE FREQUÊNCIA DE OPERAÇÃO*/
  #define SCK_LORA           18
  #define MISO_LORA          19
  #define MOSI_LORA          23
  #define RESET_PIN_LORA     14
  #define SS_PIN_LORA        5
  #define HIGH_GAIN_LORA     20  /* dBm */
  #define BAND               915E6  /* 915MHz de frequencia */


/*DEFINE A REDE A QUAL IRÁ SE CONECTAR, DEFINE A SENHA E O INTERVALO DE ATUALIZAÇÃO DO THINGSPEAK*/
  #define SSID_REDE     "ZikTrak" //coloque aqui o nome da rede que se deseja conectar
  #define SENHA_REDE    "************"  //coloque aqui a senha da rede que se deseja conectar
  #define INTERVALO_ENVIO_THINGSPEAK  15000  //intervalo entre envios de dados ao ThingSpeak (em ms)
  int keyIndex = 0; // your network key Index number (needed only for WEP)


/*DECLARA AS VARIÁVEIS GLOBAIS DO PROJETO, ENDEREÇO DA API THINGSPEAK, A CHAVE DE ESCRITA E ETC...*/
  char EnderecoAPIThingSpeak[] = "api.thingspeak.com";
  String ChaveEscritaThingSpeak = "E1FVDYFLJAOOF5G5"; //update
  long lastConnectionTime; 
  WiFiClient client;

/* Definicoes gerais */
  #define DEBUG_SERIAL_BAUDRATE    115200

  bool init_comunicacao_lora(void);



/* Funcao: inicia comunicação com chip LoRa
 * Parametros: nenhum
 * Retorno: true: comunicacao ok
 *          false: falha na comunicacao
*/
bool init_comunicacao_lora(void);


 /* typedefs */
typedef struct __attribute__((__packed__))  
{

    double volt1;
    double volt2;
    double amp1; 
    double amp2; 
    double WReal; 

}TDadosLora;



//prototypes
void EnviaInformacoesThingspeak(String StringDados);
void FazConexaoWiFi(void);
 
/*
 * Implementações
 */
 
//Função: envia informações ao ThingSpeak
//Parâmetros: String com a  informação a ser enviada
//Retorno: nenhum
void EnviaInformacoesThingspeak(String StringDados)
{
    if (client.connect(EnderecoAPIThingSpeak, 80))
    {         
        //faz a requisição HTTP ao ThingSpeak
        client.print("POST /update HTTP/1.1\n");
        client.print("Host: api.thingspeak.com\n");
        client.print("Connection: close\n");
        client.print("X-THINGSPEAKAPIKEY: "+ChaveEscritaThingSpeak+"\n");
        client.print("Content-Type: application/x-www-form-urlencoded\n");
        client.print("Content-Length: ");
        client.print(StringDados.length());
        client.print("\n\n");
        client.print(StringDados);
   
        lastConnectionTime = millis();
        Serial.println("- Informações enviadas ao ThingSpeak!");
     }   
}
 
//Função: faz a conexão WiFI
//Parâmetros: nenhum
//Retorno: nenhum
void FazConexaoWiFi(void)
{
    client.stop();
    Serial.println("Conectando-se à rede WiFi...");
    Serial.println();  
    delay(1000);
    WiFi.begin(SSID_REDE, SENHA_REDE);
 
    while (WiFi.status() != WL_CONNECTED) 
    {
        delay(500);
        Serial.print(".");
    }
 
    Serial.println("");
    Serial.println("WiFi connectado com sucesso!");  
    Serial.println("IP obtido: ");
    Serial.println(WiFi.localIP());
 
    delay(1000);
}


bool init_comunicacao_lora(void)
{
    bool status_init = false;
    Serial.println("[LoRa Receiver] Tentando iniciar comunicacao com o radio LoRa...");
    SPI.begin(SCK_LORA, MISO_LORA, MOSI_LORA, SS_PIN_LORA);
    LoRa.setPins(SS_PIN_LORA, RESET_PIN_LORA, LORA_DEFAULT_DIO0_PIN);
    
    if (!LoRa.begin(BAND)) 
    {
        Serial.println("[LoRa Receiver] Comunicacao com o radio LoRa falhou. Nova tentativa em 1 segundo...");        
        delay(1000);
        status_init = false;
    }
    else
    {
        /* Configura o ganho do receptor LoRa para 20dBm, o maior ganho possível (visando maior alcance possível) */ 
        LoRa.setTxPower(HIGH_GAIN_LORA); 
        LoRa.setSyncWord(0xF3);
        Serial.println("[LoRa Receiver] Comunicacao com o radio LoRa ok");
        status_init = true;
    }

    return status_init;
}


void setup() {

  //initialize Serial Monitor
  Serial.begin(DEBUG_SERIAL_BAUDRATE);
  while (!Serial);
  Serial.println("LoRa Receiver");


  lastConnectionTime = 0; 
  FazConexaoWiFi();

/* Tenta, até obter sucesso, comunicacao com o chip LoRa */
    while(init_comunicacao_lora() == false);    
    

  WiFi.mode(WIFI_STA);
  ThingSpeak.begin(client); // Initialize ThingSpeak

  
}

void loop() {

    char byte_recebido;
    int packet_size = 0;
    int lora_rssi = 0;
    TDadosLora dados_lora;
    char * ptInformaraoRecebida = NULL;
  

    float VFase1;
    float VFase2;
    float IFase1; 
    float IFase2; 
    float Kwh; 

    char strData[100];

    
    
    /* Verifica se chegou alguma informação do tamanho esperado */
    packet_size = LoRa.parsePacket();
    
    if (packet_size == sizeof(TDadosLora)) 
    {
        Serial.print("[LoRa Receiver] Há dados a serem lidos");
        
        /* Recebe os dados conforme protocolo */               
        ptInformaraoRecebida = (char *)&dados_lora;  
        while (LoRa.available()) 
        {
            byte_recebido = (char)LoRa.read();
            *ptInformaraoRecebida = byte_recebido;
            ptInformaraoRecebida++;
        }

        /* Escreve RSSI de recepção e informação recebida */
        lora_rssi = LoRa.packetRssi();

     }


     
    //Força desconexão ao ThingSpeak (se ainda estiver desconectado)
    if (client.connected())
    {
        client.stop();
        Serial.println("- Desconectado do ThingSpeak");
        Serial.println();
    }
 
    VFase1 = (float)dados_lora.volt1 ;
    VFase2 = (float)dados_lora.volt2;
    IFase1 = (float)dados_lora.amp1; 
    IFase2 = (float)dados_lora.amp2; 
    Kwh = (float)dados_lora.WReal;

 
  
    
    //verifica se está conectado no WiFi e se é o momento de enviar dados ao ThingSpeak
    if(!client.connected() && 
      (millis() - lastConnectionTime > INTERVALO_ENVIO_THINGSPEAK))
    {
        sprintf(strData,"field1=%.2f&field2=%.2f&field3=%.2f&field4=%.2f&field5=%.2f",VFase1,VFase2,IFase1,IFase2,Kwh);
        EnviaInformacoesThingspeak(strData);


    }


    Serial.println("Tensões no Circuito");
    Serial.println("");
    Serial.println(VFase1);
    Serial.println(VFase2);
    Serial.println("");
    
    Serial.println("Correntes no Circuito");
    Serial.println("");
    Serial.println(IFase1);  
    Serial.println(IFase2);
    Serial.println(""); 
    
    
    Serial.println("Potencia consumida no Circuito");
    Serial.println("");
    Serial.println(Kwh);  
    Serial.println(""); 
    
    

delay(1000);

}
