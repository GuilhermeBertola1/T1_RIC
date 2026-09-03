# Estação de Monitoramento Ambiental IoT — ESP32 Heltec LoRa V2

Estação de telemetria ambiental construída sobre um ESP32. O dispositivo lê temperatura, umidade, pressão barométrica e altitude estimada de um sensor BME280 e distribui essa informação por **quatro canais simultâneos e independentes**, cada um resolvendo um problema diferente de acesso ao dado:

| Canal | Para quê serve | Latência |
|---|---|---|
| **Display OLED** | Diagnóstico local, sem rede | imediata |
| **Servidor web + gráfico** | Visualização em tempo real na rede local | ~1 s |
| **Google Sheets** | Histórico persistente, auditável e exportável | ~10 s |
| **Telegram** | Consulta e comando remotos, de fora da rede | sob demanda |
| **E-mail (SMTP)** | Notificação passiva de marco atingido | a cada 25 registros |

A ideia central do projeto é que um dado de sensor tem valores diferentes dependendo de *quando* e *onde* alguém precisa dele. Um gráfico ao vivo não substitui um histórico; um histórico não avisa ninguém sozinho; e nenhum dos dois funciona quando você está longe da rede local. Daí a arquitetura de múltiplos canais em paralelo.

---

## Sumário

