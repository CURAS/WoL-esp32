#define BLINKER_PRINT Serial
#define BLINKER_WIFI
#define BLINKER_APCONFIG
#define BLINKER_MIOT_OUTLET

#include <Blinker.h>
#include <WiFiUdp.h>
#include <WakeOnLan.h>
#include <EEPROM.h>

const uint8_t eeprom_header[] = { 0x00, 0xFF, 0x4D, 0x41, 0x43 };
const char mac_addr_init[] = "FF:FF:FF:FF:FF:FF";
const char auth[] = "************"; // blinker secret key

WiFiUDP UDP;
WakeOnLan WOL(UDP);
BlinkerButton Button1("btn-wol");
char MACAddress[64];
bool wsState = false;

void button1_callback(const String & state) {
    BLINKER_LOG("Blinker get button state: ", state);

    WOL.sendMagicPacket(MACAddress);

    Blinker.print("result", "success");
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
    }
}

void dataRead(const String & data) {
    BLINKER_LOG("Blinker read string: ", data);

    EEPROM.begin(4096);
    data.toCharArray(MACAddress, 64);
    EEPROM.put(3200 + 5, MACAddress);
    EEPROM.commit();

    Blinker.print("MAC", MACAddress);
    for (int i = 0; i < 1; i++) {
        digitalWrite(LED_BUILTIN, LOW);
        delay(1000);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(1000);
    }
}

String summary() {
    return STRING_format(MACAddress);
}

void miotPowerState(const String & state) {
    BLINKER_LOG("MIOT Set power state: ", state);

    if (state == BLINKER_CMD_ON) {
        WOL.sendMagicPacket(MACAddress);

        BlinkerMIOT.powerState("on");
        BlinkerMIOT.print();
        wsState = true;
    }
    else if (state == BLINKER_CMD_OFF) {
        BlinkerMIOT.powerState("off");
        BlinkerMIOT.print();
        wsState = false;
    }
}

void miotQuery(int32_t queryCode) {
    BLINKER_LOG("MIOT Query codes: ", queryCode);

    switch (queryCode) {
        case BLINKER_CMD_QUERY_POWERSTATE_NUMBER:
            BLINKER_LOG("MIOT Query Power State");
            BlinkerMIOT.powerState(wsState ? "on" : "off");
            BlinkerMIOT.print();
            break;
        default:
            BlinkerMIOT.powerState(wsState ? "on" : "off");
            BlinkerMIOT.print();
            break;
    }
}

bool checkEEPROMHeader() {
    Serial.print("Checking EEPROM ...\n");
    for (int i = 0; i < 5; i++) {
        Serial.printf("%d -> %d\n", EEPROM.read(3200 + i), eeprom_header[i]);
        if (EEPROM.read(3200 + i) != eeprom_header[i]) {
            return false;
        }
    }

    return true;
}

void initEEPROM() {
    Serial.print("Initializing EEPROM ...\n");

    for (int i = 0; i < 5; i++) {
        EEPROM.write(3200 + i, eeprom_header[i]);
    }

    EEPROM.put(3200 + 5, mac_addr_init);
    EEPROM.commit();
}

void setup() {
    // 初始化串口
    Serial.begin(9600);

    #ifdef BLINKER_PRINT
        BLINKER_DEBUG.stream(BLINKER_PRINT);
    #endif
    
    // 初始化内置LED的IO
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    // 初始化blinker
    Blinker.begin(auth);
    Blinker.attachSummary(summary); 
    Blinker.attachData(dataRead);
    Button1.attach(button1_callback);

    // 小爱同学注册回调函数
    BlinkerMIOT.attachPowerState(miotPowerState);
    BlinkerMIOT.attachQuery(miotQuery);

    // 重复发送3次，间隔100ms
    WOL.setRepeat(3, 100);

    // 读取EEPROM中配置的MAC
    EEPROM.begin(4096);
    if (!checkEEPROMHeader()) {
        initEEPROM();
    }
    EEPROM.get(3200 + 5, MACAddress);
    Serial.printf("MAC: %s\n", MACAddress);
}

void loop() {
    Blinker.run();
}