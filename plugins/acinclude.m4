AC_DEFUN([TUXBOX_APPS],[
AM_CONFIG_HEADER(config.h)
AM_MAINTAINER_MODE

AC_GNU_SOURCE
AC_SYS_LARGEFILE

AC_ARG_WITH(debug,
	[  --without-debug         disable debugging code],
	[DEBUG="$withval"],[DEBUG="yes"])

if test "$DEBUG" = "yes"; then
	DEBUG_CFLAGS="-g3 -ggdb"
	AC_DEFINE(DEBUG,1,[Enable debug messages])
fi

if test "$CFLAGS" = "" -a "$CXXFLAGS" = ""; then
	CFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
	CXXFLAGS="-Wall -O2 -pipe $DEBUG_CFLAGS"
fi

if test "$prefix" = "NONE" -o "$prefix" = "/usr" ; then
	prefix="/usr"
	
	# workaround
	datadir="/usr/share"
	localstatedir="/var"
	targetlocalstatedir="/var"
else
	datadir="\${prefix}/share"
	localstatedir="\${prefix}/var"
	targetlocalstatedir="\${prefix}/var"
fi

targetprefix=$prefix
TARGET_PREFIX=$prefix

if test "$exec_prefix" = "NONE"; then
	exec_prefix=$prefix
fi

AC_CANONICAL_BUILD
AC_CANONICAL_HOST

check_path () {
	return $(perl -e "if(\"$1\"=~m#^/usr/(local/)?bin#){print \"0\"}else{print \"1\";}")
}
])

AC_DEFUN([TUXBOX_APPS_DIRECTORY_ONE],[
AC_ARG_WITH($1,[  $6$7 [[PREFIX$4$5]]],[
	_$2=$withval
	$2=`eval echo "$TARGET_PREFIX$withval"`
	TARGET_$2=${$2}
],[
	$2="\${$3}$5"
	_$2=`eval echo "${$3}$5"`
	TARGET_$2=$_$2
])

dnl automake <= 1.6 don't support this
dnl AC_SUBST($2)
AC_DEFINE_UNQUOTED($2,"$_$2",$7)
AC_SUBST(TARGET_$2)
])

AC_DEFUN([TUXBOX_APPS_DIRECTORY],[
AC_REQUIRE([TUXBOX_APPS])

TUXBOX_APPS_DIRECTORY_ONE(configdir,CONFIGDIR,localstatedir,/var,/tuxbox/config,
	[--with-configdir=PATH   ],[where to find the config files])

TUXBOX_APPS_DIRECTORY_ONE(datadir,DATADIR,datadir,/share,/tuxbox/neutrino2,
	[--with-datadir=PATH     ],[where to find data])

TUXBOX_APPS_DIRECTORY_ONE(plugindir,PLUGINDIR,localstatedir,/var,/tuxbox/plugins,
	[--with-plugindir=PATH   ],[where to find the plugins])
	
TUXBOX_APPS_DIRECTORY_ONE(localedir,LOCALEDIR,localstatedir,/var,/tuxbox/locale,
	[--with-localedir=PATH   ],[where to find locales])
])

dnl automake <= 1.6 needs this specifications
AC_SUBST(CONFIGDIR)
AC_SUBST(DATADIR)
AC_SUBST(PLUGINDIR)
AC_SUBST(LOCALEDIR)
dnl end workaround

AC_DEFUN([TUXBOX_APPS_ENDIAN],[
AC_CHECK_HEADERS(endian.h)
AC_C_BIGENDIAN
])

AC_DEFUN([TUXBOX_APPS_DRIVER],[
AC_ARG_WITH(driver,
	[  --with-driver=PATH      path for driver sources [[NONE]]],
	[DRIVER="$withval"],[DRIVER=""])

if test "$DRIVER"; then
	AC_SUBST(DRIVER)
	CPPFLAGS="$CPPFLAGS -I$DRIVER/include"
fi
])

AC_DEFUN([TUXBOX_APPS_DVB],[
AC_ARG_WITH(dvbincludes,
	[  --with-dvbincludes=PATH  path for dvb includes [[NONE]]],
	[DVBINCLUDES="$withval"],[DVBINCLUDES=""])

if test "$DVBINCLUDES"; then
	CPPFLAGS="$CPPFLAGS -I$DVBINCLUDES"
fi

AC_CHECK_HEADERS(ost/dmx.h,[
	DVB_API_VERSION=1
	AC_MSG_NOTICE([found dvb version 1])
])

if test -z "$DVB_API_VERSION"; then
AC_CHECK_HEADERS(linux/dvb/version.h,[
	AC_LANG_PREPROC_REQUIRE()
	AC_REQUIRE([AC_PROG_EGREP])
	AC_LANG_CONFTEST([AC_LANG_SOURCE([[
#include <linux/dvb/version.h>
version DVB_API_VERSION
	]])])
	DVB_API_VERSION=`(eval "$ac_cpp -traditional-cpp conftest.$ac_ext") 2>&AS_MESSAGE_LOG_FD | $EGREP "^version" | sed "s,version\ ,,"`
	rm -f conftest*

	AC_MSG_NOTICE([found dvb version $DVB_API_VERSION])
])
fi

if test "$DVB_API_VERSION"; then
	AC_DEFINE(HAVE_DVB,1,[Define to 1 if you have the dvb includes])
	AC_DEFINE_UNQUOTED(HAVE_DVB_API_VERSION,$DVB_API_VERSION,[Define to the version of the dvb api])
else
	AC_MSG_ERROR([can't find dvb headers])
fi
])

dnl backward compatiblity
AC_DEFUN([AC_GNU_SOURCE],
[AH_VERBATIM([_GNU_SOURCE],
[/* Enable GNU extensions on systems that have them.  */
#ifndef _GNU_SOURCE
# undef _GNU_SOURCE
#endif])dnl
AC_BEFORE([$0], [AC_COMPILE_IFELSE])dnl
AC_BEFORE([$0], [AC_RUN_IFELSE])dnl
AC_DEFINE([_GNU_SOURCE])
])

AC_DEFUN([AC_PROG_EGREP],
[AC_CACHE_CHECK([for egrep], [ac_cv_prog_egrep],
   [if echo a | (grep -E '(a|b)') >/dev/null 2>&1
    then ac_cv_prog_egrep='grep -E'
    else ac_cv_prog_egrep='egrep'
    fi])
 EGREP=$ac_cv_prog_egrep
 AC_SUBST([EGREP])
])

AC_DEFUN([TUXBOX_BOXTYPE],[

AC_ARG_WITH(boxtype,
	[  --with-boxtype          valid values: generic,dgs,gigablue,dreambox,xtrend,fulan,kathrein,ipbox,topfield,fortis_hdbox,octagon,atevio,adb_box,whitebox,vip,homecast,vuplus,azbox,technomate,coolstream,hypercube,venton,xp1000,odin,ixuss,iqonios,ebox5000,wetek,edision,hd,gi,xpeedc,formuler,miraclebox,spycat,xsarius,zgemma,wwio,axas,abcom, maxytec],
	[case "${withval}" in
		generic|dgs|gigablue|dreambox|xtrend|fulan|kathrein|ipbox|hl101|topfield|fortis_hdbox|octagon|atevio|adb_box|whitebox|vip|homecast|vuplus|azbox|technomate|coolstream|hypercube|venton|xp1000|odin|ixuss|iqonios|ebox5000|wetek|edision|hd|gi|xpeedc|formuler|miraclebox|spycat|xsarius|zgemma|wwio|axas|abcom)
			BOXTYPE="$withval"
			;;
		cu*)
			BOXTYPE="dgs"
			BOXMODEL="$withval"
			;;
		gb*)
			BOXTYPE="gigablue"
			BOXMODEL="$withval"
			;;

		tf*)
			BOXTYPE="topfield"
			BOXMODEL="$withval"
			;;
		dm*)
			BOXTYPE="dreambox"
			BOXMODEL="$withval"
			;;
		et*)
			BOXTYPE="xtrend"
			BOXMODEL="$withval"
			;;
		spa*)
			BOXTYPE="fulan"
			BOXMODEL="$withval"
			;;
		uf*)
			BOXTYPE="kathrein"
			BOXMODEL="$withval"
			;;
		ip*)
			BOXTYPE="ipbox"
			BOXMODEL="$withval"
			;;
		hl101*)
			BOXTYPE="duckbox"
			BOXMODEL="$withval"
			;;	
		at*)
			BOXTYPE="atevio"
			BOXMODEL="$withval"
			;;
		oct*|sf*|sx*)
			BOXTYPE="octagon"
			BOXMODEL="$withval"
			;;
		vu*)
			BOXTYPE="vuplus"
			BOXMODEL="$withval"
			;;
		az*)
			BOXTYPE="azbox"
			BOXMODEL="$withval"
			;;
		tm*)
			BOXTYPE="technomate"
			BOXMODEL="$withval"
			;;

		ven*)
			BOXTYPE="venton"
			BOXMODEL="$withval"
			;;

		ixu*)
			BOXTYPE="ixuss"
			BOXMODEL="$withval"
			;;

		iqo*)
			BOXTYPE="iqonios"
			BOXMODEL="$withval"
			;;
		
		odi*)
			BOXTYPE="odin"
			BOXMODEL="$withval"
			;;

		wet*)
			BOXTYPE="wetek"
			BOXMODEL="$withval"
			;;

		os*)
			BOXTYPE="edision"
			BOXMODEL="$withval"
			;;

		hd*)
			BOXTYPE="hd"
			BOXMODEL="$withval"
			;;

		gi*)
			BOXTYPE="gi"
			BOXMODEL="$withval"
			;;

		xpe*)
			BOXTYPE="xpeedc"
			BOXMODEL="$withval"
			;;

		for*)
			BOXTYPE="formuler"
			BOXMODEL="$withval"
			;;

		mir*)
			BOXTYPE="miraclebox"
			BOXMODEL="$withval"
			;;

		spy*)
			BOXTYPE="spycat"
			BOXMODEL="$withval"
			;;

		xsa*)
			BOXTYPE="xsarius"
			BOXMODEL="$withval"
			;;

		h*)
			BOXTYPE="zgemma"
			BOXMODEL="$withval"
			;;
	
		bre*)
			BOXTYPE="wwio"
			BOXMODEL="$withval"
			;;
			
		e*hd)
			BOXTYPE="axas"
			BOXMODEL="$withval"
			;;
			
		pul*)
			BOXTYPE="abcom"
			BOXMODEL="$withval"
			;;
			
		multi*)
			BOXTYPE="maxytec"
			BOXMODEL="$withval"
			;;

		*)
			AC_MSG_ERROR([unsupported value $withval for --with-boxtype])
			;;
	esac], [BOXTYPE="generic"])

