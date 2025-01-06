document.getElementById('connectButton').addEventListener('click', async () => {
    const statusElement = document.getElementById('status');
    const dataElement = document.getElementById('data');

    const SERVICE_ID = "0000180a-0000-1000-8000-00805f9b34fb"
    const SERVICE_DATA_IMAGES_UUID = "12345678-1234-5678-1234-56789abcdef0"
    const CHAR_DATA_IMAGES_UUID = "12345678-1234-5678-1234-56789abcdef1"

    const MAC_ADDRESS = "2c:cf:67:ac:68:82"

    console.log("MADE CHANGES");
    try {
        console.log('Requesting Bluetooth Device...');
        // Connect to the device with a specific MAC address
        // Identify through mac address
        const device = await navigator.bluetooth.requestDevice({
            acceptAllDevices: true,
            // filters: [
            //     { name: 'PECAN' }
            // ],
            optionalServices: [SERVICE_ID, SERVICE_DATA_IMAGES_UUID],
        });
        statusElement.textContent = 'Status: Connecting to the device';
        // write to status element the device name, or none found if device is none through a ternary operator
        // Wait for ten seconds using system sleep

        // // Request a BLE device
        // const device = await navigator.bluetooth.requestDevice({
        //     acceptAllDevices: true,
        //     optionalServices: ['battery_service'] // Replace with the service UUID you need
        // });
        // console.log('Device: ', device);

        // statusElement.textContent = `Status: Connecting to ${device.name || 'Unknown Device'}`;

        // Connect to the GATT server
        const server = await device.gatt.connect();

        statusElement.textContent = `Status: Connected to ${device.name || 'Unknown Device'}`;
        // Wait until the server is connected 

        // Do not move ahead until the server is connected
        console.log('Connected to GATT Server');

        // List out all services
        console.log('Getting Services...');
        const all_services = await server.getPrimaryServices();

        const serviceListElement = document.getElementById('services');
        all_services.forEach(service => {
            console.log('Service: ', service.uuid);
            console.log(service)
            const serviceItem = document.createElement('ul');
            serviceItem.innerHTML = `<li>Service: ${service.uuid}</li>`;
            serviceListElement.appendChild(serviceItem);
        });

        const service_system = await server.getPrimaryService('device_information');
        // console.log(service_system);

        statusElement.textContent = 'Status: Getting Services';

        const characteristics = await service_system.getCharacteristics();
        characteristics.forEach(characteristic => {
            console.log('Characteristic: ', characteristic.uuid);
            console.log(characteristic)
            const charItem = document.createElement('ul');
            charItem.innerHTML = `<li>Characteristic: ${characteristic.uuid}</li>`;
            serviceListElement.appendChild(charItem);
        });

        console.log("Connecting to images service: ", SERVICE_DATA_IMAGES_UUID);
        const serviceDataImages = await server.getPrimaryService(SERVICE_DATA_IMAGES_UUID);
        console.log(serviceDataImages);

        console.log("Getting Characteristics of the image service: ", SERVICE_DATA_IMAGES_UUID);
        const imageCharacteristic = await serviceDataImages.getCharacteristic(CHAR_DATA_IMAGES_UUID);


        console.log("Image Characteristics: ", imageCharacteristic);
        console.log("Notifying image characteristic: ", imageCharacteristic.properties.notify);
        if (imageCharacteristic.properties.notify) {
            imageCharacteristic.addEventListener('imageCharecteristicValueChanged', (event) => {
                const value = event.target.value;
                console.log('Characteristic value changed:', value);
            });
        } else {

            console.error('Characteristic does not support notifications');

        }
        imageCharacteristic.startNotifications().then(() => {
            imageCharacteristic.addEventListener('characteristicvaluechanged', (event) => {
                const value = event.target.value;
                const printableValue = new TextDecoder().decode(value);
                console.log('Characteristic value changed:', printableValue);
            });
        });



        // console.log("Image Characteristics: ", imageCharacteristics);
        // imageCharacteristics.forEach(characteristic => {
        //     console.log('Characteristic: ', characteristic.uuid);
        //     console.log(characteristic)
        //     const charItem = document.createElement('ul');
        //     charItem.innerHTML = `<li>Characteristic: ${characteristic.uuid}</li>`;
        //     serviceListElement.appendChild(charItem);
        // });
        // Access a service
        // const service = await server.getPrimaryService(SERVICE_ID);

        // // Access a characteristic
        // const characteristic = await service.getCharacteristic('00002A29-0000-1000-8000-00805F9B34FB');

        // // Read the characteristic value
        // const value = await characteristic.readValue();
        // const batteryLevel = value.getUint8(0);

        // dataElement.textContent = `Data: Battery Level is ${batteryLevel}%`;
    } catch (error) {
        // console.error(error.mnes);
        statusElement.textContent = `Status: Error - ${error.message}`;
    }
});