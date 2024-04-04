#!/bin/sh
# -----------------------------------------------------------
# Tools (yjogol)
# $Date$
# $Revision$
# -----------------------------------------------------------
. ./_Y_Globals.sh
. ./_Y_Library.sh
# ===========================================================
# Settings : Styles
# ===========================================================
# -----------------------------------------------------------
# Style List
# -----------------------------------------------------------
style_get()
{
	check_Y_Web_conf
	active_style=$(config_get_value_direct $y_config_Y_Web 'style')

	y_path_directory=$(config_get_value_direct $y_config_nhttpd 'WebsiteMain.directory')

	style_list=""
	style_list="$style_list $(find $y_path_directory/styles -name 'Y_Dist-*')"

	f_list=""
	html_option_list=""
	for f in $style_list
	do
		echo $f_list | grep ${f##*/}
		if [ $? == 0 ]; then
			continue
		fi
		f_list="$f_list ${f##*/}"

		style=$(echo "$f" | sed -e s/^.*Y_Dist-//g | sed -e s/.css//g)
		if [ "$style" = "$active_style" ]
		then
			sel="selected='selected'"
		else
			sel=""
		fi
		opt="<option value='$style' $sel>${style//_/ }</option>"
		html_option_list="$html_option_list $opt"
	done
	echo "$html_option_list"
}
# -----------------------------------------------------------
# Set Style: override Y_Main.css   $1=Style-Name
# -----------------------------------------------------------
style_set()
{
	# This function should be called one time after installing a new image
	# to get sure, the right skin is installed too

	style=${1:-$(config_get_value_direct $y_config_Y_Web 'style')}
	test -n "$style" || return

	y_path_directory=$(config_get_value_direct $y_config_nhttpd 'WebsiteMain.directory')

	cd $y_path_directory
	if [ -e $y_path_directory/styles/Y_Dist-$style.css ]; then
		cp $y_path_directory/styles/Y_Dist-$style.css Y_Dist.css
	else
		config_set_value_direct $y_config_Y_Web 'style'
	fi
}

# -----------------------------------------------------------
# copies Uploadfile to $1
# -----------------------------------------------------------
upload_copy()
{
	if [ -s "$y_upload_file" ]
	then
		cp "$y_upload_file" "$1"
	else
		msg="Upload-Problem.<br>Try again, please."
	fi
}

# -----------------------------------------------------------
zapit_upload()
{
	msg="$1 hochgeladen<br><a href='/Y_Settings_zapit.yhtm'><u>next file</u></a>"
	upload_copy "$y_path_zapit/$1"
	y_format_message_html
}
# -----------------------------------------------------------
# Mount from Neutrino-Settings $1=nr
# -----------------------------------------------------------
do_mount()
{
	config_open $y_config_neutrino
	fstype=`config_get_value "network_nfs_type_$1"`
	ip=`config_get_value "network_nfs_ip_$1"`
	local_dir=`config_get_value "network_nfs_local_dir_$1"`
	dir=`config_get_value "network_nfs_dir_$1"`
	options1=`config_get_value "network_nfs_mount_options1_$1"`
	options2=`config_get_value "network_nfs_mount_options2_$1"`
	username=`config_get_value "network_nfs_username_$1"`
	password=`config_get_value "network_nfs_password_$1"`

	# check options
	if [ "$options1" = "" ]
	then
		options1=options2
		options2=""
	fi

	# default options
	if [ "$options1" = "" ]
	then
		if [ "$options2" = "" ]
		then
			if [ "$fstype" = "0" ]
			then
				options1="ro,soft,udp"
				options2="nolock,rsize=8192,wsize=8192"
			fi
			if [ "$fstype" = "1" ]
			then
				options1="ro"
			fi
		fi
	fi
	# build mount command
	case "$fstype" in
		0) #nfs
			cmd="mount -t nfs $ip:$dir $local_dir -o $options1"
			;;
		1)
			cmd="mount -t cifs $ip/$dir $local_dir -o username=$username,password=$password,unc=//$ip/$dir,$options1";
			;;
		default)
			echo "mount type not supported"
	esac

	if [ "$options2" != "" ]
	then
		cmd="$cmd,$options2"
	fi
	res=`$cmd`
	echo "$cmd" >/tmp/mount.log
	echo "$res" >>/tmp/mount.log
	echo "$res"
	echo "view mounts"
	m=`mount`
	msg="mount cmd:$cmd<br><br>res=$res<br>view Mounts;<br>$m"
	y_format_message_html
}
# -----------------------------------------------------------
# unmount $1=local_dir
# -----------------------------------------------------------
do_unmount()
{
	umount $1
}
# -----------------------------------------------------------
# Execute shell command
# 1: directory 2: append [true|false] 3+: cmd
# -----------------------------------------------------------
do_cmd()
{
	cd $1
	pw1="$1"
	app="$2"
	shift 2

	if [ "$1" = "cd" ]
	then
		cd $2
	else
		tmp=`$*` #Execute command
	fi
	pw=`pwd`
	echo '<html><body><form name="o"><textarea name="ot">'
	echo "$pw1>$*"
	echo "$tmp"
	echo '</textarea></form>'
	echo '<script language="JavaScript" type="text/javascript">'
	if [ "$app" = "true" ]
	then
		echo 'parent.document.co.cmds.value += document.o.ot.value;'
	else
		echo 'parent.document.co.cmds.value = document.o.ot.value;'
	fi
	echo "parent.set_pwd('$pw');"
	echo 'parent.setCaretToEnd(parent.document.co.cmds);'
	echo 'parent.document.f.cmd.focus();'
	echo '</script></body></html>'
}
# -----------------------------------------------------------
# view /proc/$1 Informations
# -----------------------------------------------------------
proc()
{
	msg=`cat /proc/$1`
	msg="<b>proc: $1</b><br><br>$msg"
	y_format_message_html
}
# -----------------------------------------------------------
# wake up $1=MAC
# -----------------------------------------------------------
wol()
{
	if [ -e $y_path_sbin/ether-wake ]; then
		msg=`ether-wake $1`
	fi
	msg="<b>Wake on LAN $1</b><br><br>$msg"
	y_format_message_html
}