AC_ARG_WITH(boxmodel,
	[  --with-boxmodel	valid for dgs: cuberevo,cuberevo_mini,cuberevo_mini2,cuberevo_mini_fta,cuberevo_250hd,cuberevo_2000hd,cuberevo_9500hd
				valid for gigablue: gbsolo,gb800se,gb800ue,gb800seplus,gb800ueplus,gbquad
				valid for dreambox: dm500, dm500plus, dm600pvr, dm56x0, dm7000, dm7020, dm7025, dm500hd, dm7020hd, dm8000, dm800, dm800se, dm520, dm900, dm920
				valid for xtrend: et4x00,et5x00,et6x00,et7x00, et8000,et8500,et9x00, et10000
				valid for fulan: spark, spark7162
				valid for kathrein: ufs910, ufs922, ufs912, ufs913, ufc960
				valid for ipbox: ipbox55, ipbox99, ipbox9900
				valid for hl: hl101
				valid for atevio: atevio700,atevio7000,atevio7500,atevio7600
				valid for octagon: octagon1008 sf4008 sf8008 sf8008m sfx6008 sx88v2 sx998
				valid for topfield: tf7700
				valid for vuplus: vusolo,vuduo,vuuno,vuultimo,vuduo2,vusolo2,vusolo4k,vusolose,vuzero,vuduo4k
				valid for azbox: azboxhd,azboxme,azboxminime
				valid for technomate: tmtwin,tm2t,tmsingle,tmnano
				valid for venton: ventonhde,ventonhdx,inihde,inihdp
				valid for ixuss: ixusszero,ixussone
				valid for iqonios: iqonios100hd,iqonios300hd,mediabox,optimussos1,optimussos2
				valid for odin: odinm6,odinm7,odinm9
				valid for wetek: wetekplay
				valid for edision: osmini, osminiplus, osnino, osninoplus, osninopro, osmio4k, osmio4plus
				valid for hd: hd11, hd51, hd500c, hd1100, hd1200, hd1265, hdhd1500, hd2400, ax51
				valid for gi: et7000mini
				valid for xpeedc: xpeedc
				valid for formuler: formuler1, formuler3, formuler4
				valid for miraclebox: mbmicro, mbtwinplus
				valid for spycat: spycat, spycatmini
				valid for xsarius: fusionhd, fusionhdse, purehd
				valid for zgemma: h3, h4, h5, h7, i55, lc, sh1
				valid for wwio: bre2ze4k
				valid for axas: e3hd e4hdultra
				valid for abcom: pulse4k pulse4kmini
				valid for maxytec: multibox multiboxse],
	[case "${withval}" in
		cuberevo|cuberevo_mini|cuberevo_mini2|cuberevo_mini_fta|cuberevo_250hd|cuberevo_2000hd|cuberevo_9500hd)
			if test "$BOXTYPE" = "dgs"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		gb800solo|gb800se|gb800ue|gb800seplus|gb800ueplus|gbquad)
			if test "$BOXTYPE" = "gigablue"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		tf7700)
			if test "$BOXTYPE" = "topfield"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		dm500|dm500plus|dm600pvr|dm56x0|dm7000|dm7020|dm7025|dm500hd|dm7020hd|dm8000|dm800|dm800se|dm520|dm900|dm920)
			if test "$BOXTYPE" = "dreambox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		et4x00|et5x00|et6x00|et7x00|et8000|et8500|et9x00|et10000)
			if test "$BOXTYPE" = "xtrend"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		spark|spark7162)
			if test "$BOXTYPE" = "fulan"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		ufs910|ufs912|ufs913|ufs922|ufc960)
			if test "$BOXTYPE" = "kathrein"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		ipbox55|ipbox99|ipbox9900)
			if test "$BOXTYPE" = "ipbox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		hl101)
			if test "$BOXTYPE" = "duckbox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		atevio700|atevio7000|atevio7500|atevio7600)
			if test "$BOXTYPE" = "atevio"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		octagon1008|sf4008|sf8008|sf8008m|sfx6008|sx88v2|sx988)
			if test "$BOXTYPE" = "octagon"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		vusolo2|vuduo2|vusolo|vuduo|vuuno|vuultimo|vusolose|vusolo4k|vuzero|vuduo4k)
			if test "$BOXTYPE" = "vuplus"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		azboxhd|azboxme|azboxminime)
			if test "$BOXTYPE" = "azbox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		tmtwin|tm2t|tmsingle|tmnano)
			if test "$BOXTYPE" = "technomate"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		coolstream)
			if test "$BOXTYPE" = "coolstream"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		hypercube|su980)
			if test "$BOXTYPE" = "hypercube"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		ventonhde|ventonhdx|inihde|inihdp)
			if test "$BOXTYPE" = "venton"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		ixusszero|ixussone)
			if test "$BOXTYPE" = "ixuss"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		iqonios100hd|iqonios300hd|mediabox|optimussos1|optimussos2)
			if test "$BOXTYPE" = "iqonios"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		odinm6|odinm7|odinm9)
			if test "$BOXTYPE" = "odin"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		xp1000)
			if test "$BOXTYPE" = "xp1000"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		ebox5000)
			if test "$BOXTYPE" = "ebox5000"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		wetekplay)
			if test "$BOXTYPE" = "wetek"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		osmini|osminiplus|osnino|osninoplus|osninopro|osmio4k|osmio4kplus)
			if test "$BOXTYPE" = "edision"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		hd11|hd51|ax51|hd500c|hd1100|hd1200|hd1265|hd1500|hd2400)
			if test "$BOXTYPE" = "hd"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		et7000mini)
			if test "$BOXTYPE" = "gi"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		xpeedc)
			if test "$BOXTYPE" = "xpeedc"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		formuler1|formuler3|formuler4)
			if test "$BOXTYPE" = "formuler"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		mbmicro|mbtwinplus)
			if test "$BOXTYPE" = "miraclebox"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		spycat|spycatmini)
			if test "$BOXTYPE" = "spycat"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		fusionhd|fusionhdse|purehd)
			if test "$BOXTYPE" = "xsarius"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		h3|h4|h5|h7|i55|lc|sh1)
			if test "$BOXTYPE" = "zgemma"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		bre2ze4k|bre2zet2c)
			if test "$BOXTYPE" = "wwio"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		e3hd|e4hdultra)
			if test "$BOXTYPE" = "axas"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		pulse4k|pulse4kmini)
			if test "$BOXTYPE" = "abcom"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		multibox|multiboxse)
			if test "$BOXTYPE" = "maxytec"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		qemu*)
			if test "$BOXTYPE" = "generic"; then
				BOXMODEL="$withval"
			else
				AC_MSG_ERROR([unknown model $withval for boxtype $BOXTYPE])
			fi
			;;
		*)
			AC_MSG_ERROR([unsupported value $withval for --with-boxmodel])
			;;
	esac]
	#,
	#[if test "$BOXTYPE" = "dgs" -o "$BOXTYPE" = "gigablue" -o "$BOXTYPE" = "dreambox" -o "$BOXTYPE" = "xtrend" -o "$BOXTYPE" = "fulan" -o "$BOXTYPE" = "kathrein" -o "$BOXTYPE" = "ipbox" -o "$BOXTYPE" = "atevio" -o "$BOXTYPE" = "octagon" -o "$BOXTYPE" = "vuplus" -o "$BOXTYPE" = "technomate" -o "$BOXTYPE" = "venton" -o "$BOXTYPE" = "ixuss" -o "$BOXTYPE" = "iqonios" -o "$BOXTYPE" = "odin" -o "$BOXTYPE" = "edision" -o "$BOXTYPE" = "hd" -o "$BOXTYPE" = "gi" -o "$BOXTYPE" = "formuler" -o "$BOXTYPE" = "miraclebox" -o "$BOXTYPE" = "spycat" -o "$BOXTYPE" = "xsarius" -o "$BOXTYPE" = "zgemma" -o "$BOXTYPE" = "wwio" -o "$BOXTYPE" = "axas" -o "$BOXTYPE" = "abcom" -o "$BOXTYPE" = "maxytec" && test -z "$BOXMODEL"; then
		#AC_MSG_ERROR([this boxtype $BOXTYPE needs --with-boxmodel])
	#fi]
	)

AC_SUBST(BOXTYPE)
AC_SUBST(BOXMODEL)

AM_CONDITIONAL(BOXTYPE_GENERIC, test "$BOXTYPE" = "generic")
AM_CONDITIONAL(BOXTYPE_DGS, test "$BOXTYPE" = "dgs")
AM_CONDITIONAL(BOXTYPE_GIGABLUE, test "$BOXTYPE" = "gigablue")
AM_CONDITIONAL(BOXTYPE_DREAMBOX, test "$BOXTYPE" = "dreambox")
AM_CONDITIONAL(BOXTYPE_XTREND, test "$BOXTYPE" = "xtrend")
AM_CONDITIONAL(BOXTYPE_FULAN, test "$BOXTYPE" = "fulan")
AM_CONDITIONAL(BOXTYPE_KATHREIN, test "$BOXTYPE" = "kathrein")
AM_CONDITIONAL(BOXTYPE_IPBOX, test "$BOXTYPE" = "ipbox")
AM_CONDITIONAL(BOXTYPE_HL101, test "$BOXTYPE" = "hl101")
AM_CONDITIONAL(BOXTYPE_TOPFIELD, test "$BOXTYPE" = "toptfield")
AM_CONDITIONAL(BOXTYPE_FORTIS_HDBOX, test "$BOXTYPE" = "fortis_hdbox")
AM_CONDITIONAL(BOXTYPE_OCTAGON, test "$BOXTYPE" = "octagon")
AM_CONDITIONAL(BOXTYPE_ATEVIO, test "$BOXTYPE" = "atevio")
AM_CONDITIONAL(BOXTYPE_ADB_BOX, test "$BOXTYPE" = "adb_box")
AM_CONDITIONAL(BOXTYPE_WHITEBOX, test "$BOXTYPE" = "whitebox")
AM_CONDITIONAL(BOXTYPE_VIP, test "$BOXTYPE" = "vip")
AM_CONDITIONAL(BOXTYPE_HOMECAST, test "$BOXTYPE" = "homecast")
AM_CONDITIONAL(BOXTYPE_VUPLUS, test "$BOXTYPE" = "vuplus")
AM_CONDITIONAL(BOXTYPE_AZBOX, test "$BOXTYPE" = "azbox")
AM_CONDITIONAL(BOXTYPE_TECHNOMATE, test "$BOXTYPE" = "technomate")
AM_CONDITIONAL(BOXTYPE_COOLSTREAM, test "$BOXTYPE" = "coolstream")
AM_CONDITIONAL(BOXTYPE_HYPERCUBE, test "$BOXTYPE" = "hypercube")
AM_CONDITIONAL(BOXTYPE_VENTON, test "$BOXTYPE" = "venton")
AM_CONDITIONAL(BOXTYPE_IXUSS, test "$BOXTYPE" = "ixuss")
AM_CONDITIONAL(BOXTYPE_IQONIOS, test "$BOXTYPE" = "iqonios")
AM_CONDITIONAL(BOXTYPE_ODIN, test "$BOXTYPE" = "odin")
AM_CONDITIONAL(BOXTYPE_XP1000, test "$BOXTYPE" = "xp1000")
AM_CONDITIONAL(BOXTYPE_EBOX5000, test "$BOXTYPE" = "ebox5000")
AM_CONDITIONAL(BOXTYPE_WETEK, test "$BOXTYPE" = "wetek")
AM_CONDITIONAL(BOXTYPE_EDISION, test "$BOXTYPE" = "edision")
AM_CONDITIONAL(BOXTYPE_HD, test "$BOXTYPE" = "hd")
AM_CONDITIONAL(BOXTYPE_GI, test "$BOXTYPE" = "gi")
AM_CONDITIONAL(BOXTYPE_XPEEDC, test "$BOXTYPE" = "xpeedc")
AM_CONDITIONAL(BOXTYPE_FORMULER, test "$BOXTYPE" = "formuler")
AM_CONDITIONAL(BOXTYPE_MIRACLEBOX, test "$BOXTYPE" = "miraclebox")
AM_CONDITIONAL(BOXTYPE_SPYCAT, test "$BOXTYPE" = "spycat")
AM_CONDITIONAL(BOXTYPE_XSARIUS, test "$BOXTYPE" = "xsarius")
AM_CONDITIONAL(BOXTYPE_ZGEMMA, test "$BOXTYPE" = "zgemma")
AM_CONDITIONAL(BOXTYPE_WWIO, test "$BOXTYPE" = "wwio")
AM_CONDITIONAL(BOXTYPE_AXAS, test "$BOXTYPE" = "axas")
AM_CONDITIONAL(BOXTYPE_ABCOM, test "$BOXTYPE" = "abcom")
AM_CONDITIONAL(BOXTYPE_MAXYTEC, test "$BOXTYPE" = "maxytec")

AM_CONDITIONAL(BOXMODEL_CUBEREVO, test "$BOXMODEL" = "cuberevo")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_MINI, test "$BOXMODEL" = "cuberevo_mini")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_MINI2, test "$BOXMODEL" = "cuberevo_mini2")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_MINI_FTA, test "$BOXMODEL" = "cuberevo_mini_fta")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_250HD, test "$BOXMODEL" = "cuberevo_250hd")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_2000HD, test "$BOXMODEL" = "cuberevo_2000hd")
AM_CONDITIONAL(BOXMODEL_CUBEREVO_9500HD, test "$BOXMODEL" = "cuberevo_9500HD")

AM_CONDITIONAL(BOXMODEL_TOPFIELD_TF7700, test "$BOXMODEL" = "tf7700")

AM_CONDITIONAL(BOXMODEL_GB800SOLO,test "$BOXMODEL" = "gb800solo")
AM_CONDITIONAL(BOXMODEL_GB800SE,test "$BOXMODEL" = "gb800se")
AM_CONDITIONAL(BOXMODEL_GB800UE,test "$BOXMODEL" = "gb800ue")
AM_CONDITIONAL(BOXMODEL_GB800SEPLUS,test "$BOXMODEL" = "gb800seplus")
AM_CONDITIONAL(BOXMODEL_GB800UEPLUS,test "$BOXMODEL" = "gb800ueplus")
AM_CONDITIONAL(BOXMODEL_GBQUAD,test "$BOXMODEL" = "gbquad")

AM_CONDITIONAL(BOXMODEL_DM500,test "$BOXMODEL" = "dm500")
AM_CONDITIONAL(BOXMODEL_DM500PLUS,test "$BOXMODEL" = "dm500plus")
AM_CONDITIONAL(BOXMODEL_DM600PVR,test "$BOXMODEL" = "dm600pvr")
AM_CONDITIONAL(BOXMODEL_DM56x0,test "$BOXMODEL" = "dm56x0")
AM_CONDITIONAL(BOXMODEL_DM7000,test "$BOXMODEL" = "dm7000" -o "$BOXMODEL" = "dm7020" -o "$BOXMODEL" = "dm7025")
AM_CONDITIONAL(BOXMODEL_DM500HD,test "$BOXMODEL" = "dm500hd")
AM_CONDITIONAL(BOXMODEL_DM800HD,test "$BOXMODEL" = "dm800")
AM_CONDITIONAL(BOXMODEL_DM800SE,test "$BOXMODEL" = "dm800se")
AM_CONDITIONAL(BOXMODEL_DM7000HD,test "$BOXMODEL" = "dm7020hd")
AM_CONDITIONAL(BOXMODEL_DM8000HD,test "$BOXMODEL" = "dm8000")
AM_CONDITIONAL(BOXMODEL_DM520,test "$BOXMODEL" = "dm520")
AM_CONDITIONAL(BOXMODEL_DM900,test "$BOXMODEL" = "dm900")
AM_CONDITIONAL(BOXMODEL_DM920,test "$BOXMODEL" = "dm920")

