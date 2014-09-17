PV_VER="2.7.1.7"


rm -f /lib/libpvcam.*

cp libpvcam.so.$PV_VER /usr/lib/
cp pi133b.dat /usr/lib/
cp pi133b2.dat /usr/lib/
cp pi133b5.dat /usr/lib/
ln -s /usr/lib/libpvcam.so.$PV_VER /usr/lib/libpvcam.so
ldconfig /usr/lib

#echo Enter The Directory to Put the example source..
#echo Note: Program will make subdirectory example.
echo Note: Examples placed in /usr/local/pvcam/examples
#read $path_examples
if [ ! -d /usr/local/pvcam ]
then
	mkdir /usr/local/pvcam
fi

if [ ! -d /usr/local/pvcam/examples ]
then
	mkdir /usr/local/pvcam/examples
fi

cp -r examples/* /usr/local/pvcam/examples/

echo Pvcam Release Version $PV_VER for Linux installation complete.


