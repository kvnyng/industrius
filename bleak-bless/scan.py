import asyncio
from bleak import BleakScanner


async def main():
    print("Scanning for BLE devices...")
    devices = await BleakScanner.discover()

    if devices:
        print("Found the following devices:")
        for device in devices:
            print(
                f"Address: {device.address}, Name: {device.name or 'Unknown'}, RSSI: {device.rssi}"
            )
    else:
        print("No BLE devices found.")


asyncio.run(main())
