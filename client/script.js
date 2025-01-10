const connectButton = document.getElementById("connectButton");
const statusText = document.getElementById("status");
const canvas = document.getElementById("imageCanvas");
const ctx = canvas.getContext("2d");

let imgServiceUuid = "12345678-0000-0001-1234-56789abcdef0";
let imgCharUuid = "12345678-0000-0001-1234-56789abcdef0";

connectButton.addEventListener("click", async () => {
    try {
        statusText.textContent = "Status: Scanning for devices...";
        const device = await navigator.bluetooth.requestDevice({
            acceptAllDevices: true,
            optionalServices: [imgServiceUuid],
        });

        statusText.textContent = "Status: Connecting...";
        const server = await device.gatt.connect();

        statusText.textContent = "Status: Connected to " + device.name;
        const service = await server.getPrimaryService(imgServiceUuid);
        const characteristic = await service.getCharacteristic(imgCharUuid);

        await characteristic.startNotifications();
        characteristic.addEventListener("characteristicvaluechanged", handleStreamData);

        statusText.textContent = "Status: Streaming image data...";
    } catch (error) {
        console.error("Error:", error);
        statusText.textContent = "Status: Failed to connect";
    }
});

let receivedChunks = [];
let totalBytes = 0;
let expectedLength = null;

const STREAM_BOUNDARY = "--123456789000000000000987654321";
// const PART_BOUNDARY = "123456789000000000000987654321";
// const START_BOUNDARY = "\r\n--" + PART_BOUNDARY + "--START--\r\n";
// const END_BOUNDARY = "\r\n--" + PART_BOUNDARY + "--END--\r\n";

// const STREAM_BOUNDARY = "--123456789000000000000987654321";
let currentPhase = "boundary"; // Can be "boundary", "length", "image", "end"

function handleStreamData(event) {
    const chunk = new Uint8Array(event.target.value.buffer);
    // console.log("Received chunk:", chunk);

    // Each packet starts with a header "%lX\r\n"
    // uint8_t chunk_len = snprintf((char *)chunk_buf, 64, "%lX\r\n", len);
    // chunk = processChunk(chunk);
    processStream(chunk);
}

// let currentChunk
// function processChunk(chunk) {
//     let length = 0;
//     let data = [];

//     // Take 
//     chunk = new TextDecoder().decode(chunk)
// }
// Function to process received chunks
function processStream(chunk) {
    // Convert the chunk to a string to check for boundaries (boundary phase)
    if (currentPhase === "boundary") {
        const chunkString = new TextDecoder().decode(chunk).replace(/\r\n/g, "");
        if (chunkString.includes(STREAM_BOUNDARY)) {
            console.log("Stream boundary detected!");
            currentPhase = "length"; // Move to the next phase
            // chunk = chunk.slice(STREAM_BOUNDARY.length); // Remove the boundary from the chunk
            // processStream(chunk); // Process the remaining chunk
        } else {
            console.log("Boundary not detected yet.");
        }
        return;
    }

    // Read the image length (length phase)
    if (currentPhase === "length") {
        console.log("Image Length Chunk:", chunk);

        if (chunk.length >= 4) {
            // Extract and decode the image length
            const lengthBuffer = chunk.slice(0, 4);
            imageLength = new DataView(lengthBuffer.buffer).getUint32(0, true); // Assuming little-endian
            imageLength = imageLength * 4
            console.log("Image length:", imageLength);

            // Remove the length bytes from the current chunk
            chunk = chunk.slice(4);

            currentPhase = "image"; // Move to the image phase

            // Add the remaining chunk to the image data
            // processStream(chunk);
        } else {
            console.error("Incomplete length data received.");
            console.error("Expected 4 bytes, received", chunk.length);
            console.error("Data:", chunk);
            return;
        }
    }

    // Accumulate image chunks (image phase)
    if (currentPhase === "image") {
        data = new Uint8Array(chunk);
        receivedChunks.push(data);
        totalBytes += data.length;
        console.log("Total Bytes:", totalBytes);

        // Check if the full image has been received
        if (totalBytes >= imageLength) {
            console.log("Image received!");
            let imageData = concatenateChunks(receivedChunks, totalBytes).slice(0, imageLength);

            // Reset state for the next stream
            receivedChunks = [];
            totalBytes = 0;
            imageLength = null;
            currentPhase = "end"; // Expecting the closing boundary

            // imageData = imageData.slice(0, imageLength);
            console.log("Received This Many Bytes:", imageData.length);
            console.log("Received Image Data:", imageData);
            displayImage(imageData); // Process and display the image
        }
    }

    // Detect closing stream boundary (end phase)
    if (currentPhase === "end") {
        const chunkString = new TextDecoder().decode(chunk);
        if (chunkString.includes(STREAM_BOUNDARY)) {
            // Remove the boundary from the chunk
            // chunk = chunk.slice(STREAM_BOUNDARY.length);
            console.log("Stream boundary detected at end!");
            currentPhase = "boundary"; // Ready for the next stream
            receivedChunks = [];
            totalBytes = 0;
            expectedLength = null;

            // Remove the boundary from the chunk
            // chunk = chunk.slice(STREAM_BOUNDARY.length);
            // processStream(chunk); // Process the remaining chunk
        }
    }
}

