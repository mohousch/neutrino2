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
feedentries = {} --don't make local
local nothing,hva,hvb,hvc,hvd,hve="nichts",nil,nil,nil,nil,nil
local vPlay = neutrino2.CMoviePlayerGui()
local picdir = "/tmp/rssPics"

function _(string)
	return dgettext("media_one", string)
end

feedentries = {
    { name = "Krimi",                                   exec = "https://mediathekviewweb.de/feed?query=%23Krimi&everywhere=true"},
    { name = "Spielfilm",                               exec = "https://mediathekviewweb.de/feed?query=%23spielfilm&everywhere=true"},
    { name = "Arte Film",                               exec = "https://mediathekviewweb.de/feed?query=!Arte.de%20Film&everywhere=true&page=2"},
    { name = "Tatort",                                  exec = "https://mediathekviewweb.de/feed?query=%23tatort&everywhere=true"},
    { name = "Polizeiruf",                              exec = "https://mediathekviewweb.de/feed?query=%23polizeiruf&everywhere=true"},
    { name = "Wunderschön",                             exec = "https://mediathekviewweb.de/feed?query=%23wundersch%C3%B6n&everywhere=true"},
    { name = "Traumreisen",                             exec = "https://mediathekviewweb.de/feed?query=Traumorte&everywhere=true"},
    { name = "Städtetripp",                             exec = "https://mediathekviewweb.de/feed?query=st%C3%A4dtetrip&everywhere=true"}, 
    { name = "Sagenhaft",                               exec = "https://mediathekviewweb.de/feed?query=%23Sagenhaft"},
    { name = "Eisenbahn Romantik",                      exec = "https://mediathekviewweb.de/feed?query=%23Eisenbahn-&everywhere=true"},
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

function getdata(Url)
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
		window:exec(self)
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

function info(infotxt, cap)
	if cap == nil then
		cap = _("Information")
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

	local UrlVideo, UrlAudio, UrlExtra = nil,nil,nil
	local picUrl = nil
	local feed = fp.entries[idNr]
	
	for i, link in ipairs(feed.enclosures) do
		local urlType = link.type
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
	
	if picUrl == nil then
		local urls = {feed.summary, feed.content}
		for i,v in ipairs(urls) do
			if type(v) == "string" then
				v = v:gsub('src=""','')
				v = v:gsub("src=''",'')
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
						if url ~= picUrl then
							--if check_if_double(picUrl,url) then
								picUrl =  url
							--end
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
						if url ~= picUrl then
							--if check_if_double(picUrl,url) then
								picUrl =  url
							--end
						end
					end
				end

				if UrlExtra == nil then
					UrlExtra  = v:match('%w+://[%w+%p]+%.json')
				end
			end
		end
	end
	
	if not UrlVideo and not UrlAudio and not UrlExtra and fp.entries[idNr].link:find("www.youtube.com")then
		UrlExtra = fp.entries[idNr].link
	end

	return UrlVideo , UrlAudio , UrlExtra, picUrl
end

function play(idNr)
	local title = fp.entries[idNr].title
	
	if title then
		title = title:gsub("\n", " ")
	end
	
	local text = fp.entries[idNr].summary
	local UrlLink = fp.entries[idNr].link
	local UrlVideo,UrlAudio,UrlExtra, UrlPic = getMediUrls(idNr)

	if UrlVideo == UrlLink then
		UrlLink = nil
	end
	
	if UrlAudio == UrlLink then
		UrlLink = nil
	end
	
	if UrlLink and UrlLink:sub(1, 4) ~= 'http' then
		UrlLink = nil
	end

	local fpic = neutrino2.DATADIR .. "/icons/nopreview.jpg"
	if UrlPic ~= nil then
		fpic = downloadPic(UrlPic, title)
	end

	if text == nil and fp.entries[idNr].content then
		text = fp.entries[idNr].content
	end
	
	if UrlVideo and UrlVideo:sub(1, 2) == '//' then
		UrlVideo =  'http:' .. UrlVideo
	end
	
	if UrlAudio and UrlAudio:sub(1, 2) == '//' then
		UrlAudio =  'http:' .. UrlAudio
	end

	if UrlVideo then
		vPlay:addToPlaylist(UrlVideo, title, text, "", fpic)
	elseif UrlAudio then
		vPlay:addToPlaylist(UrlAudio, title, text, "", fpic)
	end

	if UrlAudio or UrlVideo then
		vPlay:exec(null, "")
	end
end

function downloadPic(url, picname)
	print("downloadPic:")
	local fpic = nil
	
	fpic = picdir .. "/" .. picname
	
	if neutrino2.file_exists(fpic) == false then
		local ok = neutrino2.downloadUrl(url, fpic)
		if not ok then
			fpic = nil
		end
	end
	
	return fpic
end

--local selected = 0
function rssurlmenu(url)
	glob.feedpersed = getFeedDataFromUrl(url)
	
	if glob.feedpersed == nil then 
		return 
	end

	local selected = 0
	local ret = neutrino2.CTarget_RETURN_REPAINT
	local m = neutrino2.ClistBox(40, 40, 1200, 640)
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

::RETRY::
	ret = m:exec(self)
	
	if m:getExitPressed() == true or ret == neutrino2.CTarget_RETURN_NONE then
		return neutrino2.CTarget_RETURN_EXIT
	end

	selected = m:getSelected()

	if selected >= 0 then
		play(selected + 1)
--		vPlay:exec(null, "")
		ret = neutrino2.CTarget_RETURN_REPAINT
	end

	if ret == neutrino2.CTarget_RETURN_REPAINT then
		goto RETRY
	end

--	glob.feedpersed = nil
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
--	nr = id
--	addon = feedentries[id].addon
	rssurlmenu(feedentries[id].exec)
end

local s_selected = 0
function start()
	local submenus = {}
	local grupmenus = {}
	
	local ret = neutrino2.CTarget_RETURN_REPAINT

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
	
::RETRY::	
	ret = sm:exec(self)
	
	if sm:getExitPressed() == true or ret == neutrino2.CTarget_RETURN_NONE then
		return neutrino2.CTarget_RETURN_EXIT
	end

	s_selected = sm:getSelected()

	if s_selected >= 0 then
		exec_url(s_selected + 1)
		ret = neutrino2.CTarget_RETURN_REPAINT	
	end

	if ret == neutrino2.CTarget_RETURN_REPAINT then
		goto RETRY
	end
end

function main()	
	neutrino2.CFileHelpers():createDir(picdir)

	start()

 	neutrino2.CFileHelpers():removeDir(picdir)
end

main()

