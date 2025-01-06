#include "BLEDevice.h"
#include "VideoStream.h"
#include <ArduinoJson.h>
#include "image.pb.h"
#include <SnappyProto.h>

// Device Information Service UUID
#define DEVICE_INFORMATION_SERVICE_UUID "180a"

// Device Information Characteristics UUIDs
// // Device Information Characteristics UUIDs
#define MANUFACTURER_NAME_UUID "2a29"
#define MODEL_NUMBER_UUID "2a24"
#define SERIAL_NUMBER_UUID "2a25"
#define HARDWARE_REVISION_UUID "2a27"
#define FIRMWARE_REVISION_UUID "2a26"
#define SOFTWARE_REVISION_UUID "2a28"
#define SYSTEM_ID_UUID "2a23"

// // BLE UUIDs for the GATT service and characteristic
#define IMG_SERVICE_UUID "12345678-0000-0001-1234-56789abcdef0"
#define IMG_CHAR_IMAGE_METADATA "12345678-0000-0002-1234-56789abcdef0"
#define IMG_CHAR_STREAM "12345678-0000-0001-1234-56789abcdef0"

#define DEVICE_INFO_MANUFACTURER_NAME "Industrius"
#define DEVICE_INFO_MODEL_NUMBER "1"
#define DEVICE_INFO_SERIAL_NUMBER "0001"
#define DEVICE_INFO_HARDWARE_REVISION "1"
#define DEVICE_INFO_FIRMWARE_REVISION "1"
#define DEVICE_INFO_SOFTWARE_REVISION "1"
#define DEVICE_INFO_SYSTEM_ID "1"

#define MTU_SIZE 230

uint16_t DEVICE_APPEARANCE = 900; // Camera appearance value for BLE

// BLE Service and Characteristic
// // Device Information Service
BLEService deviceInfoService(BLEUUID(DEVICE_INFORMATION_SERVICE_UUID));

BLECharacteristic deviceInfoChar_manufacturerName(MANUFACTURER_NAME_UUID);
BLECharacteristic deviceInfoChar_modelNumber(MODEL_NUMBER_UUID);
BLECharacteristic deviceInfoChar_serialNumber(SERIAL_NUMBER_UUID);
BLECharacteristic deviceInfoChar_hardwareRevision(HARDWARE_REVISION_UUID);
BLECharacteristic deviceInfoChar_firmwareRevision(FIRMWARE_REVISION_UUID);
BLECharacteristic deviceInfoChar_softwareRevision(SOFTWARE_REVISION_UUID);
BLECharacteristic deviceInfoChar_systemID(SYSTEM_ID_UUID);

// // Image Service
BLEService imgService(BLEUUID(IMG_SERVICE_UUID));
// BLECharacteristic imgCharMetadata(BLEUUID(IMG_CHAR_IMAGE_METADATA));
BLECharacteristic imgChar(BLEUUID(IMG_CHAR_STREAM));

// Advertising Data
BLEAdvertData advMainData;
BLEAdvertData advImageData;
BLEAdvertData advDeviceInfoData;

#define DEVICE_NAME_LONG "Neckie"
#define DEVICE_NAME_SHORT "Neck"

// Camera settings
#define CHANNEL 0
VideoSetting config(VIDEO_VGA, 10, VIDEO_JPEG, 1); // VGA resolution for simplicity

uint32_t img_addr = 0;
uint32_t img_len = 0;
bool notify = false;

unsigned long previousMillis = 0;        // Stores the last time the LED was toggled
const unsigned long flashInterval = 10; // Interval at which to flash the LED (in milliseconds)

struct ChunkData
{
    uint8_t *data;
    size_t len;
};


#define PART_BOUNDARY "123456789000000000000987654321"
char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
char *IMG_HEADER = "Content-Type: image/jpeg\r\nContent-Length: %lu\r\n\r\n";

void send(BLECharacteristic *imageChr, uint8_t *buf, uint32_t len)
{
    // if the data is larger than the MTU size, split it into chunks and send
    if (len > MTU_SIZE - 3)
    {
        uint8_t *ptr = buf;
        uint32_t remaining = len;
        while (remaining > 0)
        {
            uint32_t bytesToWrite = (remaining > (MTU_SIZE -3)) ? (MTU_SIZE-3) : remaining;
            if (!imageChr->setData(ptr, bytesToWrite))
            {
                Serial.println("Failed to write image data to Protobuf stream.");
                return;
            }
            imageChr->notify(0);
            updateGreenLED();
            // Serial.println("Writing im")
            ptr += bytesToWrite;
            remaining -= bytesToWrite;
        }
    }
    else
    {
        if (!imageChr->setData(buf, len))
        {
            Serial.println("Failed to write image data to Protobuf stream.");
            return;
        }
        imageChr->notify(0);
    }
}