// Function to concatenate received chunks
function concatenateChunks(chunks, totalBytes) {
    const result = new Uint8Array(totalBytes);
    let offset = 0;
    for (const chunk of chunks) {
        result.set(chunk, offset);
        offset += chunk.length;
    }
    return result;
}

function reinterpretAsUint32Array(byteArray) {
    // Ensure the byteArray length is a multiple of 4
    if (byteArray.length % 4 !== 0) {
        throw new Error("Received data size is not a multiple of 4 bytes.");
    }

    // Create a Uint32Array from the underlying buffer
    const uint32Array = new Uint32Array(byteArray.buffer, byteArray.byteOffset, byteArray.byteLength / 4);
    return uint32Array;
}

function displayImage(imageData) {
    try {
        // Reinterpret the image data as a Uint32Array
        const castedData = reinterpretAsUint32Array(imageData); // Ensure this function works correctly
        console.log("Casted Data (Uint32Array):", castedData);

        // Convert the Uint32Array back to a Uint8Array with proper handling of byte order
        const uint8View = new Uint8Array(castedData.buffer);

        // // Optional: Verify and adjust endianness if needed
        // if (!verifyJPEGHeader(uint8View)) {
        //     console.log("Incorrect byte order detected, swapping endianness...");
        //     swapEndianness(castedData);
        // }

        // // Create a Blob from the corrected Uint8Array
        // const correctedUint8View = new Uint8Array(castedData.buffer); // Use the corrected buffer
        // if (!verifyJPEGHeader(correctedUint8View)) {
        //     throw new Error("JPEG header verification failed. Image data may be corrupted.");
        // }

        // console.log("JPEG header verified successfully.");

        const blob = new Blob([uint8View], { type: "image/jpeg" });
        const url = URL.createObjectURL(blob);

        // Display the image
        const img = new Image();
        img.onload = () => {
            canvas.width = img.width;
            canvas.height = img.height;
            ctx.drawImage(img, 0, 0);
            // Allow the url to work for 1 second
            setTimeout(() => {
                URL.revokeObjectURL(url);
            }
                , 1000);
        };
        img.src = url;

        console.log("Image URL:", url);
    } catch (error) {
        console.error("Error processing image data:", error);
    }
}

/**
 * Verifies the JPEG header of a Uint8Array.
 * JPEG files start with the bytes 0xFF 0xD8 (SOI marker) and end with 0xFF 0xD9 (EOI marker).
 */
function verifyJPEGHeader(data) {
    return data[0] === 0xFF && data[1] === 0xD8 && data[data.length - 2] === 0xFF && data[data.length - 1] === 0xD9;
}

/**
 * Swaps the byte order (endianness) of a Uint32Array in place.
 */
function swapEndianness(uint32Array) {
    for (let i = 0; i < uint32Array.length; i++) {
        const value = uint32Array[i];
        uint32Array[i] =
            ((value & 0xFF) << 24) | // Byte 0
            ((value & 0xFF00) << 8) | // Byte 1
            ((value & 0xFF0000) >> 8) | // Byte 2
            ((value >> 24) & 0xFF); // Byte 3
    }
}

// function displayImage(imageData) {
//     try {
//         const castedData = reinterpretAsUint32Array(imageData);
//         console.log("Casted Data:", castedData);
//     }
//     catch (error) {
//         console.error("Error:", error);
//     }
//     finally {
//         const blob = new Blob([castedData], { type: "image/jpeg" });
//         const url = URL.createObjectURL(blob);

//         const img = new Image();
//         img.onload = () => {
//             canvas.width = img.width;
//             canvas.height = img.height;
//             ctx.drawImage(img, 0, 0);
//             URL.revokeObjectURL(url);
//         };
//         img.src = url;
//         // Provide where to view the image
//         console.log("Image URL:", url);
//     }

// }