AM_CONDITIONAL(BOXMODEL_ET4X00,test "$BOXMODEL" = "et4x00")
AM_CONDITIONAL(BOXMODEL_ET5X00,test "$BOXMODEL" = "et5x00")
AM_CONDITIONAL(BOXMODEL_ET6X00,test "$BOXMODEL" = "et6x00")
AM_CONDITIONAL(BOXMODEL_ET7X00,test "$BOXMODEL" = "et7x00")
AM_CONDITIONAL(BOXMODEL_ET8000,test "$BOXMODEL" = "et8000")
AM_CONDITIONAL(BOXMODEL_ET8500,test "$BOXMODEL" = "et8500")
AM_CONDITIONAL(BOXMODEL_ET9X00,test "$BOXMODEL" = "et9x00")
AM_CONDITIONAL(BOXMODEL_ET10000,test "$BOXMODEL" = "et10000")

AM_CONDITIONAL(BOXMODEL_SPARK,test "$BOXMODEL" = "spark")
AM_CONDITIONAL(BOXMODEL_SPARK7162,test "$BOXMODEL" = "spark7162")

AM_CONDITIONAL(BOXMODEL_UFS910, test "$BOXMODEL" = "ufs910")
AM_CONDITIONAL(BOXMODEL_UFS912, test "$BOXMODEL" = "ufs912")
AM_CONDITIONAL(BOXMODEL_UFS913, test "$BOXMODEL" = "ufs913")
AM_CONDITIONAL(BOXMODEL_UFS922, test "$BOXMODEL" = "ufs922")
AM_CONDITIONAL(BOXMODEL_UFC960, test "$BOXMODEL" = "ufc960")

AM_CONDITIONAL(BOXMODEL_IPBOX55, test "$BOXMODEL" = "ipbox55")
AM_CONDITIONAL(BOXMODEL_IPBOX99, test "$BOXMODEL" = "ipbox99")
AM_CONDITIONAL(BOXMODEL_IPBOX9900, test "$BOXMODEL" = "ipbox9900")

AM_CONDITIONAL(BOXMODEL_HL101, test "$BOXMODEL" = "hl101")

AM_CONDITIONAL(BOXMODEL_ATEVIO700, test "$BOXMODEL" = "atevio700")
AM_CONDITIONAL(BOXMODEL_ATEVIO7000, test "$BOXMODEL" = "atevio7000")
AM_CONDITIONAL(BOXMODEL_ATEVIO7500, test "$BOXMODEL" = "atevio7500")
AM_CONDITIONAL(BOXMODEL_ATEVIO7600, test "$BOXMODEL" = "atevio7600")

AM_CONDITIONAL(BOXMODEL_OCTAGON1008, test "$BOXMODEL" = "octagon1008")
AM_CONDITIONAL(BOXMODEL_OCTAGONSF4008, test "$BOXMODEL" = "octagonsf4008")
AM_CONDITIONAL(BOXMODEL_OCTAGONSF8008, test "$BOXMODEL" = "octagonsf8008")
AM_CONDITIONAL(BOXMODEL_OCTAGONSF008M, test "$BOXMODEL" = "octagonsf8008m")
AM_CONDITIONAL(BOXMODEL_OCTAGONSFX6008, test "$BOXMODEL" = "octagonsfx6008")
AM_CONDITIONAL(BOXMODEL_OCTAGONSX88V2, test "$BOXMODEL" = "octagonsx88v2")
AM_CONDITIONAL(BOXMODEL_OCTAGONSX988, test "$BOXMODEL" = "octagonsx988")

AM_CONDITIONAL(BOXMODEL_VUSOLO2, test "$BOXMODEL" = "vusolo2")
AM_CONDITIONAL(BOXMODEL_VUDUO2, test "$BOXMODEL" = "vuduo2")
AM_CONDITIONAL(BOXMODEL_VUSOLO, test "$BOXMODEL" = "vusolo")
AM_CONDITIONAL(BOXMODEL_VUDUO, test "$BOXMODEL" = "vuduo")
AM_CONDITIONAL(BOXMODEL_VUUNO, test "$BOXMODEL" = "vuuno")
AM_CONDITIONAL(BOXMODEL_VUULTIMO, test "$BOXMODEL" = "vuultimo")
AM_CONDITIONAL(BOXMODEL_VUSOLOSE, test "$BOXMODEL" = "vusolose")
AM_CONDITIONAL(BOXMODEL_VUSOLO4K, test "$BOXMODEL" = "vusolo4k")
AM_CONDITIONAL(BOXMODEL_VUZERO, test "$BOXMODEL" = "vuzero")
AM_CONDITIONAL(BOXMODEL_VUDUO4K, test "$BOXMODEL" = "vuduo4k")

AM_CONDITIONAL(BOXMODEL_AZBOXHD, test "$BOXMODEL" = "azboxhd")
AM_CONDITIONAL(BOXMODEL_AZBOXME, test "$BOXMODEL" = "azboxme")
AM_CONDITIONAL(BOXMODEL_AZBOXMINIME, test "$BOXMODEL" = "azboxminime")

AM_CONDITIONAL(BOXMODEL_TMTWIN, test "$BOXMODEL" = "tmtwin")
AM_CONDITIONAL(BOXMODEL_TM2T, test "$BOXMODEL" = "tm2t")
AM_CONDITIONAL(BOXMODEL_TMSINGLE, test "$BOXMODEL" = "tmsingle")
AM_CONDITIONAL(BOXMODEL_TMNANO, test "$BOXMODEL" = "tmnano")

AM_CONDITIONAL(BOXMODEL_VENTONHDE, test "$BOXMODEL" = "ventonhde")
AM_CONDITIONAL(BOXMODEL_VENTONHDX, test "$BOXMODEL" = "ventonhdx")
AM_CONDITIONAL(BOXMODEL_INIHDE, test "$BOXMODEL" = "inihde")
AM_CONDITIONAL(BOXMODEL_INIHDP, test "$BOXMODEL" = "inihdp")

AM_CONDITIONAL(BOXMODEL_IXUSSZERO, test "$BOXMODEL" = "ixusszero")
AM_CONDITIONAL(BOXMODEL_IXUSSONE, test "$BOXMODEL" = "ixussone")

AM_CONDITIONAL(BOXMODEL_IQONIOS100HD, test "$BOXMODEL" = "iqonios100hd")
AM_CONDITIONAL(BOXMODEL_IQONIOS300HD, test "$BOXMODEL" = "iqonios300hd")
AM_CONDITIONAL(BOXMODEL_MEDIABOX, test "$BOXMODEL" = "mediabox")
AM_CONDITIONAL(BOXMODEL_OPTIMUSSOS1, test "$BOXMODEL" = "optimussos1")
AM_CONDITIONAL(BOXMODEL_OPTIMUSSOS2, test "$BOXMODEL" = "optimussos2")

AM_CONDITIONAL(BOXMODEL_ODINM6, test "$BOXMODEL" = "odinm6")
AM_CONDITIONAL(BOXMODEL_ODINM7, test "$BOXMODEL" = "odinm7")
AM_CONDITIONAL(BOXMODEL_ODINM9, test "$BOXMODEL" = "odinm9")

AM_CONDITIONAL(BOXMODEL_WETEKPLAY, test "$BOXMODEL" = "wetekplay")

AM_CONDITIONAL(BOXMODEL_OSMINI, test "$BOXMODEL" = "osmini")
AM_CONDITIONAL(BOXMODEL_OSMINIPLUS, test "$BOXMODEL" = "osminiplus")
AM_CONDITIONAL(BOXMODEL_OSNINO, test "$BOXMODEL" = "osnino")
AM_CONDITIONAL(BOXMODEL_OSNINOPLUS, test "$BOXMODEL" = "osninoplus")
AM_CONDITIONAL(BOXMODEL_OSNINOPRO, test "$BOXMODEL" = "osninopro")
AM_CONDITIONAL(BOXMODEL_OSMIO4K, test "$BOXMODEL" = "osmio4k")
AM_CONDITIONAL(BOXMODEL_OSMIO4KPLUS, test "$BOXMODEL" = "osmio4kplus")

AM_CONDITIONAL(BOXMODEL_HD11, test "$BOXMODEL" = "hd11")
AM_CONDITIONAL(BOXMODEL_HD51, test "$BOXMODEL" = "hd51")
AM_CONDITIONAL(BOXMODEL_AX51, test "$BOXMODEL" = "ax51")
AM_CONDITIONAL(BOXMODEL_HD500C, test "$BOXMODEL" = "hd500c")
AM_CONDITIONAL(BOXMODEL_HD1100, test "$BOXMODEL" = "hd1100")
AM_CONDITIONAL(BOXMODEL_HD1200, test "$BOXMODEL" = "hd1200")
AM_CONDITIONAL(BOXMODEL_HD1265, test "$BOXMODEL" = "hd1265")
AM_CONDITIONAL(BOXMODEL_HD1500, test "$BOXMODEL" = "hd1500")
AM_CONDITIONAL(BOXMODEL_HD2400, test "$BOXMODEL" = "hd2400")

AM_CONDITIONAL(BOXMODEL_ET7000MINI, test "$BOXMODEL" = "et7000mini")

AM_CONDITIONAL(BOXMODEL_XPEEDC, test "$BOXMODEL" = "xpeedc")

AM_CONDITIONAL(BOXMODEL_FORMULER1, test "$BOXMODEL" = "formuler1")
AM_CONDITIONAL(BOXMODEL_FORMULER3, test "$BOXMODEL" = "formuler3")
AM_CONDITIONAL(BOXMODEL_FORMULER4, test "$BOXMODEL" = "formuler4")

AM_CONDITIONAL(BOXMODEL_MBMICRO, test "$BOXMODEL" = "mbmicro")
AM_CONDITIONAL(BOXMODEL_MBTWINPLUS, test "$BOXMODEL" = "mbtwinplus")

AM_CONDITIONAL(BOXMODEL_SPYCAT, test "$BOXMODEL" = "spycat")
AM_CONDITIONAL(BOXMODEL_SPYCATMINI, test "$BOXMODEL" = "spycatmini")

AM_CONDITIONAL(BOXMODEL_FUSIONHD, test "$BOXMODEL" = "fusionhd")
AM_CONDITIONAL(BOXMODEL_FUSIONHDSE, test "$BOXMODEL" = "fusionhdse")
AM_CONDITIONAL(BOXMODEL_PUREHD, test "$BOXMODEL" = "purehd")

AM_CONDITIONAL(BOXMODEL_H3, test "$BOXMODEL" = "h3")
AM_CONDITIONAL(BOXMODEL_H4, test "$BOXMODEL" = "h4")
AM_CONDITIONAL(BOXMODEL_H5, test "$BOXMODEL" = "h5")
AM_CONDITIONAL(BOXMODEL_H7, test "$BOXMODEL" = "h7")
AM_CONDITIONAL(BOXMODEL_I55, test "$BOXMODEL" = "i55")
AM_CONDITIONAL(BOXMODEL_LC, test "$BOXMODEL" = "lc")
AM_CONDITIONAL(BOXMODEL_SH1, test "$BOXMODEL" = "sh1")

AM_CONDITIONAL(BOXMODEL_BRE2ZE4K, test "$BOXMODEL" = "bre2ze4k")
AM_CONDITIONAL(BOXMODEL_BRE2ZET2C, test "$BOXMODEL" = "bre2zet2c")

AM_CONDITIONAL(BOXMODEL_E3HD, test "$BOXMODEL" = "e3hd")
AM_CONDITIONAL(BOXMODEL_E4HDULTRA, test "$BOXMODEL" = "e4hdultra")

AM_CONDITIONAL(BOXMODEL_PULSE4K, test "$BOXMODEL" = "pulse4k")
AM_CONDITIONAL(BOXMODEL_PULSE4KMINI, test "$BOXMODEL" = "pulse4kmini")

AM_CONDITIONAL(BOXMODEL_MULTIBOX, test "$BOXMODEL" = "multibox")
AM_CONDITIONAL(BOXMODEL_MULTIBOXSE, test "$BOXMODEL" = "multiboxse")

if test "$BOXTYPE" = "generic"; then
	AC_DEFINE(PLATFORM_GENERIC, 1, [building for generic])
elif test "$BOXTYPE" = "dgs"; then
	AC_DEFINE(PLATFORM_DGS, 1, [building for dgs])
elif test "$BOXTYPE" = "gigablue"; then
	AC_DEFINE(PLATFORM_GIGABLUE, 1, [building for gigablue])
elif test "$BOXTYPE" = "dreambox"; then
	AC_DEFINE(PLATFORM_DREAMBOX, 1, [building for dreambox])
elif test "$BOXTYPE" = "xtrend"; then
	AC_DEFINE(PLATFORM_XTREND, 1, [building for xtrend])
elif test "$BOXTYPE" = "fulan"; then
	AC_DEFINE(PLATFORM_FULAN, 1, [building for fulan])
elif test "$BOXTYPE" = "kathrein"; then
	AC_DEFINE(PLATFORM_KATHREIN, 1, [building for kathrein])
elif test "$BOXTYPE" = "ipbox"; then
	AC_DEFINE(PLATFORM_IPBOX, 1, [building for ipbox])
elif test "$BOXTYPE" = "duckbox"; then
	AC_DEFINE(HAVE_DUCKBOX_HARDWARE, 1, [building for a duckbox])	
elif test "$BOXTYPE" = "topfield"; then
	AC_DEFINE(PLATFORM_TF7700, 1, [building for topfield])
elif test "$BOXTYPE" = "fortis_hdbox"; then
	AC_DEFINE(PLATFORM_FORTIS_HDBOX, 1, [building for fortis_hdbox])
elif test "$BOXTYPE" = "octagon"; then
	AC_DEFINE(PLATFORM_OCTAGON, 1, [building for octagon])
