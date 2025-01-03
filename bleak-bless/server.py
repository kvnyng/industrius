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
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)


logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(name=__name__)

# NOTE: Some systems require different synchronization methods.
trigger: Union[asyncio.Event, threading.Event]
if sys.platform in ["darwin", "win32"]:
    trigger = threading.Event()
else:
    trigger = asyncio.Event()


def read_request(characteristic: BlessGATTCharacteristic, **kwargs) -> bytearray:
    logger.debug(f"Reading {characteristic.value}")
    return characteristic.value


def write_request(characteristic: BlessGATTCharacteristic, value: Any, **kwargs):
    characteristic.value = value
    logger.debug(f"Char value set to {characteristic.value}")
    # if characteristic.value == b"\x0f":
    #     logger.debug("NICE")
    #     trigger.set()


SERVICE_NAME = "IMAGE_SERVER"
SERVICE_UUID = "A07498CA-AD5B-474E-940D-16F1FBE7E8CD"
CHARACTERISTIC_UUID = "51FF12BB-3ED8-46E5-B4F9-D64E2FEC021B"


async def run(loop):
    trigger.clear()
    # Instantiate the server
    server = BlessServer(name=SERVICE_NAME, loop=loop)
    server.read_request_func = read_request
    server.write_request_func = write_request

    # Add Service
    await server.add_new_service(SERVICE_UUID)

    # Add a Characteristic to the service
    char_flags = (
        GATTCharacteristicProperties.read
        | GATTCharacteristicProperties.write
        | GATTCharacteristicProperties.indicate
    )
    permissions = GATTAttributePermissions.readable | GATTAttributePermissions.writeable
    value = "hi".encode()
    await server.add_new_characteristic(
        SERVICE_UUID, CHARACTERISTIC_UUID, char_flags, None, permissions
    )

    logger.debug(server.get_characteristic(CHARACTERISTIC_UUID))
    await server.start()

    logger.debug("Advertising")

    # Update the characteristic value between 0-10 every 2 seconds
    counter = 0
    while True:
        logger.debug(f"Updating value: {counter}")
        write_request(
            server.get_characteristic(CHARACTERISTIC_UUID), str(counter).encode()
        )
        server.update_value(SERVICE_UUID, CHARACTERISTIC_UUID)
        print(
            f"Newly written value: {read_request(server.get_characteristic(CHARACTERISTIC_UUID))}"
        )
        counter += 1
        await asyncio.sleep(2)

    await server.stop()


loop = asyncio.get_event_loop()
loop.run_until_complete(run(loop))
