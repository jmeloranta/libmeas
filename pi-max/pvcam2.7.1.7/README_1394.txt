PVCAM Support for IEEE1394( FireWire )

The PVCAM library beginning with version 2.7.1.7 supports PI 1394 devices.  
When compiling your application, an additional parameter needs to be passed to support FireWire.  
This parameter is -lraw1394.

At the time of the writing of this document, the current released Kernel is 2.6.18.
There is a problem with IEEE1394 support with all 2.6.X Kernels up to and including 2.6.18.
Therefore an unofficial patch is included in this archive which allows the user to patch the driver.

*Note*
Before running your application, 'modprobe raw1394' must be executed from a command line.
