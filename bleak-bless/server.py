"""
Example for a BLE 4.0 Server
"""

import sys
import logging
import asyncio
import threading

from typing import Any, Union

from bless import (  # type: ignore
    BlessServer,
    BlessGATTService,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)


logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(name=__name__)


def read_request(characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
    logger.debug(f"Reading {characteristic.value}")
    return characteristic.value


def write_request(characteristic: BlessGATTCharacteristic, value: Any, **kwargs):
    characteristic.value = value
    logger.debug(f"Char value set to {characteristic.value}")
    # if characteristic.value == b"\x0f":
    #     logger.debug("NICE")
    #     trigger.set()


CHAR_UUID_COUNTER = 1


def generate_service_UUID():
    global CHAR_UUID_COUNTER
    CHAR_UUID_COUNTER += 1
    return SERVICE_BASE_UUID[0:19] + str(CHAR_UUID_COUNTER) + SERVICE_BASE_UUID[:-13]


def generate_char_UUID():
    global CHAR_UUID_COUNTER
    CHAR_UUID_COUNTER += 1
    return (
        CHARACTERISTIC_BASE_UUID[0:19]
        + str(CHAR_UUID_COUNTER)
        + CHARACTERISTIC_BASE_UUID[:-13]
    )


SERVICE_NAME = "BLE_IMAGES"
SERVICE_BASE_UUID = "AA0098CA-AD5B-474E-0000-16F1FBE7E8CD"
CHARACTERISTIC_BASE_UUID = "BB0098CA-AD5B-474E-0000-16F1FBE7E8CD"

server = None


async def run(loop):

    # Instantiate the server
    server = BlessServer(name=SERVICE_NAME, loop=loop, prioritize_local_name=True)
    server.read_request_func = read_request
    server.write_request_func = write_request

    # DIS_SERVICE_UUID = "0000180A-0000-1000-8000-00805F9B34FB"
    # MANUFACTURER_UUID = "00002A29-0000-1000-8000-00805F9B34FB"
    # MODEL_NUMBER_UUID = "00002A24-0000-1000-8000-00805F9B34FB"

    SERVICE_DEVICE_INFO_UUID = "0000180A-0000-1000-8000-00805F9B34FB"
    SERVICE_DATA_IMAGES_UUID = "61518535-6b6a-4f68-bc4d-82f5d4994cd7"

    # Add Service
    await server.add_new_service(SERVICE_DEVICE_INFO_UUID)
    await server.add_new_service(SERVICE_DATA_IMAGES_UUID)

    MANUFACTURER_NAME = "Industrius"
    MODEL_NUMBER = "01"
    SERIAL_NUMBER = "0001"
    HARDWARE_REVISION = "Hardware Revision 1"
    FIRMWARE_REVISION = "Firmware Revision 1"
    SOFTWARE_REVISION = "Software Revision 1"
    SYSTEM_ID = "System ID 1"

    from uuid import UUID

    MANUFACTURER_NAME_UUID = UUID("00002A29-0000-1000-8000-00805F9B34FB")
    MODEL_NUMBER_UUID = UUID("00002A24-0000-1000-8000-00805F9B34FB")
    SERIAL_NUMBER_UUID = UUID("00002A25-0000-1000-8000-00805F9B34FB")
    HARDWARE_REVISION_UUID = UUID("00002A27-0000-1000-8000-00805F9B34FB")
    FIRMWARE_REVISION_UUID = UUID("00002A26-0000-1000-8000-00805F9B34FB")
    SOFTWARE_REVISION_UUID = UUID("00002A28-0000-1000-8000-00805F9B34FB")
    SYSTEM_ID_UUID = UUID("00002A23-0000-1000-8000-00805F9B34FB")

    # Add Characteristics
    MANUFACUTER_NAME_CHAR = await server.add_new_characteristic(
        SERVICE_DEVICE_INFO_UUID,
        MANUFACTURER_NAME_UUID,
        GATTCharacteristicProperties.read,
        MANUFACTURER_NAME.encode(),
        GATTAttributePermissions.readable,
    )
    MODEL_NUMBER_CHAR = await server.add_new_characteristic(
        SERVICE_DEVICE_INFO_UUID,
        MODEL_NUMBER_UUID,
        GATTCharacteristicProperties.read,
        MODEL_NUMBER.encode(),
        GATTAttributePermissions.readable,
    )
    SERIAL_NUMBER_CHAR = await server.add_new_characteristic(
        SERVICE_DEVICE_INFO_UUID,
        SERIAL_NUMBER_UUID,
        GATTCharacteristicProperties.read,
        SERIAL_NUMBER.encode(),
        GATTAttributePermissions.readable,
    )
    HARDWARE_REVISION_CHAR = await server.add_new_characteristic(
        SERVICE_DEVICE_INFO_UUID,
        HARDWARE_REVISION_UUID,
        GATTCharacteristicProperties.read,
        HARDWARE_REVISION.encode(),
        GATTAttributePermissions.readable,
    )
    FIRMWARE_REVISION_CHAR = await server.add_new_characteristic(
        SERVICE_DEVICE_INFO_UUID,
        FIRMWARE_REVISION_UUID,
        GATTCharacteristicProperties.read,
        FIRMWARE_REVISION.encode(),
        GATTAttributePermissions.readable,
    )
    SOFTWARE_REVISION_CHAR = await server.add_new_characteristic(
        SERVICE_DEVICE_INFO_UUID,
        SOFTWARE_REVISION_UUID,
        GATTCharacteristicProperties.read,
        SOFTWARE_REVISION.encode(),
        GATTAttributePermissions.readable,
    )
    SYSTEM_ID_CHAR = await server.add_new_characteristic(
        SERVICE_DEVICE_INFO_UUID,
        SYSTEM_ID_UUID,
        GATTCharacteristicProperties.read,
        SYSTEM_ID.encode(),
        GATTAttributePermissions.readable,
    )

    DATA_IMAGES_UUID = UUID("2e701fa5-06e1-4dac-a668-089e91437fc7")

    DATA_IMAGES_CHAR = await server.add_new_characteristic(
        SERVICE_DATA_IMAGES_UUID,
        DATA_IMAGES_UUID,
        GATTCharacteristicProperties.notify | GATTCharacteristicProperties.read,
        None,
        GATTAttributePermissions.readable,
    )

    # # Add a Characteristic to the service
    # char_flags = (
    #     GATTCharacteristicProperties.read
    #     | GATTCharacteristicProperties.write
    #     | GATTCharacteristicProperties.indicate
    # )
    # permissions = GATTAttributePermissions.readable | GATTAttributePermissions.writeable
    # value = "hi".encode()
    # await server.add_new_characteristic(
    #     SERVICE_BASE_UUID, CHARACTERISTIC_UUID, char_flags, None, permissions
    # )

    # logger.debug(server.get_characteristic(CHARACTERISTIC_UUID))
    await server.start()
    logger.debug("Advertising")

    # Load an image from a file
    with open("../assets/sample.jpg", "rb") as f:
        image_data = f.read()

    # chunk the image data into 20 byte chunks
    chunk_size = 20
    image_data_chunks = [
        image_data[i : i + chunk_size] for i in range(0, len(image_data), chunk_size)
    ]
    print(image_data_chunks)

    print("Waiting for client to connect...")

    # proceed only if the client is connected
    while not await server.is_connected():
        await asyncio.sleep(0.1)

    # send the image data in chunks
    for chunk in image_data_chunks:
        write_request(server.get_characteristic(str(DATA_IMAGES_UUID)), chunk)
        server.update_value(str(SERVICE_DATA_IMAGES_UUID), str(DATA_IMAGES_UUID))

    await server.stop()

    # Update the characteristic value between 0-10 every 2 seconds
    # counter = 0
    # while True:
    #     logger.debug(f"Updating value: {counter}")
    #     write_request(
    #         server.get_characteristic(CHARACTERISTIC_UUID), str(counter).encode()
    #     )
    #     server.update_value(SERVICE_BASE_UUID, CHARACTERISTIC_UUID)
    #     print(
    #         f"Newly written value: {read_request(server.get_characteristic(CHARACTERISTIC_UUID))}"
    #     )
    #     counter += 1
    #     await asyncio.sleep(2)


import signal
import sys


def handle_sigterm(signum, frame):
    print("SIGTERM received. Cleaning up...")

    # Add the line of code you want to execute here
    # For example, closing a file or stopping a service
    sys.exit(0)  # Exit the script cleanly


# Register the signal handler
signal.signal(signal.SIGTERM, handle_sigterm)

# Example long-running task
print("Script running. Send SIGTERM to terminate.")
try:
    while True:
        loop = asyncio.get_event_loop()
        loop.run_until_complete(run(loop))
except KeyboardInterrupt:
    print("Interrupted by user.")
