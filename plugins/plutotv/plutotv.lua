--[[
	plutotv-vod.lua v1.11

	Copyright (C) 2021 TangoCash
	License: WTFPLv2
]]

plugin_title = "Pluto TV VOD"

json = require "json"

catlist = {}

itemlist = {}
itemlist_details = {}

episodelist = {}
episodelist_details = {}

playback_details = {}

function convert(s)
	s=s:gsub("&","&amp;")
	s=s:gsub("'","&apos;")
	s=s:gsub('"',"&quot;")
	s=s:gsub("<","&lt;")
	s=s:gsub(">","&gt;")
	s=s:gsub("\x0d"," ")
	s=s:gsub("\x0a"," ")
	return s
end


function get_channels()
	httpTool = neutrino2.CHTTPTool()
	httpTool:setTitle("Pluto TV Channels downlaoding please wait ...")
		
	local obj_file = "https://i.mjh.nz/PlutoTV/de.m3u8"
	local target = neutrino2.CONFIGDIR .. "/webtv/plutotv.m3u8"
		
	if httpTool:downloadFile(obj_file, target, 100) == true then
		--os.execute("pzapit -c")
		neutrino2.CZapit_getInstance():reinitChannels()
	end
end

-- UTF8 in Umlaute wandeln
function conv_utf8(_string)
	if _string ~= nil then
		_string = string.gsub(_string,"\\u0026","&");
		_string = string.gsub(_string,"\\u00a0"," ");
		_string = string.gsub(_string,"\\u00b4","´");
		_string = string.gsub(_string,"\\u00c4","Ä");
		_string = string.gsub(_string,"\\u00d6","Ö");
		_string = string.gsub(_string,"\\u00dc","Ü");
		_string = string.gsub(_string,"\\u00df","ß");
		_string = string.gsub(_string,"\\u00e1","á");
		_string = string.gsub(_string,"\\u00e4","ä");
		_string = string.gsub(_string,"\\u00e8","è");
		_string = string.gsub(_string,"\\u00e9","é");
		_string = string.gsub(_string,"\\u00f4","ô");
		_string = string.gsub(_string,"\\u00f6","ö");
		_string = string.gsub(_string,"\\u00fb","û");
		_string = string.gsub(_string,"\\u00fc","ü");
		_string = string.gsub(_string,"\\u2013","–");
		_string = string.gsub(_string,"\\u201c","“");
		_string = string.gsub(_string,"\\u201e","„");
		_string = string.gsub(_string,"\\u2026","…");
		_string = string.gsub(_string,"&#038;","&");
		_string = string.gsub(_string,"&#8211;","–");
		_string = string.gsub(_string,"&#8212;","—");
		_string = string.gsub(_string,"&#8216;","‘");
		_string = string.gsub(_string,"&#8217;","’");
		_string = string.gsub(_string,"&#8230;","…");
		_string = string.gsub(_string,"&#8243;","″");
		_string = string.gsub(_string,"<[^>]*>","");
		_string = string.gsub(_string,"\\/","/");
		_string = string.gsub(_string,"\\n","");
	end
	return _string
end

function gen_ids() -- Generation of a random sid 
	local a = string.format("%x", math.random(1000000000,9999999999)) 
	local b = string.format("%x", math.random(1000,9999)) 
	local c = string.format("%x", math.random(1000,9999)) 
	local d = string.format("%x", math.random(10000000000000,99999999999999))
	local id = tostring(a) .. '-' .. tostring(b) .. '-' .. tostring(c) .. '-' .. tostring(d)
	return id
end

function getVideoData(url) -- Generate stream address and evaluate it according to the best resolution
	re_url = ""
	
	http = "http://service-stitcher-ipv4.clusters.pluto.tv/stitch/hls/episode/"
	token = "?advertisingId=&appName=web&appVersion=unknown&appStoreUrl=&architecture=&buildVersion=&clientTime=0&deviceDNT=0&deviceId=" .. gen_ids() .. "&deviceMake=Chrome&deviceModel=web&deviceType=web&deviceVersion=unknown&includeExtendedEvents=false&sid=" .. gen_ids() .. "&userId=&serverSideAds=true"

	local data = neutrino2.getUrlAnswer(http .. url .."/master.m3u8" ..token, "Mozilla/5.0") -- Calling the generated master.m3u8
	local count = 0
	
	if data then
		local res = 0
		for band, url2 in data:gmatch(',BANDWIDTH=(%d+).-\n(%d+.-m3u8)') do
			if band and url2 then
				local nr = tonumber(band)
				if nr > res then
					res=nr
					re_url = http .. url .. "/" .. url2 .. token 
				end
			end
		end
	end
	
	return re_url
