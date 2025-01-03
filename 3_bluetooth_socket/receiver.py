import cv2
import bluetooth
import pickle

# Bluetooth setup
server_address = "XX:XX:XX:XX:XX:XX"  # Replace with the Raspberry Pi's Bluetooth address
port = 1

client_socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
print("Connecting to server...")
client_socket.connect((server_address, port))
print("Connected!")

try:
    while True:
        # Receive frame size
        frame_size_bytes = client_socket.recv(4)
        if not frame_size_bytes:
            print("Disconnected from server.")
            break
        frame_size = int.from_bytes(frame_size_bytes, 'big')

        # Receive frame data
        data = b""
        while len(data) < frame_size:
            packet = client_socket.recv(frame_size - len(data))
            if not packet:
                break
            data += packet

        # Decode and display the frame
        frame = pickle.loads(data)
        frame = cv2.imdecode(frame, cv2.IMREAD_COLOR)
        cv2.imshow("Received Video", frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
except KeyboardInterrupt:
    print("Streaming stopped.")
finally:
    client_socket.close()
    cv2.destroyAllWindows()