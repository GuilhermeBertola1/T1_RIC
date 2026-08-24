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
#include "GoogleSheetsClient.h"


// Do JSON da service account:
#define PROJECT_ID "fit-bulwark-263523"
#define CLIENT_EMAIL "projeto-ric1@fit-bulwark-263523.iam.gserviceaccount.com"

const char PRIVATE_KEY[] PROGMEM =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDILmkfzHhVljz8\nEYIRw6IR52TnSP74y94qVOhcaF6+Sr0VDbmf1Gw34M5wCn5C8dgPXDzGloXWQSjb\n74M10R5pwUqq2hapq3M10YNkelri6OW191VfDL6mteJiVNp8qE0rrY9ULLFO1cHH\nlrs3+Fbal0FUj4GQFQd27aXC2odSBdSkD+HxVNVnoS4yZRJJpo996B8WJm02zMKr\n8UbE4NRcT/E5gU9wvm6eqrqitv3K3l4vNtLbeCTprr4EjjEIONYwxBffESQrgmHC\nW932W04ZSSYKXoUKcj9tmkd7uuvsYjVMlRRtv/fe9sZcHdv+FN9fYEybeb9G7dhI\n15r+xbEHAgMBAAECggEAFKj0O45toEkEW/BDUCm0iTbXAKcq0tWuRQiEqhOG17U8\npOFzliMlKh00YSGMHMT7YMEgb/nVWqKt9gkBreUPx6gQgz1FQoH0lDTejoOxlqKl\nIUMjbKRoQX9wcRKYhsNoojM5oYfdTX7H5Ef9DuPoCYu7zTTkZnuXzMvueYOhR13Y\nvqzO5ra3h9+UT9soAZ+hslGGRME8hluD6YbIkkYF6IrXFTuMPIQjFHIBM+6z/aQb\nXS7qyFf/aYCiJmWJ1QSCwWv7yK4But1nsCp/ciZHYxawwPJ69yqTz69w/BRNmw1u\nPUcpbkeWOJCJooEXEdCA348NuztLnjGTqY2wTLzHkQKBgQD5Dwy92Lm/Po1SCgEu\n7xuToTi7Q8MCD5vPV3dI6H/CZ7yztfWAizAfa0zYRvfQ4XHUTxp8OtGEJ5UUhNRo\nhCGK4LRUfZuK47K67LjgVCEKmT4IAiTcXdTFaL8aCo05wA85YlqUv9OXiZfWU2Zu\n4fTI8M/mZVLok/Vv71DzOy2+cQKBgQDNwqLvE3dxbakz3hN9o2O0PGUvVz+fUtD0\n9LTa0ULEARMbMfv5FSRzP4NKrVn1juSwIQ96QHyMwV3lnRUlmHjTTFMP+sdELVGI\nW4sfhXXshfnz5tWIy0okRrPnRTz/p3mcIB871QUhDTWf+hRhBVtWo6fMKGyjAUVv\nOhRj768S9wKBgQC7FnEN01WEsapa7CUDap4egccp9LbyP2Ndl2iFnhEKqf54SMFI\ndEYL9ABfnr/datylDV7p3w0v5sP4C+e7MzY3KqCLr2c+J9lEzTn7WXcQjRipz2J9\nHwcL26liGHcJ4JoPEzDT+F0rwjIKz89sj9PfdfXWLlrfVvXRxcXbUKMGIQKBgCTL\nBCWqHnaq/FGpCWUy/VYyYomXmWnc/j5/L4PdXYn9AzSeuIbuz+jH16tX2jk9xwjz\nsHfU5jCpNygi8bAaVqi6AjReTJmVXPn8HKGTVOXpgyupAFAIwgZAmlTlnaW9hdJ0\nMAGb8uHZIdiseE5tDcstUokARYTUal0cTp8cMfs7AoGBAKoeXKJiOwc3+kQZ6194\nsL2dNjaIwpy0lPkZfAoiKaJOvHp6j3sRGyhdkiXjRrKvw7+doEFAB4KHSHGyua2g\nt0mD6bgwRcMb5Vu3IDFHkDMbTHjxmgxlRbtNR4PzVT8n/Nj0ICIREmzG0Jm6jhgl\nypik9+5mhW3pvy6wwrRVawlO\n"
"-----END PRIVATE KEY-----\n";

