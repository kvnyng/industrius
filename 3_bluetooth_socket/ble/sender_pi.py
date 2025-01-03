# pip install bleak

from bleak import BleakServer, GATTCharacteristic

# Define a characteristic for the image transfer
CHARACTERISTIC_UUID = (
    "0000abcd-0000-1000-8000-00805f9b34fb"  # Replace with your custom UUID
)
SERVICE_UUID = "0000abcd-0000-1000-8000-00805f9b34fb"


async def main():
    server = BleakServer()

    # Add service
    service = server.add_service(SERVICE_UUID)

    # Add characteristic
    characteristic = service.add_characteristic(
        CHARACTERISTIC_UUID,
        GATTCharacteristic.PROPERTY_READ | GATTCharacteristic.PROPERTY_NOTIFY,
    )

    async def on_read_request(handle):
        # Read the image in chunks
        with open("../assets/image.jpg", "rb") as f:
            data = f.read()
            # Split data into chunks of 20 bytes (or negotiate MTU for larger chunks)
            for i in range(0, len(data), 20):
                chunk = data[i : i + 20]
                # Notify the client with the chunk
                await characteristic.notify_value(chunk)
        return b""

    characteristic.on_read_request = on_read_request

    # Start advertising
    await server.start_advertising()
    print("Server is running...")
    await server.wait_for_connections()


import asyncio

asyncio.run(main())
