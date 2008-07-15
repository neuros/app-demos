@ECHO OFF
REM -----------------------------------------------------------------
REM
REM Description: Test script for osd network function
REM
REM Note: This script running on windows and ping osd target board.
REM       when an osd board testing is finished, just plugin another
REM       one, do not need to re-run the batch file.
REM
REM version: 1.0
REM -----------------------------------------------------------------

ECHO ---------------------------------------------------------------
ECHO "                Testing OSD network function                 "
ECHO " Note:                                                       "
ECHO "   This script pings osd target board. If everything is ok,  "
ECHO "   some messages will be printed on screen like this:        "
ECHO "                                                             "
ECHO "      Reply from 192.168.1.100: bytes=32 time<10ms TTL=64    "
ECHO "      Reply from 192.168.1.100: bytes=32 time<10ms TTL=64    "
ECHO "      Reply from 192.168.1.100: bytes=32 time<10ms TTL=64    "
ECHO "                                                             "
ECHO " when an osd board testing is finished, just plugin another  "
ECHO " one, do not need to re-run this script.                     "
ECHO "             Press CTRL+C to exit this script                "
ECHO ---------------------------------------------------------------
SET OSDIP=192.168.1.100
ECHO Input OSD board ip address and press ENTER:
ECHO default: [ %OSDIP% ]

set /p OSDIP=
ping -t %OSDIP%