#define SPREADSHEET_ID "1c7nE1SQTm5SBzU9dcC4Gqow2yPVllpm7s7lgnBHRF9Y/edit?gid=0#gid=0"


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

#define MY_ID "8895625689"
#define BOT_TOKEN "8943822736:AAFj5-B3cNi1ybGmDPWTOkC0Yb8QrjS9Vo4"

const char * ssid = "labautomacao";
const char * pwd = "L@bA1to25";

/*const char* validoChatIds[] = {
  "8895625689",   // Pessoa 1
  "XXXX",   // Pessoa 2
};*/

const int numUserAutorizado =
  sizeof(validoChatIds) / sizeof(validoChatIds[0]);

bool isAuthorized(String chat_id) {

  for (int i = 0; i < numUserAutorizado; i++) {

    if (chat_id == validoChatIds[i]) {
      return true;
    }

  }

  return false;
}
  
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


void tokenStatusCallback(TokenInfo info);


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

    WiFi.setAutoReconnect(true);

    
    clientTelegram.setInsecure();
    clientEmail.setInsecure();

    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Sincronizando hora via NTP");
    while (time(nullptr) < 100000) {
        Serial.print(".");
        delay(100);
    }
    Serial.println(" OK");


    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.setPrerefreshSeconds(10 * 60);
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

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

    bool enviado = bot.sendMessage(
        MY_ID,
        "Lora conectado ao Telegram!",
        ""
    );

    if (enviado)
    {
        Serial.println("Mensagem enviada com sucesso!");
    }
    else
    {
        Serial.println("Erro ao enviar mensagem.");
    }
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
            value.add("majorDimension", "ROWS");


            if (GSheet.values.append(&response, SPREADSHEET_ID, "Página1!A:D", &value)) {
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
    googleSheetsLoop();
     googleSheetsSendData(
            temperatura,
            umidade,
            pressao,
            altitude
        );

    if (tempoAtual - tempoAnteriorTelegram >= 1500) {
    
        int numMensagens = bot.getUpdates(bot.last_message_received + 1);
        while (numMensagens) {
            VerificaMsgTele(numMensagens);
            numMensagens = bot.getUpdates(bot.last_message_received + 1);
        }
         tempoAnteriorTelegram = tempoAtual;
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
    const char* smtp_host = "smtp.gmail.com";
    const int smtp_port = 465;
 
    const char* email_remetente = "seu_email@gmail.com";
    const char* senha_app = "Senha_API";
    const char* email_destinatario = "email_destino@gmail.com";
 
    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };
 
    smtp.connect(smtp_host, smtp_port, statusCallback);
 
    if (!smtp.isConnected()) {
        Serial.println("Falha ao conectar no servidor SMTP.");
        return;
    }
 
    smtp.authenticate(email_remetente, senha_app, readymail_auth_password);
 
    if (!smtp.isAuthenticated()) {
        Serial.println("Falha na autenticação SMTP (verifique a senha de app).");
        return;
    }
 
    SMTPMessage msg;
    msg.headers.add(rfc822_from, String("ESP32 <") + email_remetente + ">");
    msg.headers.add(rfc822_to, email_destinatario);
    msg.headers.add(rfc822_subject, "Alerta: 25 Leituras Concluidas!");
    msg.text.body("O ESP32 acabou de registrar e salvar 25 novas leituras no Google Sheets.");
 
    msg.timestamp = time(nullptr);
 
    Serial.println("Enviando e-mail de alerta...");
    smtp.send(msg);
}


void VerificaMsgTele(int numMensagens) {
    for (int i = 0; i < numMensagens; i++) {
        String chat_id = bot.messages[i].chat_id;
        
        if (!isAuthorized(chat_id)) {
            bot.sendMessage(chat_id, "Usuário não autorizado", "");
            continue;
        }

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


void tokenStatusCallback(TokenInfo info) {
    if (info.status == token_status_error) {
        Serial.printf("Erro no token: %s\n", GSheet.getTokenError(info).c_str());
    } else {
        Serial.printf("Token: %s\n", GSheet.getTokenStatus(info).c_str());
    }
}