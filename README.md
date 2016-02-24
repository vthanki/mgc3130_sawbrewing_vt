

#MGC3130 Sawbrewing

Simple test program for Raspberry, can also be used with a standard serial drive (115200 standard no parity)

Make sure you run the values if using the standard open, the raspberry wiringpi library will take care of it.
```
stty -F /dev/ttyS0 speed 115200 -parity
```

optional using the wiringpi library (set this with cmake options):

##wiringpi
```
#From here:
git clone git://git.drogon.net/wiringPi
#or here (sometimes down the above one)
wget wget https://git.drogon.net/\?p\=wiringPi\;a\=snapshot\;h\=78b5c323b74de782df58ee558c249e4e4fadd25f\;sf\=tgz -O wiringpi.tar.gz
tar zxf wiringpi.tar.gz
cd wiringpi
./build
```
#Build 
make a directory build in the mgc3130 folder and create the makefiles with cmake ..

```
mkdir build
cd build
cmake ..
make
#run it
./mgc3130 /dev/tty_YOURUSB_THING
```

you get some output like this:
```
GestCode=31 TapE DTapW DTapE DTapC

 TchC
 TapC
 TchE
 TapE
 GestCode=3 east-west Flick

 GestCode=2 west-east Flick

 GestCode=1 garbage

 GestCode=1 garbage

 GestCode=2 west-east Flick

 GestCode=3 east-west Flick
 ```
