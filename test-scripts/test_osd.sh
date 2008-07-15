#!/bin/sh
##############################################################################
#
# Description: test script for osd hardware verifying
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
	echo "TEST IDE HDD:" >> ${LOGFILE}
	hdparm -i /dev/hda 1>>${LOGFILE} 2>>${LOGFILE}
	if [ $? -eq 0 ]
	then
		echo "IDE HDD TEST OK"
	else
		echo "IDE HDD TEST FAILED"
	fi
}

# route for audio testing
test_audio()
{
	echo "TEST AUDIO:" >> ${LOGFILE}
}

# route for video testing
test_vedio()
{
	echo "TEST VIDEO:" >> ${LOGFILE}
}

# BOF main
prepare_for_test
test_hdd
test_audio
test_vedio
# EOF main

