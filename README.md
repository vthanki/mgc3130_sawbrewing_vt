

#MGC3130 Sawbrewing

Simple test program for Raspberry, can also be used with a standard serial drive (115200 standard no parity)

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
#Multimediay Key Mapping
By providing --mapped command line option also emits multimedia key strokes for following events.

Touch-North --> Volume UP Key

Touch-South --> Volume Down Key

Touch-East  --> Next Song Key

Touch-West  --> Previous Song Key

Flick West-to-East --> Forward Key

Flick East-to-West --> Rewind Key
