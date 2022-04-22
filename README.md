
# WheatSystem for WheatBox 55A1

This is a C implementation of [WheatSystem](http://www.ostracodfiles.com/wheatsystem/menu.html) for the WheatBox 55A1 computer. I hope that a [Tractor](https://github.com/ostracod/tractor) implementation will supersede this one eventually. However, I need to satisfy my craving to build a cute little computer, so the Tractor version will need to wait.

## Compilation

This project has the following system-wide dependencies:

* avr-gcc
* gcc (to compile the emulator for Unix-like systems)

To compile the AVR firmware:

```
make avr
```

To compile the emulator:

```
make unix
```


