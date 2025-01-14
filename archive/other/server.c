#include "BLEDevice.h"
#include "VideoStream.h"
// #include <ArduinoJson.h>
// #include "pb_encode.h"
// #include "pb_decode.h"
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
#define IMG_SERVICE_UUID "12345678-0001-0000-1234-56789abcdef0"
#define IMG_CHAR_IMAGE_METADATA "12345678-0001-0001-1234-56789abcdef0"
#define IMG_CHAR_STREAM "12345678-0001-0002-1234-56789abcdef0"

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
BLECharacteristic imgCharMetadata(BLEUUID(IMG_CHAR_IMAGE_METADATA));
BLECharacteristic imgChar(BLEUUID(IMG_CHAR_STREAM));

// Advertising Data
BLEAdvertData advMainData;
BLEAdvertData advImageData;
BLEAdvertData advDeviceInfoData;

struct ImageDataArg
{
    uint8_t *img_ptr; // Pointer to image data
    size_t img_len;   // Length of the image
    uint8_t *current_ptr;
};

#define DEVICE_NAME_LONG "Neckie"
#define DEVICE_NAME_SHORT "Neck"

// Camera settings
#define CHANNEL 0
VideoSetting config(VIDEO_FHD, CAM_NN_FPS, VIDEO_JPEG, 1); // VGA resolution for simplicity

uint32_t img_addr = 0;
uint32_t img_len = 0;
bool notify = false;

unsigned long previousMillis = 0;        // Stores the last time the LED was toggled
const unsigned long flashInterval = 500; // Interval at which to flash the LED (in milliseconds)

// // Create JSON object
// JsonDocument jsonPacket;

// char jsonBuffer[256];

// BLE Callbacks

bool bleStreamWriter(pb_ostream_t *stream, const uint8_t *buf, size_t count)
{
    BLECharacteristic *bleChar = (BLECharacteristic *)stream->state;

    // Write directly to the BLE characteristic
    if (!bleChar->setData((uint8_t *)buf, count))
    {
        Serial.println("Failed to write data to BLE characteristic.");
        return false;
    }

    updateGreenLED();

    // Notify the BLE client
    bleChar->notify(0);

    // Add a small delay to allow the client to process notifications
    delay(10);

    return true;
}

// Create a BLE-based Protobuf stream
pb_ostream_t pb_ostream_from_ble(BLECharacteristic *bleChar)
{
    pb_ostream_t stream;
    stream.callback = bleStreamWriter; // Custom write function
    stream.state = bleChar;            // Pass BLE characteristic as state
    stream.max_size = MTU_SIZE - 3;    // No buffer size limit for streaming
    stream.bytes_written = 0;          // Track bytes written
    return stream;
}

bool readImageCallback(pb_ostream_t *stream, const pb_field_t *field, void *const *arg)
{
    // Extract the image data and length from the argument
    ImageDataArg *imageData = (ImageDataArg *)*arg;

    // Stream the image data in chunks
    size_t chunkSize = 150; // Define a chunk size
    // size_t remaining = imageData->img_len;

    // Calculate remaining bytes to write based on moving pointer
    size_t remaining = imageData->img_len - (imageData->current_ptr - imageData->img_ptr);

    uint8_t *img_ptr = imageData->img_ptr;
    uint8_t *current_ptr = imageData->current_ptr;

    if (img_ptr == NULL)
    {
        printf("Image data pointer is NULL.\n");
        return false;
    }

    size_t bytesToWrite = (remaining < chunkSize) ? remaining : chunkSize;

    Serial.print("Writing this many bytes: ");
    Serial.println(bytesToWrite);

    if (!pb_write(stream, current_ptr, bytesToWrite))
    {
        printf("Failed to write image data to Protobuf stream.\n");
        return false;
    }
    current_ptr += bytesToWrite;
    remaining -= bytesToWrite;

    if (remaining == 0)
    {
        printf("Image data stream complete.\n");
    }

    // while (remaining > 0)
    // {
    //     size_t bytesToWrite = (remaining < chunkSize) ? remaining : chunkSize;
    //     Serial.println("Bytes to write: ");
    //     Serial.println(bytesToWrite);

    //     size_t remaining_space = stream->max_size - stream->bytes_written;

    //     Serial.println("Remaining space in stream buffer: ");
    //     Serial.println(remaining_space);

    //     if (bytesToWrite > remaining_space)
    //     {
    //         Serial.println("Not enough space in stream buffer!");
    //         return false;
    //     }

    //     if (!pb_write(stream, img_ptr, bytesToWrite))
    //     {
    //         Serial.println("Failed to write image data to Protobuf stream.");
    //         return false;
    //     }

    //     img_ptr += bytesToWrite;
    //     remaining -= bytesToWrite;
    // }

    return true;
}

