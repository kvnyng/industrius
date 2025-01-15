const connectButton = document.getElementById("connectButton");
const statusText = document.getElementById("status");
const canvas = document.getElementById("imageCanvas");
const ctx = canvas.getContext("2d");

let imgServiceUuid = "12345678-0000-0001-1234-56789abcdef0";
let imgCharUuid = "12345678-0000-0001-1234-56789abcdef0";

// import { jpeg } from "jpeg-js";

connectButton.addEventListener("click", async () => {
    try {
        statusText.textContent = "Status: Scanning for devices...";
        const device = await navigator.bluetooth.requestDevice({
            acceptAllDevices: true,
            // filters: [{
            //     name: ["Neck", "Neckie", "ImgTrns"],
            // }],
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
let imageLength = null;
let dataDict = {};

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

function reset() {
    receivedChunks = [];
    totalBytes = 0;
    expectedLength = null;
    currentPhase = "boundary";
    imageLength = null;
    dataDict = {};
}

function processStream(chunk) {
    // console.log("CHUNK LENGTH:", chunk.length, "TOTAL BYTES:", totalBytes, "IMAGE LENGTH:", imageLength);

    // Start of the frame
    if (chunk.length == 45) {
        console.log("----START OF FRAME----");
        reset();
        currentPhase = "length";
        return;
    }

    if (chunk.length == 43 && !(totalBytes + 43 == imageLength)) {
        console.log("----END OF FRAME----");
        const total_packets = Math.ceil(imageLength / 227);
        const packets_received = Math.ceil(receivedChunks.length / 227);
        const missing_packets = total_packets - packets_received;

        // const imageData = concatenateChunks(receivedChunks, totalBytes);

        // Toss out the image if the length is incorrect
        if (totalBytes != imageLength) {
            console.error("Expected image length", imageLength, "but received", totalBytes, "received", packets_received, " packets", "missing", missing_packets, "packets");
            reset();
            return;
            // round up in packets

            // if (!(missing_packets < 6)) {
            //     // console.error("Data:", receivedChunks);
            //     reset();
            //     return;
            // } else {
            //     console.log("Still displaying image as it's close enough");
            // }
        } else {
            console.log("Expected image length", imageLength, "but received", totalBytes, "received", packets_received, " packets", "missing", missing_packets, "packets");

        }

        displayImage(receivedChunks); // Process and display the image

        reset();
    }

    // Read the image length (length phase)
    if (currentPhase === "length") {
        // console.log("Image Length Chunk:", chunk);
        if (chunk.length >= 4) {
            // Extract and decode the image length
            const lengthBuffer = chunk.slice(0, 4);
            imageLength = new DataView(lengthBuffer.buffer).getUint32(0, true); // Assuming little-endian
            console.log("Image length:", imageLength);

            currentPhase = "image"; // Move to the image phase
        } else {
            console.error("Incomplete length data received.");
            console.error("Expected 4 bytes, received", chunk.length);
            console.error("Data:", chunk);
        }
        return;
    }

    if (currentPhase === "image") {
        data = new Uint8Array(chunk);
        const imageNumber = data[0];
        const packetNumber = data[1];
        const packet = data.slice(2);

        // console.log("Image Number:", imageNumber, "Packet Number:", packetNumber, "Packet Length:", packet.length);

        dataDict = { ...dataDict, [packetNumber]: packet };
        // Add packets to dictionary of data

        receivedChunks.push(...packet);
        totalBytes += packet.length;
        // console.log("Total Bytes:", totalBytes);
    }
}


// function processStream(chunk) {
//     console.log("CHUNK LENGTH:", chunk.length, "TOTAL BYTES:", totalBytes,
//         "IMAGE LENGTH:", imageLength
//     );
//     // Convert the chunk to a string to check for boundaries (boundary phase)
//     if (currentPhase === "boundary") {
//         const chunkString = new TextDecoder().decode(chunk).replace(/\r\n/g, "");
//         if (chunk.length == 45) {
//             console.log("Boundary detected!");
//             currentPhase = "length"; // Move to the next phase
//         } else {
//             console.log("Boundary not detected yet.");
//         }
//         return;
//     }

//     // Read the image length (length phase)
//     if (currentPhase === "length") {
//         console.log("Image Length Chunk:", chunk);
//         if (chunk.length >= 4) {
//             // Extract and decode the image length
//             const lengthBuffer = chunk.slice(0, 4);
//             imageLength = new DataView(lengthBuffer.buffer).getUint32(0, true); // Assuming little-endian
//             console.log("Image length:", imageLength);

//             // Remove the length bytes from the current chunk
//             chunk = chunk.slice(4);

//             currentPhase = "image"; // Move to the image phase
//         } else {
//             console.error("Incomplete length data received.");
//             console.error("Expected 4 bytes, received", chunk.length);
//             console.error("Data:", chunk);
//             return;
//         }
//     }

//     // Accumulate image chunks (image phase)
//     if (currentPhase === "image") {
//         data = new Uint8Array(chunk);
//         receivedChunks.push(data);
//         totalBytes += data.length;
//         console.log("Total Bytes:", totalBytes);

//         // Check if the full image has been received
//         if (totalBytes >= 15000) {
//             console.log("Image received!");
//             const imageData = concatenateChunks(receivedChunks, totalBytes);

//             // Reset state for the next stream
//             receivedChunks = [];
//             totalBytes = 0;
//             imageLength = null;
//             currentPhase = "end"; // Expecting the closing boundary

//             displayImage(imageData); // Process and display the image
//         }
//     }

//     // Detect closing stream boundary (end phase)
//     if (currentPhase === "end") {
//         const chunkString = new TextDecoder().decode(chunk);
//         if (chunk.length == 43) {
//             console.log("Stream boundary detected at end!");
//             currentPhase = "boundary"; // Ready for the next stream
//             receivedChunks = [];
//             totalBytes = 0;
//             expectedLength = null;
//         }
//     }
// }

// // Function to concatenate received chunks
// function concatenateChunks(chunks, totalBytes) {
//     const result = new Uint8Array(totalBytes);
//     let offset = 0;
//     for (const chunk of chunks) {
//         result.set(chunk, offset);
//         offset += chunk.length;
//     }
//     return result;
// }


// function decodeJPEG(byteArray) {
//     const rawImageData = jpeg.decode(byteArray, { useTArray: true });
//     return rawImageData;
// }
// function bufferToBase64(buffer) {
//     return btoa(String.fromCharCode.apply(null, new Uint8Array(buffer)));
// }

let lastTime = Date.now();
function displayImage(imageData) {
    // Calculate fps
    const fps = 1000 / (Date.now() - lastTime);
    lastTime = Date.now();
    console.log("FPS:", fps);

    const array = new Uint8Array(imageData);
    console.log("Received image data:", array);
    // Step 1: Create a Blob from the byte array
    const blob = new Blob([array], { type: 'image/jpeg' });

    // Step 2: Create an object URL for the Blob
    const imageUrl = URL.createObjectURL(blob);

    // Step 3: Set the `src` attribute of the image element
    // const imgElement = document.getElementById('image');
    // imgElement.src = imageUrl;

    const img = new Image();
    img.onload = () => {
        canvas.width = img.width;
        canvas.height = img.height;
        ctx.drawImage(img, 0, 0);
    };
    img.src = imageUrl;

    console.log("Image URL:", imageUrl);

    // const img = new Image();
    // img.onload = () => {
    //     canvas.width = img.width;
    //     canvas.height = img.height;
    //     ctx.drawImage(img, 0, 0);
    //     // Revoke image URL only after 1 second:
    //     setTimeout(() => {
    //         URL.revokeObjectURL(url);
    //     }, 5000);
    // };
    // img.src = url;
    // // Provide where to view the image
    // console.log("Image URL:", url);
}