
import { SerialPort } from "serialport";

const serialPortPath = "/dev/tty.usbserial-AB0JTOQV";

console.log("Opening serial port...");

const serialPort = new SerialPort({ path: serialPortPath, baudRate: 9600 });

const timerEvent = () => {
    const value = Math.floor(Math.random() * 100);
    console.log(`Sending serial value ${value}...`);
    serialPort.write(Buffer.from([value]));
};

serialPort.on("open", () => {
    console.log("Serial port is open.");
    setInterval(timerEvent, 1000);
});

serialPort.on("data", (data: Buffer) => {
    console.log("Received serial data: " + data.toString("utf8"));
});


