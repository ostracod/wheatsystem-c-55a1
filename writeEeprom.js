
import { SerialPort } from "serialport";
import * as fs from "fs";

const serialPortPath = "/dev/tty.usbserial-AB0JTOQV";
const transferModeBufferSize = 128;

let hasReceivedResponseAck = false;
let responseBuffer = null;

const receiveResponseAck = () => new Promise((resolve, reject) => {
    const checkResponse = () => {
        if (!hasReceivedResponseAck) {
            setTimeout(checkResponse, 10);
            return;
        }
        hasReceivedResponseAck = false;
        resolve();
    };
    checkResponse();
});

const receiveResponseBuffer = () => new Promise((resolve, reject) => {
    const checkResponse = () => {
        if (!hasReceivedResponseAck || responseBuffer.length !== transferModeBufferSize) {
            setTimeout(checkResponse, 10);
            return;
        }
        const output = responseBuffer;
        hasReceivedResponseAck = false;
        responseBuffer = null;
        resolve(output);
    };
    checkResponse();
});

const createAddressBuffer = (address) => {
    const output = Buffer.alloc(3);
    output.writeUInt8(address & 0x0000FF, 0);
    output.writeUInt8((address & 0x00FF00) >> 8, 1);
    output.writeUInt8((address & 0xFF0000) >> 16, 2);
    return output;
}

const sendReadCommand = (address) => {
    serialPort.write(Buffer.concat([
        Buffer.from([33, 82]),
        createAddressBuffer(address),
    ]));
    return receiveResponseBuffer();
};

const sendWriteCommand = async (address, data) => {
    serialPort.write(Buffer.concat([
        Buffer.from([33, 87]),
        createAddressBuffer(address),
        data,
    ]));
    await receiveResponseAck();
};

const writeAndVerifyVolume = async (volumePath) => {
    const volumeData = fs.readFileSync(volumePath);
    for (
        let startAddress = 0;
        startAddress < volumeData.length;
        startAddress += transferModeBufferSize
    ) {
        let buffer;
        if (startAddress + transferModeBufferSize > volumeData.length) {
            buffer = volumeData.slice(startAddress, volumeData.length);
            buffer = Buffer.concat([
                buffer,
                Buffer.alloc(transferModeBufferSize - buffer.length),
            ]);
        } else {
            buffer = volumeData.slice(startAddress, startAddress + transferModeBufferSize);
        }
        console.log(`Writing at address ${startAddress}...`);
        await sendWriteCommand(startAddress, buffer);
    }
    for (
        let startAddress = 0;
        startAddress < volumeData.length;
        startAddress += transferModeBufferSize
    ) {
        console.log(`Reading at address ${startAddress}...`);
        const buffer = await sendReadCommand(startAddress);
        let offset = 0;
        let address = startAddress;
        while (offset < buffer.length && address < volumeData.length) {
            const value1 = buffer.readUInt8(offset);
            const value2 = volumeData.readUInt8(address);
            if (value1 !== value2) {
                console.log(`Found data mismatch at address ${address}! (${value1} != ${value2})`);
                return;
            }
            offset += 1;
            address += 1;
        }
    }
    console.log("Finished writing and verifying EEPROM.");
}

if (process.argv.length !== 3) {
    console.log("Usage: node ./writeEeprom.js (volumeFilePath)");
    process.exit(1);
}
const volumePath = process.argv[2];

console.log("Opening serial port...");

const serialPort = new SerialPort({ path: serialPortPath, baudRate: 9600 });

serialPort.on("open", async () => {
    console.log("Serial port is open.");
    await writeAndVerifyVolume(volumePath);
    serialPort.close();
});

serialPort.on("data", (data) => {
    if (hasReceivedResponseAck) {
        responseBuffer = Buffer.concat([responseBuffer, data]);
    } else if (data[0] === 33) {
        hasReceivedResponseAck = true;
        responseBuffer = data.slice(1, data.length);
    }
});


