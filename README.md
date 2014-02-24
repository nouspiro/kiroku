Kiroku
======

Capture OpenGL application on Linux.

Dependencies:
* Qt5
* gstreamer1.0

On 64 bit system to build 32 bit inject library
* gcc-multilib g++-multilib

Compile:

    $ qmake
    $ make

To record OpenGL application you must set LD_PRELOAD=libkiroku.so
After that you can start GUI application and you can start recording.

In Steam you can set LD_PRELOAD by right clicking on game, select Properties
where you click SET LAUNCH OPTIONS. Enter "LD_PRELOAD=libkiroku.so %command%"
without quotes. Of course libkiroku.so must be where it can be found by linker
or use absolute path.

Current limitations:
* Start the GUI after you launch OpenGL application
* Don't resize OpenGL window after GUI start
