--[[
	mtv.ch
	Copyright (C) 2015,2019,2020  Jacek Jendrzej 'satbaby'
	With Help from: Thomas(2015,2016,2017,2018,2019,2020),Dosik7(2017),BPanther(2018)

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to the
	Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
	Boston, MA  02110-1301, USA.
]]

local glob = {}
local mtv_version = "mtv.de Version 0.37" -- Lua API Version: " .. APIVERSION.MAJOR .. "." .. APIVERSION.MINOR

local conf = {}

function get_confFile()
	return neutrino2.PLUGINDIR .. "/mtv/mtv.conf"
end
function get_conf_mtvfavFile()
	return neutrino2.PLUGINDIR .. "/mtv/mtvfav.conf"
end

function saveConfig()
	--if conf.changed then
		local config = neutrino2.CConfigFile('\t')

		config:setString("path", conf.path)
		config:setString("path_m3u", conf.path_m3u)
		config:setBool  ("dlflag",conf.dlflag)
		config:setBool  ("flvflag",conf.flvflag)
		config:setBool  ("hlsflag",conf.hlsflag)
		config:setBool  ("playflvflag",conf.playflvflag)
		config:setBool  ("shuffleflag",conf.shuffleflag)
		config:setString("maxRes", conf.maxRes)
		config:setString("search", conf.search)
		config:saveConfig(get_confFile())
		conf.changed = false
	--end

	--if glob.fav_changed == true then
		local file_mtvconf = io.open(get_conf_mtvfavFile(),"w")
		for k, v in ipairs(glob.mtv) do
			if v.fav == true then
				file_mtvconf:write('name="'.. v.name ..'",url="'.. v.url ..'"\n')
			end
		end
		file_mtvconf:close()
	--end
end

function loadConfig()
	local config = neutrino2.CConfigFile('\t')

	config:loadConfig(get_confFile())
	conf.path = config:getString("path", "/media/sda1/movies/")
	conf.path_m3u = config:getString("path_m3u", "/media/sda1/movies/")
	conf.dlflag = config:getBool("dlflag", false)
	conf.flvflag = config:getBool("flvflag", false)
	conf.hlsflag = config:getBool("hlsflag", true) --hls as default, rtmp server is broken???
	conf.playflvflag = config:getBool("playflvflag", false)
	conf.shuffleflag = config:getBool("shuffleflag", false)
	conf.maxRes = config:getString("maxRes", "1280x720")
	conf.search = config:getString("search", "Justin Bieber")
	conf.changed = false
end

function which(bin_name)
	local path = os.getenv("PATH") or "/bin"
	for v in path:gmatch("([^:]+):?") do
		local file = v .. "/" .. bin_name
		if neutrino2.file_exists(file) then
			return true
		end
	end
	return false
end

function read_file(filename)
	if filename == nil then
		print("Error: FileName  is empty")
		return nil
	end

	local fp = io.open(filename, "r")

	if fp == nil then 
		print("Error opening file '" .. filename .. "'.") 
		return nil 
	end

	local data = fp:read("*a")
	fp:close()

	return data
end

function get_json_data(url)
	print("get_json_data: ")

	local data = getdata(url)

	if data == nil then 
		return nil 
	end

	local videosection = string.match(data,"triforceManifestFeed = (.-});")

	if glob.mtv_live_url == nil then
		glob.mtv_live_url = data:match("(http[%w%./:]+mtv%-germany%-live)")
	end

	data = nil
	collectgarbage()

	return videosection
end

