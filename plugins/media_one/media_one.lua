--[[
	Media One Plugin
	Copyright (C) 2014-2017,  Jacek Jendrzej 'satbaby'

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

--dependencies:  feedparser http://feedparser.luaforge.net/ ,libexpat,  lua-expat
rssReaderVersion="Lua RSS READER v0.77"
local hasgumbo,gumbo = pcall(require,"gumbo")
local glob = {}
local conf = {}
feedentries = {} --don't make local
local addon = nil
local nothing,hva,hvb,hvc,hvd,hve="nichts",nil,nil,nil,nil,nil
local vPlay = nil
local epgtext = nil
local epgtitle = nil
local LinksBrowser = "/links.so"
local picdir = "/tmp/rssPics"

feedentries = {
    { name = "Krimi",                                   exec = "https://mediathekviewweb.de/feed?query=%23Krimi&everywhere=true"},
    { name = "Spielfilm",                                exec = "https://mediathekviewweb.de/feed?query=%23spielfilm&everywhere=true"},
    { name = "Arte Film",                                exec = "https://mediathekviewweb.de/feed?query=!Arte.de%20Film&everywhere=true&page=2"},
    { name = "Tatort",                                  exec = "https://mediathekviewweb.de/feed?query=%23tatort&everywhere=true"},
    { name = "Polizeiruf",                               exec = "https://mediathekviewweb.de/feed?query=%23polizeiruf&everywhere=true"},
    { name = "Wunderschön",                             exec = "https://mediathekviewweb.de/feed?query=%23wundersch%C3%B6n&everywhere=true"},
    { name = "Traumreisen",                             exec = "https://mediathekviewweb.de/feed?query=Traumorte&everywhere=true"},
    { name = "Städtetripp",                             exec = "https://mediathekviewweb.de/feed?query=st%C3%A4dtetrip&everywhere=true"}, 
    { name = "Sagenhaft",                                exec = "https://mediathekviewweb.de/feed?query=%23Sagenhaft"},
    { name = "Eisenbahn Romantik",                                exec = "https://mediathekviewweb.de/feed?query=%23Eisenbahn-&everywhere=true"},
    { name = "Handwerkskunst",                          exec = "https://mediathekviewweb.de/feed?query=%23Handwerkskunst!&everywhere=true"},
    { name = "Geschichte Jahrhundert",                  exec = "https://mediathekviewweb.de/feed?query=%23geschichte"},
    { name = "MDR Zeitreise",                           exec = "https://mediathekviewweb.de/feed?query=%23Zeitreise&everywhere=true"},
    { name = "Zwischen Spessart und Karwendel",         exec = "https://mediathekviewweb.de/feed?query=%23spessart&everywhere=true"},
    { name = "Planet Wissen",                           exec = "http://podcast.wdr.de/planetwissen.xml"},
    { name = "Quarks & Co",                             exec = "http://podcast.wdr.de/quarks.xml"},  	
    { name = "Wer weiß denn sowas",                     exec= "https://mediathekviewweb.de/feed?query=wer%20wei%C3%9F%20den%20sowas&everywhere=true"},
    { name = "Gefragt – Gejagt",                        exec= "https://mediathekviewweb.de/feed?query=gefragt%20gejagt&everywhere=true"},      
    { name = "Monitor",                                 exec = "http://podcast.wdr.de/monitor.xml"},
    { name = "Hart aber Fair",                          exec = "https://mediathekviewweb.de/feed?query=%23Hart%20aber%20fair&everywhere=true"},
    { name = "Tierisch tierisch",                       exec = "https://mediathekviewweb.de/feed?query=%23Tierisch&everywhere=true"},				
    { name = "Wetter Mitteldeutschland",                exec = "https://mediathekviewweb.de/feed?query=%23wetter%20MDR&everywhere=true"},	
    { name = "Rockpalast",                              exec = "https://mediathekviewweb.de/feed?query=%23rockpalast&everywhere=true"},		
}

function __LINE__() return debug.getinfo(2, 'l').currentline end

function pop(cmd)
       local f = assert(io.popen(cmd, 'r'))
       local s = assert(f:read('*a'))
       f:close()
       return s
end

function getdata(Url, outputfile)
	if Url == nil then return nil end

	if Url:sub(1, 2) == '//' then
		Url =  'http:' .. Url
	end

	local data = nil

	data = neutrino2.getUrlAnswer(Url, 'Mozilla/5.0;')

	return data
end

function getFeedDataFromUrl(url)
	print("getFeedDataFromUrl:")

	local data = getdata(url)
	if data then
--		fix for >>> couldn't parse xml. lxp says: junk after document element
		local nB, nE = data:find("</rss>")
		if nE and #data > nE then
			data = string.sub(data,0,nE)
		end
	else
		print("data:NULL")
		return nil
	end

	local error = nil
	local feedparser = require "feedparser"
	fp,error = feedparser.parse(data)
	if error then
		print("DEBUG ".. __LINE__())
		print(data) --  DEBUG
		print ("ERROR >> ".. error .. "\n###")
		local window = neutrino2.CHintBox("DEBUG Output", data)
		window:exec()
	end
	data = nil
	return fp
end

function check_if_double(tab,name)
	for index,value in ipairs(tab) do
		if value == name then
			return false
		end
	end
	return true
end

function info(infotxt,cap)
	if cap == nil then
		cap = "Information"
	end
	local h = neutrino2.CHintBox(cap, infotxt)
	h:exec()
end

function tounicode(c)
	if c > 383 then
		c=c-256
		return "\xC6" .. string.format('%c', c)
	elseif c > 319 then
		c=c-192
		return "\xC5" .. string.format('%c', c)
	elseif c > 254 then
		c=c-128
		return "\xC4" .. string.format('%c', c)
	elseif c > 191 then
		c=c-64
		return "\xC3" .. string.format('%c', c)
	else
		return string.format('%c', c)
	end
end

function convHTMLentities(summary)
	if summary ~= nil then
		summary = summary:gsub("&#([0-9]+);",function(c) return tounicode(tonumber(c)) end)
		summary = summary:gsub("&#x([%x]+);",function(c) return tounicode(tonumber(c, 16)) end)
	end
	return summary
end

function removeElemetsbyTagName(document,ename)
	local t = document:getElementsByTagName(ename)

	for i, element in ipairs(t) do
		element:remove()
	end

end

function removeElemetsbyTagName2(document,tagName,atrName)
	local t = document:getElementsByTagName(tagName)
	for i, element in ipairs(t) do
		local el = element:getAttribute(atrName)
		if el then
			element:remove()
		end
	end
end

function gum(data)
	if data == nil or hasgumbo == nil then return nil end

	local document = gumbo.parse(data)
	local unlikelyCandidates = {"script"}
	for i, unlikely in ipairs(unlikelyCandidates) do
		removeElemetsbyTagName(document,unlikely)
	end
	local class ={ {"div","style"},{"ul","style"},{"section","style"},{"a","onclick"}}
	for i,unlikely  in pairs(class) do
		removeElemetsbyTagName2(document,unlikely[1],unlikely[2])
	end

	local myCandidates ={"style","ol","aside","figure","nav","form","footer"}
	for i,unlikely  in ipairs(myCandidates) do
		removeElemetsbyTagName(document,unlikely)
	end
	local div = document:getElementsByTagName("div")
	for i, element in ipairs(div) do
		local class = element:getAttribute("class")
		if class   then
			if class:find("iqad") or class:find("navi") or class:find("meta")or class:find("sub") or class:find("button") then
				element:remove()
			end

		end
	end

 	for i, element in ipairs(document.links) do
		local elink = element:getAttribute("href")
		if elink:sub(1, 4) == 'http' or elink:sub(1, 1) == '/' or elink:sub(#elink-3, #elink) ~= 'php' then
-- 			print("elonk:",elink)
		else
 			element:remove()
		end
	end
	local div = document:getElementsByTagName("ul")
	for i, element in ipairs(div) do
		local id = element:getAttribute("class")
		if id   then
			if id:find("navi")  or id:find("iqad")then
				element:remove()
			end
		end
	end

	local div = document:getElementsByTagName("p")
	for i, element in ipairs(div) do
		local id = element:getAttribute("class")
		if id   then
			if id:find("navi")  or id:find("iqad") then
				element:remove()
			end
		end
	end
	local div = document:getElementsByTagName("div")
	for i, element in ipairs(div) do
		local id = element:getAttribute("id")
		if id   then
			if id:find("navi")  or id:find("iqad")then
				element:remove()
			end
		end
	end
	local txt = document.body.textContent
	return txt
end

function all_trim(s)
	if s == nil then return "" end
	return s:match("^%s*(.-)%s*$")
end

function xml_entities(s)
	s = s:gsub('&lt;'  , '<' )
	s = s:gsub('&gt;'  , '>' )
	s = s:gsub('&quot;', '"' )
	s = s:gsub('&apos;', "'" )

	s = s:gsub('&Auml;', 'Ä' )
	s = s:gsub('&auml;', 'ä' )
	s = s:gsub('&Ouml;', 'Ö' )
	s = s:gsub('&ouml;', 'ö' )
	s = s:gsub('&uuml;', 'ü' )
	s = s:gsub('&Uuml;', 'Ü' )
	s = s:gsub('&szlig;','ß' )

	s = s:gsub('&euro;','€' )
	s = s:gsub('&copy;','©' )
	s = s:gsub('&reg;','®' )
	s = s:gsub('&nbsp;',' ' )
	s = s:gsub('&shy;','' )
	s = s:gsub('&Oacute;','Ó' )
	s = s:gsub('&oacute;','ó' )
	s = s:gsub('&bdquo;','„' )
	s = s:gsub('&ldquo;','“' )
	s = s:gsub('&ndash;','–' )
	s = s:gsub('&mdash;','—' )
	s = s:gsub('&hellip;','…' )
	s = s:gsub('&lsquo;','‘' )
	s = s:gsub('&rsquo;','’' )
	s = s:gsub('&lsaquo;','‹' )
	s = s:gsub('&rsaquo;','›' )
	s = s:gsub('&permil;','‰' )
	s = s:gsub('&egrave;','è' )
	s = s:gsub('&sbquo;','‚' )
	s = s:gsub('&raquo;','»' )
	s = s:gsub('&rdquo;','”' )

	s = s:gsub('&amp;' , '&' )
	return s
end

function checkdomain(feed,url)
	if not url then return url end
	local a,b=url:find("src=.http:")
	if a and b then
		url=url:sub(b-4,#url)
	end
	if not url:find("//") then
		local domain = nil
		if fp.feed.link then
			domain = fp.feed.link:match('^(%w+://[^/]+)')
		end
		if domain then
			url =  domain .. "/" .. url
		end
	end
	return url
end

function getMediUrls(idNr)
	print("getMediUrls:")

	local UrlVideo,UrlAudio, UrlExtra = nil,nil,nil
	local picUrl = nil -- {}
	local feed = fp.entries[idNr]
	for i, link in ipairs(feed.enclosures) do
		local urlType =link.type
		local mediaUrlFound = false
		if link.url and urlType == "image/jpeg" then
			--picUrl[#picUrl+1] =  link.url
			picUrl = link.url
			mediaUrlFound = true
		end
		if urlType == 'video/mp4' or  urlType == 'video/mpeg' or
		   urlType == 'video/x-m4v' or  urlType == 'video/quicktime' then
			UrlVideo =  link.url
			mediaUrlFound = true
		end
		if urlType == 'audio/mp3' or urlType == 'audio/mpeg' then
			UrlAudio =  link.url
			mediaUrlFound = true
		end

		if mediaUrlFound == false and link.url then
			local purl = link.url:match ('(http.-%.[JjGgPp][PpIiNn][Ee]?[GgFf])')
				if purl and #purl>4 then
					purl = checkdomain(feed,url)
					if purl ~= picUrl[#picUrl] then
						picUrl[#picUrl+1] =  purl
					end
				end
		end
	end
	if not UrlVideo and feed.summary then
		UrlVideo = feed.summary:match('<source%s+src%s?=%s?"(.-)"%s+type="video/mp4">')
	end
	--[[if #picUrl == 0 then
		local urls = {feed.summary, feed.content}
		for i,v in ipairs(urls) do
			if type(v) == "string" then
				v=v:gsub('src=""','')
				v=v:gsub("src=''",'')
				for url in v:gmatch ('[%-%s]?src%s?=%s?[%"\']?(.-%.[JjGgPp][PpIiNn][Ee]?[GgFf])[%"\'%s%?]') do
					if url and #url > 4 then
						local a,b = url:find("http[%w%p]+$")
						if a and b then
							local tmpurl = url:sub(a,b)
							if tmpurl then
								url = tmpurl
							end
						end
						url = checkdomain(feed,url)
						if url ~= picUrl[#picUrl] then
							if check_if_double(picUrl,url) then
								picUrl[#picUrl+1] =  url
							end
						end
					end
				end
				for url in v:gmatch ('[%-%s]?<a href%s?=%s?[%"\']?(.-%.[JjGgPp][PpIiNn][Ee]?[GgFf])[%"\'%s%?]') do
					if url and #url > 4 then
						local a,b = url:find("http[%w%p]+$")
						if a and b then
							local tmpurl = url:sub(a,b)
							if tmpurl then
								url = tmpurl
							end
						end
						url = checkdomain(feed,url)
						if url ~= picUrl[#picUrl] then
							if check_if_double(picUrl,url) then
								picUrl[#picUrl+1] =  url
							end
						end
					end
				end

				if UrlExtra == nil then
					UrlExtra  = v:match('%w+://[%w+%p]+%.json')
				end
			end
		end
	end
	]]
	if not UrlVideo and not UrlAudio and not UrlExtra and fp.entries[idNr].link:find("www.youtube.com")then
		UrlExtra = fp.entries[idNr].link
	end
	glob.urlPicUrls = picUrl

	--
	print(picUrl)
	--

	return UrlVideo , UrlAudio , UrlExtra
end

function html2text(viewer,text)
	if text == nil then 
		return nil 
	end
	local tmp_filename = os.tmpname()
 	local fileout = io.open(tmp_filename, 'w+')
	if fileout then
		text = text:gsub("<[sS][cC][rR][iI][pP][tT][^%w%-].-</[sS][cC][rR][iI][pP][tT]%s*>", "")
		fileout:write(text .. "\n")
		fileout:close()
		collectgarbage()
	end
	local cmd = viewer .. " " .. tmp_filename
	text = pop(cmd)
	os.remove(tmp_filename)
	return text
end

function checkHaveViewer()
	if hva == conf.htmlviewer then
		return true
	elseif hvb == conf.htmlviewer then
		return true
	elseif hvc == conf.htmlviewer then
		return true
	elseif hvd == conf.htmlviewer then
		return true
	elseif hve == conf.htmlviewer then
		return true
	elseif nothing == conf.htmlviewer then
		return false
	end
		return false
end

function showWithHtmlViewer(data)
	local txt = nil
	local viewer = nil
	if hve == conf.htmlviewer then
		viewer = conf.linksbrowserdir .. LinksBrowser .. " -dump"
		txt=html2text(viewer,data)
	elseif hvb == conf.htmlviewer then
		viewer= conf.bindir .. "/html2text -nobs -utf8"
		txt=html2text(viewer,data)
	elseif hvc == conf.htmlviewer then
		viewer= conf.bindir .. "/w3m -dump"
		txt=html2text(viewer,data)
	elseif hvd == conf.htmlviewer then
		txt = gum(data)
	elseif nothing == conf.htmlviewer then
		return nil
	end

	return txt
end

function showMenuItem(id)
	print("showMenuItem:")

	local nr = id
	local stop = false
	local selected = paintMenuItem(nr)
end

function paintMenuItem(idNr)
	print("paintMenuItem:")

	glob.urlPicUrls  = nil --{}
	local title    = fp.entries[idNr].title
	if title then
		title = title:gsub("\n", " ")
	end
	local text    = fp.entries[idNr].summary
	local UrlLink = fp.entries[idNr].link
	local fpic = nil
	local UrlVideo,UrlAudio,UrlExtra = getMediUrls(idNr)

	if addon and UrlLink then
		local hasaddon,a = pcall(require, addon)
		if hasaddon then
			a.getAddonMedia(UrlLink,UrlExtra)
			if a.VideoUrl then
				UrlVideo = a.VideoUrl
				a.VideoUrl = nil
			end
			if a.AudioUrl then
				UrlAudio = a.AudioUrl
				a.AudioUrl = nil
			end
			--[[if a.PicUrl then
				if type(a.PicUrl) == 'table' then
					for i=1,#a.PicUrl do
						if check_if_double(glob.urlPicUrls,a.PicUrl[i]) then
							glob.urlPicUrls[#glob.urlPicUrls+1] = a.PicUrl[i]
						end
					end
				else
					if check_if_double(glob.urlPicUrls,a.PicUrl) then
						glob.urlPicUrls[#glob.urlPicUrls+1] = a.PicUrl
					end
				end
				a.PicUrl = nil
			end]]
			if a.addText then
				if text == nil then
					text=""
				end
				text = text .. a.addText
				a.addText = nil
			end
			if a.newText then
				text = a.newText
				a.newText = nil
			end
		else
			local errMsg = ".lua not found in directory: " .. conf.addonsdir
			info( addon .. errMsg ,"ADDON: Error")
		end
	end

	if  not vPlay  and (UrlVideo or UrlAudio) then
		vPlay = neutrino2.CMoviePlayerGui()
	end
	if  vPlay and text and #text > 1 then
		epgtext = text
		epgtitle = title
	end

	if UrlVideo == UrlLink then
		UrlLink = nil
	end
	if UrlAudio == UrlLink then
		UrlLink = nil
	end
	if UrlLink and UrlLink:sub(1, 4) ~= 'http' then
		UrlLink = nil
	end

	--if #glob.urlPicUrls > 0 then
		fpic = downloadPic(idNr,1)
	--end

	if text == nil and fp.entries[idNr].content then
		text = fp.entries[idNr].content
	end
	if UrlVideo and UrlVideo:sub(1, 2) == '//' then
		UrlVideo =  'http:' .. UrlVideo
	end
	if UrlAudio and UrlAudio:sub(1, 2) == '//' then
		UrlAudio =  'http:' .. UrlAudio
	end

	if text == nil then
		if vPlay and UrlVideo then
			vPlay:addToPlaylist(UrlVideo, title, text)
		elseif vPlay and UrlAudio then
			vPlay:addToPlaylist(UrlAudio, title, text)
		end
		vPlay:exec(null, "")
		collectgarbage()
	end

	if vPlay and UrlVideo then
		vPlay:addToPlaylist(UrlVideo, title, text)
		vPlay:exec(null, "")

	elseif checkHaveViewer() and UrlLink then
		if hva == conf.htmlviewer and UrlLink then
			os.execute(conf.linksbrowserdir .. LinksBrowser .. " -g " .. UrlLink)
		else
			local data = getdata(UrlLink)
			if data then
				local txt = showWithHtmlViewer(data)
				data = nil
				if txt then
					show_textWindow(title,txt)
				end
			end
		end

	elseif UrlAudio and vPlay then
		vPlay:addToPlaylis(UrlAudio, title, text)
		vPlay:exec(null, "")
	end

	epgtext = nil
	epgtitle = nil
	
	--[[if #glob.urlPicUrls > 0 and conf.picdir == picdir then
		neutrino2.CFileHelpers():removeDir(picdir)
		neutrino2.CFileHelpers():createDir(picdir)
		glob.urlPicUrls = nil
	end]]
	collectgarbage()
	return selected
end

function downloadPic(idNr,nr)
	print("downloadPic:")

	local fpic = nil
	if not glob.urlPicUrls then 
		return nil 
	end
	local picname = string.find(glob.urlPicUrls, "/[^/]*$")
	if picname then
		picname = string.sub(glob.urlPicUrls,picname+1,#glob.urlPicUrls)
		local t = nil
		if fp.entries[idNr].updated_parsed then
		      t = os.date("%d%m%H%M%S_",fp.entries[idNr].updated_parsed)
		end
		local id2 = nil
		if t then
			id2 = t
		else
			id2 = idNr
		end
		fpic = conf.picdir .. "/" .. id2 .. picname
		if neutrino2.file_exists(fpic) == false then
			if nr > 1 then

			end
			local UrlPic = glob.urlPicUrls
			
			local ok = neutrino2.downloadUrl(UrlPic, fpic)
			if not ok then
				fpic = nil
			end
		end
	end
	
	print(fpic)
	return fpic
end

local selected = 0
function rssurlmenu(url)
	print("rssurlmenu:")
	glob.feedpersed = getFeedDataFromUrl(url)
	if glob.feedpersed == nil then 
		return 
	end

	local m = neutrino2.ClistBox(10, 10, 1260, 700)
	m:enablePaintHead()
	m:enablePaintDate()
	m:setTitle(glob.feedpersed.feed.title, neutrino2.NEUTRINO_ICON_MOVIE)
	m:enablePaintFoot()

	local item = nil
	for i = 1, #glob.feedpersed.entries do
		local title = ""
		if fp.entries[i].updated_parsed then
		      title = os.date("%d.%m %H:%M",fp.entries[i].updated_parsed)
		end
		if glob.feedpersed.entries[i].title then
			title = title .. " "..glob.feedpersed.entries[i].title:gsub("\n", " ")
			title = xml_entities(title)
			title = title:gsub("</i>", " ")
			title = title:gsub("<i>", " ")
		else
			title = title .. " "
		end
		item = neutrino2.CMenuForwarder(title)

		m:addItem(item)
	end
	
	if selected < 0 then
		selected = 0
	end
	
	m:setSelected(selected)

	m:exec()

	selected = m:getSelected()

	if selected >= 0 then
		showMenuItem(selected + 1)
	end

	if m:getExitPressed() ~= true then
		rssurlmenu(url)
	end

	glob.feedpersed = nil
	collectgarbage()
end

function exec_url(id)
	execUrl(id)
end

function exec_url2(id)
	execUrl(id)
end

function exec_urlsub(id)
	execUrl(id)
end

function execUrl(id)
	nr = id
	addon = feedentries[nr].addon
	rssurlmenu(feedentries[nr].exec)
end

local s_selected = 0
function start()
	local submenus = {}
	local grupmenus = {}

	local sm = neutrino2.ClistBox(340, 60, 600,600)
	sm:enablePaintHead()
	sm:enablePaintDate()
	sm:setTitle("Media One", neutrino2.NEUTRINO_ICON_MOVIE)
	sm:enablePaintFoot()
	sm:enableShrinkMenu()

	for v, w in ipairs(feedentries) do
		if not w.submenu and not w.grup then
			sm:addItem(neutrino2.CMenuForwarder(w.name))
		end
	end

	if s_selected < 0 then
		s_selected = 0
	end
	
	sm:setSelected(s_selected)
	
	sm:exec()

	s_selected = sm:getSelected()

	if s_selected >= 0 then
		exec_url(s_selected + 1)	
	end

	if sm:getExitPressed() ~= true then
		start()
	end
end

function main()
	local config = neutrino2.PLUGINDIR .. "/media_one/media_one.conf"
	
	neutrino2.CFileHelpers():createDir(picdir)

	if next(feedentries) == nil then
		print("DEBUG ".. __LINE__())
		print("failed while loading " .. config)
		return
	end

	start()

	saveConfig()

 	neutrino2.CFileHelpers():removeDir(picdir)
end

main()