elif test "$BOXTYPE" = "atevio"; then
	AC_DEFINE(PLATFORM_ATEVIO, 1, [building for atevio])
elif test "$BOXTYPE" = "adb_box"; then
	AC_DEFINE(PLATFORM_ADB_BOX, 1, [building for adb_box])
elif test "$BOXTYPE" = "whitebox"; then
	AC_DEFINE(PLATFORM_WHITEBOX, 1, [building for whitebox])
elif test "$BOXTYPE" = "vip"; then
	AC_DEFINE(PLATFORM_VIP, 1, [building for vip])
elif test "$BOXTYPE" = "homecast"; then
	AC_DEFINE(PLATFORM_HOMECAST, 1, [building for homecast])
elif test "$BOXTYPE" = "vuplus"; then
	AC_DEFINE(PLATFORM_VUPLUS, 1, [building for vuplus])
elif test "$BOXTYPE" = "azbox"; then
	AC_DEFINE(PLATFORM_AZBOX, 1, [building for azbox])
elif test "$BOXTYPE" = "technomate"; then
	AC_DEFINE(PLATFORM_TECHNOMATE, 1, [building for technomate])
elif test "$BOXTYPE" = "coolstream"; then
	AC_DEFINE(PLATFORM_COOLSTREAM, 1, [building for coolstream])
elif test "$BOXTYPE" = "hypercube"; then
	AC_DEFINE(PLATFORM_HYPERCUBE, 1, [building for hypercube])
elif test "$BOXTYPE" = "venton"; then
	AC_DEFINE(PLATFORM_VENTON, 1, [building for venton])
elif test "$BOXTYPE" = "ixuss"; then
	AC_DEFINE(PLATFORM_IXUSS, 1, [building for ixuss])
elif test "$BOXTYPE" = "iqonios"; then
	AC_DEFINE(PLATFORM_IQONIOS, 1, [building for iqonios])
elif test "$BOXTYPE" = "odin"; then
	AC_DEFINE(PLATFORM_ODIN, 1, [building for odin])
elif test "$BOXTYPE" = "xp1000"; then
	AC_DEFINE(PLATFORM_XP1000, 1, [building for xp1000])
elif test "$BOXTYPE" = "e3hd"; then
	AC_DEFINE(PLATFORM_E3HD, 1, [building for e3hd])
elif test "$BOXTYPE" = "ebox5000"; then
	AC_DEFINE(PLATFORM_EBOX5000, 1, [building for ebox5000])
elif test "$BOXTYPE" = "wetek"; then
	AC_DEFINE(PLATFORM_WETEK, 1, [building for wetek])
elif test "$BOXTYPE" = "edision"; then
	AC_DEFINE(PLATFORM_EDISION, 1, [building for edision])
elif test "$BOXTYPE" = "hd"; then
	AC_DEFINE(PLATFORM_HD, 1, [building for hd])
elif test "$BOXTYPE" = "gi"; then
	AC_DEFINE(PLATFORM_GI, 1, [building for gi])
elif test "$BOXTYPE" = "xpeedc"; then
	AC_DEFINE(PLATFORM_XPEEDC, 1, [building for xpeedc])
elif test "$BOXTYPE" = "formuler"; then
	AC_DEFINE(PLATFORM_FORMULER, 1, [building for formuler])
elif test "$BOXTYPE" = "miraclebox"; then
	AC_DEFINE(PLATFORM_MIRACLEBOX, 1, [building for miraclebox])
elif test "$BOXTYPE" = "spycat"; then
	AC_DEFINE(PLATFORM_SPYCAT, 1, [building for spycat])
elif test "$BOXTYPE" = "xsarius"; then
	AC_DEFINE(PLATFORM_XSARIUS, 1, [building for xsarius])
elif test "$BOXTYPE" = "zgemma"; then
	AC_DEFINE(PLATFORM_ZGEMMA, 1, [building for zgemma])
elif test "$BOXTYPE" = "wwio"; then
	AC_DEFINE(PLATFORM_WWIO, 1, [building for wwio])
elif test "$BOXTYPE" = "axas"; then
	AC_DEFINE(PLATFORM_AXAS, 1, [building for axas])
elif test "$BOXTYPE" = "abcom"; then
	AC_DEFINE(PLATFORM_ABCOM, 1, [building for abcom])
elif test "$BOXTYPE" = "abcom"; then
	AC_DEFINE(PLATFORM_MAXYTEC, 1, [building for maxytec])
fi

if test "$BOXMODEL" = "cuberevo"; then
	AC_DEFINE(PLATFORM_CUBEREVO, 1, [building for cuberevo])
elif test "$BOXMODEL" = "cuberevo_mini"; then
	AC_DEFINE(PLATFORM_CUBEREVO_MINI, 1, [building for cuberevo_mini])
elif test "$BOXMODEL" = "cuberevo_mini2"; then
	AC_DEFINE(PLATFORM_CUBEREVO_MINI2, 1, [building for cuberevo_mini2])
elif test "$BOXMODEL" = "cuberevo_mini_fta"; then
	AC_DEFINE(PLATFORM_CUBEREVO_MINI_FTA, 1, [building for cuberevo_mini_fta])
elif test "$BOXMODEL" = "cuberevo_250hd"; then
	AC_DEFINE(PLATFORM_CUBEREVO_250HD, 1, [building for cuberevo_250hd])
elif test "$BOXMODEL" = "cuberevo_2000hd"; then
	AC_DEFINE(PLATFORM_CUBEREVO_2000HD, 1, [building for cuberevo_2000hd])
elif test "$BOXMODEL" = "cuberevo_9500hd"; then
	AC_DEFINE(PLATFORM_CUBEREVO_9500HD, 1, [building for cuberevo_9500hd])

elif test "$BOXMODEL" = "gb800solo"; then
	AC_DEFINE(BOXMODEL_GB800SOLO, 1, [building for gigablue 800solo])
elif test "$BOXMODEL" = "gb800se"; then
	AC_DEFINE(BOXMODEL_GB800SE, 1, [building for gigablue 800se])
elif test "$BOXMODEL" = "gb800ue"; then
	AC_DEFINE(BOXMODEL_GB800UE, 1, [building for gigablue 800ue])
elif test "$BOXMODEL" = "gb800seplus"; then
	AC_DEFINE(BOXMODEL_GB800SEPLUS, 1, [building for gigablue 800seplus])
elif test "$BOXMODEL" = "gb800ueplus"; then
	AC_DEFINE(BOXMODEL_GB800UEPLUS, 1, [building for gigablue 800ueplus])
elif test "$BOXMODEL" = "gbquad"; then
	AC_DEFINE(BOXMODEL_GBQUAD, 1, [building for gigablue quad])

elif test "$BOXMODEL" = "dm500"; then
	AC_DEFINE(BOXMODEL_DM500, 1, [building for dreambox 500])
elif test "$BOXMODEL" = "dm500plus"; then
	AC_DEFINE(BOXMODEL_DM500PLUS, 1, [building for dreambox 500plus])
elif test "$BOXMODEL" = "dm600pvr"; then
	AC_DEFINE(BOXMODEL_DM600PVR, 1, [building for dreambox 600pvr])
elif test "$BOXMODEL" = "dm56x0"; then
	AC_DEFINE(BOXMODEL_DM56x0, 1, [building for dreambox 56x0])
elif test "$BOXMODEL" = "dm7000" -o "$BOXMODEL" = "dm7020" -o "$BOXMODEL" = "dm7025"; then
	AC_DEFINE(BOXMODEL_DM7000, 1, [building for dreambox 70xx])
elif test "$BOXMODEL" = "dm500hd"; then
	AC_DEFINE(BOXMODEL_DM500HD, 1, [building for dreambox 500hd])
elif test "$BOXMODEL" = "dm7020hd"; then
	AC_DEFINE(BOXMODEL_DM7000HD, 1, [building for dreambox 7020hd])
elif test "$BOXMODEL" = "dm8000"; then
	AC_DEFINE(BOXMODEL_DM8000HD, 1, [building for dreambox 8000])
elif test "$BOXMODEL" = "dm800"; then
	AC_DEFINE(BOXMODEL_DM800HD, 1, [building for dreambox 800])
elif test "$BOXMODEL" = "dm800se"; then
	AC_DEFINE(BOXMODEL_DM800SE, 1, [building for dreambox 800se])
elif test "$BOXMODEL" = "dm520"; then
	AC_DEFINE(BOXMODEL_DM520, 1, [building for dreambox 520])
elif test "$BOXMODEL" = "dm900"; then
	AC_DEFINE(BOXMODEL_DM900, 1, [building for dreambox 900])
elif test "$BOXMODEL" = "dm920"; then
	AC_DEFINE(BOXMODEL_DM920, 1, [building for dreambox 920])

elif test "$BOXMODEL" = "et4x00"; then
	AC_DEFINE(BOXMODEL_ET4X00, 1, [building for xtrend et4x00])
elif test "$BOXMODEL" = "et5x00"; then
	AC_DEFINE(BOXMODEL_ET5X00, 1, [building for xtrend et5x00])
elif test "$BOXMODEL" = "et6x00"; then
	AC_DEFINE(BOXMODEL_ET6X00, 1, [building for xtrend et6x00])
elif test "$BOXMODEL" = "et7x00"; then
	AC_DEFINE(BOXMODEL_ET7X00, 1, [building for xtrend et7x00])
elif test "$BOXMODEL" = "et8000"; then
	AC_DEFINE(BOXMODEL_ET8000, 1, [building for xtrend et8000])
elif test "$BOXMODEL" = "et8500"; then
	AC_DEFINE(BOXMODEL_ET8500, 1, [building for xtrend et8500])
elif test "$BOXMODEL" = "et9x00"; then
	AC_DEFINE(BOXMODEL_ET9X00, 1, [building for xtrend et9x00])
elif test "$BOXMODEL" = "et10000"; then
	AC_DEFINE(BOXMODEL_ET10000, 1, [building for xtrend et10000])

elif test "$BOXMODEL" = "spark"; then
	AC_DEFINE(PLATFORM_SPARK, 1, [building for spark 7111])
elif test "$BOXMODEL" = "spark7162"; then
	AC_DEFINE(PLATFORM_SPARK7162, 1, [building for spark 7162])

elif test "$BOXMODEL" = "ufs910"; then
	AC_DEFINE(PLATFORM_UFS910, 1, [building for ufs910])
elif test "$BOXMODEL" = "ufs912"; then
	AC_DEFINE(PLATFORM_UFS912, 1, [building for ufs912])
elif test "$BOXMODEL" = "ufs913"; then
	AC_DEFINE(PLATFORM_UFS913, 1, [building for ufs913])
elif test "$BOXMODEL" = "ufs922"; then
	AC_DEFINE(PLATFORM_UFS922, 1, [building for ufs922])
elif test "$BOXMODEL" = "ufc960"; then
	AC_DEFINE(PLATFORM_UFC960, 1, [building for ufc960])

elif test "$BOXMODEL" = "ipbox55"; then
	AC_DEFINE(PLATFORM_IPBOX55, 1, [building for ipbox55])
elif test "$BOXMODEL" = "ipbox99"; then
	AC_DEFINE(PLATFORM_IPBOX99, 1, [building for ipbox99])
elif test "$BOXMODEL" = "ipbox9900"; then
	AC_DEFINE(PLATFORM_IPBOX9900, 1, [building for ipbox9900])
	
elif test "$BOXMODEL" = "hl101"; then
	AC_DEFINE(BOXMODEL_HL101, 1, [hl101])

elif test "$BOXMODEL" = "atevio700"; then
	AC_DEFINE(BOXMODEL_ATEVIO700, 1, [building for atevio700])
elif test "$BOXMODEL" = "atevio7000"; then
	AC_DEFINE(BOXMODEL_ATEVIO7000, 1, [building for atevio7000])
elif test "$BOXMODEL" = "atevio7500"; then
	AC_DEFINE(BOXMODEL_ATEVIO7500, 1, [building for atevio7500])
elif test "$BOXMODEL" = "atevio7600"; then
	AC_DEFINE(BOXMODEL_ATEVIO7600, 1, [building for atevio7600])

elif test "$BOXMODEL" = "octagon1008"; then
	AC_DEFINE(BOXMODEL_OCTAGON1008, 1, [building for octagon1008])
elif test "$BOXMODEL" = "octagonsf4008"; then
	AC_DEFINE(BOXMODEL_OCTAGONSF4008, 1, [building for octagonsf4008])
elif test "$BOXMODEL" = "octagonsf8008"; then
	AC_DEFINE(BOXMODEL_OCTAGONSF8008, 1, [building for octagonsf8008])
elif test "$BOXMODEL" = "octagonsf8008m"; then
	AC_DEFINE(BOXMODEL_OCTAGONSF8008M, 1, [building for octagonsf8008m])
elif test "$BOXMODEL" = "octagonsfx6008"; then
	AC_DEFINE(BOXMODEL_OCTAGONSFX6008, 1, [building for octagonsfx6008])
elif test "$BOXMODEL" = "octagonsx88v2"; then
	AC_DEFINE(BOXMODEL_OCTAGONSX88V2, 1, [building for octagonsx88v2])
elif test "$BOXMODEL" = "octagonsx988"; then
	AC_DEFINE(BOXMODEL_OCTAGONSX988, 1, [building for octagonsx988])

elif test "$BOXMODEL" = "vusolo2"; then
	AC_DEFINE(BOXMODEL_VUSOLO2, 1, [vuplus solo2])
