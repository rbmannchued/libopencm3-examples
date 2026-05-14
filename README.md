# Libopencm3 STM32 examples

These are examples I used to learn basic STM32 and libopencm3 programming. The code is not always the best but work. Maybe it can help you :)

# Dependencies

- gcc-arm-none-eabi
- stlink-tools
# Usage

After you clone the repo, you will need to download the submodules so:

``` 
git submodules init
git submodules update
```

Then compile the libopencm3:
```
cd libopencm3
make
```
After that just cd to the chosen example and ```make``` to build and ```make flashbin``` to flash the binaries using ST-LINK.

