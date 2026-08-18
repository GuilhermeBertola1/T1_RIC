#include <Arduino.h> // conversor .ino para esp
#include <Wire.h>  // I2C lib
#include <Adafruit_Sensor.h> // Sensor lib
#include <Adafruit_BME280.h>
#include <SSD1306.h> // Display OLED
#include <WiFi.h> // Wifi esp32
#include <WiFiClientSecure.h> //necessario conexao telegram
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h> // interface servidor no IP esp32
#include <UniversalTelegramBot.h> // Bot telegram
#include <ESP_Mail_Client.h> // mensagem no email
#include <FirebaseJson.h> // lib dep
#include <ArduinoJson.h>
#include <ESP_Google_Sheet_Client.h> // Google sheet
#include <LittleFS.h> // Biblioteca para acessar os arquivos


//Definições
#define LEDPIN 25 //Led interno da placa
#define BUTTON 0 //Botão PRG ou EN da placa

// Definições do Sensor BME 280
//#define SEALEVELPRESSURE_HPA (1013.25)
#define SEALEVELPRESSURE_HPA (1029.9) //Sorocaba
#define I2C_SDA 21            // se definindo portas I2C
#define I2C_SCL 22            // se definindo portas I2C
TwoWire I2CBME = TwoWire(1);  // se definindo portas I2C entre OLED e BME280,OLED usa por padrão Wire(0) e BME280 vai usar Wire(1)
Adafruit_BME280 bme; // I2C

// Definições do Display OLED
#define OLED_I2C_ADDR 0x3C
#define OLED_RESET    16
#define OLED_SDA      4
#define OLED_SCL      15
SSD1306 display (OLED_I2C_ADDR, OLED_SDA, OLED_SCL);

#define NET_NAME "YYYY"
#define PASS "XXXX"

#define BOT_TOKEN "TOKEN_BOT"
WiFiClientSecure clientTelegram;
UniversalTelegramBot bot(BOT_TOKEN, clientTelegram);

AsyncWebServer server(80);
AsyncEventSource events("/eventos");

struct DadosBME {
  float temperatura;
  float pressao;
  float altitude;
  float umidade;
};

unsigned long tempoAnteriorSSE = 0;
unsigned long tempoAnteriorSheets = 0;
unsigned long tempoAnteriorTelegram = 0;
int contadorLeituras = 0;

SMTPSession smtp;

DadosBME readSensorBME();
void enviarEmailAlerta();
void VerificaMsgTele(int numMensagens);

void setup() {
    // Inicialização Serial  UART
    Serial.begin(115200);
    Serial.println("Iniciando...");

    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, LOW);

    //Memoria ESP32
    if(!LittleFS.begin(true)){
        Serial.println("Erro ao montar o LittleFS");
        return;
    }
    
    //WIFI
    WiFi.begin(NET_NAME, PASS);
    Serial.print("Conectando");
    while(WiFi.status() != WL_CONNECTED){
        Serial.print(".");
    }
    Serial.println();
    IPAddress IP = WiFi.localIP();
    Serial.print("IP address: ");
    Serial.println(IP);

    //Server
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/index.html", String(), false);
    });

    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/style.css", "text/css");
    });

    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/script.js", "text/javascript");
    });

    server.on("/chart.umd.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/chart.umd.js", "text/javascript");
    });

    events.onConnect([](AsyncEventSourceClient *client){
        Serial.println("Novo cliente conectado na interface web!");
    });
    server.addHandler(&events);
    
    server.begin();

    //Inicialização do display OLED
    pinMode(OLED_RESET, OUTPUT); 
    digitalWrite(OLED_RESET, LOW);
    delay(50);
    digitalWrite(OLED_RESET, HIGH);
    delay(50);
    display.init ();
    display.setFont (ArialMT_Plain_10);
    display.setTextAlignment (TEXT_ALIGN_LEFT);
    if (!display.init()) {
        Serial.println("Display indisponível!");
    }
    else {
        display.drawString (0, 0, "Display OLED - OK");
        Serial.println("Display OLED - OK!");
        display.display ();
        delay(1000);
    }

    //Inicialização do sensor BME280 
    I2CBME.begin(I2C_SDA, I2C_SCL, 100000); // se definindo portas I2C
    bool status; 
    //status = bme.begin(0x76); 
    status = bme.begin(0x76, &I2CBME);      // se definindo portas I2C
    if (!status) {
        Serial.println("Sensor BME280 -Não OK");
        display.drawString (0, 16, "Sensor BME280 - Não OK");
        display.display ();
        delay(1000);
        while (1); //Se BME não disponível, não fuciona!
    }
    else {
        Serial.println("Sensor BME280 - OK");
        display.drawString (0, 16, "Sensor BME280 - OK");
        display.display ();
        delay(1000);
    }
    //==========================================================================

}

