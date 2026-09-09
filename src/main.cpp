#include <Arduino.h>
#include <Wire.h>
#include <time.h>

#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <SSD1306.h>

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

// ---- Google Sheets ----
#include <FirebaseJson.h>
#include <ESP_Google_Sheet_Client.h>

// ---- E-mail ----
#define ENABLE_SMTP
#define ENABLE_DEBUG
#include <ReadyMail.h>

// =====================================================================
//  CONFIGURAÇÃO
// =====================================================================

// ------------------------------ WiFi ---------------------------------
const char *WIFI_SSID = "Integral";
const char *WIFI_PASS = "12345678";

// ------------------------- Google Sheets -----------------------------
#define PROJECT_ID   "t1ric-507422"
#define CLIENT_EMAIL "t1ric-teste@t1ric-507422.iam.gserviceaccount.com"

#define SPREADSHEET_ID "1XZfgr09i9FXPgj2rR4LeXOm5bVDbJbo2VzwFvYy5pNQ"

#define SHEET_RANGE "Dados!A:E"

const char PRIVATE_KEY[] PROGMEM = "-----BEGIN PRIVATE KEY-----\nMIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQDGONG+xlKczpmS\njwF2mY95Ys4hP5OY7lb1lf6sR/4G86vgiDdjv2+QWed9z641uGpQ6x+a40l12MO1\nx2sAyEkqdHzxgC99326tfStYCyyyrNOEvxsK+77bn0DicoIPTpi3jX0J+XGSM1yI\nZoRuKfOHd/J9PLZgtbquo89XhBqrs5aM1L9zLG0KTfvWLhMHB7l+lKQ6Ta66E1f4\nnp2hYT2iPArZsL02YleOHzt2mHMRaPCybjbEgue5kPXB8zjk34iVeub6p0KsLcGE\ndWCeqlWYdopEwCxM2zKMdaECUdM6IMz+Pip4ogx8RAXXH2QhI2NnNC+9lZsZK/9U\n9IJLN38NAgMBAAECggEAEvVEOjbFl/G6hmXfAfTV4AovJMbr2uiQsJRT8y4/gXK0\nsZjVk1iv1uKCXWoLg1/L60lRkILEd9zveudDuKNvO/xAJujXofZIo5chGh6Xe89D\nu6J7d6V0L5Ufo4jIH6aovIVvvBB7aDw5UWJcsBybCg3Sx1+TIdS/sgxAvGIKGvm7\nCETCMWnCkjnyp1/uukH2q5n32YLaLnzggzj/1F3S+JuvNUxFAxFJoAXpZ8PDYiB9\nMqj2fRM8PYoClPV3RvfL+hvO0rxVEi/P3UvagHh6t+bus5OrhKAoTuz4PtX1NIji\n6du0zaWwLdcPs1oveMjuG5a63NGp0up3CDUGmo4lAQKBgQDkgkXqEHuYcoB5RDrw\n2yV2SoiA5+jaKnsGEYlhsJIcY0MtSn6PHeFelrPhm5c99dhGjYtj8oAuoFt++mHC\nNMq45G75mzFnXjoD6X6afE+ZzqA5gUAPh+HlCkfcYkpf2EAtBVGqAyC8iWX9E+oi\nBSvEUn8nWkk4A7vNxh+KCo1D/QKBgQDeEcE21BIzBWcKmlbdyyQQATai70gW/2DQ\nrF8dqVleFV4pfG4K1cFP01VmsXYNKxclMjGvH2CbVWZ4SsCBIDDKYs0nwVprIVDw\n26t4B8RNqWwuriphX18LN8sgclvpO7oXnC+zidMV+MeW9XLlhVL9fL1DtZ15R0EQ\nSOhLwGOsUQKBgHKAEJZbkgLuhQ++r6YWuWvPpxyomBYRqTl+Og+4UU5mMrgUFtyR\nxWcHLrWCqENE2A1qSYlYbwBuG8rBnZCZsWb6F9189UFthraFHoo8dgqE7eZCrJ3b\nocaJ4z48BjaGfonQm34MOTAfzPoGK5DGdIrYJ5zJcdzeihFvEEi0RtrlAoGAd2VM\nYNI8xrnM4OKCedVSmlhMq+XC+Lptr0Q+D5CRwAf1DJEeCS1MNkMF06TuN75HP3Rx\nShqwslOEOIDYZ5SLJwVgEFPg7WCISDeTRYHHhYnl5GM33gqM61cgG13hFNFMCI2t\nB/a+Sz3q8bsd/1FjgW6jw9fNxvsMfFj2rGPQM0ECgYEAq8A29sy4oWj29QTI0Y/n\nnnnFuuaM1crwKw8Z8WYd1ooIzOqgH3+IQpk8CJpegU33IyEhGGKthV/4XYUadDQq\nNc1+HhmUHIJcXrreKPBKSRE5Sz67gB1tXV+6V4biE7wj7xvknlViVtiZwc1DePNt\nIwMKSW+dBCgM+SvaSt8RXLY=\n-----END PRIVATE KEY-----\n";

