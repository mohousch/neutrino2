#!/bin/sh
# -----------------------------------------------------------
# Live (yjogol)
# $Date: 2010-03-05 06:37:09 +0100 (Fr, 05 M�r 2010) $
# $Revision: 451 $
# -----------------------------------------------------------

. ./_Y_Globals.sh
. ./_Y_Library.sh

# -----------------------------------------------------------
live_lock()
{
	# un/lock lcd
	if [ "$boxtype" != "generic" ]; then
		call_webserver "control/lcd?lock=1&clear=1&rect=10,10,110,50,1,0&xpos=20&ypos=27&size=22&font=2&text=%20%20%20%20yWeb%0A%20%20LiveView&update=1" >/dev/null
	fi

	call_webserver "control/rc?lock" >/dev/null
	call_webserver "control/zapto?stopplayback" >/dev/null
}

# -----------------------------------------------------------
live_unlock()
{
	# un/lock lcd
	if [ "$boxtype" != "generic" ]; then
		call_webserver "control/lcd?lock=0" >/dev/null
	fi

	call_webserver "control/rc?unlock"  >/dev/null
	call_webserver "control/zapto?startplayback" >/dev/null
}

# -----------------------------------------------------------
prepare_tv()
{
	# SPTS on
	if [ "$boxtype" != "generic" ]; then
		call_webserver "control/system?setAViAExtPlayBack=spts" >/dev/null
	fi
}

# -----------------------------------------------------------
prepare_radio()
{
	# SPTS off
	if [ "$boxtype" != "generic" ]; then
		call_webserver "control/system?setAViAExtPlayBack=pes" >/dev/null
	fi
}

# -----------------------------------
# Main
# -----------------------------------
echo "$1" >/tmp/debug.txt
echo "$*"
case "$1" in
	zapto)
		if [ "$2" != "" ]
		then
			call_webserver "control/zapto?$2" >/dev/null
		fi
		;;

	switchto)
		if [ "$2" = "Radio" ]
		then
			call_webserver "control/setmode?radio" >/dev/null
		else
			call_webserver "control/setmode?tv" >/dev/null
		fi
		;;

	url)
		url=`buildStreamingRawURL`
		echo "$url" ;;

	audio-url)
		url=`buildStreamingAudioRawURL`
		echo "$url" ;;

	live_lock)
		live_lock ;;

	live_unlock)
		live_unlock ;;

	dboxIP)
		buildLocalIP ;;

	prepare_radio)
		prepare_radio
		Y_APid=`call_webserver "control/yweb?radio_stream_pid"`
		url="http://$2:31338/$Y_APid"
		echo "$url" > $y_tmp_m3u
		echo "$url" > $y_tmp_pls
		;;

	prepare_tv)
		prepare_tv
		;;

	*)
		echo "Parameter wrong: $*" ;;
esac



