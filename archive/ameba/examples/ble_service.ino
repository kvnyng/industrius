/*

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-ble-battery/
 */

// Raw test, read from flash the sample image, and then send as raw bytes.

#include "BLEDevice.h"

#define BATT_SERVICE_UUID "180F"
#define BATT_LEVEL_UUID "2A19"

BLEService battService(BATT_SERVICE_UUID);
BLECharacteristic battChar(BATT_LEVEL_UUID);
BLEAdvertData advdata;
BLEAdvertData scandata;
uint8_t battlevel = 0;
bool notify = false;

void readCB(BLECharacteristic *imageChr, uint8_t connID)
{
    printf("Characteristic %s read by connection %d \n", imageChr->getUUID().str(), connID);
    imageChr->writeData8(90);
}

void notifCB(BLECharacteristic *imageChr, uint8_t connID, uint16_t cccd)
{
    if (cccd & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
    {
        printf("Notifications enabled on Characteristic %s for connection %d \n", imageChr->getUUID().str(), connID);
        notify = true;
    }
    else
    {
        printf("Notifications disabled on Characteristic %s for connection %d \n", imageChr->getUUID().str(), connID);
        notify = false;
    }
}

void setup()
{
    Serial.begin(115200);

    advdata.addFlags();
    advdata.addCompleteName("AMEBA_BLE_DEV");
    scandata.addCompleteServices(BLEUUID("180F"));

    battChar.setReadProperty(true);
    battChar.setReadPermissions(GATT_PERM_READ);
    battChar.setReadCallback(readCB);
    battChar.setNotifyProperty(true);
    battChar.setCCCDCallback(notifCB);

    battService.addCharacteristic(battChar);

    BLE.init();
    BLE.configAdvert()->setAdvData(advdata);
    BLE.configAdvert()->setScanRspData(scandata);
    BLE.configServer(1);
    BLE.addService(battService);

    BLE.beginPeripheral();
}

void loop()
{
    battlevel = (battlevel + 1) % 100;
    battChar.writeData8(battlevel);
    if (BLE.connected(0) && notify)
    {
        battChar.notify(0);
    }
    Serial.println(battlevel);
    delay(1000);
}