// ---------------------------- Telegram -------------------------------
#define BOT_TOKEN "8943822736:AAFj5-B3cNi1ybGmDPWTOkC0Yb8QrjS9Vo4"
#define MY_ID     "8895625689"

const char *validoChatIds[] = {
    MY_ID,
};
const int numUserAutorizado = sizeof(validoChatIds) / sizeof(validoChatIds[0]);

// ------------------------------ E-mail -------------------------------
#define SMTP_HOST       "smtp.gmail.com"
#define SMTP_PORT       465
#define EMAIL_REMETENTE "guilherme.bertola.123@gmail.com"
#define EMAIL_SENHA_APP "ioby kcwd rugx lvqv"
#define EMAIL_DESTINO   "guilherme.bertola.123@gmail.com"

// ------------------------------ Pinos --------------------------------
#define LEDPIN 25
#define BUTTON 0

// BME280 (I2C secundário)
#define SEALEVELPRESSURE_HPA (1029.9)   // Sorocaba
#define I2C_SDA 21
#define I2C_SCL 22
TwoWire I2CBME = TwoWire(1);
Adafruit_BME280 bme;

// OLED (I2C primário)
#define OLED_I2C_ADDR 0x3C
#define OLED_RESET    16
#define OLED_SDA      4
#define OLED_SCL      15
SSD1306 display(OLED_I2C_ADDR, OLED_SDA, OLED_SCL);

// --------------------------- Temporizações ---------------------------
const unsigned long INTERVALO_SSE      = 1000UL;
const unsigned long INTERVALO_SHEETS   = 10000UL;
const unsigned long INTERVALO_TELEGRAM = 1500UL;
const unsigned long DEBOUNCE_MS        = 50UL;
const int LEITURAS_POR_RELATORIO       = 25;

// =====================================================================
//  OBJETOS GLOBAIS
// =====================================================================

WiFiClientSecure clientTelegram;
UniversalTelegramBot bot(BOT_TOKEN, clientTelegram);

WiFiClientSecure clientEmail;
SMTPClient smtp(clientEmail);

AsyncWebServer server(80);
AsyncEventSource events("/eventos");

struct DadosBME {
    float temperatura;
    float pressao;
    float altitude;
    float umidade;
};

struct Estatistica {
    uint32_t n     = 0;
    double   media = 0.0;
    double   m2    = 0.0;
    double   minimo = 0.0;
    double   maximo = 0.0;

    void adicionar(double x) {
        if (n == 0) {
            minimo = maximo = x;
        } else {
            if (x < minimo) minimo = x;
            if (x > maximo) maximo = x;
        }
        n++;
        double delta = x - media;
        media += delta / (double)n;
        m2    += delta * (x - media);
    }
    double desvioPadrao() const {
        return (n < 2) ? 0.0 : sqrt(m2 / (double)(n - 1));
    }
    void reiniciar() {
        n = 0; media = 0.0; m2 = 0.0; minimo = 0.0; maximo = 0.0;
    }
};

Estatistica estatTemp, estatUmid, estatPres, estatAlt;
String horaInicioJanela = "";

bool ledEstado = false; 
bool botaoEstado = false;
int  botaoLeituraAnterior = HIGH;
unsigned long tempoUltimaMudanca = 0;
uint16_t botaoContagemPressoes = 0;

unsigned long tempoAnteriorSSE      = 0;
unsigned long tempoAnteriorSheets   = 0;
unsigned long tempoAnteriorTelegram = 0;
bool sensorOk = false;