elif test "$BOXMODEL" = "vuduo2"; then
	AC_DEFINE(BOXMODEL_VUDUO2, 1, [vuplus duo2])
elif test "$BOXMODEL" = "vusolo"; then
	AC_DEFINE(BOXMODEL_VUSOLO, 1, [building for vuplus solo])
elif test "$BOXMODEL" = "vuduo"; then
	AC_DEFINE(BOXMODEL_VUDUO, 1, [building for vuplus duo])
elif test "$BOXMODEL" = "vuuno"; then
	AC_DEFINE(BOXMODEL_VUUNO, 1, [building for vuplus uno])
elif test "$BOXMODEL" = "vuultimo"; then
	AC_DEFINE(BOXMODEL_VUULTIMO, 1, [building for vuplus ultimo])
elif test "$BOXMODEL" = "vusolose"; then
	AC_DEFINE(BOXMODEL_VUSOLOSE, 1, [building for vuplus solose])
elif test "$BOXMODEL" = "vusolo4k"; then
	AC_DEFINE(BOXMODEL_VUSOLO4K, 1, [building for vuplus solo4k])
elif test "$BOXMODEL" = "vuzero"; then
	AC_DEFINE(BOXMODEL_VUZERO, 1, [building for vuplus zero])
elif test "$BOXMODEL" = "vuduo4k"; then
	AC_DEFINE(BOXMODEL_VUDUO4K, 1, [building for vuplus duo4k])

elif test "$BOXMODEL" = "azboxhd"; then
	AC_DEFINE(BOXMODEL_AZBOXHD, 1, [building for azbox hd])
elif test "$BOXMODEL" = "azboxme"; then
	AC_DEFINE(BOXMODEL_AZBOXME, 1, [building for azbox me])
elif test "$BOXMODEL" = "azboxminime"; then
	AC_DEFINE(BOXMODEL_AZBOXMINIME, 1, [building for azbox minime])

elif test "$BOXMODEL" = "tmtwin"; then
	AC_DEFINE(BOXMODEL_TMTWIN, 1, [building for technomate twin])
elif test "$BOXMODEL" = "tm2t"; then
	AC_DEFINE(BOXMODEL_TM2T, 1, [building for technomate 2t])
elif test "$BOXMODEL" = "tmsingle"; then
	AC_DEFINE(BOXMODEL_TMSINGLE, 1, [building for technomate single])
elif test "$BOXMODEL" = "tmnano"; then
	AC_DEFINE(BOXMODEL_TMNANO, 1, [building for technomate nano])

elif test "$BOXMODEL" = "ventonhde"; then
	AC_DEFINE(BOXMODEL_VENTONHDE, 1, [building for ventonhde])
elif test "$BOXMODEL" = "ventonhdx"; then
	AC_DEFINE(BOXMODEL_VENTONHDX, 1, [building for ventonhdx])
elif test "$BOXMODEL" = "inihde"; then
	AC_DEFINE(BOXMODEL_INIHDE, 1, [building for inihde])
elif test "$BOXMODEL" = "inihdp"; then
	AC_DEFINE(BOXMODEL_INIHDP, 1, [building for inihdp])

elif test "$BOXMODEL" = "ixusszero"; then
	AC_DEFINE(BOXMODEL_IXUSSZERO, 1, [building for ixusszero])
elif test "$BOXMODEL" = "ixussone"; then
	AC_DEFINE(BOXMODEL_IXUSSONE, 1, [building for ixussone])

elif test "$BOXMODEL" = "iqonios100hd"; then
	AC_DEFINE(BOXMODEL_IQONIOS100HD, 1, [building for iqonios100hd])
elif test "$BOXMODEL" = "iqonios300hd"; then
	AC_DEFINE(BOXMODEL_IQONIOS300HD, 1, [building for iqonios300hd])
elif test "$BOXMODEL" = "mediabox"; then
	AC_DEFINE(BOXMODEL_MEDIABOX, 1, [building for mediabox])
elif test "$BOXMODEL" = "optimussos1"; then
	AC_DEFINE(BOXMODEL_OPTIMUSSOS1, 1, [building for optimussos1])
elif test "$BOXMODEL" = "optimussos2"; then
	AC_DEFINE(BOXMODEL_OPTIMUSSOS2, 1, [building for optimussos2])
 
elif test "$BOXMODEL" = "odinm6"; then
	AC_DEFINE(BOXMODEL_ODINM6, 1, [building for odin m6])
elif test "$BOXMODEL" = "odinm7"; then
	AC_DEFINE(BOXMODEL_ODINM7, 1, [building for odinm7])
elif test "$BOXMODEL" = "odinm9"; then
	AC_DEFINE(BOXMODEL_ODINM9, 1, [building for odinm9])

elif test "$BOXMODEL" = "wetekplay"; then
	AC_DEFINE(BOXMODEL_WETEKPLAY, 1, [building for wetekplay])

elif test "$BOXMODEL" = "osmini"; then
	AC_DEFINE(BOXMODEL_OSMINI, 1, [building for osmini])
elif test "$BOXMODEL" = "osminiplus"; then
	AC_DEFINE(BOXMODEL_OSMINIPLUS, 1, [building for osminiplus])
elif test "$BOXMODEL" = "osnino"; then
	AC_DEFINE(BOXMODEL_OSNINO, 1, [building for osnino])
elif test "$BOXMODEL" = "osninoplus"; then
	AC_DEFINE(BOXMODEL_OSNINOPLUS, 1, [building for osninoplus])
elif test "$BOXMODEL" = "osninopro"; then
	AC_DEFINE(BOXMODEL_OSNINOPRO, 1, [building for osninopro])
elif test "$BOXMODEL" = "osmio4k"; then
	AC_DEFINE(BOXMODEL_OSMIO4K, 1, [building for osmio4k])
elif test "$BOXMODEL" = "osmio4kplus"; then
	AC_DEFINE(BOXMODEL_OSMIO4KPLUS, 1, [building for osmio4kplus])

elif test "$BOXMODEL" = "hd11"; then
	AC_DEFINE(BOXMODEL_HD11, 1, [building for hd11])
elif test "$BOXMODEL" = "hd51"; then
	AC_DEFINE(BOXMODEL_HD51, 1, [building for hd51])
elif test "$BOXMODEL" = "ax51"; then
	AC_DEFINE(BOXMODEL_AX51, 1, [building for ax51])
elif test "$BOXMODEL" = "hd500c"; then
	AC_DEFINE(BOXMODEL_HD500C, 1, [building for hd500c])
elif test "$BOXMODEL" = "hd1100"; then
	AC_DEFINE(BOXMODEL_HD1100, 1, [building for hd1100])
elif test "$BOXMODEL" = "hd1200"; then
	AC_DEFINE(BOXMODEL_HD1200, 1, [building for hd1200])
elif test "$BOXMODEL" = "hd1265"; then
	AC_DEFINE(BOXMODEL_HD1265, 1, [building for hd1265])
elif test "$BOXMODEL" = "hd1500"; then
	AC_DEFINE(BOXMODEL_HD1500, 1, [building for hd1500])
elif test "$BOXMODEL" = "hd2400"; then
	AC_DEFINE(BOXMODEL_HD2400, 1, [building for hd2400])

elif test "$BOXMODEL" = "et7000mini"; then
	AC_DEFINE(BOXMODEL_ET7000MINI, 1, [building for et7000mini])

elif test "$BOXMODEL" = "xpeedc"; then
	AC_DEFINE(BOXMODEL_XPEEDC, 1, [building for xpeedc])

elif test "$BOXMODEL" = "formuler1"; then
	AC_DEFINE(BOXMODEL_FORMULER1, 1, [building for formuler1])
elif test "$BOXMODEL" = "formuler3"; then
	AC_DEFINE(BOXMODEL_FORMULER3, 1, [building for formuler3])
elif test "$BOXMODEL" = "formuler4"; then
	AC_DEFINE(BOXMODEL_FORMULER4, 1, [building for formuler4])

elif test "$BOXMODEL" = "mbmini"; then
	AC_DEFINE(BOXMODEL_MBMINI, 1, [building for mbmini])
elif test "$BOXMODEL" = "mbtwinplus"; then
	AC_DEFINE(BOXMODEL_MBTWINPLUS, 1, [building for mbtwinplus])

elif test "$BOXMODEL" = "spycat"; then
	AC_DEFINE(BOXMODEL_SPYCAT, 1, [building for spycat])
elif test "$BOXMODEL" = "spycatmini"; then
	AC_DEFINE(BOXMODEL_SPYCATMINI, 1, [building for spycatmini])

elif test "$BOXMODEL" = "fusionhd"; then
	AC_DEFINE(BOXMODEL_FUSIONHD, 1, [building for fusionhd])
elif test "$BOXMODEL" = "fusionhdse"; then
	AC_DEFINE(BOXMODEL_FUSIONHDSE, 1, [building for fusionhdse])
elif test "$BOXMODEL" = "purehd"; then
	AC_DEFINE(BOXMODEL_PUREHD, 1, [building for purehd])

elif test "$BOXMODEL" = "h3"; then
	AC_DEFINE(BOXMODEL_H3, 1, [building for h3])
elif test "$BOXMODEL" = "h4"; then
	AC_DEFINE(BOXMODEL_H4, 1, [building for h4])
elif test "$BOXMODEL" = "h5"; then
	AC_DEFINE(BOXMODEL_H5, 1, [building for h5])
elif test "$BOXMODEL" = "h7"; then
	AC_DEFINE(BOXMODEL_H7, 1, [building for h7])
elif test "$BOXMODEL" = "i55"; then
	AC_DEFINE(BOXMODEL_I55, 1, [building for i55])
elif test "$BOXMODEL" = "lc"; then
	AC_DEFINE(BOXMODEL_LC, 1, [building for lc])
elif test "$BOXMODEL" = "sh1"; then
	AC_DEFINE(BOXMODEL_SH1, 1, [building for sh1])

elif test "$BOXMODEL" = "bre2ze4k"; then
	AC_DEFINE(BOXMODEL_BRE2ZE4K, 1, [building for bre2ze4k])
elif test "$BOXMODEL" = "bre2zet2c"; then
	AC_DEFINE(BOXMODEL_BRE2ZET2C, 1, [building for bre2zet2c])

elif test "$BOXMODEL" = "e3hd"; then
	AC_DEFINE(BOXMODEL_E3HD, 1, [building for e3hd])	
elif test "$BOXMODEL" = "e4hdultra"; then
	AC_DEFINE(BOXMODEL_E4HDULTRA, 1, [building for e4hdultra])
	
elif test "$BOXMODEL" = "pulse4k"; then
	AC_DEFINE(BOXMODEL_PULSE4K, 1, [building for pulse4k])
elif test "$BOXMODEL" = "pulse4kmini"; then
	AC_DEFINE(BOXMODEL_PULSE4KMINI, 1, [building for pulse4kmini])
	
elif test "$BOXMODEL" = "multibox"; then
	AC_DEFINE(BOXMODEL_MULTIBOX, 1, [building for multibox])
elif test "$BOXMODEL" = "multiboxse"; then
	AC_DEFINE(BOXMODEL_MULTIBOXSE, 1, [building for multiboxse])
fi
])

# ax_pthread
AU_ALIAS([ACX_PTHREAD], [AX_PTHREAD])
AC_DEFUN([AX_PTHREAD], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_LANG_SAVE
AC_LANG_C
ax_pthread_ok=no

# We used to check for pthread.h first, but this fails if pthread.h
# requires special compiler flags (e.g. on True64 or Sequent).
# It gets checked for in the link test anyway.

# First of all, check if the user has set any of the PTHREAD_LIBS,
# etcetera environment variables, and if threads linking works using
# them:
if test x"$PTHREAD_LIBS$PTHREAD_CFLAGS" != x; then
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        AC_MSG_CHECKING([for pthread_join in LIBS=$PTHREAD_LIBS with CFLAGS=$PTHREAD_CFLAGS])
        AC_TRY_LINK_FUNC(pthread_join, ax_pthread_ok=yes)
        AC_MSG_RESULT($ax_pthread_ok)
        if test x"$ax_pthread_ok" = xno; then
                PTHREAD_LIBS=""
                PTHREAD_CFLAGS=""
        fi
        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"
fi

# We must check for the threads library under a number of different
# names; the ordering is very important because some systems
# (e.g. DEC) have both -lpthread and -lpthreads, where one of the
# libraries is broken (non-POSIX).

# Create a list of thread flags to try.  Items starting with a "-" are
# C compiler flags, and other items are library names, except for "none"
# which indicates that we try without any flags at all, and "pthread-config"
# which is a program returning the flags for the Pth emulation library.

ax_pthread_flags="pthreads none -Kthread -kthread lthread -pthread -pthreads -mthreads pthread --thread-safe -mt pthread-config"

# The ordering *is* (sometimes) important.  Some notes on the
# individual items follow:

# pthreads: AIX (must check this before -lpthread)
# none: in case threads are in libc; should be tried before -Kthread and
#       other compiler flags to prevent continual compiler warnings
# -Kthread: Sequent (threads in libc, but -Kthread needed for pthread.h)
# -kthread: FreeBSD kernel threads (preferred to -pthread since SMP-able)
# lthread: LinuxThreads port on FreeBSD (also preferred to -pthread)
# -pthread: Linux/gcc (kernel threads), BSD/gcc (userland threads)
# -pthreads: Solaris/gcc
# -mthreads: Mingw32/gcc, Lynx/gcc
# -mt: Sun Workshop C (may only link SunOS threads [-lthread], but it
#      doesn't hurt to check since this sometimes defines pthreads too;
#      also defines -D_REENTRANT)
#      ... -mt is also the pthreads flag for HP/aCC
# pthread: Linux, etcetera
# --thread-safe: KAI C++
# pthread-config: use pthread-config program (for GNU Pth library)

case "${host_cpu}-${host_os}" in
        *solaris*)

        # On Solaris (at least, for some versions), libc contains stubbed
        # (non-functional) versions of the pthreads routines, so link-based
        # tests will erroneously succeed.  (We need to link with -pthreads/-mt/
        # -lpthread.)  (The stubs are missing pthread_cleanup_push, or rather
        # a function called by this macro, so we could check for that, but
        # who knows whether they'll stub that too in a future libc.)  So,
        # we'll just look for -pthreads and -lpthread first:

        ax_pthread_flags="-pthreads pthread -mt -pthread $ax_pthread_flags"
        ;;

	*-darwin*)
	ax_pthread_flags="-pthread $ax_pthread_flags"
	;;
