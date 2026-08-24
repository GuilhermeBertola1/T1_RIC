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
#define PROJECT_ID "fit-bulwark-263523"
#define CLIENT_EMAIL "projeto-ric1@fit-bulwark-263523.iam.gserviceaccount.com"

const char PRIVATE_KEY[] PROGMEM =
"-----BEGIN PRIVATE KEY-----\n"
"MIIEvgIBADANBgkqhkiG9w0BAQEFAASCBKgwggSkAgEAAoIBAQDILmkfzHhVljz8\nEYIRw6IR52TnSP74y94qVOhcaF6+Sr0VDbmf1Gw34M5wCn5C8dgPXDzGloXWQSjb\n74M10R5pwUqq2hapq3M10YNkelri6OW191VfDL6mteJiVNp8qE0rrY9ULLFO1cHH\nlrs3+Fbal0FUj4GQFQd27aXC2odSBdSkD+HxVNVnoS4yZRJJpo996B8WJm02zMKr\n8UbE4NRcT/E5gU9wvm6eqrqitv3K3l4vNtLbeCTprr4EjjEIONYwxBffESQrgmHC\nW932W04ZSSYKXoUKcj9tmkd7uuvsYjVMlRRtv/fe9sZcHdv+FN9fYEybeb9G7dhI\n15r+xbEHAgMBAAECggEAFKj0O45toEkEW/BDUCm0iTbXAKcq0tWuRQiEqhOG17U8\npOFzliMlKh00YSGMHMT7YMEgb/nVWqKt9gkBreUPx6gQgz1FQoH0lDTejoOxlqKl\nIUMjbKRoQX9wcRKYhsNoojM5oYfdTX7H5Ef9DuPoCYu7zTTkZnuXzMvueYOhR13Y\nvqzO5ra3h9+UT9soAZ+hslGGRME8hluD6YbIkkYF6IrXFTuMPIQjFHIBM+6z/aQb\nXS7qyFf/aYCiJmWJ1QSCwWv7yK4But1nsCp/ciZHYxawwPJ69yqTz69w/BRNmw1u\nPUcpbkeWOJCJooEXEdCA348NuztLnjGTqY2wTLzHkQKBgQD5Dwy92Lm/Po1SCgEu\n7xuToTi7Q8MCD5vPV3dI6H/CZ7yztfWAizAfa0zYRvfQ4XHUTxp8OtGEJ5UUhNRo\nhCGK4LRUfZuK47K67LjgVCEKmT4IAiTcXdTFaL8aCo05wA85YlqUv9OXiZfWU2Zu\n4fTI8M/mZVLok/Vv71DzOy2+cQKBgQDNwqLvE3dxbakz3hN9o2O0PGUvVz+fUtD0\n9LTa0ULEARMbMfv5FSRzP4NKrVn1juSwIQ96QHyMwV3lnRUlmHjTTFMP+sdELVGI\nW4sfhXXshfnz5tWIy0okRrPnRTz/p3mcIB871QUhDTWf+hRhBVtWo6fMKGyjAUVv\nOhRj768S9wKBgQC7FnEN01WEsapa7CUDap4egccp9LbyP2Ndl2iFnhEKqf54SMFI\ndEYL9ABfnr/datylDV7p3w0v5sP4C+e7MzY3KqCLr2c+J9lEzTn7WXcQjRipz2J9\nHwcL26liGHcJ4JoPEzDT+F0rwjIKz89sj9PfdfXWLlrfVvXRxcXbUKMGIQKBgCTL\nBCWqHnaq/FGpCWUy/VYyYomXmWnc/j5/L4PdXYn9AzSeuIbuz+jH16tX2jk9xwjz\nsHfU5jCpNygi8bAaVqi6AjReTJmVXPn8HKGTVOXpgyupAFAIwgZAmlTlnaW9hdJ0\nMAGb8uHZIdiseE5tDcstUokARYTUal0cTp8cMfs7AoGBAKoeXKJiOwc3+kQZ6194\nsL2dNjaIwpy0lPkZfAoiKaJOvHp6j3sRGyhdkiXjRrKvw7+doEFAB4KHSHGyua2g\nt0mD6bgwRcMb5Vu3IDFHkDMbTHjxmgxlRbtNR4PzVT8n/Nj0ICIREmzG0Jm6jhgl\nypik9+5mhW3pvy6wwrRVawlO\n", 
"-----END PRIVATE KEY-----\n";

#define SPREADSHEET_ID "1c7nE1SQTm5SBzU9dcC4Gqow2yPVllpm7s7lgnBHRF9Y/edit?gid=0#gid=0"
const char *SPREADSHEET_ID = SPREADSHEET_ID;

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