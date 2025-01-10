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
let currentPhase = "boundary"; // Can be "boundary", "length", "image", "end"
// const PART_BOUNDARY = "123456789000000000000987654321";
// const START_BOUNDARY = "\r\n--" + PART_BOUNDARY + "--START--\r\n";
// const END_BOUNDARY = "\r\n--" + PART_BOUNDARY + "--END--\r\n";

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
    console.log("Chunk Length:", chunk.length, "Total Bytes:", totalBytes);

    if (chunk.length === 36) {
        console.log("START OF STREAM");
        return;
    }

    // Convert the chunk to a string to check for boundaries (boundary phase)
    if (currentPhase === "boundary") {
        const chunkString = new TextDecoder().decode(chunk).replace(/\r\n/g, "");
        if (chunkString.includes(STREAM_BOUNDARY)) {
            console.log("Stream boundary detected!");
            currentPhase = "length"; // Move to the next phase
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
            imageLength = imageLength;
            console.log("Image length:", imageLength);

            // Remove the length bytes from the current chunk
            chunk = chunk.slice(4);

            currentPhase = "image"; // Move to the image phase
            processStream(chunk); // Process the remaining chunk
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

        // Check if the full image has been received
        if (totalBytes >= imageLength) {
            console.log("Image received!");
            const imageData = concatenateChunks(receivedChunks, totalBytes).slice(0, imageLength);

            // Reset state for the next stream
            receivedChunks = [];
            totalBytes = 0;
            imageLength = null;
            currentPhase = "end"; // Expecting the closing boundary

            console.log("Received This Many Bytes:", imageData.length);
            displayImage(imageData); // Process and display the image
        }
    }

    // Detect closing stream boundary (end phase)
    if (currentPhase === "end") {
        const chunkString = new TextDecoder().decode(chunk);
        if (chunkString.includes(STREAM_BOUNDARY)) {
            console.log("Stream boundary detected at end!");
            currentPhase = "boundary"; // Ready for the next stream
            receivedChunks = [];
            totalBytes = 0;
            expectedLength = null;
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


function displayImage(imageData) {
    console.log("Displaying image...");
    const blob = new Blob([imageData], { type: "image/jpeg" });
    const url = URL.createObjectURL(blob);

    console.log("VALID JPEG HEADER:", verifyJPEGHeader(imageData));

    const img = new Image();
    img.onload = () => {
        if (img.complete) {
            console.log("Image loaded successfully!");
            canvas.width = img.width;
            canvas.height = img.height;
            ctx.drawImage(img, 0, 0);

            // // Create a link to view/download the image
            // const link = document.createElement("a");
            // link.href = url;
            // link.textContent = "View or Download Image";
            // link.download = "image.jpeg"; // Suggested file name for download
            // link.target = "_blank"; // Open in a new tab
            // document.body.appendChild(link);
        } else {
            console.error("Image failed to load.");
        }
        // Revoke the URL after some time to free memory
        setTimeout(() => URL.revokeObjectURL(url), 1000);
    };
    img.src = url;
    // document.body.appendChild(img);
    // Provide where to view the image
    console.log("Image URL:", url);

    console.log("Checking for corruption...");
    // console.log(binarySearchCorruption(imageData));
}

function verifyJPEGHeader(byteArray) {
    if (byteArray.length < 4) {
        return false; // Too short to be a valid JPEG
    }

    // Check Start of Image (SOI) marker
    const isSOI = byteArray[0] === 0xFF && byteArray[1] === 0xD8;

    // Check End of Image (EOI) marker
    const isEOI = byteArray[byteArray.length - 2] === 0xFF &&
        byteArray[byteArray.length - 1] === 0xD9;

    return isSOI && isEOI;
}

function binarySearchCorruption(imageData) {
    let start = 0;
    let end = imageData.length;
    let mid;

    while (start < end) {
        mid = Math.floor((start + end) / 2);
        const partialData = imageData.slice(0, mid);
        const blob = new Blob([partialData], { type: "image/jpeg" });
        const url = URL.createObjectURL(blob);

        const img = new Image();
        img.onload = () => {
            console.log(`Valid up to byte ${mid}`);
            start = mid + 1; // Move forward
        };
        img.onerror = () => {
            console.log(`Corruption begins at byte ${mid}`);
            end = mid; // Move backward
        };
        img.src = url;
    }
}