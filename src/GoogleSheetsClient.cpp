#include "GoogleSheetsClient.h"

#include <WiFi.h>
#include <time.h>

#include <ESP_Google_Sheet_Client.h>
#include <GS_SDHelper.h>

// =====================================================
// CONFIGURAÇÕES DO WI-FI
// =====================================================

#define WIFI_SSID "SEU_WIFI"
#define WIFI_PASSWORD "SUA_SENHA"


// =====================================================
// GOOGLE CLOUD
// =====================================================

// ID do projeto criado no Google Cloud
#define PROJECT_ID "SEU_PROJECT_ID"

// E-mail da Service Account
#define CLIENT_EMAIL "SEU_CLIENT_EMAIL"

// Private Key da Service Account
const char PRIVATE_KEY[] PROGMEM =
"-----BEGIN PRIVATE KEY-----\n"
"SUA_CHAVE_PRIVADA_AQUI\n"
"-----END PRIVATE KEY-----\n";


// =====================================================
// GOOGLE SHEETS
// =====================================================

// ID da planilha
const char SPREADSHEET_ID[] =
"SEU_SPREADSHEET_ID";


// =====================================================
// NTP
// =====================================================

const char* ntpServer = "pool.ntp.org";


// =====================================================
// CONTROLE DE TEMPO
// =====================================================

static unsigned long lastSendTime = 0;

// Enviar a cada 30 segundos
static const unsigned long SEND_INTERVAL = 30000;


// =====================================================
// PROTÓTIPOS
// =====================================================

static unsigned long getEpochTime();
static void tokenStatusCallback(TokenInfo info);


// =====================================================
// OBTÉM HORÁRIO
// =====================================================

static unsigned long getEpochTime()
{
    time_t now;
    struct tm timeinfo;

    if (!getLocalTime(&timeinfo))
    {
        Serial.println("[Google Sheets] Falha ao obter horário.");
        return 0;
    }

    time(&now);

    return (unsigned long)now;
}


// =====================================================
// CALLBACK DO TOKEN GOOGLE
// =====================================================

static void tokenStatusCallback(TokenInfo info)
{
    GSheet.printf(
        "Token info: type = %s, status = %s\n",
        GSheet.getTokenType(info).c_str(),
        GSheet.getTokenStatus(info).c_str()
    );

    if (info.status == token_status_error)
    {
        GSheet.printf(
            "Token error: %s\n",
            GSheet.getTokenError(info).c_str()
        );
    }
}


// =====================================================
// INICIALIZAÇÃO
// =====================================================

bool googleSheetsBegin()
{
    Serial.println();
    Serial.println("=================================");
    Serial.println("Inicializando Google Sheets");
    Serial.println("=================================");

    // -----------------------------------------------
    // Wi-Fi
    // -----------------------------------------------

    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);

    WiFi.begin(
        WIFI_SSID,
        WIFI_PASSWORD
    );

    Serial.print("Conectando ao Wi-Fi");

    unsigned long timeout = millis();

    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(500);

        // Timeout de 20 segundos
        if (millis() - timeout > 20000)
        {
            Serial.println();
            Serial.println("[ERRO] Não foi possível conectar ao Wi-Fi.");

            return false;
        }
    }

    Serial.println();
    Serial.println("Wi-Fi conectado.");

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());


    // -----------------------------------------------
    // Configuração do horário NTP
    // -----------------------------------------------

    configTime(
        0,
        0,
        ntpServer
    );

    Serial.println("Sincronizando horário...");

    unsigned long startTime = millis();

    while (getEpochTime() == 0)
    {
        delay(500);

        if (millis() - startTime > 15000)
        {
            Serial.println("[ERRO] Falha na sincronização NTP.");

            return false;
        }
    }

    Serial.println("Horário sincronizado.");


    // -----------------------------------------------
    // Google Sheets
    // -----------------------------------------------

    GSheet.printf(
        "ESP Google Sheet Client v%s\n",
        ESP_GOOGLE_SHEET_CLIENT_VERSION
    );

    // Callback usado para debug
    GSheet.setTokenCallback(
        tokenStatusCallback
    );

    // Atualiza token antes de expirar
    GSheet.setPrerefreshSeconds(
        10 * 60
    );

    // Inicia autenticação
    GSheet.begin(
        CLIENT_EMAIL,
        PROJECT_ID,
        PRIVATE_KEY
    );

    Serial.println("Google Sheets iniciado.");

    return true;
}


// =====================================================
// ENVIO PARA GOOGLE SHEETS
// =====================================================

bool googleSheetsSendData(
    float temperatura,
    float umidade,
    float pressao
)
{
    if (!GSheet.ready())
    {
        Serial.println(
            "[Google Sheets] Serviço ainda não está pronto."
        );

        return false;
    }

    FirebaseJson response;
    FirebaseJson valueRange;

    unsigned long timestamp = getEpochTime();


    // =================================================
    // ORGANIZA OS DADOS
    //
    // Coluna A = timestamp
    // Coluna B = temperatura
    // Coluna C = umidade
    // Coluna D = pressão
    // =================================================

    valueRange.add(
        "majorDimension",
        "COLUMNS"
    );

    valueRange.set(
        "values/[0]/[0]",
        timestamp
    );

    valueRange.set(
        "values/[1]/[0]",
        temperatura
    );

    valueRange.set(
        "values/[2]/[0]",
        umidade
    );

    valueRange.set(
        "values/[3]/[0]",
        pressao
    );


    Serial.println();
    Serial.println(
        "Enviando dados para Google Sheets..."
    );

    Serial.printf(
        "Timestamp: %lu\n",
        timestamp
    );

    Serial.printf(
        "Temperatura: %.2f C\n",
        temperatura
    );

    Serial.printf(
        "Umidade: %.2f %%\n",
        umidade
    );

    Serial.printf(
        "Pressao: %.2f hPa\n",
        pressao
    );


    // =================================================
    // ENVIA PARA A PLANILHA
    // =================================================

    bool success =
        GSheet.values.append(
            &response,
            SPREADSHEET_ID,
            "Sheet1!A1",
            &valueRange
        );


    if (success)
    {
        Serial.println(
            "[Google Sheets] Dados enviados!"
        );

        response.toString(
            Serial,
            true
        );

        valueRange.clear();

        return true;
    }


    // =================================================
    // ERRO
    // =================================================

    Serial.print(
        "[Google Sheets] ERRO: "
    );

    Serial.println(
        GSheet.errorReason()
    );

    return false;
}


// =====================================================
// LOOP DO GOOGLE SHEETS
// =====================================================

void googleSheetsLoop()
{
    // Necessário para processar autenticação
    bool ready = GSheet.ready();

    if (!ready)
        return;

    // Aqui não enviamos dados automaticamente,
    // apenas mantemos a autenticação funcionando.
}