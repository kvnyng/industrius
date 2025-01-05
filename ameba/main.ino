#include "BLEDevice.h"
#include "VideoStream.h"

// BLE UUIDs for the GATT service and characteristic
#define IMG_SERVICE_UUID "12345678-1234-5678-1234-56789abcdef0"
#define IMG_CHAR_UUID "12345678-1234-5678-1234-56789abcdef1"

// BLE Service and Characteristic
BLEService imgService(IMG_SERVICE_UUID);
BLECharacteristic imgChar(IMG_CHAR_UUID);
BLEAdvertData advData;
BLEAdvertData scanData;

// Camera settings
#define CHANNEL 0
VideoSetting config(VIDEO_VGA, CAM_FPS, VIDEO_JPEG, 1); // VGA resolution for simplicity

uint32_t img_addr = 0;
uint32_t img_len = 0;
bool notify = false;

// BLE Callbacks
void readCB(BLECharacteristic *chr, uint8_t connID)
{
    printf("Characteristic %s read by connection %d\n", chr->getUUID().str(), connID);

    // Capture an image
    Camera.getImage(CHANNEL, &img_addr, &img_len);

    // Split the image into chunks and write each chunk to the characteristic buffer
    const uint16_t CHUNK_SIZE = CHAR_VALUE_MAX_LEN; // Max size defined in BLECharacteristic
    uint8_t *img_ptr = (uint8_t *)img_addr;
    uint16_t bytes_remaining = img_len;

    while (bytes_remaining > 0)
    {
        Serial.print("Writing chunks. Bytes Remaining:");
        Serial.println(bytes_remaining);
        uint16_t chunk_len = (bytes_remaining > CHUNK_SIZE) ? CHUNK_SIZE : bytes_remaining;

        // Write the current chunk to the characteristic
        if (!chr->setData(img_ptr, chunk_len))
        {
            printf("Failed to write chunk to characteristic.\n");
            break;
        }

        // Notify client with the current chunk
        chr->notify(connID);

        // Advance the pointer and decrease the remaining byte count
        img_ptr += chunk_len;
        bytes_remaining -= chunk_len;

        delay(10); // Small delay to allow client to process notifications
    }

    printf("Image transfer complete. Total size: %lu bytes\n", img_len);
}

void notifCB(BLECharacteristic *chr, uint8_t connID, uint16_t cccd)
{
    if (cccd & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
    {
        printf("Notifications enabled on Characteristic %s for connection %d\n", chr->getUUID().str(), connID);
        notify = true;
    }
    else
    {
        printf("Notifications disabled on Characteristic %s for connection %d\n", chr->getUUID().str(), connID);
        notify = false;
    }
}

void setup()
{
    Serial.begin(115200);

    // Configure BLE characteristic
    imgChar.setReadProperty(true);
    imgChar.setReadPermissions(GATT_PERM_READ);
    imgChar.setBufferLen(CHAR_VALUE_MAX_LEN); // Ensure the buffer can handle the maximum characteristic value length
    imgChar.setNotifyProperty(true);
    imgChar.setReadCallback(readCB);
    imgChar.setCCCDCallback(notifCB);

    // Add the characteristic to the service
    imgService.addCharacteristic(imgChar);

    // Advertise the BLE service
    advData.addFlags();
    advData.addCompleteName("ImageTransferBLE");
    scanData.addCompleteServices(BLEUUID(IMG_SERVICE_UUID));

    // Initialize BLE
    BLE.init();

    BLE.configAdvert()->setAdvData(advData);
    BLE.configAdvert()->setScanRspData(scandata);
    BLE.configServer(1);

    // Add the service to the BLE device
    BLE.addService(imgService);

    // Start BLE peripheral mode
    BLE.beginPeripheral();

    // Initialize the camera
    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();
    Camera.channelBegin(CHANNEL);

    Serial.println("Setup complete, BLE and Camera initialized.");
}

void loop()
{
    // Handle BLE notifications
    if (BLE.connected(0) && notify)
    {
        // Capture an image
        Serial.print("Grabbing image");
        Camera.getImage(CHANNEL, &img_addr, &img_len);

        // Send the image in chunks via notifications
        readCB(&imgChar, 0);

        delay(1000); // Add delay to reduce CPU usage
    }
}