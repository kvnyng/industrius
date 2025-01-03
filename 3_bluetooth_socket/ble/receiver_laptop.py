from bleak import BleakClient


# To get mac addr, run bluetoothctl and then show
CHARACTERISTIC_UUID = "0000abcd-0000-1000-8000-00805f9b34fb"
DEVICE_ADDRESS = "B8:27:EB:19:77:0A"  # Replace with your Raspberry Pi's BLE MAC address


def handle_notification(sender, data):
    print(f"Received chunk: {data}")
    with open("received_image.jpg", "ab") as f:
        f.write(data)


async def main():
    async with BleakClient(DEVICE_ADDRESS) as client:
        print("Connected to server")

        # Subscribe to notifications
        await client.start_notify(CHARACTERISTIC_UUID, handle_notification)

        # Keep running to receive all chunks
        await asyncio.sleep(10)


import asyncio

asyncio.run(main())
