
#include <WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ArduinoJson.h>


const char* WIFI_SSID = "Wokwi-GUEST";
const char* WIFI_PASS = "";
const char* MQTT_SERVER = "44.223.43.74";


#define DHT_PIN 15
#define DHT_TYPE DHT22
#define MQ2_PIN 32       
#define LDR_PIN 35       
#define BUZZER_PIN 23    


const unsigned long PUBLISH_INTERVAL_MS = 15000;
const unsigned long ALERT_HOLD_MS = 8000; 
const int ADC_MAX = 4095;


const float TEMP_HIGH = 30.0;
const float TEMP_LOW  = 15.0;
const int HUMID_LOW = 20;
const int HUMID_HIGH = 75;
const int MQ2_OK = 1200;
const int MQ2_WARN = 2500;
const int LDR_DARK = 300;
const int LDR_TOO_BRIGHT = 3800;


DHT dht(DHT_PIN, DHT_TYPE);
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned long lastPublish = 0;
unsigned long lastStateChangeTime_temp = 0;
unsigned long lastStateChangeTime_hum = 0;
unsigned long lastStateChangeTime_mq = 0;
unsigned long lastStateChangeTime_ldr = 0;

enum RiskLevel { RL_OK = 0, RL_WARN = 1, RL_DANGER = 2 };
RiskLevel lastTempState = RL_OK;
RiskLevel lastHumState  = RL_OK;
RiskLevel lastMqState   = RL_OK;
RiskLevel lastLdrState  = RL_OK;

void beepShort() {
  if (BUZZER_PIN < 0) return;
  digitalWrite(BUZZER_PIN, HIGH);
  delay(120);
  digitalWrite(BUZZER_PIN, LOW);
}


void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando ao Wi-Fi: ");
  Serial.println(WIFI_SSID);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  int tentativas = 0;
  while (WiFi.status() != WL_CONNECTED && tentativas < 20) {
    delay(500);
    Serial.print(".");
    tentativas++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWi-Fi conectado com sucesso!");
    Serial.print("Endereço IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFalha ao conectar no Wi-Fi!");
  }
}


void reconnect() {
  while (!mqttClient.connected()) {
    Serial.print("Conectando ao broker MQTT...");
    String clientId = "ESP32_WorkWell_";
    clientId += String((uint64_t)ESP.getEfuseMac(), HEX);
    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("Conectado!");
      mqttClient.publish("workwell/status", "ONLINE");
      mqttClient.subscribe("workwell/command");
    } else {
      Serial.print("Falha, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" tentando novamente em 5 segundos...");
      delay(5000);
    }
  }
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("MQTT msg em ");
  Serial.print(topic);
  Serial.print(": ");
  String msg;
  for (unsigned int i=0;i<length;i++) { msg += (char)payload[i]; Serial.print((char)payload[i]); }
  Serial.println();
  if (String(topic) == String("workwell/command")) {
    if (msg == "BUZZER_ON") { digitalWrite(BUZZER_PIN, HIGH); }
    else if (msg == "BUZZER_OFF") { digitalWrite(BUZZER_PIN, LOW); }
  }
}

const char* riskText(RiskLevel r) {
  if (r == RL_OK) return "OK";
  if (r == RL_WARN) return "Atenção";
  return "Perigo";
}