- [Hardware](#hardware)
- [Arquitetura e fluxo de dados](#arquitetura-e-fluxo-de-dados)
- [Estrutura de arquivos](#estrutura-de-arquivos)
- [Configuração antes de compilar](#configuração-antes-de-compilar)
- [Bibliotecas](#bibliotecas)
- [Implementação detalhada](#implementação-detalhada)
  - [O modelo de execução: por que não existe `delay()` no loop](#o-modelo-de-execução-por-que-não-existe-delay-no-loop)
  - [`setup()` — ordem de inicialização e por que ela importa](#setup--ordem-de-inicialização-e-por-que-ela-importa)
  - [`readSensorBME()` — leitura e desacoplamento do hardware](#readsensorbme--leitura-e-desacoplamento-do-hardware)
  - [Canal 1 — Servidor web com Server-Sent Events](#canal-1--servidor-web-com-server-sent-events)
  - [Canal 2 — `salvarNoSheets()` e a autenticação OAuth 2.0](#canal-2--salvarnosheets-e-a-autenticação-oauth-20)
  - [Canal 3 — `enviarEmailAlerta()` via SMTP](#canal-3--enviaremailalerta-via-smtp)
  - [Canal 4 — `VerificaMsgTele()` e o long polling do Telegram](#canal-4--verificamsgtele-e-o-long-polling-do-telegram)
  - [Funções de apoio](#funções-de-apoio)
- [Consumo de memória e limitações conhecidas](#consumo-de-memória-e-limitações-conhecidas)
- [Roteiro de testes](#roteiro-de-testes)

---

## Hardware

| Item | Detalhe |
|---|---|
| Placa | Heltec WiFi LoRa 32 V2 (ESP32-D0WDQ6, 520 KB SRAM, 8 MB Flash) |
| Sensor | BME280 (temperatura, umidade relativa, pressão) — I²C, endereço `0x76` |
| Display | SSD1306 128×64 OLED, integrado à placa |
| LED | GPIO 25 (integrado) |
| Botão | GPIO 0 (PRG/BOOT) |

### Barramentos I²C

A placa Heltec já usa o barramento I²C primário (`Wire`, instância 0) para o OLED, em pinos fixos de fábrica. O BME280 é ligado a um **segundo barramento**, instanciado por software:

```cpp
TwoWire I2CBME = TwoWire(1);           // controlador I²C nº 1
I2CBME.begin(I2C_SDA, I2C_SCL, 100000); // SDA=21, SCL=22, 100 kHz
bme.begin(0x76, &I2CBME);              // aponta o driver para esse barramento
```

Isso evita disputa de barramento entre display e sensor, e permite clock independente (o SSD1306 tolera 400 kHz; sensores em cabo longo às vezes não).

| Periférico | Barramento | SDA | SCL | Reset |
|---|---|---|---|---|
| OLED SSD1306 | `Wire` (0) | 4 | 15 | 16 |
| BME280 | `I2CBME` (1) | 21 | 22 | — |

> **Portabilidade:** os GPIOs 22–25 **não existem** no ESP32-S3. Ao migrar para outra placa, `LEDPIN` e os pinos do OLED precisam ser remapeados.

---

## Arquitetura e fluxo de dados

```mermaid
flowchart LR
    BME[BME280<br/>I2C bus 1] --> ESP

    subgraph ESP[ESP32 · loop cooperativo por millis]
        direction TB
        T1[Timer 1 s]
        T2[Timer 10 s]
        T3[Timer 1,5 s]
        CNT[contador ≥ 25]
    end

    ESP --> OLED[Display OLED<br/>status local]
    T1  --> SSE[AsyncEventSource<br/>SSE /eventos]
    SSE --> WEB[Navegador<br/>Chart.js]
    T2  --> GS[Google Sheets API<br/>values.append]
    T2  --> CNT
    CNT --> MAIL[SMTP Gmail<br/>ReadyMail]
    T3  --> TG[Telegram Bot API<br/>getUpdates]
    TG  --> ESP
```

O ESP32 é **cliente** em três dos canais (Sheets, SMTP, Telegram) e **servidor** em um (HTTP + SSE). O Telegram é o único canal bidirecional: além de responder consultas, aceita comandos que atuam sobre o hardware.

---

## Estrutura de arquivos

```
projeto/
├── src/
│   └── main.cpp              # todo o firmware
├── data/                     # conteúdo gravado no LittleFS
│   ├── index.html            # página do dashboard
│   ├── style.css
│   ├── script.js             # cliente SSE + Chart.js
│   └── chart.umd.js          # Chart.js servido localmente
├── platformio.ini
└── README.md
```

A pasta `data/` é enviada separadamente do firmware, com **Upload Filesystem Image** (PlatformIO) ou **ESP32 Sketch Data Upload** (Arduino IDE). Se você gravar só o firmware, o servidor sobe mas devolve 404 em tudo.

O `chart.umd.js` fica hospedado no próprio ESP32, e não em CDN, de propósito: o dashboard precisa funcionar mesmo quando a rede local não tem saída para a internet.

---

## Configuração antes de compilar

### 1. Google Sheets — conta de serviço

O ESP32 não faz login com usuário e senha no Google. Ele usa uma **service account**: uma identidade de máquina que assina um token com criptografia assimétrica.

1. No [Google Cloud Console](https://console.cloud.google.com), crie (ou escolha) um projeto e anote o **Project ID**.
2. Ative a **Google Sheets API** nesse projeto.
3. Em *IAM & Admin → Service Accounts*, crie uma conta de serviço e gere uma chave no formato **JSON**.
4. Do JSON baixado, extraia três campos:
   - `project_id` → `PROJECT_ID`
   - `client_email` → `CLIENT_EMAIL`
   - `private_key` → `PRIVATE_KEY`
5. **Compartilhe a planilha** com o `client_email`, com permissão de **Editor**. Sem esse passo o token é válido mas a escrita retorna `403`.
6. Renomeie a aba de destino para `Dados` (sem acento — `Página1` gera problema de encoding na URL da API).

A chave privada entra no código com os `\n` preservados:

```cpp
const char PRIVATE_KEY[] PROGMEM =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIIEvQIBADANBgkqhkiG9w0BAQEFAASCBKcwggSjAgEAAoIBAQ...\n"
    "-----END PRIVATE KEY-----\n";
```

O `SPREADSHEET_ID` é **somente o ID**, extraído da URL:

```
https://docs.google.com/spreadsheets/d/1c7nE1SQTm5SBzU9dcC4Gqow2yPVllpm7s7lgnBHRF9Y/edit?gid=0
                                      └──────────── isto é o ID ────────────────────┘
```

### 2. E-mail — senha de aplicativo

O Gmail bloqueia autenticação com a senha da conta desde 2022. É preciso:

1. Ativar a verificação em duas etapas na conta.
2. Gerar uma **senha de app** de 16 caracteres em *Conta Google → Segurança → Senhas de app*.
3. Usar essa senha em `EMAIL_SENHA_APP`.

### 3. Telegram

1. Fale com o [@BotFather](https://t.me/BotFather), envie `/newbot` e guarde o token → `BOT_TOKEN`.
2. Envie qualquer mensagem ao seu bot e pegue o seu `chat_id` com o [@userinfobot](https://t.me/userinfobot) → `MY_ID`.
3. Acrescente outros IDs ao vetor `validoChatIds[]` se mais gente precisar de acesso.

---

## Bibliotecas

```ini
lib_deps =
    adafruit/Adafruit BME280 Library@^2.3.0
    thingpulse/ESP8266 and ESP32 OLED driver for SSD1306 displays@^4.6.2
    esp32async/ESPAsyncWebServer@^3.12.0
    esp32async/AsyncTCP@^3.5.0
    mobizt/ESP-Google-Sheet-Client@^1.4.13
    witnessmenow/UniversalTelegramBot@^1.3.0
    bblanchon/ArduinoJson@^7.2.2
    mobizt/FirebaseJson
    mobizt/ReadyMail@^0.4.2
```

> **Não instale `mobizt/ESP Mail Client` junto com `ReadyMail`.** As duas são do mesmo autor e definem os mesmos símbolos (`SMTPMessage`, `SMTPStatus`, `SMTPSession`/`SMTPClient`). O projeto usa exclusivamente a ReadyMail.

---

## Implementação detalhada

### O modelo de execução: por que não existe `delay()` no loop

Cinco tarefas precisam acontecer em cadências diferentes no mesmo núcleo, sem sistema operacional. Um `delay()` bloquearia tudo: o servidor web pararia de responder, o Telegram perderia mensagens, o watchdog do ESP32 poderia reiniciar a placa.

A solução é um **escalonador cooperativo** baseado em `millis()`. Cada tarefa guarda o instante em que rodou pela última vez e só executa de novo quando o intervalo passou:

```cpp
unsigned long tempoAtual = millis();

if (tempoAtual - tempoAnteriorSSE >= INTERVALO_SSE) {
    tempoAnteriorSSE = tempoAtual;
    // ... tarefa
}
```

A subtração `tempoAtual - tempoAnterior` com variáveis `unsigned long` é imune ao overflow de `millis()`, que ocorre a cada ~49,7 dias. Quando o contador dá a volta, a aritmética sem sinal ainda produz o intervalo correto — algo que uma comparação do tipo `if (tempoAtual >= tempoAnterior + INTERVALO)` não garante.

Os três intervalos foram escolhidos assim:

| Tarefa | Intervalo | Justificativa |
|---|---|---|
| SSE | 1 000 ms | Percepção de "tempo real" no gráfico sem saturar a rede |
| Google Sheets | 10 000 ms | A API tem cota por minuto; 6 escritas/min ficam confortáveis |
| Telegram | 1 500 ms | Resposta que parece instantânea sem esgotar o rate limit da Bot API |

As duas exceções ao não bloqueio são o envio de e-mail e as chamadas HTTPS síncronas (Sheets e Telegram). São operações que a biblioteca executa de forma bloqueante, na casa de centenas de milissegundos a alguns segundos. É uma limitação aceita conscientemente: nenhuma delas é frequente o bastante para prejudicar as demais.

---

### `setup()` — ordem de inicialização e por que ela importa

A sequência não é arbitrária. Cada etapa depende da anterior:

```
Serial → LittleFS → WiFi → NTP → TLS → Google Sheets → WebServer → OLED → BME280 → Telegram
```

**1. Serial e GPIO.** Console de diagnóstico a 115200 baud, `LEDPIN` como saída em nível baixo.

**2. LittleFS.**

```cpp
if (!LittleFS.begin(true)) { ... }
```

O argumento `true` é `formatOnFail`: se a partição estiver corrompida ou não formatada, ela é formatada automaticamente em vez de falhar. Isso salva a primeira gravação em uma placa nova.

**3. WiFi.** Modo `WIFI_STA` (estação, não ponto de acesso) e espera ativa até conectar:

```cpp
while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
}
WiFi.setAutoReconnect(true);
```

O `delay(500)` dentro do laço é obrigatório. Sem ele, o laço apertado impede que o watchdog de tarefa seja alimentado e a placa reinicia sozinha. O `setAutoReconnect(true)` faz a stack tentar reconectar automaticamente se o AP cair, sem intervenção do código.

**4. NTP — a etapa mais crítica e menos óbvia.**

```cpp
configTime(-3 * 3600, 0, "pool.ntp.org", "time.nist.gov");
while (time(nullptr) < 1700000000) { delay(300); }
```

O ESP32 não tem relógio de tempo real com bateria: ao ligar, o relógio começa em 1970. Três coisas quebram por causa disso:

- **O JWT do Google** carrega os campos `iat` (issued at) e `exp` (expiration). Com data de 1970, o Google rejeita o token com `invalid_grant`.
- **A validação TLS** verifica o período de validade dos certificados. Certificado emitido em 2024 é "do futuro" para um relógio em 1970.
- **O cabeçalho `Date` do e-mail** sai errado e vários servidores classificam a mensagem como spam.

A condição de parada compara com `1700000000` (novembro de 2023) em vez de zero. Isso distingue "relógio ainda não sincronizado" de "relógio sincronizado", porque valores intermediários pequenos podem aparecer durante a inicialização da stack.

**5. TLS.**

```cpp
clientTelegram.setInsecure();
clientEmail.setInsecure();
```

`setInsecure()` desliga a validação da cadeia de certificados: a conexão continua criptografada, mas o cliente não confirma a identidade do servidor. É a escolha pragmática aqui, já que embutir e manter atualizada a CA raiz de cada serviço custaria flash e manutenção. Em ambiente de produção exposto, o correto é usar `setCACert()` com o certificado raiz apropriado.

**6. Google Sheets.**

```cpp
GSheet.setTokenCallback(tokenStatusCallback);
GSheet.setPrerefreshSeconds(10 * 60);
GSheet.begin(CLIENT_EMAIL, PROJECT_ID, PRIVATE_KEY);
```

O `setPrerefreshSeconds(600)` manda a biblioteca renovar o token 10 minutos **antes** da expiração (tokens do Google duram 1 hora). Sem essa margem, uma escrita pode cair exatamente na janela de expiração e falhar.

**7. Servidor web.** Rotas registradas e servidor iniciado — detalhado na seção do Canal 1.

**8. OLED.** Reset por hardware via GPIO 16 (pulso baixo → alto) antes do `init()`, porque o SSD1306 da Heltec precisa dessa sequência para sair do estado indefinido do power-up.

**9. BME280.**

```cpp
sensorOk = bme.begin(0x76, &I2CBME);
```

O resultado vira uma **flag global** em vez de um `while(1)` travando o boot. A decisão de projeto é que a falha de um periférico não deve derrubar os outros quatro canais: sem sensor, o dispositivo ainda serve a página, responde no Telegram e reporta o problema, em vez de ficar morto sem explicação.

**10. Telegram.** Mensagem de boot como confirmação de que WiFi, TLS e credenciais estão todos funcionando — um teste de fumaça de ponta a ponta em uma linha.

---

### `readSensorBME()` — leitura e desacoplamento do hardware

```cpp
struct DadosBME { float temperatura, pressao, altitude, umidade; };

DadosBME readSensorBME() {
    DadosBME leitura = {0, 0, 0, 0};
    if (!sensorOk) return leitura;

    leitura.temperatura = bme.readTemperature();
    leitura.pressao     = bme.readPressure() / 100.0F;
    leitura.altitude    = bme.readAltitude(SEALEVELPRESSURE_HPA);
    leitura.umidade     = bme.readHumidity();
    return leitura;
}
```

Três decisões aqui:

**Struct em vez de variáveis globais.** As quatro grandezas viajam juntas por quatro canais diferentes. Agrupá-las garante que o gráfico, a planilha e a resposta do Telegram sempre mostrem uma leitura coerente entre si, e não uma mistura de instantes diferentes.

**Conversão de unidade na fonte.** O BME280 devolve pressão em pascal. A divisão por 100 converte para hectopascal (equivalente a milibar), a unidade usada em meteorologia. Fazendo a conversão em um único lugar, nenhum dos quatro consumidores precisa saber disso.

**Altitude é estimada, não medida.** `readAltitude()` aplica a fórmula barométrica internacional comparando a pressão lida com uma referência ao nível do mar:

```
h = 44330 × [1 − (P / P₀)^0,1903]
```

`SEALEVELPRESSURE_HPA` está em `1029.9`, calibrado para Sorocaba. O valor padrão de 1013,25 hPa é a atmosfera padrão, que só coincide com a realidade em condições específicas. **Esse número precisa ser recalibrado para o local de instalação**, ou a altitude sai com dezenas de metros de erro. Como a pressão atmosférica varia com o tempo, a leitura de altitude flutua mesmo com o sensor parado — é característica do método, não defeito.

**Quando a função é chamada.** Só dentro dos temporizadores, não a cada volta do `loop()`. Uma leitura completa do BME280 envolve várias transações I²C e leva alguns milissegundos; chamá-la milhares de vezes por segundo desperdiçaria ciclos sem nenhum ganho, já que os consumidores só olham o valor a cada 1 ou 10 segundos.

---

### Canal 1 — Servidor web com Server-Sent Events

#### Por que SSE e não polling

O caminho ingênuo seria o navegador consultar `/dados` a cada segundo. Cada consulta abriria uma conexão TCP, faria requisição e resposta HTTP completas e fecharia — sobrecarga alta para transportar 60 bytes de JSON, e com latência limitada pelo intervalo de consulta.

**Server-Sent Events** inverte a relação: o navegador abre **uma** conexão HTTP e a mantém aberta; o servidor empurra mensagens por ela quando quiser. É unidirecional (servidor → cliente), o que basta aqui, e tem reconexão automática embutida no navegador — se o ESP32 reiniciar, a página volta a receber sozinha, sem código de retry.

#### Servindo os arquivos estáticos

```cpp
server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", String(), false);
});
server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/style.css", "text/css");
});
```

Cada rota é uma **função lambda** registrada como callback. A `ESPAsyncWebServer` roda sobre a `AsyncTCP`, que trata as conexões em uma task separada — por isso o servidor continua respondendo mesmo enquanto o `loop()` está ocupado com uma chamada bloqueante ao Google.

O quarto parâmetro `false` em `send()` significa "não force download": o navegador renderiza o HTML em vez de baixá-lo.

#### O canal de eventos

```cpp
AsyncEventSource events("/eventos");

events.onConnect([](AsyncEventSourceClient *client) {
    Serial.println("Novo cliente conectado na interface web!");
});
server.addHandler(&events);
```

#### O envio periódico

```cpp
JsonDocument doc;
doc["temp"] = d.temperatura;
doc["pres"] = d.pressao;
doc["alti"] = d.altitude;
doc["humi"] = d.umidade;

String jsonString;
serializeJson(doc, jsonString);
events.send(jsonString.c_str(), "nova_leitura", millis());
```

`JsonDocument` sem tamanho fixo é a API da **ArduinoJson v7**, que aloca conforme a necessidade (na v6 era preciso dimensionar `StaticJsonDocument<N>` na mão).

Os três argumentos de `events.send()` são: **payload**, **nome do evento** e **id**. O nome `"nova_leitura"` permite ao cliente registrar um handler específico:

```javascript
const source = new EventSource('/eventos');
source.addEventListener('nova_leitura', (e) => {
    const d = JSON.parse(e.data);
    grafico.data.datasets[0].data.push(d.temp);
    grafico.update();
});
```

O `id` (aqui `millis()`) é usado pelo mecanismo de reconexão do SSE: ao reconectar, o navegador envia o cabeçalho `Last-Event-ID` com o último id recebido, permitindo que o servidor retome do ponto certo. Como aqui o dado é sempre o valor instantâneo, isso não é explorado, mas o campo é preenchido por conformidade com o protocolo.

O broadcast é para **todos** os clientes conectados — várias pessoas podem abrir o dashboard ao mesmo tempo.

---

### Canal 2 — `salvarNoSheets()` e a autenticação OAuth 2.0

#### O problema de autenticação

A API do Google Sheets exige OAuth 2.0. O fluxo comum (usuário clica, autoriza no navegador, recebe token) é inviável num microcontrolador sem interface. A alternativa é o fluxo **JWT Bearer para service account**, que a `ESP-Google-Sheet-Client` implementa por baixo:

1. O ESP32 monta um JWT com `iss` (o `client_email`), `scope` (`spreadsheets`), `aud` (endpoint de token do Google), `iat` e `exp`.
2. Assina esse JWT com **RS256**, usando a chave privada RSA da service account.
3. Troca o JWT assinado por um **access token** no endpoint `oauth2.googleapis.com/token`.
4. Usa o access token no header `Authorization: Bearer` das chamadas à API.
5. Repete a cada hora, quando o token expira.

Nada disso aparece no código do projeto — é o que `GSheet.begin()` e `GSheet.ready()` encapsulam. Mas é por isso que **o relógio precisa estar certo** (etapas 1 e 2) e por que **a placa precisa de fôlego de RAM** (a assinatura RS256 é a operação mais pesada do firmware).

#### `GSheet.ready()` no topo do loop

```cpp
bool sheetsPronto = GSheet.ready();
```

Esta chamada não é apenas uma consulta de estado: é ela que **executa a máquina de estados** de obtenção e renovação do token. Se não for chamada com frequência, o token nunca é gerado nem renovado, e as escritas falham em silêncio. Por isso ela fica no topo do `loop()`, fora de qualquer temporizador.

#### A escrita

```cpp
bool salvarNoSheets(const DadosBME &d) {
    FirebaseJson response;
    FirebaseJson value;

    value.set("majorDimension", "ROWS");
    value.set("values/[0]/[0]", horaFormatada());
    value.set("values/[0]/[1]", d.temperatura);
    value.set("values/[0]/[2]", d.umidade);
    value.set("values/[0]/[3]", d.pressao);
    value.set("values/[0]/[4]", d.altitude);

    if (GSheet.values.append(&response, SPREADSHEET_ID, SHEET_RANGE, &value)) {
        Serial.println("Dado salvo com sucesso no Google Sheets!");
        return true;
    }

    Serial.printf("Erro ao salvar no Sheets: %s\n", GSheet.errorReason().c_str());
    return false;
}
```

**`FirebaseJson` e a sintaxe de caminho.** A biblioteca usa uma notação de caminho com barras para construir JSON aninhado sem objetos intermediários. `"values/[0]/[2]"` produz:

```json
{
  "majorDimension": "ROWS",
  "values": [ [ "2026-09-03 14:22:10", 24.31, 61.20, 1013.44, 612.85 ] ]
}
```

`values` é uma matriz — uma lista de linhas, cada linha uma lista de células. Como só existe `[0]`, cada chamada acrescenta exatamente uma linha.

**`majorDimension: "ROWS"`** diz à API para interpretar cada vetor interno como uma linha horizontal. Com `"COLUMNS"`, os mesmos dados seriam gravados verticalmente.

**`values.append` e o range `Dados!A:E`.** O método `append` da API procura a primeira linha vazia dentro do intervalo e escreve ali — o ESP32 não precisa rastrear em que linha parou, o que sobreviveria inclusive a um reset. O range é uma *dica de tabela*, não um destino fixo: `A:E` sem números de linha indica as colunas de interesse.

O nome da aba é `Dados`, sem acento, deliberadamente. O range vai codificado na URL da requisição, e caracteres não-ASCII como o `á` de `Página1` são uma fonte recorrente de falha de encoding.

**Colunas gravadas:**

| A | B | C | D | E |
|---|---|---|---|---|
| Timestamp | Temperatura (°C) | Umidade (%) | Pressão (hPa) | Altitude (m) |

O timestamp é gerado **no dispositivo**, não pela planilha. Uma fórmula `NOW()` na planilha registraria o momento da escrita, que pode divergir do momento da leitura se houver retentativa de rede. Gravar a hora do ESP32 mantém o dado fiel ao instante da medição.

**Retorno booleano.** O valor de retorno é o que alimenta o contador do canal de e-mail: só uma gravação confirmada incrementa `contadorLeituras`. Se a rede cair, o contador não avança e nenhum alerta falso é disparado.

---

### Canal 3 — `enviarEmailAlerta()` via SMTP

#### O gatilho

```cpp
if (salvarNoSheets(d)) {
    contadorLeituras++;
    if (contadorLeituras >= LEITURAS_PARA_EMAIL) {
        enviarEmailAlerta();
        contadorLeituras = 0;
    }
}
```

O disparo é por **contagem de registros bem-sucedidos**, não por tempo. A diferença é relevante: com um gatilho temporal, o e-mail chegaria mesmo que a rede estivesse fora e nada tivesse sido gravado. Contando gravações confirmadas, a chegada do e-mail é em si a prova de que 25 registros entraram na planilha. Com o intervalo de 10 s, isso dá um e-mail a cada ~4 min 10 s.

#### A implementação

```cpp
bool enviarEmailAlerta() {
    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };

    smtp.connect(SMTP_HOST, SMTP_PORT, statusCallback);
    if (!smtp.isConnected()) { ... return false; }

    smtp.authenticate(EMAIL_REMETENTE, EMAIL_SENHA_APP, readymail_auth_password);
    if (!smtp.isAuthenticated()) { ... return false; }

    SMTPMessage msg;
    msg.headers.add(rfc822_from, String("ESP32 <") + EMAIL_REMETENTE + ">");
    msg.headers.add(rfc822_to, EMAIL_DESTINO);
    msg.headers.add(rfc822_subject, "Alerta: 25 Leituras Concluidas!");
    msg.text.body(String("O ESP32 acabou de registrar e salvar ") + LEITURAS_PARA_EMAIL +
                  " novas leituras no Google Sheets.\r\n" +
                  "Hora: " + horaFormatada() + "\r\n");
    msg.timestamp = time(nullptr);

    return smtp.send(msg);
}
```

**O cliente TLS vem no construtor.**

```cpp
WiFiClientSecure clientEmail;
SMTPClient smtp(clientEmail);
```

A ReadyMail não abre socket sozinha: ela recebe um cliente por injeção de dependência. Isso é o que torna a mesma biblioteca utilizável com `WiFiClientSecure`, `EthernetClient` ou qualquer outro transporte compatível — a lógica SMTP fica independente do meio físico.

**Porta 465 (SMTPS).** Conexão já criptografada desde o primeiro byte. A alternativa, porta 587 com STARTTLS, começa em texto claro e negocia a criptografia depois — mais uma etapa e mais um ponto de falha, sem ganho neste cenário.

**Callback de status.** Uma lambda que recebe `SMTPStatus` e imprime `status.text` no Serial. Fornece o diálogo SMTP passo a passo (`220 ready`, `250 OK`, `235 authenticated`, `354 start mail input`), que é praticamente a única forma prática de diagnosticar falha de autenticação ou rejeição pelo servidor.

**Verificação em duas etapas.** `isConnected()` e `isAuthenticated()` separam dois modos de falha distintos: problema de rede/TLS (não conectou) e credencial inválida (conectou mas não autenticou). Cada um pede uma correção diferente, e distingui-los no log poupa tempo.

**`readymail_auth_password`** seleciona autenticação por senha (`AUTH LOGIN`/`AUTH PLAIN`). A biblioteca também oferece OAuth2, desnecessário aqui já que a senha de app já é uma credencial de escopo restrito e revogável.

**Cabeçalhos RFC 822.** As constantes `rfc822_from`, `rfc822_to` e `rfc822_subject` são enums que mapeiam para os cabeçalhos definidos na RFC 822 (base do formato de e-mail na internet). O remetente segue o formato `Nome <endereço>`, que é o que os clientes de e-mail exibem como nome amigável.

**`\r\n` no corpo.** CRLF, não `\n` sozinho. A RFC exige CRLF como terminador de linha em SMTP; servidores mais rigorosos rejeitam a mensagem ou marcam como suspeita quando encontram LF isolado.

**`msg.timestamp = time(nullptr)`** preenche o cabeçalho `Date`. Faltando ou incorreto — o que aconteceria sem o NTP — é um forte indicador de spam para os filtros.

---

### Canal 4 — `VerificaMsgTele()` e o long polling do Telegram

#### Como o ESP32 recebe mensagens estando atrás de um NAT

Um bot do Telegram pode receber atualizações de dois jeitos: **webhook** (o Telegram chama uma URL pública sua) ou **polling** (você pergunta periodicamente). Webhook exige IP público, domínio e certificado válido — nada disso existe num ESP32 em rede doméstica. Então o dispositivo pergunta:

```cpp
int numMensagens = bot.getUpdates(bot.last_message_received + 1);
```

O argumento é o **offset**: "me dê as atualizações a partir deste id". Passando `last_message_received + 1`, o ESP32 confirma implicitamente o recebimento das anteriores, e o servidor do Telegram as descarta da fila. Sem isso, as mesmas mensagens voltariam para sempre.

#### O laço de drenagem

```cpp
int voltas = 0;
while (numMensagens && voltas++ < 5) {
    VerificaMsgTele(numMensagens);
    numMensagens = bot.getUpdates(bot.last_message_received + 1);
}
```

Uma consulta pode devolver várias mensagens (se chegaram em rajada durante o intervalo). O laço continua buscando até a fila esvaziar. O limite de **5 voltas** é uma trava de segurança: se algo mantiver a fila sempre cheia, o `loop()` volta a rodar em vez de ficar preso indefinidamente, prejudicando os outros canais.

#### Autorização

```cpp
bool isAuthorized(const String &chat_id) {
    for (int i = 0; i < numUserAutorizado; i++) {
        if (chat_id == validoChatIds[i]) return true;
    }
    return false;
}
```

O token do bot é o segredo, mas **qualquer pessoa que descubra o nome do bot pode mandar mensagem para ele**. Sem essa verificação, um desconhecido acenderia o LED e leria os dados do sensor. A checagem acontece antes de qualquer processamento de comando, e quem não está na lista recebe recusa explícita:

```cpp
if (!isAuthorized(chat_id)) {
    bot.sendMessage(chat_id, "Usuario nao autorizado", "");
    continue;
}
```

O tamanho da lista é calculado em tempo de compilação:

```cpp
const int numUserAutorizado = sizeof(validoChatIds) / sizeof(validoChatIds[0]);
```

Assim, acrescentar um ID ao vetor não exige tocar em mais nada.

#### Despacho de comandos

| Comando | Ação |
|---|---|
| `/status` | Lê o sensor na hora e devolve as quatro grandezas formatadas em Markdown |
| `/led_on` | `digitalWrite(LEDPIN, HIGH)` |
| `/led_off` | `digitalWrite(LEDPIN, LOW)` |
| `/email` | Dispara `enviarEmailAlerta()` sem esperar as 25 leituras — comando de teste |
| qualquer outro | Devolve a lista de comandos válidos |

```cpp
if (texto == "/status") {
    DadosBME d = readSensorBME();
    String resposta = "📊 *Status Atual do ESP32*\n\n";
    resposta += "🌡️ Temperatura: " + String(d.temperatura, 2) + " °C\n";
    // ...
    if (!sensorOk) resposta += "\n\n⚠️ BME280 nao detectado.";
    bot.sendMessage(chat_id, resposta, "Markdown");
}
```

O `/status` lê o sensor **naquele instante**, e não reaproveita o último valor do SSE — o usuário remoto pediu o estado agora, e é isso que recebe.

O terceiro argumento de `sendMessage()` é o modo de formatação. Com `"Markdown"`, o `*texto*` vira negrito no app. O `String(valor, 2)` fixa duas casas decimais, evitando a notação com seis dígitos que o `String(float)` produz por padrão.

A menção ao `sensorOk` na resposta fecha o ciclo da decisão tomada no `setup()`: em vez de a placa travar sem sensor, o problema é reportado pelo canal remoto, onde alguém pode agir.

---

### Funções de apoio

#### `horaFormatada()`

```cpp
String horaFormatada() {
    time_t now = time(nullptr);
    struct tm t;
    localtime_r(&now, &t);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &t);
    return String(buf);
}
```

Converte o timestamp Unix em texto legível. Usa `localtime_r` — a variante **reentrante**, que escreve num `struct tm` fornecido pelo chamador em vez de num buffer estático compartilhado. Como a `AsyncTCP` roda em outra task, a versão não reentrante poderia produzir corrupção de dados se duas tasks a chamassem ao mesmo tempo.

O formato `YYYY-MM-DD HH:MM:SS` (ISO 8601) é ordenável alfabeticamente, o que faz a planilha classificar corretamente por data mesmo tratando a coluna como texto.

#### `tokenStatusCallback()`

```cpp
void tokenStatusCallback(TokenInfo info) {
    if (info.status == token_status_error) {
        Serial.printf("Erro no token: %s\n", GSheet.getTokenError(info).c_str());
    } else {
        Serial.printf("Token: %s\n", GSheet.getTokenStatus(info).c_str());
    }
}
```

Registrada em `GSheet.setTokenCallback()`, é chamada a cada transição da máquina de estados de autenticação: `initialize` → `on_signing` → `on_request` → `on_refresh` → `ready`. Como o processo é assíncrono e opaco, esse log é a principal ferramenta de diagnóstico do canal Sheets — permite distinguir chave privada malformada de relógio dessincronizado ou de API não habilitada no projeto.

---

## Consumo de memória e limitações conhecidas

**Três clientes TLS simultâneos.** `clientTelegram`, `clientEmail` e o cliente interno da `ESP-Google-Sheet-Client` coexistem. Cada handshake TLS aloca buffers de alguns kilobytes, e a assinatura RS256 do JWT é a operação de pico. Se a placa reiniciar de forma aparentemente aleatória durante uma chamada de rede, esgotamento de heap é a primeira hipótese. Mitigação: chamar `stop()` no cliente de e-mail após o envio, liberando o buffer entre alertas.

**Chamadas de rede bloqueantes.** Uma escrita no Sheets ou um envio de e-mail congela o `loop()` por centenas de milissegundos a alguns segundos. O servidor web continua respondendo (roda na task da `AsyncTCP`), mas os temporizadores de SSE e Telegram atrasam. Solução mais robusta seria mover a telemetria para uma task própria via FreeRTOS.

**Altitude flutuante.** Consequência do método barométrico, não defeito. Ver [`readSensorBME()`](#readsensorbme--leitura-e-desacoplamento-do-hardware).

**`setInsecure()` no TLS.** Tráfego criptografado, servidor não autenticado. Aceitável em rede controlada; num deployment exposto, substituir por `setCACert()`.

**Sem buffer offline.** Se a rede cair, as leituras daquele período são perdidas — não há fila local. Uma evolução natural é enfileirar em LittleFS e drenar quando a conexão voltar.

**Cota da API do Google.** O plano gratuito limita as requisições de escrita por minuto por projeto. A 6 escritas/min o projeto fica folgado, mas reduzir muito `INTERVALO_SHEETS` esbarra no limite.

---

## Roteiro de testes

Cada canal é independente e pode ser validado isoladamente. Sugestão de ordem, do mais simples ao mais acoplado:

1. **Serial + OLED** — grave o firmware e confirme no monitor serial as mensagens de boot e o "Display OLED - OK" na tela.
2. **WiFi + NTP** — o IP deve aparecer no serial, e a hora sincronizada logo em seguida. Se o NTP não fechar, nada além disto vai funcionar.
3. **BME280** — confirme "Sensor BME280 - OK". Se falhar, verifique endereço (`0x76` ou `0x77`) e os pinos 21/22.
4. **Servidor web** — abra o IP no navegador; a página deve carregar e o gráfico começar a se mover em ~1 s. Se der 404, você esqueceu de subir a imagem do LittleFS.
5. **Google Sheets** — acompanhe os logs de token no serial. `Token: ready` seguido de "Dado salvo com sucesso" fecha o canal. Erro `403` significa planilha não compartilhada com o `client_email`.
6. **Telegram** — mande `/status` ao bot.
7. **E-mail** — use `/email` pelo Telegram em vez de esperar 25 leituras (~4 min). O diálogo SMTP completo aparece no serial.

Para testar Sheets e e-mail em uma placa sem BME280, o `sensorOk` já garante que o firmware sobe e roda com valores zerados.
