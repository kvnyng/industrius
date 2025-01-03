import asyncio
from bleak import BleakScanner


async def scan():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()
    devices = sorted(devices, key=lambda x: x.rssi, reverse=True)
    for device in devices:
        print(f"Found device: {device.name} ({device.address})")


asyncio.run(scan())
