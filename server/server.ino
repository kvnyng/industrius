#include "BLEDevice.h"
#include "VideoStream.h"
#include <ArduinoJson.h>
#include "image.pb.h"
#include <SnappyProto.h>
#include <cstring> // Required for memset

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
#define IMG_CHAR_PACKET_RECOVERY "12345678-0000-0003-1234-56789abcdef0"

#define DEVICE_INFO_MANUFACTURER_NAME "Industrius"
#define DEVICE_INFO_MODEL_NUMBER "1"
#define DEVICE_INFO_SERIAL_NUMBER "0001"
#define DEVICE_INFO_HARDWARE_REVISION "1"
#define DEVICE_INFO_FIRMWARE_REVISION "1"
#define DEVICE_INFO_SOFTWARE_REVISION "1"
#define DEVICE_INFO_SYSTEM_ID "1"

#define MTU_SIZE CHAR_VALUE_MAX_LEN

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
BLECharacteristic packetRecoveryChar(IMG_CHAR_PACKET_RECOVERY);

// Advertising Data
BLEAdvertData advMainData;
BLEAdvertData advImageData;
BLEAdvertData advDeviceInfoData;

#define DEVICE_NAME_LONG "Neckie"
#define DEVICE_NAME_SHORT "Neck"

// Camera settings
#define CHANNEL 0
VideoSetting config(VIDEO_VGA, 30, VIDEO_JPEG, 1); // VGA resolution for simplicity

uint32_t camera_img_addr = 0;
uint32_t camera_img_len = 0;

uint32_t contiguous_buffer[30000];

bool notify = false;

unsigned long previousMillis = 0;       // Stores the last time the LED was toggled
const unsigned long flashInterval = 10; // Interval at which to flash the LED (in milliseconds)

#define PART_BOUNDARY "123456789000000000000987654321"
char *START_BOUNDARY = "\r\n--" PART_BOUNDARY "--START--\r\n";
char *END_BOUNDARY = "\r\n--" PART_BOUNDARY "--END--\r\n";
char *STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";

// Create a 3D array to store the image data by chunk
#define NUMBER_OF_STORED_IMAGES 10
#define MAX_NUMBER_OF_PACKETS 400
#define MAX_PACKET_SIZE MTU_SIZE - 3

uint8_t image_data[NUMBER_OF_STORED_IMAGES][MAX_NUMBER_OF_PACKETS][MAX_PACKET_SIZE];

uint8_t image_data_index = 0;

uint16_t calculateBytesToWrite(uint16_t remaining)
{
    return (remaining > (MAX_PACKET_SIZE - 2)) ? (MAX_PACKET_SIZE - 2) : remaining;
}

uint8_t *ptr;
uint16_t remaining;
uint16_t bytesToWrite;
uint16_t packet_size;
uint8_t packet[MAX_PACKET_SIZE] = {0}; // All elements initialized to 0
int packet_number;
int shotgunCounter;

void sendImage(BLECharacteristic *imageChr, uint8_t *buf, uint16_t len)
{
    ptr = buf;
    remaining = len;
    bytesToWrite = calculateBytesToWrite(remaining);
    packet_size = bytesToWrite + 2;

    while (bytesToWrite > 0)
    {
        packet_size = bytesToWrite + 2;
        packet[0] = image_data_index;
        packet[1] = packet_number;

        memcpy(packet + 2, ptr, bytesToWrite);

        if (!imageChr->setData(packet, packet_size))
        {
            Serial.println("Failed to write image data to Protobuf stream.");
            return;
        }
        imageChr->notify(0);

        // Store the image data
        memcpy(image_data[image_data_index][packet_number], packet, packet_size);
        packet_number++;

        if (packet_number >= MAX_NUMBER_OF_PACKETS)
        {
            packet_number = 0;
        }

        updateGreenLED();
        ptr += bytesToWrite;
        remaining -= bytesToWrite;
        bytesToWrite = calculateBytesToWrite(remaining);

        if (shotgunCounter++ % 40 == 0)
        {
            delay(7);
            shotgunCounter = 0;
        } else {
          delay(1);
        }
    }

    image_data_index++;
    if (image_data_index >= NUMBER_OF_STORED_IMAGES)
    {
        image_data_index = 0;
    }

    // Serial.print("Amount of packets sent: ");
    // Serial.println(packet_number);
}

void send(BLECharacteristic *imageChr, uint8_t *buf, uint16_t len, bool image = false)
{
    // if the data is larger than the MTU size, split it into chunks and send
    if (len > MTU_SIZE - 3)
    {
        ptr = buf;
        remaining = len;
        bytesToWrite = (remaining > (MTU_SIZE - 3)) ? (MTU_SIZE - 3) : remaining;

        while (bytesToWrite > 0)
        {
            if (!imageChr->setData(ptr, bytesToWrite))
            {
                Serial.println("Failed to write image data to Protobuf stream.");
                return;
            }
            imageChr->notify(0);
            updateGreenLED();
            ptr += bytesToWrite;
            remaining -= bytesToWrite;
            bytesToWrite = (remaining > (MTU_SIZE - 3)) ? (MTU_SIZE - 3) : remaining;
        }
    }
    else
    {
        if (!imageChr->setData(buf, len))
        {
            Serial.println("Failed to send data over BLE");
            return;
        }
        imageChr->notify(0);
    }
    delay(1);
}

