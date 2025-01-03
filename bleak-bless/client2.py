import asyncio
from bleak import BleakClient

# Bluetooth address of the device
DEVICE_ADDRESS = "B8:27:EB:19:77:0A"


async def read_characteristics(address):
    async with BleakClient(address, timeout=20) as client:
        print(f"Connected: {client.is_connected}")

        # Get all services and their characteristics
        services = await client.get_services()
        for service in services:
            print(f"Service: {service.uuid}")
            for char in service.characteristics:
                print(f"  Characteristic: {char.uuid}, properties: {char.properties}")
                if "read" in char.properties:
                    try:
                        value = await client.read_gatt_char(char.uuid)
                        print(f"    Value: {value}")
                    except Exception as e:
                        print(f"    Could not read characteristic {char.uuid}: {e}")


async def main():
    await read_characteristics(DEVICE_ADDRESS)


# Run the main loop
if __name__ == "__main__":
    asyncio.run(main())