void loop() {
    unsigned long tempoAtual = millis();
    DadosBME valor_sensor = readSensorBME();

    if (tempoAtual - tempoAnteriorSSE >= 1000) {
        tempoAnteriorSSE = tempoAtual;

        JsonDocument doc;
        doc["temp"] = valor_sensor.temperatura;
        doc["pres"] = valor_sensor.pressao;
        doc["alti"] = valor_sensor.altitude;
        doc["humi"] = valor_sensor.umidade;
        String jsonString;
        serializeJson(doc, jsonString);
        events.send(jsonString.c_str(), "nova_leitura", millis());
    }

    if (tempoAtual - tempoAnteriorSheets >= 10000) {
        tempoAnteriorSheets = tempoAtual;

        if (GSheet.ready()) {
            FirebaseJson response;
            FirebaseJson value;
            value.set("values/[0]/[0]", valor_sensor.temperatura);
            value.set("values/[0]/[1]", valor_sensor.umidade);
            value.set("values/[0]/[2]", valor_sensor.pressao);
            value.set("values/[0]/[3]", valor_sensor.altitude);
            
            if (GSheet.values.append(&response, "SEU_ID_DA_PLANILHA", "Página1!A:D", &value)) {
                Serial.println("Dado salvo com sucesso no Google Sheets!");
                contadorLeituras++;
                Serial.print("Total de leituras salvas: ");
                Serial.println(contadorLeituras);

                if (contadorLeituras >= 25) {
                    enviarEmailAlerta();
                    contadorLeituras = 0;
                }
            } else {
                Serial.print("Erro ao salvar no Sheets: ");
                Serial.println(GSheet.errorReason());
            }
        }
    }
    
    if (tempoAtual - tempoAnteriorTelegram >= 1500) {
        tempoAnteriorTelegram = tempoAtual;

        int numMensagens = bot.getUpdates(bot.last_message_received + 1);
        while (numMensagens) {
            VerificaMsgTele(numMensagens);
            numMensagens = bot.getUpdates(bot.last_message_received + 1);
        }
    }
}

DadosBME readSensorBME() {
    DadosBME leitura;
    leitura.temperatura = bme.readTemperature();
    leitura.pressao = bme.readPressure() / 100.0F;
    leitura.altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
    leitura.umidade = bme.readHumidity();
    return leitura;
}

void enviarEmailAlerta() {
    ESP_Mail_Session session;
    session.server.host_name = "smtp.gmail.com";
    session.server.port = 465;
    session.login.email = "seu_email@gmail.com";
    session.login.password = "Senha_API";

    SMTP_Message message;
    message.sender.name = "ESP32 LoRa";
    message.sender.email = "seu_email@gmail.com";
    message.subject = "Alerta: 25 Leituras Concluídas!";
    message.addRecipient("Responsável", "email_destino@gmail.com");
    message.text.content = "O ESP32 acabou de registrar e salvar 25 novas leituras no Google Sheets.";

    if (smtp.connect(&session)) {
        MailClient.sendMail(&smtp, &message);
        Serial.println("E-mail de alerta enviado com sucesso!");
    } else {
        Serial.print("Erro ao enviar e-mail: ");
        Serial.println(smtp.errorReason());
    }
}

void VerificaMsgTele(int numMensagens) {
    for (int i = 0; i < numMensagens; i++) {
        String chat_id = bot.messages[i].chat_id;
        String texto = bot.messages[i].text;

        if (texto == "/status") {
            DadosBME valor_sensor = readSensorBME();
            String resposta = String("📊 *Status Atual do ESP32*\n\n");
            resposta += "🌡️ Temperatura: " + String(valor_sensor.temperatura) + " °C\n";
            resposta += "💧 Umidade: " + String(valor_sensor.umidade) + " %\n";
            resposta += "🌍 Pressão: " + String(valor_sensor.pressao) + " hPa\n";
            resposta += "⛰️ Altitude: " + String(valor_sensor.altitude) + " m";
            bot.sendMessage(chat_id, resposta, "Markdown");
        }
        else if (texto == "/led_on") {
            digitalWrite(LEDPIN, HIGH);
            bot.sendMessage(chat_id, "💡 LED interno ligado com sucesso!", "");
        } 
        else if (texto == "/led_off") {
            digitalWrite(LEDPIN, LOW);
            bot.sendMessage(chat_id, "🌙 LED interno desligado com sucesso!", "");
        } 
        else {
            String ajuda = "Comando desconhecido. Use:\n";
            ajuda += "/status - Vê os dados do sensor\n";
            ajuda += "/led_on - Liga o LED\n";
            ajuda += "/led_off - Desliga o LED";
            bot.sendMessage(chat_id, ajuda, "");
        }
    }
}