DadosBME readSensorBME();
void     atualizarBotao();
void     setLed(bool ligado);
bool     salvarNoSheets(const DadosBME &d);
void     acumularEstatisticas(const DadosBME &d);
bool     enviarRelatorioEstatistico();
String   linhaRelatorioTexto(const char *nome, const char *un, const Estatistica &e);
String   linhaRelatorioHtml(const char *nome, const char *un, const Estatistica &e);
void     VerificaMsgTele(int numMensagens);
bool     isAuthorized(const String &chat_id);
String   horaFormatada();
void     tokenStatusCallback(TokenInfo info);

// =====================================================================
//  SETUP
// =====================================================================
void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\nIniciando...");

    pinMode(LEDPIN, OUTPUT);
    digitalWrite(LEDPIN, LOW);

    pinMode(BUTTON, INPUT_PULLUP);
    botaoLeituraAnterior = digitalRead(BUTTON);

    // ---------------------------- LittleFS ---------------------------
    if (!LittleFS.begin(true)) {
        Serial.println("Erro ao montar o LittleFS");
        return;
    }

    // ------------------------------ WiFi -----------------------------
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    Serial.print("Conectando");
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println();
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    WiFi.setAutoReconnect(true);

    clientTelegram.setInsecure();
    clientEmail.setInsecure();

    // ------------------------------- NTP -----------------------------
    configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
    Serial.print("Sincronizando hora via NTP");
    while (time(nullptr) < 1700000000) {
        Serial.print(".");
        delay(300);
    }
    Serial.printf(" OK -> %s\n", horaFormatada().c_str());

    // -------------------------- Google Sheets ------------------------
    GSheet.setTokenCallback(tokenStatusCallback);
    GSheet.setPrerefreshSeconds(10 * 60);
    GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);

    // ----------------------------- Servidor --------------------------
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/index.html", String(), false);
    });
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/style.css", "text/css");
    });
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/script.js", "text/javascript");
    });
    server.on("/chart.umd.js", HTTP_GET, [](AsyncWebServerRequest *request) {
        request->send(LittleFS, "/chart.umd.js", "text/javascript");
    });

    events.onConnect([](AsyncEventSourceClient *client) {
        Serial.println("Novo cliente conectado na interface web!");
    });
    server.addHandler(&events);
    server.begin();

    // ------------------------------ OLED -----------------------------
    pinMode(OLED_RESET, OUTPUT);
    digitalWrite(OLED_RESET, LOW);
    delay(50);
    digitalWrite(OLED_RESET, HIGH);
    delay(50);

    if (!display.init()) {
        Serial.println("Display indisponivel!");
    } else {
        display.setFont(ArialMT_Plain_10);
        display.setTextAlignment(TEXT_ALIGN_LEFT);
        display.drawString(0, 0, "Display OLED - OK");
        display.display();
        Serial.println("Display OLED - OK!");
        delay(1000);
    }

    // ----------------------------- BME280 ----------------------------
    I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
    sensorOk = bme.begin(0x76, &I2CBME);
    if (!sensorOk) {
        Serial.println("Sensor BME280 - NAO OK (seguindo com valores 0)");
        display.drawString(0, 16, "BME280 - Nao OK");
    } else {
        Serial.println("Sensor BME280 - OK");
        display.drawString(0, 16, "Sensor BME280 - OK");
    }
    display.display();
    delay(1000);

    // ---------------------------- Telegram ---------------------------
    if (bot.sendMessage(MY_ID, "Lora conectado ao Telegram!", "")) {
        Serial.println("Mensagem enviada com sucesso!");
    } else {
        Serial.println("Erro ao enviar mensagem.");
    }
}

