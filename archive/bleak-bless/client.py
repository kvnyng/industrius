import asyncio
from bleak import BleakClient, BleakGATTCharacteristic

# Define the target device's service UUID and characteristic UUID
DEVICE_ADDRESS = "B8:27:EB:19:77:0A"
SERVICE_UUID = "A07498CA-AD5B-474E-940D-16F1FBE7E8CD"
CHARACTERISTIC_UUID = "51FF12BB-3ED8-46E5-B4F9-D64E2FEC021B"


async def read_characteristic(device_address):
    """
    Connects to the BLE device, reads the characteristic, and prints its value.
    """
    async with BleakClient(device_address) as client:
        print(f"Connected to {device_address}")

        # Check if the service and characteristic are available
        services = await client.get_services()
        if SERVICE_UUID not in [service.uuid for service in services]:
            print(f"Service {SERVICE_UUID} not found on the device.")
            return

        # subscribe to the characteristic
        await client.start_notify(CHARACTERISTIC_UUID, notification_handler)

        # Read the characteristic value
        try:
            value = await client.read_gatt_char(CHARACTERISTIC_UUID)
            print(f"Characteristic Value (UUID {CHARACTERISTIC_UUID}): {value}")
        except Exception as e:
            print(f"Failed to read characteristic: {e}")


def notification_handler(sender: BleakGATTCharacteristic, data: bytearray):
    print(f"{sender}: {data}")


if __name__ == "__main__":
    asyncio.run(read_characteristic(DEVICE_ADDRESS))
