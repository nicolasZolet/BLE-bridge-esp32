
# 🔗 BLEBridge — Ponte Serial ↔️ BLE

## 🚀 Descrição
O **BLEBridge** é um firmware para ESP32 que permite criar uma ponte de comunicação entre uma interface **Serial TTL (UART)** e um dispositivo BLE (**Bluetooth Low Energy**). 

Toda mensagem recebida pelo **BLE** é transmitida pela **Serial2**, e toda mensagem recebida pela **Serial2** é enviada pelo **BLE**.

Permite também configuração dinâmica do nome do dispositivo BLE e dos UUIDs via comandos pela Serial.

---

## 📜 Protocolo de Comunicação

### 🔗 **Formato dos Pacotes**
- Todos os pacotes são delimitados por:
```
[ ... ]
```
- Exemplo de pacote válido:
```
[CONFIG:NAME=Scheer Firetech;SVC=30e3d633-a01d-4005-90f5-0754c9c5891f;RX=c5b922f2-f00d-4026-beb3-cdf0c00a5a41;TX=db6da158-884e-4977-ace1-a3af800bae6d]
```

---

## ⚙️ Comandos Serial → BLEBridge

| Comando | Descrição |
|---------|------------|
| `[CONFIG:NAME=<nome>;SVC=<serviceUUID>;RX=<rxUUID>;TX=<txUUID>]` | 🔧 Configura nome e UUIDs do BLE. Salva na memória e reinicia. |
| `[VERSION]` | 🔖 Retorna a versão atual do firmware. |
| `[RESET_CONFIG]` | 🧹 Limpa as configurações salvas (Preferences) e reinicia. |
| `[<qualquer dado>]` | 🚀 Qualquer dado dentro de `[...]` será enviado via BLE se houver conexão. |

---

## 🔔 Respostas BLEBridge → Serial

| Mensagem | Descrição |
|----------|-----------|
| `STATUS:BLE_CONNECTED` | ✅ Cliente BLE conectado. |
| `STATUS:BLE_DISCONNECTED` | ⚠️ Cliente BLE desconectado. |
| `OK:BLE_ADVERTISING_STARTED` | 🔵 BLE está em modo advertising. |
| `OK:CONFIG_SAVED_RESTARTING` | ✅ Configuração salva, dispositivo reiniciando. |
| `OK:FIRMWARE:BLE_BRIDGE_V1.0.0` | 🔖 Versão atual do firmware. |
| `OK:CONFIG_CLEARED_RESTARTING` | 🧹 Configuração apagada, reiniciando. |
| `ERROR:INVALID_CONFIG_FORMAT` | ❌ Formato de configuração inválido. |
| `ERROR:INPUT_OVERFLOW` | ❌ Dados recebidos excederam o limite permitido (256 bytes). |
| `ERROR:SERIAL_TIMEOUT` | ⚠️ Timeout aguardando fechamento do pacote (`]`). |

---

## 💾 Memória Persistente (Preferences)

- O BLEBridge salva as seguintes informações na memória flash:
  - 🔹 Nome do dispositivo BLE (`NAME`)
  - 🔹 Service UUID (`SVC`)
  - 🔹 RX UUID (`RX`)
  - 🔹 TX UUID (`TX`)

Se não houver dados salvos, ele utiliza os valores padrão:
- Name: `Scheer Firetech`
- Service UUID: `30e3d633-a01d-4115-90f5-0754c9c5891f`
- RX UUID: `c5b922f2-f58d-4026-beb3-cdf0c83a5a41`
- TX UUID: `db6da158-884e-4977-ace1-a3af822bae6d`

---

## 📦 Arquitetura do Projeto

```
.
├── src/
│   ├── BLEBridge.cpp     # Implementação da ponte BLE ↔️ Serial
│   ├── BLEBridge.h       # Header da classe BLEBridge
│   └── main.cpp          # Inicialização principal
├── include/
│   └── (headers se necessário)
├── platformio.ini        # Configuração PlatformIO
├── README.md             # Documentação
```

---

## 🚀 Exemplo Completo de Uso

1️⃣ Enviar configuração:
```
[CONFIG:NAME=Scheer Firetech;SVC=30e3d633-a01d-4115-90f5-0754c9c5891f;RX=c5b922f2-f58d-4026-beb3-cdf0c83a5a41;TX=db6da158-884e-4977-ace1-a3af822bae6d]
```

2️⃣ Após reinício, enviar dados:
```
[8040862A13D8,GL,1,0,RT,0,LP,0,FW,1.1.11,BM,PLUS,DM,0,WF,1]
```
→ Dados são enviados para qualquer cliente BLE conectado.

3️⃣ Receber status:
```
STATUS:BLE_CONNECTED
STATUS:BLE_DISCONNECTED
```

---

## 🔥 Cuidados

- ✔️ Sempre enviar dados dentro de `[ ... ]`.
- ✔️ Máximo de 256 caracteres por pacote.
- ✔️ Se enviar `[` sem `]`, ocorre timeout e descarte do pacote.
- ✔️ Reset do ESP ocorre automaticamente após configuração.

---

## 📜 Licença

Este projeto é de uso privado e controlado pela empresa **Scheer Firetech**.