// =====================================================================
//  LOOP
// =====================================================================
void loop() {
    unsigned long tempoAtual = millis();
    bool sheetsPronto = GSheet.ready();
    atualizarBotao();

    // ---------------------- SSE para a página web --------------------
    if (tempoAtual - tempoAnteriorSSE >= INTERVALO_SSE) {
        tempoAnteriorSSE = tempoAtual;

        DadosBME d = readSensorBME();

        JsonDocument doc;
        doc["temp"] = d.temperatura;
        doc["pres"] = d.pressao;
        doc["alti"] = d.altitude;
        doc["humi"] = d.umidade;
        doc["botao"] = botaoEstado;
        doc["led"] = ledEstado;

        String jsonString;
        serializeJson(doc, jsonString);
        events.send(jsonString.c_str(), "nova_leitura", millis());
    }

    // -------------------------- Google Sheets ------------------------
    if (sheetsPronto && (tempoAtual - tempoAnteriorSheets >= INTERVALO_SHEETS)) {
        tempoAnteriorSheets = tempoAtual;

        DadosBME d = readSensorBME();

        if (salvarNoSheets(d)) {
            acumularEstatisticas(d);
            Serial.printf("Leituras na janela: %u/%d\n",
                          estatTemp.n, LEITURAS_POR_RELATORIO);

            if ((int)estatTemp.n >= LEITURAS_POR_RELATORIO) {
                enviarRelatorioEstatistico();

                estatTemp.reiniciar();
                estatUmid.reiniciar();
                estatPres.reiniciar();
                estatAlt.reiniciar();
                botaoContagemPressoes = 0;
                horaInicioJanela = "";
            }
        }
    }

    // ---------------------------- Telegram ---------------------------
    if (tempoAtual - tempoAnteriorTelegram >= INTERVALO_TELEGRAM) {
        tempoAnteriorTelegram = tempoAtual;

        int numMensagens = bot.getUpdates(bot.last_message_received + 1);
        int voltas = 0;
        while (numMensagens && voltas++ < 5) {
            VerificaMsgTele(numMensagens);
            numMensagens = bot.getUpdates(bot.last_message_received + 1);
        }
    }
}

// =====================================================================
//  SENSOR
// =====================================================================
DadosBME readSensorBME() {
    DadosBME leitura = {0, 0, 0, 0};
    if (!sensorOk) return leitura;

    leitura.temperatura = bme.readTemperature();
    leitura.pressao     = bme.readPressure() / 100.0F;
    leitura.altitude    = bme.readAltitude(SEALEVELPRESSURE_HPA);
    leitura.umidade     = bme.readHumidity();
    return leitura;
}

// =====================================================================
//  BOTÃO E LED
// =====================================================================
void atualizarBotao() {
    int leitura = digitalRead(BUTTON);

    if (leitura != botaoLeituraAnterior) {
        tempoUltimaMudanca = millis();
        botaoLeituraAnterior = leitura;
    }

    if (millis() - tempoUltimaMudanca >= DEBOUNCE_MS) {
        bool novoEstado = (leitura == LOW);
        if (novoEstado != botaoEstado) {
            botaoEstado = novoEstado;
            if (botaoEstado) botaoContagemPressoes++;
        }
    }
}

void setLed(bool ligado) {
    ledEstado = ligado;
    digitalWrite(LEDPIN, ligado ? HIGH : LOW);
}

// =====================================================================
//  GOOGLE SHEETS
// =====================================================================
bool salvarNoSheets(const DadosBME &d) {
    FirebaseJson response;
    FirebaseJson value;

    value.set("majorDimension", "ROWS");
    value.set("values/[0]/[0]", horaFormatada());
    value.set("values/[0]/[1]", d.temperatura);
    value.set("values/[0]/[2]", d.umidade);
    value.set("values/[0]/[3]", d.pressao);
    value.set("values/[0]/[4]", d.altitude);
    value.set("values/[0]/[5]", botaoEstado ? "PRESSIONADO" : "SOLTO");
    value.set("values/[0]/[6]", ledEstado   ? "LIGADO"      : "DESLIGADO");


    if (GSheet.values.append(&response, SPREADSHEET_ID, SHEET_RANGE, &value)) {
        Serial.println("Dado salvo com sucesso no Google Sheets!");
        return true;
    }

    Serial.printf("Erro ao salvar no Sheets: %s\n", GSheet.errorReason().c_str());
    return false;
}

// =====================================================================
//  ESTATÍSTICA
// =====================================================================

void acumularEstatisticas(const DadosBME &d) {
    if (estatTemp.n == 0) horaInicioJanela = horaFormatada();

    estatTemp.adicionar(d.temperatura);
    estatUmid.adicionar(d.umidade);
    estatPres.adicionar(d.pressao);
    estatAlt.adicionar(d.altitude);
}

String linhaRelatorioTexto(const char *nome, const char *un, const Estatistica &e) {
    char buf[128];
    snprintf(buf, sizeof(buf),
             "%-13s media %8.2f  dp %6.2f  min %8.2f  max %8.2f  %s\r\n",
             nome, e.media, e.desvioPadrao(), e.minimo, e.maximo, un);
    return String(buf);
}

