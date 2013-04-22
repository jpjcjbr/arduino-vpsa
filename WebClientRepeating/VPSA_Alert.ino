
#include <SPI.h>
#include <Ethernet.h>

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

EthernetClient client;

IPAddress server(10,201,1,11);

unsigned long lastConnectionTime = 0;
unsigned long lastConnectionTimeBuzzer = 0;
boolean lastConnected = false;
String jsonRetorno = "";
// Intervalo de consulta
const unsigned long postingInterval = 225L*1000L;
String cor = "blue";

const String STATUS_TESTES_INTEGRACAO_QUEBRADOS = "red";
const String STATUS_TESTES_UNITARIOS_QUEBRADOS = "yellow";
const int BUZZER_PIN =  9;
const int GIROFLEX_PIN = 8;
const int COMPRAS = 0;
const int ESTOQUE = 1;
const int PDV = 2;
const int VPSA = 3;

String projetos[] = { "compras-java", "estoque-java-integration-test", "offlinemanager", "vpsa-java-integration-test" };
int bipes[] = { 1, 2, 3, 4 };
int indexProjetoAtual = -1;
int retryCount = 0;

void setup() {
  pinMode(BUZZER_PIN, OUTPUT);  
  pinMode(GIROFLEX_PIN, OUTPUT); 
  
  //Serial.begin(115200);
  delay(1000);
  Ethernet.begin(mac);
  //Serial.print("My IP address: ");
  //Serial.println(Ethernet.localIP());
  tone(BUZZER_PIN, 440, 500);
}

void loop() {
  if (client.available()) {
    char c = client.read();
    jsonRetorno += c;
  }

  if (!client.connected() && lastConnected) {
    int posicaoSeparador = jsonRetorno.indexOf(":\"");

    cor = jsonRetorno.substring(jsonRetorno.indexOf("\"}", posicaoSeparador), posicaoSeparador + 2);
    
    if(cor == STATUS_TESTES_INTEGRACAO_QUEBRADOS || cor == STATUS_TESTES_UNITARIOS_QUEBRADOS)
    {
      tocarAlarme();
    }
    
    //Serial.println(cor);
    //Serial.println("disconnecting.");
    client.stop();
    jsonRetorno = "";
  }

  if(!client.connected() && (millis() - lastConnectionTime > postingInterval)) {
    if (indexProjetoAtual == VPSA) {
      indexProjetoAtual = COMPRAS;
    } else {
      indexProjetoAtual++;
    }
    httpRequest();
  }
  
  lastConnected = client.connected();
}

void tocarAlarme() {
    digitalWrite(GIROFLEX_PIN, HIGH);
  
    const int delayNota = 500;
    const int delayPausa = delayNota * 2;
    const int nota = 440;
    
    for (int intervalo = 0; intervalo < 3; intervalo++) {
      for (int vezes = 0; vezes < bipes[indexProjetoAtual]; vezes++) {
        tone(BUZZER_PIN, nota, delayNota);
        delay(delayPausa);
      }
      delay(4000);
    }
    
    digitalWrite(GIROFLEX_PIN, LOW);
}

void httpRequest() {
  if (client.connect(server, 9080)) {
    //Serial.println("connecting...");

    client.println("GET /hudson/job/" + projetos[indexProjetoAtual] + "/api/json?tree=color HTTP/1.0");
    client.println();

    //Serial.println();
    //Serial.println("Projeto: " + projetos[indexProjetoAtual]);
    lastConnectionTime = millis();
  } 
  else {
    //Serial.println("connection failed");
    //Serial.println("disconnecting.");
    client.stop();
    
    if (retryCount < 3) {
      retryCount++;
      httpRequest(); 
    } else {
      retryCount = 0;  
    }
  }
}
