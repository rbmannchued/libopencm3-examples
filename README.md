# Libopencm3 stm32 examples

These are examples that I used to learn basic stm32 and libopencm3 programming. The code is not the best but works

# Usage

After you clone the repo, you will need download the submodules so:

``` 
git submodules init
git submodules update
```

then compile the libopencm3:
```
cd libopencm3
make
```
after that just cd to the choosen example and ```make``` to build and ```make flashbin``` to flash the binaries using ST-LINK.