String linhaRelatorioHtml(const char *nome, const char *un, const Estatistica &e) {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "<tr><td>%s (%s)</td><td align=\"right\">%.2f</td>"
             "<td align=\"right\">%.2f</td><td align=\"right\">%.2f</td>"
             "<td align=\"right\">%.2f</td></tr>",
             nome, un, e.media, e.desvioPadrao(), e.minimo, e.maximo);
    return String(buf);
}

// =====================================================================
//  E-MAIL
// =====================================================================
bool enviarRelatorioEstatistico() {
    if (estatTemp.n == 0) {
        Serial.println("Sem leituras na janela; relatorio nao enviado.");
        return false;
    }

    const uint32_t n       = estatTemp.n;
    const String   agora   = horaFormatada();
    const String   inicio  = horaInicioJanela.length() ? horaInicioJanela : agora;

    // ------------------------- corpo texto ---------------------------
    String texto;
    texto  = "Relatorio automatico da estacao ESP32\r\n";
    texto += "=====================================\r\n\r\n";
    texto += "Amostras: " + String(n) + "\r\n";
    texto += "Periodo:  " + inicio + "  ate  " + agora + "\r\n\r\n";
    texto += "Grandeza         Media       DP       Min        Max\r\n";
    texto += "-----------------------------------------------------------\r\n";
    texto += linhaRelatorioTexto("Temperatura", "C",   estatTemp);
    texto += linhaRelatorioTexto("Umidade",     "%",   estatUmid);
    texto += linhaRelatorioTexto("Pressao",     "hPa", estatPres);
    texto += linhaRelatorioTexto("Altitude",    "m",   estatAlt);
    texto += "\r\n";
    texto += "Botao: " + String(botaoEstado ? "PRESSIONADO" : "SOLTO");
    texto += "  (" + String(botaoContagemPressoes) + " toque(s) no periodo)\r\n";
    texto += "LED:   " + String(ledEstado ? "LIGADO" : "DESLIGADO") + "\r\n";
    if (!sensorOk) texto += "\r\nAVISO: BME280 nao detectado. Valores zerados.\r\n";

    // ------------------------- corpo HTML ----------------------------
    String html;
    html  = "<html><body style=\"font-family:Arial,sans-serif;color:#222\">";
    html += "<h2>Relatorio automatico da estacao ESP32</h2>";
    html += "<p><b>Amostras:</b> " + String(n) + "<br>";
    html += "<b>Periodo:</b> " + inicio + " &rarr; " + agora + "</p>";
    html += "<table border=\"1\" cellpadding=\"6\" cellspacing=\"0\" "
            "style=\"border-collapse:collapse\">";
    html += "<tr style=\"background:#f0f0f0\"><th>Grandeza</th><th>Media</th>"
            "<th>Desvio padrao</th><th>Minimo</th><th>Maximo</th></tr>";
    html += linhaRelatorioHtml("Temperatura", "C",   estatTemp);
    html += linhaRelatorioHtml("Umidade",     "%",   estatUmid);
    html += linhaRelatorioHtml("Pressao",     "hPa", estatPres);
    html += linhaRelatorioHtml("Altitude",    "m",   estatAlt);
    html += "</table>";
    html += "<p><b>Botao:</b> " + String(botaoEstado ? "PRESSIONADO" : "SOLTO") +
            " (" + String(botaoContagemPressoes) + " toque(s) no periodo)<br>";
    html += "<b>LED:</b> " + String(ledEstado ? "LIGADO" : "DESLIGADO") + "</p>";
    if (!sensorOk) html += "<p style=\"color:#b00\">AVISO: BME280 nao detectado.</p>";
    html += "</body></html>";

    // --------------------------- envio -------------------------------
    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };

    smtp.connect(SMTP_HOST, SMTP_PORT, statusCallback);
    if (!smtp.isConnected()) {
        Serial.println("Falha ao conectar no servidor SMTP.");
        return false;
    }

    smtp.authenticate(EMAIL_REMETENTE, EMAIL_SENHA_APP, readymail_auth_password);
    if (!smtp.isAuthenticated()) {
        Serial.println("Falha na autenticacao SMTP (verifique a senha de app).");
        return false;
    }

    SMTPMessage msg;
    msg.headers.add(rfc822_from, String("ESP32 <") + EMAIL_REMETENTE + ">");
    msg.headers.add(rfc822_to, EMAIL_DESTINO);
    msg.headers.add(rfc822_subject,
                    String("Relatorio de ") + n + " leituras - " + agora);
    msg.text.body(texto);
    msg.html.body(html);
    msg.timestamp = time(nullptr);

    Serial.println("Enviando relatorio por e-mail...");
    bool ok = smtp.send(msg);
    Serial.println(ok ? "Relatorio enviado." : "Falha no envio do relatorio.");
    return ok;
}

