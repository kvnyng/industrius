# Setup

The following code contains the client and server code for the necklace wearable.

## BLE Necklace Setup

After following the AMB82-Mini tutorial on uploading via arduino, upload the server code to the AMB82-mini and reset the board. The device will be discoverable over the name "Neckie"

## Client Setup
Run the code in the web browser through whatever hosting system you prefer. I've been setting it up as a local host through executing `python3 -m http.server` within the client folder. This makes the client reachable over your device's ip addr at the specific hosting port (8000).