function init()
	print("init:")

	collectgarbage()

	if vodeoPlay == nil then
		vodeoPlay = neutrino2.CMoviePlayerGui()
	end

	neutrino2.CFileHelpers():createDir("/tmp/mtv")

	glob.fav_changed = false
	glob.mtv_artist={}
	glob.mtv={
			--{name = "Playlists",url="http://www.mtv.de/playlists",fav=false},
			{name = "MTV MUSIK",url="http://www.mtv.de/musik",fav=false},
			--{name = "MTV Unplugged",url="http://www.mtv.de/shows/w2nofb/mtv-unplugged",fav=false},
			{name = "Hitlist Germany - Top100",url="http://www.mtv.de/shows/fdc2nt/mtv-top-100",fav=false},
			{name = "MTV Shows",url="http://www.mtv.de/shows",fav=false},
			{name = "MTV Buzz",url="http://www.mtv.de/buzz",fav=false},
			--{name = "MTV PUSH",url="http://www.mtv.de/shows/z4pet5/mtv-push",fav=false},
			--{name = "MTV PLAYLIST",url="http://www.mtv.de/playlists",fav=false},
			{name = "ALLE MUSIKVIDEOS",url="http://www.mtv.de/musik",fav=false},
			{name = "Singel Top 100",url="http://www.mtv.de/charts/c6mc86/single-top-100",fav=false},
			--{name = "Album Top 100",url="http://www.mtv.de/charts/70tgrf/album-top-100",fav=false},
			{name = "Midweek Singel Top100",url="http://www.mtv.de/charts/n91ory/midweek-single-top-100",fav=false},
			--{name = "Midweek Album Top100",url="http://www.mtv.de/charts/ew735d/midweek-album-top-100",fav=false},
			{name = "Singel Trending",url="http://www.mtv.de/charts/9gtiy5/single-trending",fav=false},
			{name = "Top100 Musik Streaming",url="http://www.mtv.de/charts/h4oi23/top100-music-streaming",fav=false},
			--{name = "Download Charts Singel",url="http://www.mtv.de/charts/pcbqpc/downloads-charts-single",fav=false},
			--{name = "Download Charts Album",url="http://www.mtv.de/charts/ha7dbg/downloads-charts-album",fav=false},
			{name = "Top15 deutsche Singel",url="http://www.mtv.de/charts/jlyhaa/top-15-deutschsprachige-single-charts",fav=false},
			--{name = "Top15 deutsche Alben",url="http://www.mtv.de/charts/wp4sh5/top-15-deutschsprachige-alben-charts",fav=false},
			--{name = "Hip Hop",url="http://www.mtv.de/charts/vh9pco/hip-hop-charts",fav=false},
			{name = "Dance Charts",url="http://www.mtv.de/charts/2ny5w9/dance-charts",fav=false},
	}

	--local url = "http://www.mtv.de/charts"
	local url = "http://www.mtv.de/charts/2ny5w9/dance-charts"
	glob.mtv_live_url = nil

	videosection = get_json_data(url)
	if videosection == nil then 
		return nil 
	end

	local json = require "json"
	local jnTab = json:decode(videosection)
	if jnTab == nil  then 
		return nil 
	end

	local videosection_url = jnTab.manifest.zones.t5_lc_promo1.feed
	videosection = getdata(videosection_url)
	if videosection == nil then 
		return nil 
	end

	jnTab = json:decode(videosection)
	local pages = 1
	videosection_url = videosection_url:match("(.*)%d+$")

	local add = true
	local add2016 = false
	if jnTab.result and jnTab.result.chartTypeList then
		for k, v in ipairs(jnTab.result.chartTypeList) do
			if v.label and v.link then
				if v.label:find("Offizielle") == nil and v.label ~= "Deine Lieblingsvideos bei MTV" and v.label:find("Album") == nil and v.label:find("Alben") == nil and v.label:find("Vinyl") == nil then
					table.insert(glob.mtv,{name=v.label, url=v.link,})
					if add2016 == false and v.label == "Top 100 Jahrescharts 2015" then
						add2016 = true
						table.insert(glob.mtv,{name="Top 100 Jahrescharts 2016", url="http://www.mtv.de/charts/yrk67s/top-100-jahrescharts-2016",})
					end
				end
			end
		end
	end

	local mtvconf = get_conf_mtvfavFile()
	local havefile = neutrino2.file_exists(mtvconf)

	if havefile == true then
		local favdata = read_file(mtvconf)
		if havefile ~= nil then
			for _name ,_url in favdata:gmatch('name%s-=%s-"(.-)"%s-,%s-url%s-=%s-"(.-)"') do
				table.insert(glob.mtv,{name=_name, url=_url,fav=true})
			end
		end
	end
end

function info(captxt, infotxt, sleep)
	if captxt == mtv_version and infotxt == nil then
		infotxt = captxt
		captxt = "Information"
	end
	
	local h = neutrino2.CHintBox(captxt, infotxt)
	h:exec(sleep)
end

function getdata(Url, outputfile)
	print("getdata:")

	if Url == nil then 
		return nil 
	end

	local data = nil

	data = neutrino2.getUrlAnswer(Url, "Mozilla/5.0;")

	return data
end

function exist_url(tab,_url)
	for i, v in ipairs(tab) do
		if v.url == _url then
			return true
		end
	end

	return false
end

