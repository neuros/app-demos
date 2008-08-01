NTSC_H=480
NTSC_W=720
NTSC_MODE="U:720x480i-50"

PAL_H=576
PAL_W=720
PAL_MODE="U:720x576i-50"

M480P_H=480
M480P_W=720
M480P_MODE="U:720x480p-30"

M720P_H=720
M720P_W=1280
M720p_MODE="U:1280x720p-30"


if [ "$1" == "mode" ] ; then
	if [ "$2" == "pal" ] ; then
		W=$PAL_W
		H=$PAL_H
		MODE=$PAL_MODE
	elif [ "$2" == "ntsc" ] ; then
		W=$NTSC_W
		H=$NTSC_H
		MODE=$NTSC_MODE
	elif [ "$2" == "720p" ] ; then
		W=$M720P_W
		H=$M720P_H
		MODE=$M720p_MODE
	elif [ "$2" == "480p" ] ; then
		W=$M480P_W
		H=$M480P_H
		MODE=$M480p_MODE
	else
		echo "Mode \"$2\" not supported. Valid modes are ntsc,pal,480p,720p"
		exit 1
	fi

	# set all slave planes to the right size, the switch master plane using sysfs
	for plane in osd1 osd0 vid1 ; do fb-demo $plane output $W $H ; done
	echo $MODE > /sys/class/graphics/fb1/mode

elif [ "$1" == "plane" ] ; then
	PLANE=X
	for plane in vid0 vid1 osd0 osd1 ; do
		if [ "$2" == "$plane" ] ; then
			PLANE=$plane
		fi
	done
	if [ "$PLANE" == "X" ] ; then
		echo "Plane \"$2\" not supported. Valid planes are vid0 vid1 osd0 osd1"
		exit 1
	fi
	for plane in vid0 vid1 osd0 osd1 ; do
		if [ "$PLANE" == "$plane" ] ; then
			fb-demo $plane enable 1
		else
			fb-demo $plane enable 0
		fi
	done
else
	echo "Command \"$1\" not supported. Valid commands are mode,plane"
	exit 1
fi


