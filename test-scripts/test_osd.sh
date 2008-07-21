#!/bin/sh
##############################################################################
#
# Description: test script for osd hardware verifying
#    		This script is used only for factory testing.
#
#    This script verify the functions of osd hardware. The result will be
# printed on screen. The tester/operator simply look if the testing is ok
# or not. At the same time, output message will be saved to a log file.
# If test failed, engineer can check this log file to find out what's happened.
#
# version: 1.0
#
##############################################################################

LOGFILE=/tmp/osd_test.log

prepare_for_test()
{
	now_time=`date '+%Y-%m-%d %H:%M:%S'`
	echo "--- Test on ${now_time} ---" >> ${LOGFILE}
	
	echo "dmesg:" >> ${LOGFILE}
	dmesg >> ${LOGFILE}
}

# routine for ide hdd testing
test_hdd()
{
	echo "TEST IDE HDD:"
	echo "TEST IDE HDD:" >> ${LOGFILE}
	if [ ! -f /dev/hda ]
	then
		echo "ide hdd test failed"
		echo "ide hdd test failed" >> ${LOGFILE}
		return 1
	fi
	hdparm -i /dev/hda 1>>${LOGFILE} 2>>${LOGFILE}
	if [ $? -eq 0 ]
	then
		echo "ide hdd test ok"
		echo "ide hdd test ok" >> ${LOGFILE}
	else
		echo "ide hdd test failed"
		echo "ide hdd test failed" >> ${LOGFILE}
	fi
}

# routine for ir blaster testing
test_ir_blaster()
{
	echo "TEST IR BLASTER:"
	echo "TEST IR BLASTER:" >> ${LOGFILE}
	./blaster-test
	if [ $? -eq 0 ]
	then
		echo "ir blaster test ok"
		echo "ir blaster test ok" >> ${LOGFILE}
	else
		echo "ir blaster test failed"
		echo "ir blaster test failed" >> ${LOGFILE}
	fi
}

# routine for audio testing
test_audio()
{
	echo "TEST AUDIO:"
	echo "TEST AUDIO:" >> ${LOGFILE}
	./audio_test
	if [ $? -eq 0 ]
	then
		echo "audio test finished"
		echo "audio test finished" >> ${LOGFILE}
	else
		echo "audio test failed"
		echo "audio test failed" >> ${LOGFILE}
	fi
}

# routine for video testing
test_video()
{
	echo "TEST VIDEO:"
	echo "TEST VIDEO:" >> ${LOGFILE}
}

# BOF main
prepare_for_test
test_hdd
test_ir_blaster
test_audio
test_video
# EOF main