function getliste(url)
	local h = neutrino2.CHintBox("Info", "Liste wird erstellt\n")
	h:paint()

	local videosection = get_json_data(url)

	if videosection == nil then 
		h:hide() 
		return nil 
	end

	local liste = {}
	local json = require "json"
	local urlTab = json:decode(videosection)
	local tc = {"t4_lc_promo1"}
	if url == "http://www.mtv.de/musik" then 
		tc  = {"t4_lc_promo1","t5_lc_promo1","t6_lc_promo1","t7_lc_promo1","t8_lc_promo1","t9_lc_promo1","t10_lc_promo1","t11_lc_promo1"}  		end

	for t in pairs(tc) do
		if urlTab.manifest.zones[tc[t]] and urlTab.manifest.zones[tc[t]].feed then
			local videosection_url = urlTab.manifest.zones[tc[t]].feed
			for p = 1,6,1 do
				if videosection_url then
					local  pc = "?"
					if videosection_url:find("?") then 
						pc = "&" 
					end
					videosection = getdata(videosection_url .. pc .. "pageNumber=" .. p)
					if videosection == nil then
						if #liste > 0 then 
							h:hide() 
							return liste 
						else 
							return nil 
						end
					end
				else
					h:hide()
					return nil
				end

				local jnTab = json:decode(videosection)
				if jnTab == nil or jnTab.result == nil or jnTab.result.data == nil then 					h:hide()  
					if #liste > 0 then 
						return liste 
					else 
						return nil 
					end 
				end

				for k, v in ipairs(jnTab.result.data.items) do
					if v.videoUrl or v.canonicalURL then
						local video_url = v.videoUrl or v.canonicalURL
						if exist_url(liste,video_url) == false then
							-- artist
							local artist = v.shortTitle or v.artist or ""
							if #artist == 0 and v.artists then 
								artist = v.artists[1].name 
							end

							-- logo
							local _logo = nil
							if v.images and v.images.url then 
								_logo = v.images.url 
							end
							
							tfile = "/tmp/mtv/" .. v.title ..".jpg"
							if _logo ~= nil and neutrino2.file_exists(tfile) ~= true then
								neutrino2.downloadUrl(_logo, tfile)
								_logo = tfile
							else
								_logo = neutrino2.DATADIR .. "/icons/nopreview.jpg"
							end
							
							-- chart pos
							local chpos = nil
							if v.chartPosition and v.chartPosition.current then 									chpos = v.chartPosition.current 
							end

							-- movie url
							local VideoUrl = nil
							VideoUrl = getvideourl(video_url, v.title, conf.hlsflag)

							-- our list
							table.insert(liste,{name=artist .. ": " .. v.title, url=video_url,
								logo =_logo,enabled=conf.dlflag, vid=id,chartpos=chpos,
vidurl = VideoUrl
								})
						end
					end
				end
			end
		end
	end
	h:hide()

	return liste
end