end

function get_cat()
	local hint = neutrino2.CHintBox(plugin_title, "loading...", neutrino2.HINTBOX_WIDTH, neutrino2.PLUGINDIR .. "/plutotv/plutotv.png")
	hint:paint()
	
	local r = false
	local c_data = neutrino2.getUrlAnswer("http://api.pluto.tv/v3/vod/categories?includeItems=true&deviceType=web", "Mozilla/5.0")
	
	if c_data then
		local jd = json:decode(c_data)
		if jd then
			for i = 1, jd.totalCategories do
				if jd.categories[i] then
					table.insert(catlist, i, jd.categories[i].name)
					itemlist_details = {}
					for k = 1, #jd.categories[i].items do
						local _duration = 0
						if jd.categories[i].items[k].duration then
							_duration = tonumber(jd.categories[i].items[k].duration) / 1000 / 60
						end
						
						itemlist_details[k] =
						{
							cat  = i;
							item = k;
							name = jd.categories[i].items[k].name;
							desc = jd.categories[i].items[k].description;
							uuid = jd.categories[i].items[k]._id;
							type = jd.categories[i].items[k].type;
							duration = _duration;
							cover = jd.categories[i].items[k].covers[1].url;
							rating = jd.categories[i].items[k].rating;
							genre = jd.categories[i].items[k].genre;
						}
					end
					table.insert(itemlist, i , itemlist_details)
				end
			end
		end
		hint:hide()
		
		categories_menu()
	else
		local mBox = neutrino2.CMessageBox("Fehler", "Keinen Kategorien gefunden!")
		mBox:exec()
	end
end

cm_selected = 0
function cat_menu(_id)
	neutrino2.CFileHelpers():createDir("/tmp/plutotv")

	local cm = neutrino2.CMenuWidget(catlist[tonumber(_id)], neutrino2.PLUGINDIR .. "/plutotv/plutotv.png")
	cm:setWidgetType(neutrino2.TYPE_FRAME)
	cm:enablePaintItemInfo(70)
	cm:setItemsPerPage(6, 2)
	cm:enablePaintDate()
	cm:clearAll()
	
	local red = neutrino2.button_label_struct()
	red.button = neutrino2.NEUTRINO_ICON_BUTTON_RED
	red.localename = "Download"

	local green = neutrino2.button_label_struct()
	green.button = neutrino2.NEUTRINO_ICON_BUTTON_GREEN
	green.localename = "Info"
	
	cm:setFootButtons(red)
	cm:setFootButtons(green)
	
	cm:addKey(neutrino2.RC_red, null, "rec")
	cm:addKey(neutrino2.RC_green, null, "info")
	
	local hint = neutrino2.CHintBox(plugin_title, "loading...", neutrino2.HINTBOX_WIDTH, neutrino2.PLUGINDIR .. "/plutotv/plutotv.png")
	hint:paint()
	
	for cat, itemlist_detail in pairs (itemlist) do
		if cat == tonumber(_id) then
			local count = 1
			for item, item_detail in pairs(itemlist_detail) do	
				tfile = neutrino2.DATADIR .. "/icons/nopreview.jpg"
					
				if item_detail.cover ~= nil then
					tfile = "/tmp/plutotv/" .. conv_utf8(item_detail.name) .. ".jpg"
					
					if neutrino2.file_exists(tfile) ~= true then
						neutrino2.downloadUrl(conv_utf8(item_detail.cover) .. "?h=640&w=480", tfile, "Mozilla/5.0")
					end
				end
					
				playback_details[item] = 
				{
					uuid = item_detail.uuid;
					name = item_detail.name;
					desc = item_detail.desc;
					--cover = item_detail.cover;
					cover = tfile;
					type = item_detail.type;
					duration = item_detail.duration;
					rating = item_detail.rating;
					genre = item_detail.genre;
				}
					
				item = neutrino2.ClistBoxItem(conv_utf8(item_detail.name))
				item:setHintIcon(tfile)
				item:setHint(item_detail.desc)
				
				if item_detail.type == "movie" then
					item:setActionKey(null, "info");
				else
					item:setActionKey(null, "season_menu");
				end
					
				cm:addItem(item)
			end
		end
	end
	
	hint:hide()
	
	if cm_selected < 0 then
		cm_selected = 0
	end
	
	cm:setSelected(cm_selected)
	
	cm:exec(null, "")
	
	cm_selected = cm:getSelected()
	
	local actionKey = cm:getActionKey()
	
	if actionKey == "info" then
		playStream(cm_selected + 1)
	end
	if actionKey == "season_menu" then
		season_menu(playback_details[cm_selected + 1].uuid)
	end
	if actionKey == "rec" then
		recStream(cm_selected + 1)
	end
	
	if cm:getExitPressed() ~= true then
		cat_menu(_id)
	end
	
	neutrino2.CFileHelpers():removeDir("/tmp/plutotv")