esac

if test x"$ax_pthread_ok" = xno; then
for flag in $ax_pthread_flags; do

        case $flag in
                none)
                AC_MSG_CHECKING([whether pthreads work without any flags])
                ;;

                -*)
                AC_MSG_CHECKING([whether pthreads work with $flag])
                PTHREAD_CFLAGS="$flag"
                ;;

		pthread-config)
		AC_CHECK_PROG(ax_pthread_config, pthread-config, yes, no)
		if test x"$ax_pthread_config" = xno; then continue; fi
		PTHREAD_CFLAGS="`pthread-config --cflags`"
		PTHREAD_LIBS="`pthread-config --ldflags` `pthread-config --libs`"
		;;

                *)
                AC_MSG_CHECKING([for the pthreads library -l$flag])
                PTHREAD_LIBS="-l$flag"
                ;;
        esac

        save_LIBS="$LIBS"
        save_CFLAGS="$CFLAGS"
        LIBS="$PTHREAD_LIBS $LIBS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Check for various functions.  We must include pthread.h,
        # since some functions may be macros.  (On the Sequent, we
        # need a special flag -Kthread to make this header compile.)
        # We check for pthread_join because it is in -lpthread on IRIX
        # while pthread_create is in libc.  We check for pthread_attr_init
        # due to DEC craziness with -lpthreads.  We check for
        # pthread_cleanup_push because it is one of the few pthread
        # functions on Solaris that doesn't have a non-functional libc stub.
        # We try pthread_create on general principles.
        AC_TRY_LINK([#include <pthread.h>
		     static void routine(void* a) {a=0;}
		     static void* start_routine(void* a) {return a;}],
                    [pthread_t th; pthread_attr_t attr;
                     pthread_create(&th,0,start_routine,0);
                     pthread_join(th, 0);
                     pthread_attr_init(&attr);
                     pthread_cleanup_push(routine, 0);
                     pthread_cleanup_pop(0); ],
                    [ax_pthread_ok=yes])

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        AC_MSG_RESULT($ax_pthread_ok)
        if test "x$ax_pthread_ok" = xyes; then
                break;
        fi

        PTHREAD_LIBS=""
        PTHREAD_CFLAGS=""
done
fi

# Various other checks:
if test "x$ax_pthread_ok" = xyes; then
        save_LIBS="$LIBS"
        LIBS="$PTHREAD_LIBS $LIBS"
        save_CFLAGS="$CFLAGS"
        CFLAGS="$CFLAGS $PTHREAD_CFLAGS"

        # Detect AIX lossage: JOINABLE attribute is called UNDETACHED.
	AC_MSG_CHECKING([for joinable pthread attribute])
	attr_name=unknown
	for attr in PTHREAD_CREATE_JOINABLE PTHREAD_CREATE_UNDETACHED; do
	    AC_TRY_LINK([#include <pthread.h>], [int attr=$attr; return attr;],
                        [attr_name=$attr; break])
	done
        AC_MSG_RESULT($attr_name)
        if test "$attr_name" != PTHREAD_CREATE_JOINABLE; then
            AC_DEFINE_UNQUOTED(PTHREAD_CREATE_JOINABLE, $attr_name,
                               [Define to necessary symbol if this constant
                                uses a non-standard name on your system.])
        fi

        AC_MSG_CHECKING([if more special flags are required for pthreads])
        flag=no
        case "${host_cpu}-${host_os}" in
            *-aix* | *-freebsd* | *-darwin*) flag="-D_THREAD_SAFE";;
            *solaris* | *-osf* | *-hpux*) flag="-D_REENTRANT";;
        esac
        AC_MSG_RESULT(${flag})
        if test "x$flag" != xno; then
            PTHREAD_CFLAGS="$flag $PTHREAD_CFLAGS"
        fi

        LIBS="$save_LIBS"
        CFLAGS="$save_CFLAGS"

        # More AIX lossage: must compile with xlc_r or cc_r
	if test x"$GCC" != xyes; then
          AC_CHECK_PROGS(PTHREAD_CC, xlc_r cc_r, ${CC})
        else
          PTHREAD_CC=$CC
	fi
else
        PTHREAD_CC="$CC"
fi

AC_SUBST(PTHREAD_LIBS)
AC_SUBST(PTHREAD_CFLAGS)
AC_SUBST(PTHREAD_CC)

# Finally, execute ACTION-IF-FOUND/ACTION-IF-NOT-FOUND:
if test x"$ax_pthread_ok" = xyes; then
        ifelse([$1],,AC_DEFINE(HAVE_PTHREAD,1,[Define if you have POSIX threads libraries and header files.]),[$1])
        :
else
        ax_pthread_ok=no
        $2
fi
AC_LANG_RESTORE
])dnl AX_PTHREAD