// =====================================================================
//  TELEGRAM
// =====================================================================
bool isAuthorized(const String &chat_id) {
    for (int i = 0; i < numUserAutorizado; i++) {
        if (chat_id == validoChatIds[i]) return true;
    }
    return false;
}

void VerificaMsgTele(int numMensagens) {
    for (int i = 0; i < numMensagens; i++) {
        String chat_id = bot.messages[i].chat_id;

        if (!isAuthorized(chat_id)) {
            bot.sendMessage(chat_id, "Usuario nao autorizado", "");
            continue;
        }

        String texto = bot.messages[i].text;

        if (texto == "/status") {
            DadosBME d = readSensorBME();
            String resposta = "📊 *Status Atual do ESP32*\n\n";
            resposta += "🌡️ Temperatura: " + String(d.temperatura, 2) + " °C\n";
            resposta += "💧 Umidade: "     + String(d.umidade, 2)     + " %\n";
            resposta += "🌍 Pressão: "     + String(d.pressao, 2)     + " hPa\n";
            resposta += "⛰️ Altitude: "    + String(d.altitude, 2)    + " m\n\n";
            resposta += "🔘 Botão: " + String(botaoEstado ? "PRESSIONADO" : "SOLTO") + "\n";
            resposta += "💡 LED: "   + String(ledEstado   ? "LIGADO"      : "DESLIGADO");
            if (!sensorOk) resposta += "\n\n⚠️ BME280 nao detectado.";
            bot.sendMessage(chat_id, resposta, "Markdown");
        }
        else if (texto == "/stats") {
            String r = "📈 *Janela atual*\n\n";
            r += "Amostras: " + String(estatTemp.n) + "/" +
                 String(LEITURAS_POR_RELATORIO) + "\n\n";
            r += "🌡️ " + String(estatTemp.media, 2) + " ± " +
                 String(estatTemp.desvioPadrao(), 2) + " °C\n";
            r += "💧 " + String(estatUmid.media, 2) + " ± " +
                 String(estatUmid.desvioPadrao(), 2) + " %\n";
            r += "🌍 " + String(estatPres.media, 2) + " ± " +
                 String(estatPres.desvioPadrao(), 2) + " hPa\n";
            r += "⛰️ " + String(estatAlt.media, 2) + " ± " +
                 String(estatAlt.desvioPadrao(), 2) + " m\n\n";
            r += "🔘 " + String(botaoContagemPressoes) + " toque(s) na janela";
            bot.sendMessage(chat_id, r, "Markdown");
        }
        else if (texto == "/led_on") {
            setLed(true);
            bot.sendMessage(chat_id, "💡 LED interno ligado com sucesso!", "");
        }
        else if (texto == "/led_off") {
            setLed(false);
            bot.sendMessage(chat_id, "🌙 LED interno desligado com sucesso!", "");
        }
        else if (texto == "/relatorio") {
            bot.sendMessage(chat_id, "Enviando relatorio por e-mail...", "");
            bool ok = enviarRelatorioEstatistico();
            bot.sendMessage(chat_id, ok ? "Relatorio enviado." : "Falha no envio.", "");
        }
        else {
            String ajuda = "Comando desconhecido. Use:\n";
            ajuda += "/status    - Leitura instantanea + botao e LED\n";
            ajuda += "/stats     - Media e desvio da janela atual\n";
            ajuda += "/led_on    - Liga o LED\n";
            ajuda += "/led_off   - Desliga o LED\n";
            ajuda += "/relatorio - Envia o relatorio por e-mail agora";
            bot.sendMessage(chat_id, ajuda, "");
        }
    }
}

String horaFormatada() {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    return String(buf);
}

void tokenStatusCallback(TokenInfo info) {
    if (info.status == token_status_error) {
        Serial.printf("Erro no token: %s\n", GSheet.getTokenError(info).c_str());
    } else {
        Serial.printf("Token: %s\n", GSheet.getTokenStatus(info).c_str());
    }
}