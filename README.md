
# WheatSystem for WheatBox 55A1

This is a C implementation of [WheatSystem](http://www.ostracodfiles.com/wheatsystem/menu.html) for the WheatBox 55A1 computer. I hope that a [Tractor](https://github.com/ostracod/tractor) implementation will supersede this one eventually. However, I need to satisfy my craving to build a cute little computer, so the Tractor version will need to wait. The code in this repo is largely derived from my older [wheatsystem-c](https://github.com/ostracod/wheatsystem-c) repo.

## Usage

This project has the following system-wide dependencies:

* avr-gcc
* gcc
* Node.js version ^16.4
* TypeScript version ^4.5
* pnpm version ^6.24

To compile the AVR firmware:

```
make avr
```

To compile the emulator:

```
make unix
```

To set up utility scripts:

1. Install dependencies: `pnpm install`
1. Compile the scripts: `npm run build`

To assemble example volumes:

```
node ./dist/assembleVolumes.js
```

To run the emulator:

```
./build/main_unix (volumeFilePath)
```

The emulator also supports an integration test mode which communicates over a domain socket. Please see [wheatsystem-tests](https://github.com/ostracod/wheatsystem-tests) for tests to run:

```
./build/main_unix --integration-test (socketPath)
```

To flash the firmware:

```
make flash
```

To write an example volume to EEPROM:

1. Assemble the example volumes as described earlier.
1. Connect a USB serial cable to your WheatBox 55A1.
1. Power on WheatBox 55A1 while holding the ACTIONS button to activate storage transfer mode.
1. Write the volume: `node ./dist/writeEeprom.js (volumeFilePath)`

## Example Volumes

This repository includes the following example volumes:

* `addNumbers` provides a minimal example of a system volume. It only contains a `wsBoot` bytecode app which adds two integers in memory, and does not display the result.
* `primes` generates prime numbers and displays them using the terminal driver.
* `keys` waits for the user to press a key, and displays the corresponding key code.
* `serial` provides example usage of the serial driver. It waits to receive a byte over a serial connection, displays the byte as decimal text, and sends the decimal text back over serial. You can use the script `tsSrc/testSerialVolume.ts` to test the example volume.
* `shell` contains the files below:
    * `wsShell` is an implementation of the standard system shell. It allows multiple applications to display their own windows. To switch between windows, press the system menu key (the home key in the emulator)
    * `fileManager` allows the user to perform various file operations:
        * Launch applications
        * View file attributes
        * Modify file attributes
        * Rename files
        * Duplicate files
        * Delete files
    * `dummy` creates a window and displays a message.
    * `utils` is a library which provides functions to various applications.
* `demoApps` includes `wsShell`, `fileManager`, and `utils` as defined in the `shell` volume, and also includes several demo applications:
    * `primes` generates prime numbers and displays them in a window.
    * `calculator` performs addition, subtraction, multiplication, and division on floating-point numbers.
    * `wheatText` is a text editor for multi-line text files.
    * `hexEdit` allows the user to view and edit binary files.