end

sm_selected = 0
function season_menu(_id)
	local h = neutrino2.CHintBox(plugin_title, "Suche Episoden ...")
	h:paint()
	local seasons = 1
	local c_data = neutrino2.getUrlAnswer("http://api.pluto.tv/v3/vod/series/".. _id .."/seasons?includeItems=true&deviceType=web", "Mozilla/5.0")
	
	if c_data then
		local jd = json:decode(c_data)
		if jd then
			sm = neutrino2.CMenuWidget(jd.name, neutrino2.PLUGINDIR .. "/plutotv/plutotv.png")
			sm:clearAll()
			
			episodelist = {}
			local count = 1
			for i=1, #jd.seasons do
				item = neutrino2.ClistBoxItem("Season "..i)
				item:setActionKey(null, "episode_menu")
				
				sm:addItem(item)
				
				seasons = i
				episodelist_details = {}
				for k = 1, #jd.seasons[i].episodes do
					episodelist_details[k] =
					{
						title = jd.name;
						season = i;
						episode = k;
						name = i.."x"..string.format("%02d",k).." - ".. jd.seasons[i].episodes[k].name;
						desc = jd.seasons[i].episodes[k].description;
						duration = tonumber(jd.seasons[i].episodes[k].duration) / 1000 / 60;
						uuid = jd.seasons[i].episodes[k]._id;
						cover = jd.seasons[i].episodes[k].covers[1].url;
						type = jd.seasons[i].episodes[k].type;
						rating = jd.seasons[i].episodes[k].rating;
						genre = jd.seasons[i].episodes[k].genre;
					}
				end
				table.insert(episodelist, i, episodelist_details)
			end
		end
	end
	
	if sm_selected < 0 then
		sm_selected = 0
	end
	
	sm:setSelected(sm_selected)
	
	sm:exec(null, "")
	
	sm_selected = sm:getSelected()
	local actionKey = sm:getActionKey()
	
	if actionKey == "episode_menu" then
		episode_menu(sm_selected + 1)
	end
	
	if sm:getExitPressed() ~= true then
		season_menu(_id)
	end
	
	h:hide()
end

