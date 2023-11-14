IROM boot loader from UART
==========================

This contains the code necessary to boot an Arty A7 by loading the firmware from the UART.
This produces a `cpu0_irom.vhx` file that should be baked into the bitfile.
On boot, this will listen on the UART and expects to read the file that would normally by in `cpu_iram.vhx`.

It reads data until it encounters a blank line.

**NOTE:** This is intended for testing and for software development on the CHERIoT Ibex, it is absolutely not a good idea to use this in production!

Building
--------

This is built with a simple Makefile that expects to be passed the locations of various tools and the RTOS directory as part of the make invocation:

```
$ make make CHERIOT_LLVM_ROOT=path/to/llvm/bin/ CHERIOT_RTOS_SDK=/path/to/cheriot-rtos/sdk/
```

This command will produce the `cpu0_irom.vhx` file.
Copy this into `cheriot-safe/build/firmware` and then run the `build_arty_a7` script to build the FPGA image.


Loading firmware
----------------

Make sure that you have an IRAM vhx file *that ends with a blank line*.
By default, vhx files do not and so you will need to add one.
Then:

1. Connect a serial terminal to the UART on the device.
2. Wait for the 'Ready to load firmware' message.
3. Instruct your serial terminal to paste the vhx file.
   In minicom, this is meta-z, y, then select the file.
4. Validate that the first word, last word, and length match.

If you do it correctly, you will see something like this in the UART output:

```
Ready to load firmware
Starting loading.  First word was: 40812A15
..............................................................................
Finished loading.  Last word was: 020000B8
Number of words loaded to IRAM: 00004DBA
Loaded firmware, jumping to IRAM.
```

The UART will then usually be reinitialised by the firmware that you've loaded.

The loader runs the UART at 115,200 Baud, eight data bits, one stop bit, no flow control.

Modifying
---------

The assembly stub runs with
Feel free to submit patches to improve this.


