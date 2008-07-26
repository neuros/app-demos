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
	if [ -e /dev/hda ]
	then
		hdparm -i /dev/hda 1>>${LOGFILE} 2>>${LOGFILE}
		if [ $? -eq 0 ]
		then
			echo "ide hdd test ok"
			echo "ide hdd test ok" >> ${LOGFILE}
		else
			echo "ide hdd test failed"
			echo "ide hdd test failed" >> ${LOGFILE}
		fi
	else
		echo "ide hdd test failed"
		echo "ide hdd test failed" >> ${LOGFILE}
		return 1
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

# BOF main
currdir=`dirname $0`                                 
cd ${currdir}
prepare_for_test
echo ""
test_hdd
echo ""
test_ir_blaster
echo ""
# EOF main

