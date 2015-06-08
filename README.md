# ProcSpy
A tool I occasionally use for snooping on processes

On linux:
---------

Compiling:
`gcc -o procspy.so procspy.c -shared -fPIC -std=c99`

Running:
`LD_PRELOAD="$(pwd)/procspy.so" /path/to/executable`

Example:

![Image of Linux Usage](https://i.imgur.com/n8VL8rz.png)

On OS X:
--------
Compiling:
`gcc -o procspy.dylib procspy.c -dynamiclib`

Running:
`DYLD_INSERT_LIBRARIES=procspy.dylib /path/to/executable`

Example:

![Image of OS X Usage](https://i.imgur.com/lA7cvsp.png)
