
import * as fs from "fs";
import * as pathUtils from "path";
import * as childProcess from "child_process";
import * as url from "url";

const __dirname = url.fileURLToPath(new URL(".", import.meta.url));
const volumesDirectoryPath = pathUtils.join(__dirname, "volumes");
const assemblerDirectoryPath = pathUtils.join(__dirname, "../wheatbytecode-asm");
const fileTypeSet = {
    generic: 0,
    bytecodeApp: 1,
    systemApp: 2,
};

let logIndentationLevel = 0;

class AssemblerError extends Error {
    
    constructor(assemblerMessages) {
        super();
        this.assemblerMessages = assemblerMessages;
    }
}

class WheatSystemFile {
    
    constructor(name, attributes, content) {
        this.name = name;
        this.attributes = attributes;
        this.content = content;
    }
}

class WheatSystemVolume {
    
    constructor() {
        const buffer = Buffer.alloc(4);
        buffer.writeUInt32LE(0, 0);
        this.buffers = [buffer];
        this.address = buffer.length;
    }
    
    addFile(file) {
        const lastBuffer = this.buffers[this.buffers.length - 1];
        const offset = (this.buffers.length > 1) ? 6 : 0;
        lastBuffer.writeUInt32LE(this.address, offset);
        const header = Buffer.alloc(10);
        header.writeUInt8(file.attributes, 0);
        header.writeUInt8(file.name.length, 1);
        header.writeUInt32LE(file.content.length, 2);
        header.writeUInt32LE(0, 6);
        const buffer = Buffer.concat([header, Buffer.from(file.name), file.content]);
        this.buffers.push(buffer);
        this.address += buffer.length;
    }
    
    createBuffer() {
        return Buffer.concat(this.buffers);
    }
}

const getLogIndentation = () => {
    let output = "";
    for (let count = 0; count < logIndentationLevel; count++) {
        output += "    ";
    }
    return output;
};

const logMessage = (message) => {
    console.log(getLogIndentation() + message);
};

const logPush = (message) => {
    logMessage(message);
    logIndentationLevel += 1;
};

const logPop = (message) => {
    logIndentationLevel -= 1;
    logMessage(message);
};

const createFileAttributes = (fileConfig) => {
    const fileType = fileTypeSet[fileConfig.type];
    if (typeof fileType === "undefined") {
        throw new Error("Invalid file type. Possible file types include: " + Object.keys(fileTypeSet).join(", "));
    }
    let output = fileType;
    if (fileConfig.hasAdminPerm) {
        output |= 0x08;
    }
    if (fileConfig.isGuarded) {
        output |= 0x04;
    }
    return output;
};

const assembleBytecodeFile = (sourcePath, destinationPath) => {
    const sourceName = pathUtils.basename(sourcePath);
    logPush(`Assembling file "${sourceName}"...`);
    if (fs.existsSync(destinationPath)) {
        fs.unlinkSync(destinationPath);
    }
    const assemblerMessages = childProcess.execFileSync("node", [
        pathUtils.join(assemblerDirectoryPath, "/dist/assemble.js"),
        sourcePath,
    ]).toString();
    if (!fs.existsSync(destinationPath)) {
        throw new AssemblerError(assemblerMessages);
    }
    logPop(`Finished assembling file "${sourceName}".`);
};

const assembleVolume = (volumeDirectoryPath) => {
    const volume = new WheatSystemVolume();
    const volumeConfigPath = pathUtils.join(volumeDirectoryPath, "volumeConfig.json");
    const volumeConfig = JSON.parse(fs.readFileSync(volumeConfigPath, "utf8"));
    volumeConfig.files.forEach((fileConfig) => {
        const fileName = fileConfig.name;
        const filePath = pathUtils.join(volumeDirectoryPath, fileName);
        if (fileConfig.type === "bytecodeApp") {
            const assemblyFilePath = filePath + ".wbasm";
            if (fs.existsSync(assemblyFilePath)) {
                assembleBytecodeFile(assemblyFilePath, filePath);
            }
        }
        const fileAttributes = createFileAttributes(fileConfig);
        const fileContent = fs.readFileSync(filePath);
        const file = new WheatSystemFile(fileName, fileAttributes, fileContent);
        volume.addFile(file);
    });
    const volumePath = volumeDirectoryPath + ".dat";
    const volumeBuffer = volume.createBuffer();
    fs.writeFileSync(volumePath, volumeBuffer);
    return volumePath;
};

try {
    if (process.argv.length === 3) {
        logPush("Assembling volume...");
        const volumePath = assembleVolume(process.argv[2]);
        logPop("Finished assembling volume.");
        logMessage("Assembled volume path: " + volumePath);
    } else if (process.argv.length === 2) {
        logPush("Assembling volumes...");
        fs.readdirSync(volumesDirectoryPath).forEach((name) => {
            const volumeDirectoryPath = pathUtils.join(volumesDirectoryPath, name);
            if (fs.lstatSync(volumeDirectoryPath).isDirectory()) {
                logPush(`Assembling volume "${name}"...`);
                const volumePath = assembleVolume(volumeDirectoryPath);
                logPop(`Finished assembling volume "${name}".`);
                logMessage("Assembled volume path: " + volumePath);
            }
        });
        logPop("Finished assembling volumes.");
    } else {
        console.log("Usage: node ./assembleVolumes.js (path?)");
        process.exit(1);
    }
} catch (error) {
    if (error instanceof AssemblerError) {
        console.log("Assembler failed with the following output:");
        console.log("===============================================");
        console.log(error.assemblerMessages);
        process.exit(1);
    } else {
        throw error;
    }
}