void evaluateAndMaybeAlert(float temp, int hum, int mqRaw, int ldrRaw) {
  unsigned long now = millis();

  // temperatura
  RiskLevel curTemp = RL_OK;
  if (temp >= TEMP_HIGH) curTemp = RL_DANGER;
  else if (temp <= TEMP_LOW) curTemp = RL_DANGER;
  if (curTemp != lastTempState) lastStateChangeTime_temp = now;

  
  RiskLevel curHum = RL_OK;
  if (hum < HUMID_LOW || hum > HUMID_HIGH) curHum = RL_DANGER;
  if (curHum != lastHumState) lastStateChangeTime_hum = now;


  RiskLevel curMq = RL_OK;
  if (mqRaw > MQ2_WARN) curMq = RL_DANGER;
  else if (mqRaw > MQ2_OK) curMq = RL_WARN;
  if (curMq != lastMqState) lastStateChangeTime_mq = now;


  RiskLevel curLdr = RL_OK;
  if (ldrRaw <= LDR_DARK) curLdr = RL_DANGER;
  else if (ldrRaw >= LDR_TOO_BRIGHT) curLdr = RL_WARN;
  if (curLdr != lastLdrState) lastStateChangeTime_ldr = now;


  bool tempConfirmed = (curTemp == lastTempState) || (now - lastStateChangeTime_temp >= ALERT_HOLD_MS);
  bool humConfirmed  = (curHum  == lastHumState)  || (now - lastStateChangeTime_hum  >= ALERT_HOLD_MS);
  bool mqConfirmed   = (curMq   == lastMqState)   || (now - lastStateChangeTime_mq   >= ALERT_HOLD_MS);
  bool ldrConfirmed  = (curLdr  == lastLdrState)  || (now - lastStateChangeTime_ldr  >= ALERT_HOLD_MS);

  if (tempConfirmed) lastTempState = curTemp;
  if (humConfirmed)  lastHumState  = curHum;
  if (mqConfirmed)   lastMqState   = curMq;
  if (ldrConfirmed)  lastLdrState  = curLdr;

  bool anyRisk = (lastTempState != RL_OK) || (lastHumState != RL_OK) || (lastMqState != RL_OK) || (lastLdrState != RL_OK);

  static RiskLevel prevTemp = RL_OK, prevHum = RL_OK, prevMq = RL_OK, prevLdr = RL_OK;
  bool stateChanged = (prevTemp != lastTempState) || (prevHum != lastHumState) || (prevMq != lastMqState) || (prevLdr != lastLdrState);

  if (stateChanged) {
    String texto = "";
    if (lastTempState != RL_OK) {
      texto += "Temperatura: ";
      texto += riskText(lastTempState);
      texto += ". ";
      if (temp >= TEMP_HIGH) texto += "Alto (" + String(temp) + " C). ";
      else if (temp <= TEMP_LOW) texto += "Baixo (" + String(temp) + " C). ";
    }
    if (lastHumState != RL_OK) {
      texto += "Umidade: ";
      texto += riskText(lastHumState);
      texto += ". ";
      texto += String(hum) + "% ";
    }
    if (lastMqState != RL_OK) {
      texto += "Qualidade do ar: ";
      texto += riskText(lastMqState);
      texto += ". ";
    }
    if (lastLdrState != RL_OK) {
      texto += "Luminosidade: ";
      texto += riskText(lastLdrState);
      texto += ". ";
    }

    if (texto.length() == 0) texto = "Todas as métricas dentro do esperado.";

    if (mqttClient.connected()) {
      mqttClient.publish("workwell/alerts", texto.c_str());
      Serial.print("Alerta publicado: ");
      Serial.println(texto);
    } else {
      Serial.print("Alerta (offline): ");
      Serial.println(texto);
    }

    if (anyRisk) beepShort();

    prevTemp = lastTempState;
    prevHum  = lastHumState;
    prevMq   = lastMqState;
    prevLdr  = lastLdrState;
  }
}

void setup() {
  Serial.begin(115200);
  delay(50);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  dht.begin();
  analogReadResolution(12);

  // Conexões (versão que você validou)
  setup_wifi();
  mqttClient.setServer(MQTT_SERVER, 1883);
  mqttClient.setCallback(mqttCallback);
  // tenta conectar MQTT se WiFi já OK
  if (WiFi.status() == WL_CONNECTED) reconnect();

  lastPublish = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    // tenta reconectar Wi-Fi com a mesma rotina simples
    setup_wifi();
  }

  if (!mqttClient.connected() && WiFi.status() == WL_CONNECTED) {
    reconnect();
  }

  if (mqttClient.connected()) mqttClient.loop();

  unsigned long now = millis();
  if (now - lastPublish >= PUBLISH_INTERVAL_MS) {
    lastPublish = now;

    float temperatura = dht.readTemperature();
    float umidade = dht.readHumidity();
    int mqRaw = analogRead(MQ2_PIN);
    int ldrRaw = analogRead(LDR_PIN);

    bool dhtOk = true;
    if (isnan(temperatura) || isnan(umidade)) {
      Serial.println("Erro leitura DHT - aguardando próxima leitura");
      dhtOk = false;
    }

    if (dhtOk) {
      evaluateAndMaybeAlert(temperatura, (int)round(umidade), mqRaw, ldrRaw);

      StaticJsonDocument<512> doc;
      doc["temperatura_C"] = round(temperatura * 10.0) / 10.0;
      doc["umidade_pct"] = (int)round(umidade);
      doc["mq2_raw"] = mqRaw;
      const char* mqDesc = (lastMqState==RL_OK) ? "Bom" : (lastMqState==RL_WARN ? "Moderado" : "Perigoso");
      doc["gas_nivel_pt"] = mqDesc;
      doc["ldr_raw"] = ldrRaw;
      const char* ldrDesc = (lastLdrState==RL_OK) ? "OK" : (lastLdrState==RL_WARN ? "Muito claro" : "Escuro");
      doc["luz_desc_pt"] = ldrDesc;
      doc["ip"] = (WiFi.status()==WL_CONNECTED) ? WiFi.localIP().toString() : "no_wifi";

      char payload[512];
      serializeJson(doc, payload);
      Serial.print("Publicando JSON: ");
      Serial.println(payload);

      if (mqttClient.connected()) {
        bool ok = mqttClient.publish("workwell/monitoramento", payload);
        Serial.print("Publish: ");
        Serial.println(ok ? "OK" : "FALHA");
      } else {
        Serial.println("MQTT offline - não publicou JSON");
      }
    }
  }
  delay(50);
}
