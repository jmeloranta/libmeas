  PV_VER="2.7.1.7"

  if [ -e /usr/lib/libpvcam.so ]
  then
	echo removing /usr/lib/libpvcam.so
  	rm -f /usr/lib/libpvcam.so 
  fi

  echo Removing PVCam Version $PV_VER
  if [ -e /usr/lib/libpvcam.so.$PV_VER ]
  then
  	echo removing lib /usr/lib/lib/pvcam.so.$PV_VER
  	rm -f /usr/lib/libpvcam.so.$PV_VER
  fi
  
  echo Example code was not deleted.  If it is no longer needed, please delete with:
  echo rm -fr /usr/local/pvcam/examples