function get_m3u_url(m3u8_url)
	if m3u8_url == nil then 
		return nil 
	end

	local videoUrl = nil
	local res = 0
	local data = getdata(m3u8_url)

	if data then
		local host = m3u8_url:match('([%a]+[:]?//[_%w%-%.]+)/')
		if m3u8_url:find('/master.m3u8') then
			local lastpos = (m3u8_url:reverse()):find("/")
			local hosttmp = m3u8_url:sub(1,#m3u8_url-lastpos)
			if hosttmp then
				host = hosttmp .."/"
			end
		end
		local maxRes = 1280
		if conf.maxRes then
			local maxResStr = conf.maxRes:match("(%d+)x")
			maxRes = tonumber(maxResStr)
		end
		for band, res1, res2, url in data:gmatch('BANDWIDTH=(%d+).-RESOLUTION=(%d+)x(%d+).-\n(.-)\n') do
			if url and res1 then
				local nr = tonumber(res1)
				if nr <= maxRes and nr > res then
					res=nr
					if host and url:sub(1,4) ~= "http" then
						url = host .. url
					end
					videoUrl = url
				end
			end
		end
	end

	return videoUrl
end

function getvideourl(url, vidname, hls)
	print("getvideourl:")

	local	json = require "json"
	local data = getdata(url)
	local id = data:match('itemId":"(.-)"')
	local service_url = "http://media.mtvnservices.com/pmt/e1/access/index.html?uri=mgid:arc:episode:mtv.de:" .. id .. "&configtype=edge&ref=" .. url
	data = getdata(service_url)
	local jnTab = json:decode(data)
	if jnTab.feed.items[1].group.content then
		local jsUrl = jnTab.feed.items[1].group.content
		if hls then
			jsUrl = jsUrl:gsub("(&device=.-)&","")
			jsUrl = jsUrl .. "&acceptMethods=hls"
		end
		data = getdata(jsUrl .. "&format=json")
		if data then
			jnTab = json:decode(data)
		end
	end
	data = nil

	local max_w = 0
	local video_url = nil
	if jnTab and jnTab.package and jnTab.package.video and jnTab.package.video.item and jnTab.package.video.item[1].rendition then
		for k,v in pairs(jnTab.package.video.item[1].rendition) do
			if (hls or v.width or v.rdminwidth) and v.src then
				local w = tonumber(v.width or v.rdminwidth)
				if hls or w > max_w then
					video_url = v.src
					max_w = w
				end
			end
		end
	end
	local x = nil
	if video_url then
		if video_url:find("m3u8") then
			video_url = get_m3u_url(video_url)
		end
		x = video_url:find("rtmp") or video_url:find("m3u8")
	end
	if not x then
	  print("########## Error ##########")
	  print(url,video_url,clip_page)
	  print("###########################")
	end
	if video_url and video_url:find("copyright_error") then
		if h then
			h:hide()
		end
		info("Video Not Available", "Copyright Error\n" .. vidname,2)
	end

	return video_url
end

function funArt(id)
	if id  then
		if glob.MTVliste[id].name == nil then
			glob.MTVliste[id].name = "NoName_" .. id
		end

		--[[
		if glob.MTVliste[id].vidurl ~= nil then
			vodeoPlay:addToPlaylist(glob.MTVliste[id].vidurl , glob.MTVliste[id].name, "", "", glob.MTVliste[id].logo)
			vodeoPlay:exec(null, "")
		end
		]]

		-- show funArt
		local fb = neutrino2.CSwigHelpers()

		fb:loadBackgroundPic(glob.MTVliste[id].logo)

		-- show title
		
		--show frame play
		local framePlay = neutrino2.CFrame("play")
	end
end

function playMovie(id)
	if id  then
		if glob.MTVliste[id].name == nil then
			glob.MTVliste[id].name = "NoName_" .. id
		end

		if glob.MTVliste[id].vidurl ~= nil then
			vodeoPlay:addToPlaylist(glob.MTVliste[id].vidurl , glob.MTVliste[id].name, "", "", glob.MTVliste[id].logo)
			vodeoPlay:exec(null, "")
		end
	end
end

function make_shuffle_list(tab)
	local randTable={}
	for k=1 , #tab do
		randTable[k]=k
	end
	local shuffleTable = {}
	for k=1,#randTable do
		math.randomseed(os.time() *100000000000)
		local r=table.remove(randTable,math.random(#randTable))
		shuffleTable[k]=tab[r]
	end
	return shuffleTable
end

function playlist(filename)
	local tab = {}
	if conf.shuffleflag == true then
		tab = make_shuffle_list(glob.MTVliste)
	else
		tab = glob.MTVliste
	end

	local i = 1

	repeat
		if tab[i].name == nil then
			tab[i].name = "NoName"
		end

		local url = tab[i].vidurl

		if url then
			local videoformat = url:sub(-4)
			if videoformat ~= ".flv" or conf.playflvflag then
				vodeoPlay:addToPlaylist(tab[i].vidurl, "(" .. i.. "/" .. #tab .. ") " .. tab[i].name, "", "", tab[i].logo)
			end
		end

		i = i + 1
	until i == 0 or i == #tab+1

	vodeoPlay:exec(null ,"")
end

function dlstart(name)
	local infotext = "Dateien werden für Download vorbereitet.  "
	name = name:gsub([[%s+]], "_")
	name = name:gsub("[:'&()/]", "_")
	local dlname = "/tmp/" .. name ..".dl"
	local havefile = neutrino2.file_exists("/tmp/.rtmpdl")

	if havefile == true then
		info("Info", "Ein anderer Download ist bereits aktiv.", 4)
		return
	end

	local dl = io.open(dlname, "w")
	local script_start = false

	local pw = neutrino2.CProgressWindow()

	pw:setTitle(infotext)
	pw:paint()
	pw:showStatusMessageUTF("Start")

	for i, v in ipairs(glob.MTVliste) do
		if v.enabled == true then
			if glob.MTVliste[i].name == nil then
				glob.MTVliste[i].name = "NoName_" .. i
			end

			local url = getvideourl(glob.MTVliste[i].url, glob.MTVliste[i].name, conf.hlsflag)

			if url then
				local fname = v.name:gsub([[%s+]], "_")
				fname = fname:gsub("[:'()]", "_")
				fname = fname:gsub("/", "-")

				pw:showGlobalStatus(i)

				local videoformat = url:sub(-4)
				if videoformat == nil then
					videoformat = ".mp4"
				end
				if conf.hlsflag then
					dl:write("ffmpeg -y -nostdin -loglevel 30 -i " .. url .. " -c copy  " .. conf.path .. "/" .. fname   .. ".ts\n")
					script_start = true
				elseif videoformat ~= ".flv" or conf.flvflag then
					dl:write("rtmpdump -e -r " .. url .. " -o " .. conf.path .. "/" .. fname  .. videoformat .."\n")
					script_start = true
				end
			end
		end
	end

	pw:hide()

	if script_start == true then
		dl:close()
		local scriptname  = "/tmp/" .. name ..".sh"
		local script=io.open(scriptname,"w")
		script:write(
		[[#!/bin/sh
		while read -r i
		do
			$i
		done < ]]
		)
		script:write("'" .. dlname .. "'\n")
		script:write([[
		wget -q 'http://127.0.0.1/control/message?popup=Video Liste ]])
			script:write(name .. " wurde heruntergeladen.' -O /dev/null\n")
			script:write("rm '" .. dlname .. "'\n")
			script:write("rm '" .. scriptname .. "'\n")
			script:write("rm /tmp/.rtmpdl\n")

			script:close()
			os.execute("echo >/tmp/.rtmpdl")
			os.execute("sleep 2")
			os.execute("chmod 755 '" .. scriptname .. "'")
			os.execute("sh '"..scriptname.."' &")
	else
		local er = neutrino2.CHintBox("Info", name .." - \nDownload ist fehlerhaft \noder Video in FLV-Format")
		er:paint()
		os.remove(dlname)
		print("ERROR")
		os.execute("sleep 2")
		er:hide()
	end
end

function exist(_url)
	for i, v in ipairs(glob.mtv) do
		if v.fav == true and v.url == _url then
			return true
		end
	end
	return false
end

function addfav(id)
	local addinfo = false
	for i, v in ipairs(glob.mtv_artist) do
		if v.enabled and exist(v.url) == false then
			table.insert(glob.mtv,{name=v.name, url=v.url,fav=true})
			glob.mtv_artist[i].disabled=true
			glob.fav_changed = true
			addinfo = true
		end
	end
	if addinfo == true then
		info("Info","Zu Favoriten hinzugefügt",2)
	end
end

function favdel(id)
	local delinfo = false
	for i, v in ipairs(glob.mtv) do
		if v.fav and v.enabled then
			table.remove(glob.mtv,i)
			glob.fav_changed = true
			delinfo = true
		end
	end
	if delinfo == true then
		info("Info","Ausgewählten Favoriten gelöscht",2)
	end

end

function chooser_menu(id)
--[[
	if id:sub(1,29) =="Erstelle Download Liste für " then
		local forwarder_action = "dlstart"
		local forwarder_name = "Download starten"
		local chooser_action = "set_bool_in_liste"
		local hintname = "Speichert die ausgewählten Videos unter: " .. conf.path
		local _id = id:sub(30,#id)
		local name = id
		local value=conf.dlflag
		gen_chooser_menu(glob.MTVliste, name, _id, chooser_action, forwarder_action, forwarder_name, hintname, value, glob.menu_liste)
	elseif id:sub(1,15) =="Neue Favoriten" then
		local forwarder_action = "addfav"
		local forwarder_name = "Zu Favoriten hinzufügen"
		local chooser_action = "set_bool_in_searchliste"
		local hintname = "Speichert die ausgewählten Videos unter: " .. conf.path
		local name = id .. " hinzufügen"
		local value=false
		gen_chooser_menu(glob.mtv_artist, name , id, chooser_action, forwarder_action, forwarder_name, hintname, value, glob.search_artists_menu)
	elseif id =="favdel" then
		local forwarder_action = "favdel"
		local forwarder_name = "Favoriten löschen"
		local chooser_action = "set_bool_in_mtv"
		local hintname = "Lösche die ausgewählten Videos."
		local name = id .. " hinzufügen"
		local value=false
		gen_chooser_menu(glob.mtv, name , id, chooser_action, forwarder_action, forwarder_name, hintname, value, glob.settings_menu)

	end
]]
	local menu = neutrino2.CMenuWidget(name, neutrino2.PLUGINDIR .. "/mtv/mtv_hint.png")

	--if id:sub(1,29) =="Erstelle Download Liste für " then
		menu:addItem(neutrino2.ClistBoxItem("Download starten", true, "", null, "dlstart"))
	--[[elseif id:sub(1,15) =="Neue Favoriten" then
		menu:addItem(neutrino2.ClistBoxItem("Zu Favoriten hinzufügen", true, "", null, "set_bool_in_searchliste"))
	elseif id =="favdel" then
		menu:addItem(neutrino2.ClistBoxItem("Favoriten löschen", true, "", null, "set_bool_in_mtv"))
	end]]

	menu:exec(null, "")

	local selected = menu:getSelected()
	local actionKey = menu:getActionKey()

	if selected >= 0 then
		if actionKey == "dlstart" then
			dlstart(glob.MTVliste[selected + 1].name)
		end
	end

	if menu:getExitPressed() ~= true then
		settings()
	end
end

function mtv_liste(id)
	print("mtv_liste:")

	neutrino2.CFileHelpers():removeDir("/tmp/mtv")
	neutrino2.CFileHelpers():createDir("/tmp/mtv")

	glob.MTVliste = nil;
	local url = glob.mtv[id].url --liste url
	glob.MTVliste = getliste(url)

	local menu = neutrino2.CMenuWidget(glob.mtv[id].name, neutrino2.PLUGINDIR .. "/mtv/mtv_hint.png")
	menu:setWidgetType(neutrino2.TYPE_FRAME)
	menu:setItemsPerPage(3, 2)
	menu:enablePaintDate()

	--btn = neutrino2.button_label_struct()

	--btn.button = neutrino2.NEUTRINO_ICON_BUTTON_RED
	--btn.localename = "spiele die ganze Liste"

	--menu:setFootButtons(btn)
	menu:addKey(neutrino2.RC_red, null, "playlist")

	--btn2 = neutrino2.button_label_struct()

	--btn2.button = neutrino2.NEUTRINO_ICON_BUTTON_REC
	--btn2.localename = ""

	--menu:setHeadButtons(btn2)
	menu:addKey(neutrino2.RC_record, null, "record")

	local item = nil
	if glob.MTVliste ~= nil then
		for i, v in ipairs(glob.MTVliste) do
			item = neutrino2.ClistBoxItem(v.name, true, "", null, "play")
			item:setItemIcon(v.logo)

			menu:addItem(item)
		end
	end

	repeat
		menu:exec(null, "")

		selected = menu:getSelected()
		actionKey = menu:getActionKey()

		if selected < 0 then
			selected = 0
		end

		menu:setSelected(selected)

		if selected >= 0 then
			if actionKey == "playlist" then
				playlist(glob.MTVliste[selected + 1])
			elseif actionKey == "play" then
				playMovie(selected + 1)
			elseif actionKey == "record" then
				dlstart(glob.MTVliste[selected + 1].name)
			end
		end
	until menu:getExitPressed() == true
end

local m_selected = -1
function settings()
	loadConfig()

	local menu = neutrino2.CMenuWidget("Einstellungen", neutrino2.NEUTRINO_ICON_SETTINGS)
	local item = nil
	menu:setWidgetMode(neutrino2.MODE_SETUP)
	menu:enableShrinkMenu()

	menu:addItem(neutrino2.ClistBoxItem("zurück"))
	menu:addItem(neutrino2.CMenuSeparator(neutrino2.LINE))
	item = neutrino2.ClistBoxItem("Einstellungen speichern")
	item:setIconName(neutrino2.NEUTRINO_ICON_BUTTON_RED)
	item:setDirectKey(neutrino2.RC_red)
	menu:addItem(item)
	menu:addItem(neutrino2.CMenuSeparator(neutrino2.LINE))
	menu:addItem(neutrino2.ClistBoxItem("Verzeichniss:", true, conf.path))
	menu:addItem(neutrino2.ClistBoxItem("Verzeichniss M3U:", true, conf.path_m3u))
	
	hls = ""
	if conf.hlsflag == true then
		hls = "on"
	else
		hls = "off"
	end
	menu:addItem(neutrino2.ClistBoxItem("Videos in HLS-Format", true, hls))

	menu:addItem(neutrino2.ClistBoxItem("Max. Auflösung", true, conf.maxRes))

	dl = ""
	if conf.dlflag == true then
		dl = "on"
	else
		dl = "off"
	end
	menu:addItem(neutrino2.ClistBoxItem("Auswahl vorbelegen mit", true, dl))

	flv = ""
	if conf.flvflag == true then
		flv = "on"
	else
		flv = "off"
	end
	menu:addItem(neutrino2.ClistBoxItem("Videos in FLV-Format herunterladen ?", true, flv))

	playflv = ""
	if conf.playflvflag == true then
		playflv = "on"
	else
		playflv = "off"
	end
	menu:addItem(neutrino2.ClistBoxItem("Videos in FLV-Format abspielen ?", true, playflv))

	shuffle = ""
	if conf.shuffleflag == true then
		shuffle = "on"
	else
		shuffle = "off"
	end
	menu:addItem(neutrino2.ClistBoxItem("Zufällig abspielen ? ", true, shuffle))

	menu:addItem(neutrino2.ClistBoxItem("Ausgewählte Favoriten löschen.", true, "", null, "del_fav"))

	if m_selected < 0 then
		m_selected = 0
	end

	menu:setSelected(m_selected)

	menu:exec(null, "")

	m_selected = menu:getSelected()

	if m_selected >= 0 then
		if m_selected == 0 then
			return
		elseif m_selected == 2 then
			neutrino2.HintBox("MTV", "Einstellungen werden gespeichert bitte warten...")
			saveConfig()
		elseif m_selected == 4 then
			local fileBrowser = neutrino2.CFileBrowser()
			fileBrowser.Dir_Mode = true

			if fileBrowser:exec(conf.path) == true then
				conf.path = fileBrowser:getSelectedFile().Name

				saveConfig()
			end 
		elseif m_selected == 5 then
			local fileBrowser = neutrino2.CFileBrowser()
			fileBrowser.Dir_Mode = true

			if fileBrowser:exec(conf.path_m3u) == true then
				conf.path_m3u = fileBrowser:getSelectedFile().Name

				saveConfig()
			end
		elseif m_selected == 6 then 
			if conf.hlsflag == true then
				conf.hlsflag = false
			else
				conf.hlsflag = true
			end

			saveConfig()
		elseif m_selected == 7 then
			if conf.maxRes == '3840x2160' then
				conf.maxRes = '2560x1440'
			elseif conf.maxRes == '2560x1440' then
				conf.maxRes = '1920x1080'
			elseif conf.maxRes == '1920x1080' then
				conf.maxRes = '1280x720'
			elseif conf.maxRes == '1280x720' then
				conf.maxRes = '854x480'
			elseif conf.maxRes == '854x480' then
				conf.maxRes = '640x360'
			elseif conf.maxRes == '640x360' then
				conf.maxRes = '3840x2160'
			end

			saveConfig()
		elseif m_selected == 8 then 
			if conf.dlflag == true then
				conf.dlflag = false
			else
				conf.dlflag = true
			end

			saveConfig()
		elseif m_selected == 9 then 
			if conf.flvflag == true then
				conf.flvflag = false
			else
				conf.flvflag = true
			end

			saveConfig()
		elseif m_selected == 10 then 
			if conf.playflvflag == true then
				conf.playflvflag = false
			else
				conf.playflvflag = true
			end

			saveConfig()
		elseif m_selected == 11 then 
			if conf.shuffleflag == true then
				conf.shuffleflag = false
			else
				conf.shuffleflag = true
			end

			saveConfig()
		end
	end

	if menu:getExitPressed() ~= true then
		settings()
	end
end

function gen_search_list(search)
	local url = "http://www.mtv.de/kuenstler/" .. search:sub(1,1) .. "/1"
	glob.mtv_artist = {}
	local videosection = get_json_data(url)
	if videosection == nil then return nil end

	data = nil
	collectgarbage()

	if videosection == nil then return nil end

	local json = require "json"
	local jnTab = json:decode(videosection)
	if jnTab == nil  then return nil end
	local videosection_url = jnTab.manifest.zones.t5_lc_promo1.feed
	videosection = getdata(videosection_url)
	if videosection == nil then return nil end

	jnTab = json:decode(videosection)
	local pages = 1
	videosection_url = videosection_url:match("(.*)%d+$")
	if jnTab.result.pages then pages = tonumber(jnTab.result.pages) end

	for p=1,pages,1 do
		if p > 1 then
			if videosection_url then
				videosection = getdata(videosection_url .. p)
				if videosection == nil then
					return neutrino2.RETURN_EXIT
				end
			else
				return neutrino2.RETURN_EXIT
			end
			jnTab = json:decode(videosection)
		end

		if jnTab == nil or jnTab.result == nil or jnTab.result.artists == nil then 
			return neutrino2.RETURN_EXIT 
		end

		local add = true
		local use_seek = #search > 1
		for k, v in ipairs(jnTab.result.artists) do
			if v.name and v.canonicalURL then
				if use_seek then
					local name = v.name:lower()
					local a,b = name:find(search:lower())
					if a == 1 and b then add = true else add = false end
				end
				if add then
					table.insert(glob.mtv_artist,{name=v.name, url=v.canonicalURL, enabled=false,disabled=false})
				end
			end
		end
	end
end

function searchliste(id)
	glob.MTVliste = nil;

	if glob.mtv_artist[id].disabled == true then 
		return 
	end

	local url = glob.mtv_artist[id].url
	glob.MTVliste = getliste(url)

	local menu = neutrino2.CMenuWidget(glob.mtv_artist[id].name, neutrino2.PLUGINDIR .. "/mtv/mtv_hint.png")
	menu:setWidgetType(neutrino2.TYPE_FRAME)
	menu:enablePaintDate()

	--btn = neutrino2.button_label_struct()

	--btn.button = neutrino2.NEUTRINO_ICON_BUTTON_RED
	--btn.localename = "spiele die ganze Liste"

	--menu:setFootButtons(btn)
	menu:addKey(neutrino2.RC_red, null, "playlist")

	--btn2 = neutrino2.button_label_struct()

	--btn2.button = neutrino2.NEUTRINO_ICON_BUTTON_REC
	--btn2.localename = "test"

	--menu:setHeadButtons(btn2)
	menu:addKey(neutrino2.RC_record, null, "record")

	local item = nil

	for i, v in ipairs(glob.MTVliste) do
		item = neutrino2.ClistBoxItem(v.name)
		item:setActionKey(null, "playMovie")
		item:setItemIcon(v.logo)

		menu:addItem(item)
	end

	repeat
		menu:exec(null, "")

		local selected = menu:getSelected()
		local actionKey = menu:getActionKey()

		if selected >= 0 then
			if actionKey == "playMovie" then
				playMovie(selected + 1)
			elseif actionKey == "playlist" then
				playlist(glob.MTVliste[selected + 1])
			elseif actionKey == "record" then
				dlstart(glob.MTVliste[selected + 1].name)
			end
		end

	until menu:getExitPressed() == true
end

function search_artists()
	if conf.search == nil then 
		return 
	end

	local h = neutrino2.CHintBox("Info", "Suche: " .. conf.search)
	h:paint()

	if #conf.search > 0 then
		gen_search_list(conf.search)
	end

	h:hide()

	if glob.mtv_artist == nil or #glob.mtv_artist == 0 then
		info("Info", "Liste ist leer.", 1)
		return
	end

	local menu = neutrino2.CMenuWidget(conf.search, neutrino2.PLUGINDIR .. "/mtv/mtv_hint.png")

	if glob.mtv_artist then
		for i, v in ipairs(glob.mtv_artist) do
			menu:addItem(neutrino2.ClistBoxItem(i .. ": " .. v.name, true, "", null, "search"))
		end
	end

	menu:exec(null, "")

	local selected = menu:getSelected()
	local actionKey = menu:getActionKey()

	if selected >= 0 then
		--[[
		if selected == 0 then
			chooser_menu(selected + 1)
		elseif selected > 0 then
			searchliste(selected + 1)
		end
		]]
		if actionKey == "search" then
			searchliste(selected + 1)
		end
	end

	if menu:getExitPressed() ~= true then
		search_artists()
	end
end

function play_live()
	local video_url = getvideourl(glob.mtv_live_url, "live", true)

	if video_url then
		vodeoPlay:addToPlaylist(video_url, "MTV Live Stream")
		vodeoPlay:exec(null, "")
	end
end

local selected_mtl = 0
function mtv_listen_menu()
	print("mtv_listen_menu:")

	if glob.mtv == nil then
		return
	end

	menu = neutrino2.CMenuWidget("MTV Listen", neutrino2.PLUGINDIR .. "/mtv/mtv_hint.png")

	menu:enableShrinkMenu()
	menu:enablePaintDate()

	if selected_mtl < 0 then
		selected_mtl = 0
	end

	menu:setSelected(selected_mtl)

	local item = nil
	for i, v in ipairs(glob.mtv) do
		item = neutrino2.ClistBoxItem(v.name)
		item:setHint(v.url)
		item:setActionKey(null, "mtv_liste")

		menu:addItem(item)
	end

	menu:exec(null, "")

	selected_mtl = menu:getSelected()
	actionKey = menu:getActionKey()

	if actionKey == "mtv_liste" then
		mtv_liste(selected_mtl + 1)
	end
	
	if menu:getExitPressed() ~= true then
		mtv_listen_menu()
	end
end

local selected_mm = 0
function main_menu()
	print("mainMenu:")

	local menu = neutrino2.CMenuWidget("MTV", neutrino2.PLUGINDIR .. "/mtv/mtv_hint.png")
	menu:setWidgetMode(neutrino2.MODE_MENU)
	menu:enableShrinkMenu()
	menu:enablePaintDate()

	if selected_mm < 0 then
		selected_mm = 0
	end

	menu:setSelected(selected_mm)

	menu:addItem(neutrino2.ClistBoxItem("MTV Listen", true, "", null, "list"))
	menu:addItem(neutrino2.ClistBoxItem("Suche nach künstler", true, conf.search, null, "search"))
	menu:addItem(neutrino2.ClistBoxItem("MTV Live", glob.mtv_live_url ~= nil, "", null, "live"))
	menu:addItem(neutrino2.ClistBoxItem("Einstellugen", true, "", null, "settings"))

	menu:exec(null, "")

	actionKey = menu:getActionKey()

	if actionKey == "list" then
		mtv_listen_menu()
	elseif actionKey == "search" then
		search_artists()
	elseif actionKey == "live" then
		play_live()
	elseif actionKey == "settings" then
		settings()
	end

	if menu:getExitPressed() ~= true then
		main_menu()
	end
end

function main()
	init()
	loadConfig()
	glob.have_rtmpdump = nil
	if conf.hlsflag then
		glob.have_rtmpdump = which("ffmpeg")
	else
		glob.have_rtmpdump = which("rtmpdump")
	end
	main_menu()
	saveConfig()
	neutrino2.CFileHelpers():removeDir("/tmp/mtv")
	collectgarbage()
	os.execute("rm /tmp/lua*");
end

main()