# -----------------------------------------------------------
# osd shot
# $1= fbshot | grab
# -----------------------------------------------------------
do_fbshot()
{
	if [ "$1" = "fbshot" ]; then
		shift 1
		if [ -e "$y_path_varbin/fbshot" ]; then
			$y_path_varbin/fbshot $*
		else
			fbshot $*
		fi
	elif [ "$1" = "grab" ]; then
		shift 1
		if [ -e "$y_path_varbin/grab" ]; then
			$y_path_varbin/grab $*
		else
			grab $*
		fi
	fi
}
# -----------------------------------------------------------
# delete fbshot created graphics
# -----------------------------------------------------------
do_fbshot_clear()
{
	rm /tmp/*.bmp
	rm /tmp/*.png
}
# -----------------------------------------------------------
# delete screenshots
# -----------------------------------------------------------
do_screenshot_clear()
{
	rm -f /tmp/*.png
}
# -----------------------------------------------------------
# restart neutrino
# -----------------------------------------------------------
restart_neutrino()
{
	echo "fixme"
#	kill -HUP `pidof neutrino`
}
# -----------------------------------------------------------
# Main
# -----------------------------------------------------------
#debug
# echo "call:$*" >> "/tmp/debug.txt"
case "$1" in
	style_set)			style_set $2 ;;
	style_get)			style_get ;;
	zapit_upload)			zapit_upload $2 ;;
	kernel-stack)			msg=`dmesg`; y_format_message_html ;;
	ps)						msg=`ps l`; y_format_message_html ;;
	free)					f=`free`; p=`df -h`; msg="RAM Memory use\n-------------------\n$f\n\nPartitions\n-------------------\n$p"
							y_format_message_html ;;
	yreboot)				reboot; echo "Reboot..." ;;
	check_yWeb_conf) 		check_Y_Web_conf ;;
	rcsim)					rcsim $2 >/dev/null ;;
	domount)				shift 1; do_mount $* ;;
	dounmount)				shift 1; do_unmount $* ;;
	cmd)					shift 1; do_cmd $* ;;
	installer)				shift 1; do_installer $* 2>&1 ;;
	proc)					shift 1; proc $* ;;
	wol)					shift 1; wol $* ;;
	fbshot)					shift 1; do_fbshot $* ;;
	fbshot_clear)			do_fbshot_clear ;;
	screenshot_clear)		do_screenshot_clear ;;
	restart_neutrino)		restart_neutrino ;;

	mtd_space|var_space)
		df | while read fs rest; do
			case ${fs:0:3} in
				mtd)
					echo "$fs" "$rest"
					break
				;;
			esac
		done
		;;
	tmp_space)
		df /tmp|grep /tmp
		;;
	*)
		echo "[Y_Tools.sh] Parameter wrong: $*" ;;
esac



