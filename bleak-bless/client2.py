import asyncio
from bleak import BleakClient

# Bluetooth address of the device
DEVICE_ADDRESS = "48C2EAC1-6A04-866D-4149-F3DB8C67C63B"
# DB81338E-42F6-8A8B-5812-645993A33DFC
# E393F26A-BB37-576E-237B-7A360C0DF52
# 10AC148F-71B6-FE34-8EA0-9AE49BB93E87
# 29F28D79-8175-22C2-0AA4-73296B9ED0B8


async def read_characteristics(address):
    async with BleakClient(address, timeout=10) as client:
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
                        print(f"Could not read characteristic {char.uuid}: {e}")


async def main():
    await read_characteristics(DEVICE_ADDRESS)


# Run the main loop
if __name__ == "__main__":
    asyncio.run(main())
