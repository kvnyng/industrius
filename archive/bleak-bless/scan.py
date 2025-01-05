import asyncio
from bleak import BleakScanner, BleakClient


async def scan():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    devices = sorted(devices, key=lambda x: x.rssi, reverse=True)
    for device in devices:
        print(f"Found device: {device.name} ({device.address})")
        async with BleakClient(device.address) as client:
            print(f"Connected to {device.name} ({device.address})")
            services = await client.get_services()
            for service in services:
                print(f"Service: {service.uuid}")
                for char in service.characteristics:
                    print(
                        f"  Characteristic: {char.uuid}, properties: {char.properties}"
                    )
                    if "read" in char.properties:
                        try:
                            value = await client.read_gatt_char(char.uuid)
                            print(f"    Value: {value}")
                        except Exception as e:
                            print(f"Could not read characteristic {char.uuid}: {e}")
        print("-------")


asyncio.run(scan())
