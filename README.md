
# WheatSystem for WheatBox 55A1

This is a C implementation of [WheatSystem](http://www.ostracodfiles.com/wheatsystem/menu.html) for the WheatBox 55A1 computer. I hope that a [Tractor](https://github.com/ostracod/tractor) implementation will supersede this one eventually. However, I need to satisfy my craving to build a cute little computer, so the Tractor version will need to wait. The code in this repo is largely derived from my older [wheatsystem-c](https://github.com/ostracod/wheatsystem-c) repo.

## Usage

This project has the following system-wide dependencies:

* avr-gcc
* gcc (to compile the emulator for Unix-like systems)
* Node.js version 16 or later (to assemble example volumes)

To compile the AVR firmware:

```
make avr
```

To compile the emulator:

```
make unix
```

To assemble example volumes:

1. Clone the [WheatBytecode assembler](https://github.com/ostracod/wheatbytecode-asm) in the parent directory of `wheatsystem-c55a1`.
1. Follow the instructions in the README of `wheatbytecode-asm` to set up the assembler.
1. Perform this command from within the `wheatsystem-c55a1` repo: `node ./assembleVolumes.js`

To run the emulator:

```
./build/main_unix (volumeFilePath)
```