void readCamera(BLECharacteristic *imageChr, uint8_t connID)
{
    printf("Characteristic %s read by connection %d\n", imageChr->getUUID().str(), connID);

    // Capture an image
    Camera.getImage(CHANNEL, &img_addr, &img_len);

    // Split the image into chunks and write each chunk to the characteristic buffer
    // const uint16_t CHUNK_SIZE = MTU_SIZE - 3; // Max size defined in BLECharacteristic
    uint8_t *img_ptr = (uint8_t *)img_addr;
    ImageDataArg imgData = {.img_ptr = img_ptr, .img_len = img_len, .current_ptr = img_ptr};

    ImagePacket imagePacket = {};
    imagePacket.imageSize = img_len;
    imagePacket.timestamp = millis();
    imagePacket.imageData.funcs.encode = readImageCallback;
    imagePacket.imageData.arg = &imgData;

    // pb_byte_t imagePacketBuffer[4096]; // Declare buffer as pb_byte_t
    // // Create BLE Protobuf stream
    // pb_ostream_t stream = pb_ostream_from_ble(imageChr);

    // // Encode Protobuf message
    // if (!pb_encode(&stream, ImagePacket_fields, &imagePacket))
    // {
    //     printf("Failed to encode ImagePacket.\n");
    //     printf("Error: %s\n", PB_GET_ERROR(&stream));
    //     return;
    // }

    pb_ostream_t stream = pb_ostream_from_ble(imageChr);

    // pb_ostream_t stream = pb_ostream_from_buffer(imagePacketBuffer, sizeof(imagePacketBuffer));

    if (!pb_encode(&stream, ImagePacket_fields, &imagePacket))
    {
        printf("Failed to encode Image Packet to BLE.\n");
        printf("Error: %s\n", PB_GET_ERROR(&stream));
        return;
    }

    // size_t stream_size = stream.bytes_written;

    // for (size_t i = 0; i < stream_size; i++)
    // {
    //     updateGreenLED();
    //     Serial.print(imagePacketBuffer[i], HEX);
    //     Serial.print(" ");
    //     // Write the current chunk to the characteristic
    //     if (!imageChr->setData((uint8_t *)&imagePacketBuffer[i], 1))
    //     {
    //         printf("Failed to write chunk to characteristic.\n");
    //         break;
    //     }
    //     delay(10);
    // }
    // Serial.println();

    // uint16_t bytes_remaining = img_len;

    // while (bytes_remaining > 0)
    // {
    //     updateGreenLED();
    //     Serial.print("Writing chunks. Bytes Remaining:");
    //     Serial.println(bytes_remaining);
    //     uint16_t chunk_len = (bytes_remaining > CHUNK_SIZE) ? CHUNK_SIZE : bytes_remaining;

    //     // // Further update the JSON metadata
    //     // ["frameSize"] = chunk_len;
    //     // JSONmetaData["frameNumber"] = JSONmetaData["imageNumber"].as<int>() + 1; // Increment image number

    //     // // Serialize and print the updated JSON document
    //     // serializeJson(jsonPacket, jsonBuffer);
    //     // Serial.println(jsonBuffer);

    //     // Write the current chunk to the characteristic
    //     // if (!imageChr->setData(img_ptr, chunk_len))
    //     // {
    //     //     printf("Failed to write chunk to characteristic.\n");
    //     //     break;
    //     // }
    //     // imageChr->notify(connID);

    //     // if (!imgCharMetadata.setData((uint8_t *)jsonBuffer, strlen(jsonBuffer)))
    //     // {
    //     //     printf("Failed to write metadata to characteristic.\n");
    //     //     break;
    //     // }

    //     // if (!metadataChr->setData((uint8_t *)jsonBuffer, strlen(jsonBuffer)))
    //     // {
    //     //     printf("Failed to write metadata to characteristic.\n");
    //     //     break;
    //     // }

    //     // Notify client with the current chunk
    //     // imgCharMetadata.notify(connID);
    //     // imageChr->notify(connID);

    //     // // Advance the pointer and decrease the remaining byte count
    //     // img_ptr += chunk_len;
    //     // bytes_remaining -= chunk_len;

    //     delay(10); // Small delay to allow client to process notifications
    // }
    // Send over a clear byte to indicate the end of the image transfer
    // All 0000
    // uint8_t clear[4] = {0x00, 0x00, 0x00, 0x00};
    // imageChr->setData(clear, 4);

    printf("Image transfer complete. Total size: %lu bytes\n", img_len);
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

    imgCharMetadata.setReadProperty(true);
    imgCharMetadata.setReadPermissions(GATT_PERM_READ);
    imgCharMetadata.setNotifyProperty(true);
    imgCharMetadata.setBufferLen(CHAR_VALUE_MAX_LEN); // Ensure the buffer can handle the maximum characteristic value length
    // imgCharMetadata.setReadCallback(readCamera);
    // imgCharMetadata.setCCCDCallback(notifCB);

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

// void setupImagePacket()
// {
//     imagePacket.imageSize = 0;
//     imagePacket.timestamp = 0;
//     imagePacket.frameData_size = 0;
//     imagePacket.frameData = 0;
//     Serial.println("Protobuf Initialized");
// }

// void setupJSONMetaData()
// {
//     // Initialize the JSON document
//     JSONmetaData["imageNumber"] = 0;
//     JSONmetaData["imageSize"] = 0;
//     JSONmetaData["frameNumber"] = 0;
//     JSONmetaData["frameSize"] = 0;
//     JSONmetaData["timestamp"] = 0;

//     Serial.println("JSON Meta Data Initialized");
// }

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

    // setupImagePacket();

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
    }
    if (BLE.connected(0))
    {
        digitalWrite(LED_B, HIGH);
    }

    // Handle BLE notifications
    if (BLE.connected(0) && notify)
    {
        // Capture an image
        Serial.print("Grabbing image");
        Camera.getImage(CHANNEL, &img_addr, &img_len);

        // Send the image in chunks via notifications
        readCamera(&imgChar, 0);
        // delay(1000); // Add delay to reduce CPU usage
    }
}