# For Wifi

import cv2
import bluetooth
import pickle

# Bluetooth setup
server_socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
server_socket.bind(("", bluetooth.PORT_ANY))
server_socket.listen(1)

print("Waiting for connection...")
client_socket, address = server_socket.accept()
print(f"Connected to {address}")

# # Video capture setup
# cap = cv2.VideoCapture(0)  # Use the camera (change to '/dev/video0' if needed)
# cap.set(cv2.CAP_PROP_FRAME_WIDTH, 320)  # Set resolution
# cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 240)
# cap.set(cv2.CAP_PROP_FPS, 10)  # Set frame rate

# Read image on disk using OpenCV
frame = cv2.imread("image.jpg")

try:
    while True:
        # ret, frame = cap.read()
        # if not ret:
        #     print("Failed to capture frame.")
        #     break
        
        # Encode the frame as JPEG
        _, buffer = cv2.imencode('.jpg', frame)
        data = pickle.dumps(buffer)

        # Send the frame size and frame data
        client_socket.sendall(len(data).to_bytes(4, 'big'))  # Send size of the frame
        client_socket.sendall(data)  # Send the frame data
except KeyboardInterrupt:
    print("Streaming stopped.")
finally:
    client_socket.close()
    server_socket.close()