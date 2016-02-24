

#MGC3130 Sawbrewing

Simple program for Raspberry, but is easily rewritten for using a standard serial drive (115200 standard no parity)

Needs the wiringpi library:

```
#From here:
git clone git://git.drogon.net/wiringPi
#or here (sometimes down the above one)
wget wget https://git.drogon.net/\?p\=wiringPi\;a\=snapshot\;h\=78b5c323b74de782df58ee558c249e4e4fadd25f\;sf\=tgz -O wiringpi.tar.gz
tar zxf wiringpi.tar.gz
cd wiringpi
./build
```

make a directory build in the mgc3130 folder and create the makefiles with cmake ..

```
mkdir build
cd build
cmake ..
make
./mgc3130 /dev/tty_YOURUSB_THING
```



