from bless import (
    BlessServer,
    BlessGATTCharacteristic,
    GATTCharacteristicProperties,
    GATTAttributePermissions,
)
import asyncio

# Define the service and characteristic UUIDs
SERVICE_UUID = "A07498CA-AD5B-474E-940D-16F1FBE7E8CD"
CHARACTERISTIC_UUID = "51FF12BB-3ED8-46E5-B4F9-D64E2FEC021B"

# Characteristic value
characteristic_value = bytearray(b"Hello, BLE!")


# Callback for reading characteristic
async def read_callback(char):
    print(f"Characteristic {char.uuid} was read!")
    return characteristic_value


# Callback for writing characteristic
async def write_callback(char, value):
    global characteristic_value
    print(f"Characteristic {char.uuid} was written with value: {value}")
    characteristic_value = value


def main():
    # Create a Bless server instance
    server = BlessServer(name="BLESS Server Example")

    # Create a GATT Characteristic
    characteristic = BlessGATTCharacteristic(
        uuid=CHARACTERISTIC_UUID,
        value=characteristic_value,
        properties=(
            GATTCharacteristicProperties.READ | GATTCharacteristicProperties.WRITE
        ),
        permissions=(
            GATTAttributePermissions.READABLE | GATTAttributePermissions.WRITEABLE
        ),
        read_callback=read_callback,
        write_callback=write_callback,
    )

    # Add the characteristic to a service
    server.add_characteristic_to_service(SERVICE_UUID, characteristic)

    # Start the server
    print("Starting BLE Server...")
    asyncio.run(server.start())
    print("BLE Server is running!")


if __name__ == "__main__":
    main()
