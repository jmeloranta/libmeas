PVCAM Beta Support for USB

The PVCAM library beginning with version 2.7.1.6 supports PI USB devices.  
When compiling your application, an additional parameter needs to be passed to support threading.  
This parameter is -lpthread.
Also if the USB device is a Pixis, the user buffer allocated by the application MUST be a multiple of an even number of frames.  Previously a size of 3 times the frame size was recommended, so for a Pixis the user buffer should be at least 4 times the frame size.
The .dat files must be copied into /usr/lib.