# ax_pkg_swig
AC_DEFUN([AX_PKG_SWIG],[
        AC_PATH_PROG([SWIG],[swig])
        if test -z "$SWIG" ; then
                m4_ifval([$3],[$3],[:])
        elif test -n "$1" ; then
                AC_MSG_CHECKING([SWIG version])
                [swig_version=`$SWIG -version 2>&1 | grep 'SWIG Version' | sed 's/.*\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\).*/\1/g'`]
                AC_MSG_RESULT([$swig_version])
                if test -n "$swig_version" ; then
                        # Calculate the required version number components
                        [required=$1]
                        [required_major=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_major" ; then
                                [required_major=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_minor=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_minor" ; then
                                [required_minor=0]
                        fi
                        [required=`echo $required | sed 's/[0-9]*[^0-9]//'`]
                        [required_patch=`echo $required | sed 's/[^0-9].*//'`]
                        if test -z "$required_patch" ; then
                                [required_patch=0]
                        fi
                        # Calculate the available version number components
                        [available=$swig_version]
                        [available_major=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_major" ; then
                                [available_major=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_minor=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_minor" ; then
                                [available_minor=0]
                        fi
                        [available=`echo $available | sed 's/[0-9]*[^0-9]//'`]
                        [available_patch=`echo $available | sed 's/[^0-9].*//'`]
                        if test -z "$available_patch" ; then
                                [available_patch=0]
                        fi
                        # Convert the version tuple into a single number for easier comparison.
                        # Using base 100 should be safe since SWIG internally uses BCD values
                        # to encode its version number.
                        required_swig_vernum=`expr $required_major \* 10000 \
                            \+ $required_minor \* 100 \+ $required_patch`
                        available_swig_vernum=`expr $available_major \* 10000 \
                            \+ $available_minor \* 100 \+ $available_patch`

                        if test $available_swig_vernum -lt $required_swig_vernum; then
                                AC_MSG_WARN([SWIG version >= $1 is required.  You have $swig_version.])
                                SWIG=''
                                m4_ifval([$3],[$3],[])
                        else
                                AC_MSG_CHECKING([for SWIG library])
                                SWIG_LIB=`$SWIG -swiglib`
                                AC_MSG_RESULT([$SWIG_LIB])
                                m4_ifval([$2],[$2],[])
                        fi
                else
                        AC_MSG_WARN([cannot determine SWIG version])
                        SWIG=''
                        m4_ifval([$3],[$3],[])
                fi
        fi
        AC_SUBST([SWIG_LIB])
])

# ax_swig_enable_cxx
AU_ALIAS([SWIG_ENABLE_CXX], [AX_SWIG_ENABLE_CXX])
AC_DEFUN([AX_SWIG_ENABLE_CXX],[
        AC_REQUIRE([AX_PKG_SWIG])
        AC_REQUIRE([AC_PROG_CXX])
        SWIG="$SWIG -c++"
])

dnl =========================================================================
dnl AX_PROG_LUA([MINIMUM-VERSION], [TOO-BIG-VERSION],
dnl             [ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl =========================================================================
AC_DEFUN([AX_PROG_LUA],
[
  dnl Check for required tools.
  AC_REQUIRE([AC_PROG_GREP])
  AC_REQUIRE([AC_PROG_SED])

  dnl Make LUA a precious variable.
  AC_ARG_VAR([LUA], [The Lua interpreter, e.g. /usr/bin/lua5.1])

  dnl Find a Lua interpreter.
  m4_define_default([_AX_LUA_INTERPRETER_LIST],
    [lua lua5.3 lua53 lua5.2 lua52 lua5.1 lua51 lua50])

  m4_if([$1], [],
  [ dnl No version check is needed. Find any Lua interpreter.
    AS_IF([test "x$LUA" = 'x'],
      [AC_PATH_PROGS([LUA], [_AX_LUA_INTERPRETER_LIST], [:])])
    ax_display_LUA='lua'

    AS_IF([test "x$LUA" != 'x:'],
      [ dnl At least check if this is a Lua interpreter.
        AC_MSG_CHECKING([if $LUA is a Lua interpreter])
        _AX_LUA_CHK_IS_INTRP([$LUA],
          [AC_MSG_RESULT([yes])],
          [ AC_MSG_RESULT([no])
            AC_MSG_ERROR([not a Lua interpreter])
          ])
      ])
  ],
  [ dnl A version check is needed.
    AS_IF([test "x$LUA" != 'x'],
    [ dnl Check if this is a Lua interpreter.
      AC_MSG_CHECKING([if $LUA is a Lua interpreter])
      _AX_LUA_CHK_IS_INTRP([$LUA],
        [AC_MSG_RESULT([yes])],
        [ AC_MSG_RESULT([no])
          AC_MSG_ERROR([not a Lua interpreter])
        ])
      dnl Check the version.
      m4_if([$2], [],
        [_ax_check_text="whether $LUA version >= $1"],
        [_ax_check_text="whether $LUA version >= $1, < $2"])
      AC_MSG_CHECKING([$_ax_check_text])
      _AX_LUA_CHK_VER([$LUA], [$1], [$2],
        [AC_MSG_RESULT([yes])],
        [ AC_MSG_RESULT([no])
          AC_MSG_ERROR([version is out of range for specified LUA])])
      ax_display_LUA=$LUA
    ],
    [ dnl Try each interpreter until we find one that satisfies VERSION.
      m4_if([$2], [],
        [_ax_check_text="for a Lua interpreter with version >= $1"],
        [_ax_check_text="for a Lua interpreter with version >= $1, < $2"])
      AC_CACHE_CHECK([$_ax_check_text],
        [ax_cv_pathless_LUA],
        [ for ax_cv_pathless_LUA in _AX_LUA_INTERPRETER_LIST none; do
            test "x$ax_cv_pathless_LUA" = 'xnone' && break
            _AX_LUA_CHK_IS_INTRP([$ax_cv_pathless_LUA], [], [continue])
            _AX_LUA_CHK_VER([$ax_cv_pathless_LUA], [$1], [$2], [break])
          done
        ])
      dnl Set $LUA to the absolute path of $ax_cv_pathless_LUA.
      AS_IF([test "x$ax_cv_pathless_LUA" = 'xnone'],
        [LUA=':'],
        [AC_PATH_PROG([LUA], [$ax_cv_pathless_LUA])])
      ax_display_LUA=$ax_cv_pathless_LUA
    ])
  ])

  AS_IF([test "x$LUA" = 'x:'],
  [ dnl Run any user-specified action, or abort.
    m4_default([$4], [AC_MSG_ERROR([cannot find suitable Lua interpreter])])
  ],
  [ dnl Query Lua for its version number.
    AC_CACHE_CHECK([for $ax_display_LUA version],
      [ax_cv_lua_version],
      [ dnl Get the interpreter version in X.Y format. This should work for
        dnl interpreters version 5.0 and beyond.
        ax_cv_lua_version=[`$LUA -e '
          -- return a version number in X.Y format
          local _, _, ver = string.find(_VERSION, "^Lua (%d+%.%d+)")
          print(ver)'`]
      ])
    AS_IF([test "x$ax_cv_lua_version" = 'x'],
      [AC_MSG_ERROR([invalid Lua version number])])
    AC_SUBST([LUA_VERSION], [$ax_cv_lua_version])
    AC_SUBST([LUA_SHORT_VERSION], [`echo "$LUA_VERSION" | $SED 's|\.||'`])

    dnl The following check is not supported:
    dnl At times (like when building shared libraries) you may want to know
    dnl which OS platform Lua thinks this is.
    AC_CACHE_CHECK([for $ax_display_LUA platform],
      [ax_cv_lua_platform],
      [ax_cv_lua_platform=[`$LUA -e 'print("unknown")'`]])
    AC_SUBST([LUA_PLATFORM], [$ax_cv_lua_platform])

    dnl Use the values of $prefix and $exec_prefix for the corresponding
    dnl values of LUA_PREFIX and LUA_EXEC_PREFIX. These are made distinct
    dnl variables so they can be overridden if need be. However, the general
    dnl consensus is that you shouldn't need this ability.
    AC_SUBST([LUA_PREFIX], ['${prefix}'])
    AC_SUBST([LUA_EXEC_PREFIX], ['${exec_prefix}'])

    dnl Lua provides no way to query the script directory, and instead
    dnl provides LUA_PATH. However, we should be able to make a safe educated
    dnl guess. If the built-in search path contains a directory which is
    dnl prefixed by $prefix, then we can store scripts there. The first
    dnl matching path will be used.
    AC_CACHE_CHECK([for $ax_display_LUA script directory],
      [ax_cv_lua_luadir],
      [ AS_IF([test "x$prefix" = 'xNONE'],
          [ax_lua_prefix=$ac_default_prefix],
          [ax_lua_prefix=$prefix])

        dnl Initialize to the default path.
        ax_cv_lua_luadir="$LUA_PREFIX/share/lua/$LUA_VERSION"

        dnl Try to find a path with the prefix.
        _AX_LUA_FND_PRFX_PTH([$LUA], [$ax_lua_prefix], [script])
        AS_IF([test "x$ax_lua_prefixed_path" != 'x'],
        [ dnl Fix the prefix.
          _ax_strip_prefix=`echo "$ax_lua_prefix" | $SED 's|.|.|g'`
          ax_cv_lua_luadir=`echo "$ax_lua_prefixed_path" | \
            $SED "s|^$_ax_strip_prefix|$LUA_PREFIX|"`
        ])
      ])
    AC_SUBST([luadir], [$ax_cv_lua_luadir])
    AC_SUBST([pkgluadir], [\${luadir}/$PACKAGE])

    dnl Lua provides no way to query the module directory, and instead
    dnl provides LUA_PATH. However, we should be able to make a safe educated
    dnl guess. If the built-in search path contains a directory which is
    dnl prefixed by $exec_prefix, then we can store modules there. The first
    dnl matching path will be used.
    AC_CACHE_CHECK([for $ax_display_LUA module directory],
      [ax_cv_lua_luaexecdir],
      [ AS_IF([test "x$exec_prefix" = 'xNONE'],
          [ax_lua_exec_prefix=$ax_lua_prefix],
          [ax_lua_exec_prefix=$exec_prefix])

        dnl Initialize to the default path.
        ax_cv_lua_luaexecdir="$LUA_EXEC_PREFIX/lib/lua/$LUA_VERSION"

        dnl Try to find a path with the prefix.
        _AX_LUA_FND_PRFX_PTH([$LUA],
          [$ax_lua_exec_prefix], [module])
        AS_IF([test "x$ax_lua_prefixed_path" != 'x'],
        [ dnl Fix the prefix.
          _ax_strip_prefix=`echo "$ax_lua_exec_prefix" | $SED 's|.|.|g'`
          ax_cv_lua_luaexecdir=`echo "$ax_lua_prefixed_path" | \
            $SED "s|^$_ax_strip_prefix|$LUA_EXEC_PREFIX|"`
        ])
      ])
    AC_SUBST([luaexecdir], [$ax_cv_lua_luaexecdir])
    AC_SUBST([pkgluaexecdir], [\${luaexecdir}/$PACKAGE])

    dnl Run any user specified action.
    $3
  ])
])

dnl AX_WITH_LUA is now the same thing as AX_PROG_LUA.
AC_DEFUN([AX_WITH_LUA],
[
  AC_MSG_WARN([[$0 is deprecated, please use AX_PROG_LUA instead]])
  AX_PROG_LUA
])


dnl =========================================================================
dnl _AX_LUA_CHK_IS_INTRP(PROG, [ACTION-IF-TRUE], [ACTION-IF-FALSE])
dnl =========================================================================
AC_DEFUN([_AX_LUA_CHK_IS_INTRP],
[
  dnl A minimal Lua factorial to prove this is an interpreter. This should work
  dnl for Lua interpreters version 5.0 and beyond.
  _ax_lua_factorial=[`$1 2>/dev/null -e '
    -- a simple factorial
    function fact (n)
      if n == 0 then
        return 1
      else
        return n * fact(n-1)
      end
    end
    print("fact(5) is " .. fact(5))'`]
  AS_IF([test "$_ax_lua_factorial" = 'fact(5) is 120'],
    [$2], [$3])
])


dnl =========================================================================
dnl _AX_LUA_CHK_VER(PROG, MINIMUM-VERSION, [TOO-BIG-VERSION],
dnl                 [ACTION-IF-TRUE], [ACTION-IF-FALSE])
dnl =========================================================================
AC_DEFUN([_AX_LUA_CHK_VER],
[
  dnl Check that the Lua version is within the bounds. Only the major and minor
  dnl version numbers are considered. This should work for Lua interpreters
  dnl version 5.0 and beyond.
  _ax_lua_good_version=[`$1 -e '
    -- a script to compare versions
    function verstr2num(verstr)
      local _, _, majorver, minorver = string.find(verstr, "^(%d+)%.(%d+)")
      if majorver and minorver then
        return tonumber(majorver) * 100 + tonumber(minorver)
      end
    end
    local minver = verstr2num("$2")
    local _, _, trimver = string.find(_VERSION, "^Lua (.*)")
    local ver = verstr2num(trimver)
    local maxver = verstr2num("$3") or 1e9
    if minver <= ver and ver < maxver then
      print("yes")
    else
      print("no")
    end'`]
    AS_IF([test "x$_ax_lua_good_version" = "xyes"],
      [$4], [$5])
])


dnl =========================================================================
dnl _AX_LUA_FND_PRFX_PTH(PROG, PREFIX, SCRIPT-OR-MODULE-DIR)
dnl =========================================================================
AC_DEFUN([_AX_LUA_FND_PRFX_PTH],
[
  dnl Get the script or module directory by querying the Lua interpreter,
  dnl filtering on the given prefix, and selecting the shallowest path. If no
  dnl path is found matching the prefix, the result will be an empty string.
  dnl The third argument determines the type of search, it can be 'script' or
  dnl 'module'. Supplying 'script' will perform the search with package.path
  dnl and LUA_PATH, and supplying 'module' will search with package.cpath and
  dnl LUA_CPATH. This is done for compatibility with Lua 5.0.

  ax_lua_prefixed_path=[`$1 -e '
    -- get the path based on search type
    local searchtype = "$3"
    local paths = ""
    if searchtype == "script" then
      paths = (package and package.path) or LUA_PATH
    elseif searchtype == "module" then
      paths = (package and package.cpath) or LUA_CPATH
    end
    -- search for the prefix
    local prefix = "'$2'"
    local minpath = ""
    local mindepth = 1e9
    string.gsub(paths, "(@<:@^;@:>@+)",
      function (path)
        path = string.gsub(path, "%?.*$", "")
        path = string.gsub(path, "/@<:@^/@:>@*$", "")
        if string.find(path, prefix) then
          local depth = string.len(string.gsub(path, "@<:@^/@:>@", ""))
          if depth < mindepth then
            minpath = path
            mindepth = depth
          end
        end
      end)
    print(minpath)'`]
])


dnl =========================================================================
dnl AX_LUA_HEADERS([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl =========================================================================
AC_DEFUN([AX_LUA_HEADERS],
[
  dnl Check for LUA_VERSION.
  AC_MSG_CHECKING([if LUA_VERSION is defined])
  AS_IF([test "x$LUA_VERSION" != 'x'],
    [AC_MSG_RESULT([yes])],
    [ AC_MSG_RESULT([no])
      AC_MSG_ERROR([cannot check Lua headers without knowing LUA_VERSION])
    ])

  dnl Make LUA_INCLUDE a precious variable.
  AC_ARG_VAR([LUA_INCLUDE], [The Lua includes, e.g. -I/usr/include/lua5.1])

  dnl Some default directories to search.
  LUA_SHORT_VERSION=`echo "$LUA_VERSION" | $SED 's|\.||'`
  m4_define_default([_AX_LUA_INCLUDE_LIST],
    [ /usr/include/lua$LUA_VERSION \
      /usr/include/lua-$LUA_VERSION \
      /usr/include/lua/$LUA_VERSION \
      /usr/include/lua$LUA_SHORT_VERSION \
      /usr/local/include/lua$LUA_VERSION \
      /usr/local/include/lua-$LUA_VERSION \
      /usr/local/include/lua/$LUA_VERSION \
      /usr/local/include/lua$LUA_SHORT_VERSION \
    ])

  dnl Try to find the headers.
  _ax_lua_saved_cppflags=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $LUA_INCLUDE"
  AC_CHECK_HEADERS([lua.h lualib.h lauxlib.h luaconf.h])
  CPPFLAGS=$_ax_lua_saved_cppflags

  dnl Try some other directories if LUA_INCLUDE was not set.
  AS_IF([test "x$LUA_INCLUDE" = 'x' &&
         test "x$ac_cv_header_lua_h" != 'xyes'],
    [ dnl Try some common include paths.
      for _ax_include_path in _AX_LUA_INCLUDE_LIST; do
        test ! -d "$_ax_include_path" && continue

        AC_MSG_CHECKING([for Lua headers in])
        AC_MSG_RESULT([$_ax_include_path])

        AS_UNSET([ac_cv_header_lua_h])
        AS_UNSET([ac_cv_header_lualib_h])
        AS_UNSET([ac_cv_header_lauxlib_h])
        AS_UNSET([ac_cv_header_luaconf_h])

        _ax_lua_saved_cppflags=$CPPFLAGS
        CPPFLAGS="$CPPFLAGS -I$_ax_include_path"
        AC_CHECK_HEADERS([lua.h lualib.h lauxlib.h luaconf.h])
        CPPFLAGS=$_ax_lua_saved_cppflags

        AS_IF([test "x$ac_cv_header_lua_h" = 'xyes'],
          [ LUA_INCLUDE="-I$_ax_include_path"
            break
          ])
      done
    ])

  AS_IF([test "x$ac_cv_header_lua_h" = 'xyes'],
    [ dnl Make a program to print LUA_VERSION defined in the header.
      dnl TODO It would be really nice if we could do this without compiling a
      dnl program, then it would work when cross compiling. But I'm not sure how
      dnl to do this reliably. For now, assume versions match when cross compiling.

      AS_IF([test "x$cross_compiling" != 'xyes'],
        [ AC_CACHE_CHECK([for Lua header version],
            [ax_cv_lua_header_version],
            [ _ax_lua_saved_cppflags=$CPPFLAGS
              CPPFLAGS="$CPPFLAGS $LUA_INCLUDE"
              AC_RUN_IFELSE(
                [ AC_LANG_SOURCE([[
#include <lua.h>
#include <stdlib.h>
#include <stdio.h>
int main(int argc, char ** argv)
{
  if(argc > 1) printf("%s", LUA_VERSION);
  exit(EXIT_SUCCESS);
}
]])
                ],
                [ ax_cv_lua_header_version=`./conftest$EXEEXT p | \
                    $SED -n "s|^Lua \(@<:@0-9@:>@\{1,\}\.@<:@0-9@:>@\{1,\}\).\{0,\}|\1|p"`
                ],
                [ax_cv_lua_header_version='unknown'])
              CPPFLAGS=$_ax_lua_saved_cppflags
            ])

          dnl Compare this to the previously found LUA_VERSION.
          AC_MSG_CHECKING([if Lua header version matches $LUA_VERSION])
          AS_IF([test "x$ax_cv_lua_header_version" = "x$LUA_VERSION"],
            [ AC_MSG_RESULT([yes])
              ax_header_version_match='yes'
            ],
            [ AC_MSG_RESULT([no])
              ax_header_version_match='no'
            ])
        ],
        [ AC_MSG_WARN([cross compiling so assuming header version number matches])
          ax_header_version_match='yes'
        ])
    ])

  dnl Was LUA_INCLUDE specified?
  AS_IF([test "x$ax_header_version_match" != 'xyes' &&
         test "x$LUA_INCLUDE" != 'x'],
    [AC_MSG_ERROR([cannot find headers for specified LUA_INCLUDE])])

  dnl Test the final result and run user code.
  AS_IF([test "x$ax_header_version_match" = 'xyes'], [$1],
    [m4_default([$2], [AC_MSG_ERROR([cannot find Lua includes])])])
])

dnl AX_LUA_HEADERS_VERSION no longer exists, use AX_LUA_HEADERS.
AC_DEFUN([AX_LUA_HEADERS_VERSION],
[
  AC_MSG_WARN([[$0 is deprecated, please use AX_LUA_HEADERS instead]])
])


dnl =========================================================================
dnl AX_LUA_READLINE([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl =========================================================================
AC_DEFUN([AX_LUA_READLINE],
[
  AX_LIB_READLINE
  AS_IF([test "x$ac_cv_header_readline_readline_h" != 'x' &&
         test "x$ac_cv_header_readline_history_h" != 'x'],
    [ LUA_LIBS_CFLAGS="-DLUA_USE_READLINE $LUA_LIBS_CFLAGS"
      $1
    ],
    [$2])
])


dnl =========================================================================
dnl AX_LUA_LIBS([ACTION-IF-FOUND], [ACTION-IF-NOT-FOUND])
dnl =========================================================================
AC_DEFUN([AX_LUA_LIBS],
[
  dnl TODO Should this macro also check various -L flags?

  dnl Check for LUA_VERSION.
  AC_MSG_CHECKING([if LUA_VERSION is defined])
  AS_IF([test "x$LUA_VERSION" != 'x'],
    [AC_MSG_RESULT([yes])],
    [ AC_MSG_RESULT([no])
      AC_MSG_ERROR([cannot check Lua libs without knowing LUA_VERSION])
    ])

  dnl Make LUA_LIB a precious variable.
  AC_ARG_VAR([LUA_LIB], [The Lua library, e.g. -llua5.1])

  AS_IF([test "x$LUA_LIB" != 'x'],
  [ dnl Check that LUA_LIBS works.
    _ax_lua_saved_libs=$LIBS
    LIBS="$LIBS $LUA_LIB"
    AC_SEARCH_LIBS([lua_load], [],
      [_ax_found_lua_libs='yes'],
      [_ax_found_lua_libs='no'])
    LIBS=$_ax_lua_saved_libs

    dnl Check the result.
    AS_IF([test "x$_ax_found_lua_libs" != 'xyes'],
      [AC_MSG_ERROR([cannot find libs for specified LUA_LIB])])
  ],
  [ dnl First search for extra libs.
    _ax_lua_extra_libs=''

    _ax_lua_saved_libs=$LIBS
    LIBS="$LIBS $LUA_LIB"
    AC_SEARCH_LIBS([exp], [m])
    AC_SEARCH_LIBS([dlopen], [dl])
    LIBS=$_ax_lua_saved_libs

    AS_IF([test "x$ac_cv_search_exp" != 'xno' &&
           test "x$ac_cv_search_exp" != 'xnone required'],
      [_ax_lua_extra_libs="$_ax_lua_extra_libs $ac_cv_search_exp"])

    AS_IF([test "x$ac_cv_search_dlopen" != 'xno' &&
           test "x$ac_cv_search_dlopen" != 'xnone required'],
      [_ax_lua_extra_libs="$_ax_lua_extra_libs $ac_cv_search_dlopen"])

    dnl Try to find the Lua libs.
    _ax_lua_saved_libs=$LIBS
    LIBS="$LIBS $LUA_LIB"
    AC_SEARCH_LIBS([lua_load],
      [ lua$LUA_VERSION \
        lua$LUA_SHORT_VERSION \
        lua-$LUA_VERSION \
        lua-$LUA_SHORT_VERSION \
        lua \
      ],
      [_ax_found_lua_libs='yes'],
      [_ax_found_lua_libs='no'],
      [$_ax_lua_extra_libs])
    LIBS=$_ax_lua_saved_libs

    AS_IF([test "x$ac_cv_search_lua_load" != 'xno' &&
           test "x$ac_cv_search_lua_load" != 'xnone required'],
      [LUA_LIB="$ac_cv_search_lua_load $_ax_lua_extra_libs"])
  ])

  dnl Test the result and run user code.
  AS_IF([test "x$_ax_found_lua_libs" = 'xyes'], [$1],
#    [m4_default([$2], [AC_MSG_ERROR([cannot find Lua libs])])])
     LUA_LIB="-llua -ldl")
])


# ax_swig_python
AU_ALIAS([SWIG_PYTHON], [AX_SWIG_PYTHON])
AC_DEFUN([AX_SWIG_PYTHON],[
        AC_REQUIRE([AX_PKG_SWIG])
        AC_REQUIRE([AX_PYTHON_DEVEL])
        test "x$1" != "xno" || swig_shadow=" -noproxy"
        AC_SUBST([AX_SWIG_PYTHON_OPT],[-python$swig_shadow])
        AC_SUBST([AX_SWIG_PYTHON_CPPFLAGS],[$PYTHON_CPPFLAGS])
])

# ax_python_devel
AU_ALIAS([AC_PYTHON_DEVEL], [AX_PYTHON_DEVEL])
AC_DEFUN([AX_PYTHON_DEVEL],[
	#
	# Allow the use of a (user set) custom python version
	#
	AC_ARG_VAR([PYTHON_VERSION],[The installed Python
		version to use, for example '2.3'. This string
		will be appended to the Python interpreter
		canonical name.])

	AC_PATH_PROG([PYTHON],[python[$PYTHON_VERSION]])
	if test -z "$PYTHON"; then
	   AC_MSG_ERROR([Cannot find python$PYTHON_VERSION in your system path])
	   PYTHON_VERSION=""
	fi

	#
	# Check for a version of Python >= 2.1.0
	#
	AC_MSG_CHECKING([for a version of Python >= '2.1.0'])
	ac_supports_python_ver=`$PYTHON -c "import sys; \
		ver = sys.version.split ()[[0]]; \
		print (ver >= '2.1.0')"`
	if test "$ac_supports_python_ver" != "True"; then
		if test -z "$PYTHON_NOVERSIONCHECK"; then
			AC_MSG_RESULT([no])
			AC_MSG_FAILURE([
This version of the AC@&t@_PYTHON_DEVEL macro
doesn't work properly with versions of Python before
2.1.0. You may need to re-run configure, setting the
variables PYTHON_CPPFLAGS, PYTHON_LDFLAGS, PYTHON_SITE_PKG,
PYTHON_EXTRA_LIBS and PYTHON_EXTRA_LDFLAGS by hand.
Moreover, to disable this check, set PYTHON_NOVERSIONCHECK
to something else than an empty string.
])
		else
			AC_MSG_RESULT([skip at user request])
		fi
	else
		AC_MSG_RESULT([yes])
	fi

	#
	# if the macro parameter ``version'' is set, honour it
	#
	if test -n "$1"; then
		AC_MSG_CHECKING([for a version of Python $1])
		ac_supports_python_ver=`$PYTHON -c "import sys; \
			ver = sys.version.split ()[[0]]; \
			print (ver $1)"`
		if test "$ac_supports_python_ver" = "True"; then
		   AC_MSG_RESULT([yes])
		else
			AC_MSG_RESULT([no])
			AC_MSG_ERROR([this package requires Python $1.
If you have it installed, but it isn't the default Python
interpreter in your system path, please pass the PYTHON_VERSION
variable to configure. See ``configure --help'' for reference.
])
			PYTHON_VERSION=""
		fi
	fi

	#
	# Check if you have distutils, else fail
	#
	AC_MSG_CHECKING([for the distutils Python package])
	ac_distutils_result=`$PYTHON -c "import distutils" 2>&1`
	if test -z "$ac_distutils_result"; then
		AC_MSG_RESULT([yes])
	else
		AC_MSG_RESULT([no])
		AC_MSG_ERROR([cannot import Python module "distutils".
Please check your Python installation. The error was:
$ac_distutils_result])
		PYTHON_VERSION=""
	fi

	#
	# Check for Python include path
	#
	AC_MSG_CHECKING([for Python include path])
	if test -z "$PYTHON_CPPFLAGS"; then
		python_path=`$PYTHON -c "import distutils.sysconfig; \
			print (distutils.sysconfig.get_python_inc ());"`
		if test -n "${python_path}"; then
			python_path="-I$python_path"
		fi
		PYTHON_CPPFLAGS=$python_path
	fi
	AC_MSG_RESULT([$PYTHON_CPPFLAGS])
	AC_SUBST([PYTHON_CPPFLAGS])

	#
	# Check for Python library path
	#
	AC_MSG_CHECKING([for Python library path])
	if test -z "$PYTHON_LDFLAGS"; then
		# (makes two attempts to ensure we've got a version number
		# from the interpreter)
		ac_python_version=`cat<<EOD | $PYTHON -

# join all versioning strings, on some systems
# major/minor numbers could be in different list elements
from distutils.sysconfig import *
ret = ''
for e in get_config_vars ('VERSION'):
	if (e != None):
		ret += e
print (ret)
EOD`

		if test -z "$ac_python_version"; then
			if test -n "$PYTHON_VERSION"; then
				ac_python_version=$PYTHON_VERSION
			else
				ac_python_version=`$PYTHON -c "import sys; \
					print (sys.version[[:3]])"`
			fi
		fi

		# Make the versioning information available to the compiler
		AC_DEFINE_UNQUOTED([HAVE_PYTHON], ["$ac_python_version"],
                                   [If available, contains the Python version number currently in use.])

		# First, the library directory:
		ac_python_libdir=`cat<<EOD | $PYTHON -

# There should be only one
import distutils.sysconfig
for e in distutils.sysconfig.get_config_vars ('LIBDIR'):
	if e != None:
		print (e)
		break
EOD`

		# Before checking for libpythonX.Y, we need to know
		# the extension the OS we're on uses for libraries
		# (we take the first one, if there's more than one fix me!):
		ac_python_soext=`$PYTHON -c \
		  "import distutils.sysconfig; \
		  print (distutils.sysconfig.get_config_vars('SO')[[0]])"`

		# Now, for the library:
		ac_python_soname=`$PYTHON -c \
		  "import distutils.sysconfig; \
		  print (distutils.sysconfig.get_config_vars('LDLIBRARY')[[0]])"`

		# Strip away extension from the end to canonicalize its name:
		ac_python_library=`echo "$ac_python_soname" | sed "s/${ac_python_soext}$//"`

		# This small piece shamelessly adapted from PostgreSQL python macro;
		# credits goes to momjian, I think. I'd like to put the right name
		# in the credits, if someone can point me in the right direction... ?
		#
		if test -n "$ac_python_libdir" -a -n "$ac_python_library" \
			-a x"$ac_python_library" != x"$ac_python_soname"
		then
			# use the official shared library
			ac_python_library=`echo "$ac_python_library" | sed "s/^lib//"`
			PYTHON_LDFLAGS="-L$ac_python_libdir -l$ac_python_library"
		else
			# old way: use libpython from python_configdir
			ac_python_libdir=`$PYTHON -c \
			  "from distutils.sysconfig import get_python_lib as f; \
			  import os; \
			  print (os.path.join(f(plat_specific=1, standard_lib=1), 'config'));"`
			PYTHON_LDFLAGS="-L$ac_python_libdir -lpython$ac_python_version"
		fi

		if test -z "PYTHON_LDFLAGS"; then
			AC_MSG_ERROR([
  Cannot determine location of your Python DSO. Please check it was installed with
  dynamic libraries enabled, or try setting PYTHON_LDFLAGS by hand.
			])
		fi
	fi
	AC_MSG_RESULT([$PYTHON_LDFLAGS])
	AC_SUBST([PYTHON_LDFLAGS])

	#
	# Check for site packages
	#
	AC_MSG_CHECKING([for Python site-packages path])
	if test -z "$PYTHON_SITE_PKG"; then
		PYTHON_SITE_PKG=`$PYTHON -c "import distutils.sysconfig; \
			print (distutils.sysconfig.get_python_lib(0,0));"`
	fi
	AC_MSG_RESULT([$PYTHON_SITE_PKG])
	AC_SUBST([PYTHON_SITE_PKG])

	#
	# libraries which must be linked in when embedding
	#
	AC_MSG_CHECKING(python extra libraries)
	if test -z "$PYTHON_EXTRA_LIBS"; then
	   PYTHON_EXTRA_LIBS=`$PYTHON -c "import distutils.sysconfig; \
                conf = distutils.sysconfig.get_config_var; \
                print (conf('LOCALMODLIBS') + ' ' + conf('LIBS'))"`
	fi
	AC_MSG_RESULT([$PYTHON_EXTRA_LIBS])
	AC_SUBST(PYTHON_EXTRA_LIBS)

	#
	# linking flags needed when embedding
	#
	AC_MSG_CHECKING(python extra linking flags)
	if test -z "$PYTHON_EXTRA_LDFLAGS"; then
		PYTHON_EXTRA_LDFLAGS=`$PYTHON -c "import distutils.sysconfig; \
			conf = distutils.sysconfig.get_config_var; \
			print (conf('LINKFORSHARED'))"`
	fi
	AC_MSG_RESULT([$PYTHON_EXTRA_LDFLAGS])
	AC_SUBST(PYTHON_EXTRA_LDFLAGS)

	#
	# final check to see if everything compiles alright
	#
	AC_MSG_CHECKING([consistency of all components of python development environment])
	# save current global flags
	ac_save_LIBS="$LIBS"
	ac_save_CPPFLAGS="$CPPFLAGS"
	LIBS="$ac_save_LIBS $PYTHON_LDFLAGS $PYTHON_EXTRA_LDFLAGS $PYTHON_EXTRA_LIBS"
	CPPFLAGS="$ac_save_CPPFLAGS $PYTHON_CPPFLAGS"
	AC_LANG_PUSH([C])
	AC_LINK_IFELSE([
		AC_LANG_PROGRAM([[#include <Python.h>]],
				[[Py_Initialize();]])
		],[pythonexists=yes],[pythonexists=no])
	AC_LANG_POP([C])
	# turn back to default flags
	CPPFLAGS="$ac_save_CPPFLAGS"
	LIBS="$ac_save_LIBS"

	AC_MSG_RESULT([$pythonexists])

        if test ! "x$pythonexists" = "xyes"; then
	   AC_MSG_FAILURE([
  Could not link test program to Python. Maybe the main Python library has been
  installed in some non-standard library path. If so, pass it to configure,
  via the LDFLAGS environment variable.
  Example: ./configure LDFLAGS="-L/usr/non-standard-path/python/lib"
  ============================================================================
   ERROR!
   You probably have to install the development version of the Python package
   for your distribution.  The exact name of this package varies among them.
  ============================================================================
	   ])
	  PYTHON_VERSION=""
	fi

	#
	# all done!
	#
])




