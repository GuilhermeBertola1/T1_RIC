#ifndef GOOGLE_SHEETS_CLIENT_H
#define GOOGLE_SHEETS_CLIENT_H

#include <Arduino.h>

// Inicializa Wi-Fi, horário e autenticação Google
bool googleSheetsBegin();

// Deve ser chamada continuamente no loop principal
void googleSheetsLoop();

// Envia uma leitura manualmente para a planilha
bool googleSheetsSendData(
    float temperatura,
    float umidade,
    float pressao
);

#endif