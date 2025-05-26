# 📘 ESP32 BLE ↔ Serial Bridge

## 🚀 Descrição
Este firmware transforma um ESP32 em uma ponte BLE ↔ Serial, permitindo comunicação bidirecional entre dispositivos BLE e outro microcontrolador conectado via UART (Serial2).

- ✅ Permite alteração dinâmica de nome, UUIDs e configuração do BLE.
- ✅ Watchdog ativo para proteção contra travamentos.
- ✅ Proteção contra overflow da UART.
- ✅ Fail-safe para garantir que o advertising BLE nunca pare.

## 🛠️ Configuração de Hardware

| Função | ESP32 GPIO |
|--------|-------------|
| **RX2** (recebe do ESP principal) | GPIO 16 |
| **TX2** (envia para o ESP principal) | GPIO 17 |

## 🔌 Configuração de Software

- ✔️ Framework: PlatformIO ou Arduino IDE.
- ✔️ Biblioteca BLE utilizada: [`NimBLE-Arduino`](https://github.com/h2zero/NimBLE-Arduino).

## 🔥 Mapa de Comandos (Serial2 → ESP32)

| Comando         | Descrição                                                | Exemplo                                    | Resposta                             |
|-----------------|----------------------------------------------------------|--------------------------------------------|--------------------------------------|
| `NAME:<name>`   | Atualiza o nome do dispositivo BLE                       | `NAME:BLE_TEST`                            | `OK:NAME_UPDATED:BLE_TEST`           |
| `SVC:<uuid>`    | Atualiza o UUID do serviço BLE                           | `SVC:6E400001-B5A3-F393-E0A9-E50E24DCCA9E` | `OK:SERVICE_UUID_UPDATED:<uuid>`     |
| `RX:<uuid>`     | Atualiza o UUID da characteristic RX                     | `RX:6E400002-B5A3-F393-E0A9-E50E24DCCA9E`  | `OK:SERVICE_UUID_RX_UPDATED:<uuid>`  |
| `TX:<uuid>`     | Atualiza o UUID da characteristic TX                     | `TX:6E400003-B5A3-F393-E0A9-E50E24DCCA9E`  | `OK:SERVICE_UUID_TX_UPDATED:<uuid>`  |
| `BOND:CLEAR`    | Remove todos os dispositivos emparelhados (bonds)        |                                            | `OK:BONDS_CLEARED`                   |
| `RESET`         | Reinicia o ESP32                                         |                                            | `OK:RESETTING`                       |
| `VERSION`       | Retorna a versão do firmware                             |                                            | `OK:FIRMWARE:BLE_BRIDGE_V1.0.0`      |
| `<texto livre>` | Dados enviados para o cliente BLE conectado              | `Hello BLE`                                | - (Enviado via BLE)                  |

## 🔁 Mensagens automáticas da ponte BLE → Serial2

| Evento                                    | Mensagem enviada na Serial2      |
|-------------------------------------------|----------------------------------|
| ESP32 iniciado                            | `BLE_BRIDGE_STARTED`             |
| Advertising BLE iniciado                  | `OK:BLE_ADVERTISING_STARTED`     |
| Cliente BLE conectado                     | `STATUS:BLE_CONNECTED`           |
| Cliente BLE desconectado                  | `STATUS:BLE_DISCONNECTED`        |
| Advertising reiniciado (fail-safe)        | `WARNING:ADVERTISING_RESTARTED`  |
| BLE não conectado (ao tentar enviar dado) | `ERROR:BLE_NOT_CONNECTED`        |
| Overflow na UART Serial2                  | `ERROR:INPUT_OVERFLOW`           |

## ⚙️ Comportamento BLE ↔ Serial

### ✔️ BLE → Serial2
- Todo dado recebido na characteristic RX do BLE é **imediatamente enviado via Serial2**.

### ✔️ Serial2 → BLE
- Se o ESP32 estiver com um cliente BLE conectado, qualquer linha recebida na Serial2 (**terminada por `\n`**) será enviada via BLE na characteristic TX.
- Se não houver cliente conectado, retorna `ERROR:BLE_NOT_CONNECTED`.

## 🏗️ Fail-safes implementados

- ✔️ **Watchdog:** reinicia o ESP em caso de travamento de software ou sobrecarga.
- ✔️ **Proteção contra overflow:** se um comando exceder 256 caracteres sem um `\n`, o buffer é limpo e retorna `ERROR:INPUT_OVERFLOW`.
- ✔️ **Advertising fail-safe:** Se por qualquer motivo o advertising parar, ele é automaticamente reiniciado e notificado com `WARNING:ADVERTISING_RESTARTED`.

## 🧠 Exemplo de Fluxo Operacional

1️⃣ ESP32 inicializa:
```
BLE_BRIDGE_STARTED
OK:BLE_ADVERTISING_STARTED
```

2️⃣ Cliente BLE conecta:
```
STATUS:BLE_CONNECTED
```

3️⃣ Envia dados via Serial2:
```
Hello BLE\n
```
→ **Dado é transmitido via BLE**

4️⃣ Cliente desconecta:
```
STATUS:BLE_DISCONNECTED
```

## 🔩 Sugestão de Integração (Microcontrolador Principal)

- Enviar comandos como strings terminadas com `\n`.
- Ler a Serial2 continuamente para capturar:
  - Logs de status (`STATUS:*`).
  - Confirmações (`OK:*`).
  - Erros (`ERROR:*`).
  - Dados recebidos via BLE (texto puro).

## 🔥 Comandos mínimos recomendados para inicialização:
```
NAME:BLE_BRIDGE_01
SVC:6E400001-B5A3-F393-E0A9-E50E24DCCA9E
RX:6E400002-B5A3-F393-E0A9-E50E24DCCA9E
TX:6E400003-B5A3-F393-E0A9-E50E24DCCA9E
```

## 🏆 Autor e Manutenção
Desenvolvido por Nicolas Zolet.

Este firmware foi projetado para ser robusto, seguro e de alta disponibilidade para aplicações industriais, automação, sensores, gateways BLE ↔ UART e muito mais.