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

if test "$prefix" = "NONE"; then
	prefix=/usr/local
	
	# workaround for hd2 buildsystem
	datadir="/usr/share"
	localstatedir="/var"
else
	datadir="\${prefix}/share"
fi

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
	$2=$withval
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

TUXBOX_APPS_DIRECTORY_ONE(datadir,DATADIR,datadir,/share,/tuxbox,
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
	[  --with-boxtype          valid values: generic,dgs,gigablue,dreambox,xtrend,fulan,kathrein,ipbox,topfield,fortis_hdbox,octagon,atevio,adb_box,whitebox,vip,homecast,vuplus,azbox,technomate,coolstream,hypercube,venton,xp1000,odin,ixuss,iqonios,e3hd,ebox5000,wetek,edision,hd,gi,xpeedc,formuler,miraclebox,spycat,xsarius,zgemma,wwio],
	[case "${withval}" in
		generic|dgs|gigablue|dreambox|xtrend|fulan|kathrein|ipbox|hl101|topfield|fortis_hdbox|octagon|atevio|adb_box|whitebox|vip|homecast|vuplus|azbox|technomate|coolstream|hypercube|venton|xp1000|odin|ixuss|iqonios|e3hd|ebox5000|wetek|edision|hd|gi|xpeedc|formuler|miraclebox|spycat|xsarius|zgemma|wwio)
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
		oct*)
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

		h7)
			BOXTYPE="zgemma"
			BOXMODEL="$withval"
			;;
	
		bre*)
			BOXTYPE="wwio"
			BOXMODEL="$withval"
			;;

		*)
			AC_MSG_ERROR([unsupported value $withval for --with-boxtype])
			;;
	esac], [BOXTYPE="generic"])

AC_ARG_WITH(boxmodel,
	[  --with-boxmodel	valid for dgs: cuberevo,cuberevo_mini,cuberevo_mini2,cuberevo_mini_fta,cuberevo_250hd,cuberevo_2000hd,cuberevo_9500hd
				valid for gigablue: gbsolo,gb800se,gb800ue,gb800seplus,gb800ueplus,gbquad
				valid for dreambox: dm500, dm500plus, dm600pvr, dm56x0, dm7000, dm7020, dm7025, dm500hd, dm7020hd, dm8000, dm800, dm800se, dm520
				valid for xtrend: et4x00,et5x00,et6x00,et7x00, et8000,et8500,et9x00, et10000
				valid for fulan: spark, spark7162
				valid for kathrein: ufs910, ufs922, ufs912, ufs913, ufc960
				valid for ipbox: ipbox55, ipbox99, ipbox9900
				valid for ipbox: hl101
				valid for atevio: atevio700,atevio7000,atevio7500,atevio7600
				valid for octagon: octagon1008
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
				valid for wwio: bre2ze4k],
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
		dm500|dm500plus|dm600pvr|dm56x0|dm7000|dm7020|dm7025|dm500hd|dm7020hd|dm8000|dm800|dm800se|dm520)
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
		octagon1008)
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
		e3hd)
			if test "$BOXTYPE" = "e3hd"; then
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
		bre2ze4k)
			if test "$BOXTYPE" = "wwio"; then
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
	esac],
	[if test "$BOXTYPE" = "dgs" -o "$BOXTYPE" = "gigablue" -o "$BOXTYPE" = "dreambox" -o "$BOXTYPE" = "xtrend" -o "$BOXTYPE" = "fulan" -o "$BOXTYPE" = "kathrein" -o "$BOXTYPE" = "ipbox" -o "$BOXTYPE" = "atevio" -o "$BOXTYPE" = "octagon" -o "$BOXTYPE" = "vuplus" -o "$BOXTYPE" = "technomate" -o "$BOXTYPE" = "venton" -o "$BOXTYPE" = "ixuss" -o "$BOXTYPE" = "iqonios" -o "$BOXTYPE" = "odin" -o "$BOXTYPE" = "edision" -o "$BOXTYPE" = "hd" -o "$BOXTYPE" = "gi" -o "$BOXTYPE" = "formuler" -o "$BOXTYPE" = "miraclebox" -o "$BOXTYPE" = "spycat" -o "$BOXTYPE" = "xsarius" -o "$BOXTYPE" = "zgemma" -o "$BOXTYPE" = "wwio" && test -z "$BOXMODEL"; then
		AC_MSG_ERROR([this boxtype $BOXTYPE needs --with-boxmodel])
	fi])

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
AM_CONDITIONAL(BOXTYPE_IPBOX, test "$BOXTYPE" = "ipbox")
AM_CONDITIONAL(BOXTYPE_KATHREIN, test "$BOXTYPE" = "kathrein")
AM_CONDITIONAL(BOXTYPE_COOLSTREAM, test "$BOXTYPE" = "coolstream")
AM_CONDITIONAL(BOXTYPE_HYPERCUBE, test "$BOXTYPE" = "hypercube")
AM_CONDITIONAL(BOXTYPE_VENTON, test "$BOXTYPE" = "venton")
AM_CONDITIONAL(BOXTYPE_IXUSS, test "$BOXTYPE" = "ixuss")
AM_CONDITIONAL(BOXTYPE_IQONIOS, test "$BOXTYPE" = "iqonios")
AM_CONDITIONAL(BOXTYPE_ODIN, test "$BOXTYPE" = "odin")
AM_CONDITIONAL(BOXTYPE_XP1000, test "$BOXTYPE" = "xp1000")
AM_CONDITIONAL(BOXTYPE_E3HD, test "$BOXTYPE" = "e3hd")
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

AM_CONDITIONAL(BOXMODEL_OCTAGON_1008, test "$BOXMODEL" = "octagon1008")

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

AM_CONDITIONAL(BOXMODEL_E3HD,test "$BOXMODEL" = "e3hd")

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
	AC_DEFINE(BOXMODEL_OCTAGON_1008, 1, [building for octagon1008])

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

elif test "$BOXMODEL" = "e3hd"; then
	AC_DEFINE(BOXMODEL_E3HD, 1, [building for e3hd])

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
fi
])