em_selected = 0
function episode_menu(s)
	neutrino2.CFileHelpers():createDir("/tmp/plutotv")
	
	local em = neutrino2.CMenuWidget(episodelist[tonumber(s)][1].title .. " - Season "..s, neutrino2.PLUGINDIR .. "/plutotv/plutotv.png")
	em:setWidgetType(neutrino2.TYPE_FRAME)
	em:enablePaintItemInfo(70)
	em:setItemsPerPage(6, 2)
	em:enablePaintDate()
	em:clearAll()
	
	local red = neutrino2.button_label_struct()
	red.button = neutrino2.NEUTRINO_ICON_BUTTON_RED
	red.localename = "Download"

	local green = neutrino2.button_label_struct()
	green.button = neutrino2.NEUTRINO_ICON_BUTTON_GREEN
	green.localename = "Info"
	
	em:setFootButtons(red)
	em:setFootButtons(green)
	
	em:addKey(neutrino2.RC_red, null, "rec")
	em:addKey(neutrino2.RC_green, null, "info")
	
	local hint = neutrino2.CHintBox(plugin_title, "loading...", neutrino2.HINTBOX_WIDTH, neutrino2.PLUGINDIR .. "/plutotv/plutotv.png")
	hint:paint()
	
	for season, episodelist_detail in pairs (episodelist) do
		if season == tonumber(s) then
			local count = 1
			for episode, episode_detail in pairs(episodelist_detail) do
				local tfile = neutrino2.DATADIR .. "/icons/nopreview.jpg"
					
				if episode_detail.cover ~= nil then
					tfile = "/tmp/plutotv/" .. conv_utf8(episode_detail.name) .. ".jpg"
					
					if neutrino2.file_exists(tfile) ~= true then
						neutrino2.downloadUrl(conv_utf8(episode_detail.cover) .. "?h=640&w=480", tfile, "Mozilla/5.0")
					end
				end
				
				playback_details[episode] = 
				{
					uuid = episode_detail.uuid;
					name = episode_detail.name;
					desc = episode_detail.desc;
					--cover = episode_detail.cover;
					cover = tfile;
					type =  episode_detail.type;
					duration = episode_detail.duration;
					rating = episode_detail.rating;
					genre = episode_detail.genre;
					title = episodelist[tonumber(s)][1].title;
					eptitle = episode_detail.name;
				}
				
				item = neutrino2.ClistBoxItem(episode_detail.name)
				item:setHintIcon(tfile)
				item:setHint(episode_detail.desc)
				item:setActionKey(null, "info")
				
				em:addItem(item)
			end
		end
	end
	
	hint:hide()
	
	if em_selected < 0 then
		em_selected = 0
	end
	
	em:setSelected(em_selected)
	
	em:exec(null, "")
	
	em_selected = em:getSelected()
	local actionKey = em:getActionKey()
	
	if actionKey == "info" then
		playStream(em_selected + 1)
	end
	if actionKey == "rec" then
		recStream(em_selected + 1)
	end
	
	if em:getExitPressed() ~= true then
		episode_menu(s)
	end
	
	neutrino2.CFileHelpers():removeDir("/tmp/plutotv")
end

function playStream(id)
	file = getVideoData(playback_details[id].uuid)
	title = playback_details[id].name
	info1 = playback_details[id].desc
	cover = playback_details[id].cover	
	duration = playback_details[id].duration
	rating = playback_details[id].rating
		
	movieWidget = neutrino2.CMovieInfoWidget()
	movieWidget:setMovie(file, title, info1, "", cover, duration)

	movieWidget:exec(null, "")
end

function recStream(id)
	httpTool = neutrino2.CHTTPTool()
	httpTool:setTitle("Pluto TV VOD downlaoding...")
		
	local rec_file = getVideoData(playback_details[id].uuid)
		
	config = neutrino2.CConfigFile('\t')

	config:loadConfig(neutrino2.CONFIGDIR .. "/neutrino2.conf")

	local target = config:getString("network_nfs_recordingdir") .. "/" .. conv_utf8(playback_details[id].name) .. ".mp4"
		
	if httpTool:downloadFile(rec_file, target, 100) == true then
		-- save .xml
		neutrino2.CMovieInfo():saveMovieInfo(target, conv_utf8(playback_details[id].name), conv_utf8(playback_details[id].desc), "");
		
		-- save thumbnail		
		neutrino2.CFileHelpers():copyFile(playback_details[id].cover, config:getString("network_nfs_recordingdir") .. "/" .. playback_details[id].name .. ".jpg");
	end
end

-- mainMenu
m_selected = 0

function categories_menu()
	local m = neutrino2.CMenuWidget(plugin_title, neutrino2.PLUGINDIR .. "/plutotv/plutotv.png")
	m:enableShrinkMenu()
	m:enablePaintDate()
	m:setTitleHAlign(neutrino2.CC_ALIGN_CENTER)
	m:clearAll()
	
	local red = neutrino2.button_label_struct()
	red.button = neutrino2.NEUTRINO_ICON_BUTTON_RED
	red.localename = "Update Channels"
	
	m:setFootButtons(red)
	
	m:addKey(neutrino2.RC_red, null, "update")
	
	for _id,_name in pairs(catlist) do
		item = neutrino2.ClistBoxItem(conv_utf8(_name))
		item:setActionKey(null, "cat_menu")
		
		m:addItem(item)
	end
	
	if m_selected < 0 then
		m_selected = 0
	end
	
	m:setSelected(m_selected)
	
	m:exec(null, "")
	
	m_selected = m:getSelected()
	local actionKey = m:getActionKey()
	
	if actionKey == "cat_menu" then
		cat_menu(m_selected + 1)
	end
	
	if actionKey == "update" then
		get_channels()
	end
	
	if m:getExitPressed() ~= true then
		categories_menu()
	end
end

function main()
	get_cat()
end

main()