void sendChunk(BLECharacteristic *imageChr, uint8_t *buf, uint16_t len)
{
    send(imageChr, buf, len);
}

// BLE Callbacks
void readCamera(BLECharacteristic *imageChr, uint8_t connID)
{
    // printf("Characteristic %s read by connection %d\n", imageChr->getUUID().str(), connID);

    // Split the image into chunks and write each chunk to the characteristic buffer
    // const uint16_t CHUNK_SIZE = MTU_SIZE - 3; // Max size defined in BLECharacteristic

    // point to the continuous buffer
    uint8_t *img_ptr = (uint8_t *)contiguous_buffer;

    sendChunk(imageChr, (uint8_t *)START_BOUNDARY, strlen(START_BOUNDARY));
    // Serial.print("SENDING START BOUNDARY OF LENGTH: ");
    // Serial.println(strlen(START_BOUNDARY));
    sendChunk(imageChr, (uint8_t *)&camera_img_len, sizeof(camera_img_len));

    // // Send over which image it is
    // sendChunk(imageChr, &image_data_index, sizeof(image_data_index));

    sendImage(imageChr, img_ptr, camera_img_len);
    // Serial.print("SENDING END BOUNDARY OF LENGTH: ");
    // Serial.println(strlen(END_BOUNDARY));
    sendChunk(imageChr, (uint8_t *)END_BOUNDARY, strlen(END_BOUNDARY));

    printf("Image transfer complete. Total size: %lu bytes\n", camera_img_len);
    delay(10);
}

// void saveImageData(uint8_t *buf, uint16_t len)
// {
//     // Save the image data to the 3D array
//     for (uint16_t i = 0; i < len; i++)
//     {
//         image_data[image_data_index][i / CHUNK_SIZE][i % CHUNK_SIZE] = buf[i];
//     }

//     image_data_index++;

//     if (image_data_index >= NUMBER_OF_STORED_IMAGES)
//     {
//         image_data_index = 0;
//     }
// }

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

void packetRecoveryWriteCB(BLECharacteristic *packetRecoveryChar, uint8_t connID)
{
    // Read the packet recovery data
    uint8_t *packetRecoveryData = packetRecoveryChar->getDataBuff();
    uint16_t packetRecoveryDataLen = packetRecoveryChar->getDataLen();

    Serial.print("Packet Recovery Data received: ");
    for (int i = 0; i < packetRecoveryDataLen; i++)
    {
        Serial.print(packetRecoveryData[i]);
    }

    // Add rest of code later.
}

void setupImgService()
{
    // Configure BLE characteristic
    imgChar.setReadProperty(true);
    imgChar.setReadPermissions(GATT_PERM_READ);
    imgChar.setBufferLen(MTU_SIZE); // Ensure the buffer can handle the maximum characteristic value length
    imgChar.setNotifyProperty(true);
    // imgChar.setReadCallback(readCamera);
    imgChar.setCCCDCallback(notifCB);

    packetRecoveryChar.setReadProperty(true);
    packetRecoveryChar.setWriteProperty(true);
    packetRecoveryChar.setReadPermissions(GATT_PERM_READ);
    packetRecoveryChar.setWritePermissions(GATT_PERM_WRITE);
    packetRecoveryChar.setBufferLen(MTU_SIZE);
    packetRecoveryChar.setWriteCallback(packetRecoveryWriteCB);

    // Add the characteristic to the service
    imgService.addCharacteristic(imgChar);
    imgService.addCharacteristic(packetRecoveryChar);
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
    Camera.getImage(CHANNEL, &camera_img_addr, &camera_img_len);
}

// Takes
void grabCameraImage()
{
    Camera.getImage(CHANNEL, &camera_img_addr, &camera_img_len);
    if (camera_img_addr == 0 || camera_img_len == 0)
    {
        Serial.println("Camera returned invalid data. SELF GENERATED");
        return;
    }

    memcpy(contiguous_buffer, (uint8_t *)camera_img_addr, camera_img_len);
}

void loop()
{
    // Camera.getImage(CHANNEL, &camera_img_addr, &camera_img_len);
    if (!BLE.connected(0))
    {
        updateBlueLED();
    }
    else
    {
        digitalWrite(LED_B, HIGH);
    }

    // Handle BLE notifications
    if (BLE.connected(0) && notify)
    {
        // Capture an image
        // Serial.print("Grabbing image");
        // Camera.getImage(CHANNEL, &camera_img_addr, &camera_img_len);
        // Debugging
        if (camera_img_addr == 0 || camera_img_len == 0)
        {
            Serial.println("Camera returned invalid data.");
            return;
        }
        // Send the image in chunks via notifications`
        grabCameraImage();
        readCamera(&imgChar, 0);
        // delay(1000); // Add delay to reduce CPU usage
    }
}