void sendChunk(BLECharacteristic *imageChr, uint8_t *buf, uint32_t len)
{
    // uint8_t chunk_buf[64] = {0};
    // uint8_t chunk_len = snprintf((char *)chunk_buf, 64, "%lX\r\n", len);
    // send(imageChr, chunk_buf, chunk_len);
    // imageChr->notify(0);
    send(imageChr, buf, len);
    // imageChr->notify(0);
    // send(imageChr, (uint8_t *)"\r\n", 2);
    // imageChr->notify(0);
}


// BLE Callbacks
void readCamera(BLECharacteristic *imageChr, uint8_t connID)
{
    printf("Characteristic %s read by connection %d\n", imageChr->getUUID().str(), connID);

    // Capture an image
    Camera.getImage(CHANNEL, &img_addr, &img_len);

    // Split the image into chunks and write each chunk to the characteristic buffer
    const uint16_t CHUNK_SIZE = MTU_SIZE-3; // Max size defined in BLECharacteristic
    uint8_t *img_ptr = (uint8_t *)img_addr;
    // uint8_t *img_ptr = (uint8_t *)contiguous_buffer;
    uint16_t bytes_remaining = img_len;
    uint16_t chunkIndex = 0;

    sendChunk(imageChr, (uint8_t *)STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    sendChunk(imageChr, (uint8_t *)&img_len, sizeof(img_len));
    sendChunk(imageChr, img_ptr, img_len);
    sendChunk(imageChr, (uint8_t *)STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
    delay(10);

    printf("Image transfer complete. Total size: %lu bytes\n", img_len);

    delay(1000);
}

void notifCB(BLECharacteristic *imageChr, uint8_t connID, uint16_t cccd)
{
    if (cccd & GATT_CLIENT_CHAR_CONFIG_NOTIFY)
    {
        printf("Notifications enabled on Characteristic %s for connection %d\n", imageChr->getUUID().str(), connID);
        notify = true;
    }
    else
    {
        printf("Notifications disabled on Characteristic %s for connection %d\n", imageChr->getUUID().str(), connID);
        notify = false;
    }
}

void setupImgService()
{
    // Configure BLE characteristic
    imgChar.setReadProperty(true);
    imgChar.setReadPermissions(GATT_PERM_READ);
    imgChar.setBufferLen(CHAR_VALUE_MAX_LEN); // Ensure the buffer can handle the maximum characteristic value length
    imgChar.setNotifyProperty(true);
    imgChar.setReadCallback(readCamera);
    imgChar.setCCCDCallback(notifCB);

    // Add the characteristic to the service
    imgService.addCharacteristic(imgChar);
    // imgService.addCharacteristic(imgCharMetadata);
}

void setupDeviceInfoService()
{
    deviceInfoChar_manufacturerName.setReadProperty(true);
    deviceInfoChar_manufacturerName.setReadPermissions(GATT_PERM_READ);
    deviceInfoChar_manufacturerName.writeString(DEVICE_INFO_MANUFACTURER_NAME);

    deviceInfoChar_modelNumber.setReadProperty(true);
    deviceInfoChar_modelNumber.setReadPermissions(GATT_PERM_READ);
    deviceInfoChar_modelNumber.writeString(DEVICE_INFO_MODEL_NUMBER);

    deviceInfoChar_serialNumber.setReadProperty(true);
    deviceInfoChar_serialNumber.setReadPermissions(GATT_PERM_READ);
    deviceInfoChar_serialNumber.writeString(DEVICE_INFO_SERIAL_NUMBER);

    deviceInfoChar_hardwareRevision.setReadProperty(true);
    deviceInfoChar_hardwareRevision.setReadPermissions(GATT_PERM_READ);
    deviceInfoChar_hardwareRevision.writeString(DEVICE_INFO_HARDWARE_REVISION);

    deviceInfoChar_firmwareRevision.setReadProperty(true);
    deviceInfoChar_firmwareRevision.setReadPermissions(GATT_PERM_READ);
    deviceInfoChar_firmwareRevision.writeString(DEVICE_INFO_FIRMWARE_REVISION);

    deviceInfoChar_softwareRevision.setReadProperty(true);
    deviceInfoChar_softwareRevision.setReadPermissions(GATT_PERM_READ);
    deviceInfoChar_softwareRevision.writeString(DEVICE_INFO_SOFTWARE_REVISION);

    deviceInfoChar_systemID.setReadProperty(true);
    deviceInfoChar_systemID.setReadPermissions(GATT_PERM_READ);
    deviceInfoChar_systemID.writeString(DEVICE_INFO_SYSTEM_ID);
    Serial.println("Device Information Service setup complete.");

    deviceInfoService.addCharacteristic(deviceInfoChar_manufacturerName);
    deviceInfoService.addCharacteristic(deviceInfoChar_modelNumber);
    deviceInfoService.addCharacteristic(deviceInfoChar_serialNumber);
    deviceInfoService.addCharacteristic(deviceInfoChar_hardwareRevision);
    deviceInfoService.addCharacteristic(deviceInfoChar_firmwareRevision);
    deviceInfoService.addCharacteristic(deviceInfoChar_softwareRevision);
    deviceInfoService.addCharacteristic(deviceInfoChar_systemID);
    Serial.println("Device Information Service setup complete.");
}

void updateBlueLED()
{
    unsigned long currentMillis = millis();

    // Check if the interval has passed
    if (currentMillis - previousMillis >= flashInterval)
    {
        previousMillis = currentMillis; // Save the current time
        // Toggle the LED
        digitalWrite(LED_B, !digitalRead(LED_B));
    }
}

void updateGreenLED()
{
    unsigned long currentMillis = millis();

    // Check if the interval has passed
    if (currentMillis - previousMillis >= flashInterval)
    {
        previousMillis = currentMillis; // Save the current time
        // Toggle the LED
        digitalWrite(LED_G, !digitalRead(LED_G));
    }
}

void setup()
{
    Serial.begin(115200);
    digitalWrite(LED_B, LOW);
    digitalWrite(LED_G, HIGH);

    // setupJSONMetaData();

    setupImgService();
    setupDeviceInfoService();

    // Advertise the BLE service
    advMainData.addFlags();
    advMainData.addCompleteName(DEVICE_NAME_LONG);
    advMainData.addShortName(DEVICE_NAME_SHORT);
    advMainData.addAppearance(DEVICE_APPEARANCE);
    // advMainData.hasManufacturer();
    // advMainData.hasName();
    // advMainData.hasUUID();

    advMainData.addCompleteServices(BLEUUID(DEVICE_INFORMATION_SERVICE_UUID));
    advMainData.addCompleteServices(BLEUUID(IMG_SERVICE_UUID));

    advDeviceInfoData.addFlags();
    advDeviceInfoData.addCompleteName("Device Information");
    advDeviceInfoData.addShortName("DevInfo");
    advDeviceInfoData.addCompleteServices(BLEUUID(DEVICE_INFORMATION_SERVICE_UUID));
    advDeviceInfoData.addAppearance(DEVICE_APPEARANCE);

    advImageData.addFlags();
    advImageData.addCompleteName("Image Transfer");
    advImageData.addShortName("ImgTrns");
    advImageData.addCompleteServices(BLEUUID(IMG_SERVICE_UUID));

    // Initialize BLE
    BLE.init();
    BLE.setDeviceName(DEVICE_NAME_LONG);
    BLE.configSecurity()->setPairable(true);
    BLE.configAdvert()->setAdvData(advMainData);
    BLE.configAdvert()->setScanRspData(advImageData);
    BLE.setDeviceAppearance(DEVICE_APPEARANCE);
    BLE.configServer(2);

    // Add the service to the BLE device
    BLE.addService(deviceInfoService);
    BLE.addService(imgService);

    // Start BLE peripheral mode
    BLE.beginPeripheral();

    // Initialize the camera
    Camera.configVideoChannel(CHANNEL, config);
    Camera.videoInit();
    Camera.channelBegin(CHANNEL);

    Serial.println("Setup complete, BLE and Camera initialized.");
    digitalWrite(LED_G, LOW);
}

void loop()
{
    if (!BLE.connected(0))
    {
        updateBlueLED();
    }else {
      digitalWrite(LED_B, LOW);
    }

    // Handle BLE notifications
    if (BLE.connected(0) && notify)
    {
        // Capture an image
        Serial.print("Grabbing image");
        Camera.getImage(CHANNEL, &img_addr, &img_len);
        // Debugging
        if (img_addr == 0 || img_len == 0)
        {
            Serial.println("Camera returned invalid data.");
            return;
        }
        // Send the image in chunks via notifications
        readCamera(&imgChar, 0);
        // delay(1000); // Add delay to reduce CPU usage
    }
}