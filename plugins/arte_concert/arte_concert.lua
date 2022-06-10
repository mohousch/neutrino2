--[[
	arte concert
	Vers.: 1.5.4 vom 09.12.2019
	Copyright (C) 2016-2019, bazi98
	Copyright (C) 2009 - for the Base64 encoder/decoder function by Alex Kloss

        Addon Description:
        The addon evaluates Videos from the arte concert media library and 
        provides the videos for playing with the neutrino media player on.

        This addon is not endorsed, certified or otherwise approved in any 
        way by ARTE GEIE.

        The plugin respects arte's General Terms and Conditions of Use, 
        which prohibits the publishing or making publicly available of any 
        software, app or similar which allows the livestream / videos to 
        be fully or partially definitely and permanently downloaded.

        The copyright (C) for the linked videos, descriptive texts and for 
        the arte concert logo are owned by arte or the respective owners!

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

local json  = require "json"

basisurl= "http://concert.arte.tv/"

local selected_sm = 0
local pmid = 0

--[[
    language version

    Mit der Auswahl der Sprachoptionen wird die Sprache des Video und der Texte festgelegt.
    Avec le choix des options de langue langue de la vidéo et le texte du message est défini.
    Selecting the language in the language options sets the language of the videos and the message body.
    Al seleccionar el idioma en las opciones de idioma, se establece el idioma de los videos y el cuerpo del mensaje.
    Z wyborem opcji język wideo i tekst wiadomości jest ustawiony.
    Selezionando la lingua nelle opzioni della lingua si imposta la lingua dei video e il corpo del messaggio.

    language = "de" -- > deutsch
    language = "fr" -- > français
    language = "en" -- > english
    language = "es" -- > español
    language = "pl" -- > polski
    language = "it" -- > italiano
]]

language = "de" -- default = "de"

--[[
     Mit der Option "Qualität" wird festgelegt mit welcher Auflösung die Videos angezeigt werden sollen.
     Avec l'est défini « qual » à quelle résolution les vidéos à afficher.
     With the "qual" option is set with which the videos are to be displayed. 
     Con la "qual" se define en lo que la resolución de los vídeos que se mostrarán.
     „qual” definiuje w jakiej rozdzielczości filmy mają być wyświetlane. 
     L'opzione "Qualità" determina la risoluzione con cui visualizzare i video.
]]

qual = 1 -- default = "1" = HD

-- 1 = HD  for DSL >= 6000
-- 2 = MD  for DSL >= 2000
-- 3 = SD  for DSL <= 2000

-- selection menu
local subs = {
{'MUA', 'POP'},
{'MET', 'Metal'},
{'HIP', 'HIP-Hop'},
{'MUE', 'Electronic'},
{'JAZ', 'Jazz'},
{'MUD', 'Int. Music'},
{'CLA', 'Classic'},
{'OPE', 'Opera'},
{'ADS', 'Performance'}
}
--- http://www.arte.tv/hbbtvv2/services/web/index.php/EMAC/teasers/subcategory/AJO/de -- alte Abfrageadresse via hbbtv
--- https://www.arte.tv/guide/api/emac/v3/de/web/data/MOST_RECENT_SUBCATEGORY/?subCategoryCode=MUA&page=1&limit=100 -- aktuelle Abfrageadresse
--- http://www.arte.tv/hbbtvv2/services/web/index.php/OPA/v3/videos/mostViewed/page/1/limit/100/de
--- https://www.arte.tv/guide/api/api/zones/de/web/videos_subcategory_MUA/?page=1&limit=100
--- https://www.arte.tv/guide/api/emac/v3/de/web/pages/MOST_VIEWED?category=ARS&subcategories=MUA

function datetotime(s) -- > for example 23.11.2016
    local xday, xmonth, xyear, xhour, xmin, xsec = s:match("(%d+).(%d+).(%d+) (%d+):(%d+):(%d+) ")
    return os.time({day = xday, month = xmonth, year = xyear, hour = xhour, min = xmin, sec = xsec})
end

--Objekte
function script_path()
	local str = debug.getinfo(2, "S").source:sub(2)
	return str:match("(.*/)")
end

function init()
	p = {}
	func = {}
	--pmid = 0
	stream = 1
        tmpPath = "/tmp"
        arte_concert = decodeImage("data:image/png;base64,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")
end

function add_stream(t,u,e)
  	p[#p+1] = {title=t, url=u, epg=e, access=stream}
end

function getdata(Url,outputfile)
	if Url == nil then 
		return nil 
	end

	local data = nil

	data = neutrino.getUrlAnswer(Url, "Mozilla/5.0 (Linux mips; U;HbbTV/1.1.1 (+RTSP;DMM;Dreambox;0.1a;1.0;) CE-HTML/1.0; en) AppleWebKit/535.19 no/Volksbox QtWebkit/2.2")

	return data
end

-- Base64 encoder/decoder function
-- character table string

local b='ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/'

-- decode
function dec(data)
	data = string.gsub(data, '[^'..b..'=]', '')
	return (data:gsub('.', function(x)
    	if (x == '=') then return '' end
    	local r,f='',(b:find(x)-1)
    	for i=6,1,-1 do r=r..(f%2^i-f%2^(i-1)>0 and '1' or '0') end
    	return r;
	end):gsub('%d%d%d?%d?%d?%d?%d?%d?', function(x)
    	if (#x ~= 8) then return '' end
    	local c=0
    	for i=1,8 do c=c+(x:sub(i,i)=='1' and 2^(8-i) or 0) end
    	return string.char(c)
	end))
end

function decodeImage(b64Image)
	local imgTyp = b64Image:match("data:image/(.-);base64,")
	local repData = "data:image/" .. imgTyp .. ";base64,"
	local b64Data = string.gsub(b64Image, repData, "");

	local tmpImg = os.tmpname()
	local retImg = tmpImg .. "." .. imgTyp

	local f = io.open(retImg, "w+")
	f:write(dec(b64Data))
	f:close()
	os.remove(tmpImg)

	return retImg
end

-- convert stream address according to the selected quality
function conv_url(_string)
	if _string == nil then return _string end
       _string = string.gsub(_string,'\\','');
		if qual < 2 then -- = HD
				_string = string.gsub(_string,'master.m3u8','index_1_av.m3u8');
		elseif qual > 2 then -- = qual = 3 = SD
				_string = string.gsub(_string,'master.m3u8','index_2_av.m3u8');
				_string = string.gsub(_string,'2200_','800_');
		else  -- = qual = 2 = MD
				_string = string.gsub(_string,'master.m3u8','index_0_av.m3u8');
				_string = string.gsub(_string,'2200_','1500_');
		end				
 	return _string
end

-- convert special characters
function conv_str(_string)
	if _string == nil then return _string end
        _string = string.gsub(_string,'\\','');
	_string = string.gsub(_string,'#039;','’');
	_string = string.gsub(_string,'u00c4','Ä');
	_string = string.gsub(_string,'u00e4','ä');
	_string = string.gsub(_string,'u00e0','à');
	_string = string.gsub(_string,'u00e2','â');
	_string = string.gsub(_string,'u00e5','å');
	_string = string.gsub(_string,'u0105','ą');
	_string = string.gsub(_string,'u00e1','á');
	_string = string.gsub(_string,'u00c1','Á');
	_string = string.gsub(_string,'u0200','ȁ');
	_string = string.gsub(_string,'u0107','ć');
	_string = string.gsub(_string,'u00e7','ç');
	_string = string.gsub(_string,'u00c9','É');
	_string = string.gsub(_string,'u0119','ę');
	_string = string.gsub(_string,'u00e6','æ');
	_string = string.gsub(_string,'u00e8','è');
	_string = string.gsub(_string,'u00e9','é');
	_string = string.gsub(_string,'u00ea','ê');
	_string = string.gsub(_string,'u00eb','ë');
	_string = string.gsub(_string,'u00ee','î');
	_string = string.gsub(_string,'u00ed','í');
	_string = string.gsub(_string,'u00ef','ï');
	_string = string.gsub(_string,'u00cd','i');
	_string = string.gsub(_string,'u00ec','Ì');
	_string = string.gsub(_string,'u00ce','Î');
	_string = string.gsub(_string,'u00f1','ñ');
	_string = string.gsub(_string,'u0144','ń');
	_string = string.gsub(_string,'u00d6','Ö');
	_string = string.gsub(_string,'u00f4','ô');
	_string = string.gsub(_string,'u00f3','ó');
	_string = string.gsub(_string,'u00d6','Ö');
	_string = string.gsub(_string,'u00f6','ö');
	_string = string.gsub(_string,'u0153','œ');
	_string = string.gsub(_string,'u0159','ř');
	_string = string.gsub(_string,'u015b','ś');
	_string = string.gsub(_string,'u00df','ß');
	_string = string.gsub(_string,'u00fa','ú');
	_string = string.gsub(_string,'u00dc','Ü');
	_string = string.gsub(_string,'u00fa','ú');
	_string = string.gsub(_string,'u00fb','û');
	_string = string.gsub(_string,'u00dc','ü');
	_string = string.gsub(_string,'u00fc','ü');
	_string = string.gsub(_string,'u016f','ů');
	_string = string.gsub(_string,'u017c','ż');
	_string = string.gsub(_string,'u2019','’');
	_string = string.gsub(_string,'u00e7','ç');
	_string = string.gsub(_string,'u015e','S');
	_string = string.gsub(_string,'u015f','ş');
	_string = string.gsub(_string,'&lt;','');
	_string = string.gsub(_string,'&amp;','&');
	_string = string.gsub(_string,'&quot;','"');
	_string = string.gsub(_string,'&gt;','');
	_string = string.gsub(_string,'br /','');
	_string = string.gsub(_string,'u201e','„');
	_string = string.gsub(_string,'u201c','“');
	_string = string.gsub(_string,'u00d8','ø');
	_string = string.gsub(_string,'u00a0',' ');
	_string = string.gsub(_string,'u0142','ł');
	_string = string.gsub(_string,'u00b0','°');
	_string = string.gsub(_string,'u0302','̂ ');
	_string = string.gsub(_string,'u031e','̞');
	_string = string.gsub(_string,'u0301','́');
	_string = string.gsub(_string,'u201d','́');
	_string = string.gsub(_string,'u2018','‘');
	_string = string.gsub(_string,'u2013','–');
	_string = string.gsub(_string,'u2026','…');
	_string = string.gsub(_string,'u00bf',''); -- ¿
	_string = string.gsub(_string,'u00bb','»');
	_string = string.gsub(_string,'u00ab','«');
	_string = string.gsub(_string,'u00f8','ø');
	_string = string.gsub(_string,'u2026','…');
	_string = string.gsub(_string,'u00aa','ª');
	return _string
end 

function fill_playlist(id)
	print("fill_playlist:")

	p = {}

	for i,v in  pairs(subs) do
		if v[1] == id then
			nameid = v[2]	
			local data = getdata('https://www.arte.tv/guide/api/emac/v3/' .. language .. '/web/data/MOST_RECENT_SUBCATEGORY/?subCategoryCode=' .. id .. '&page=1&limit=100',nil) 

			--local data = getdata('http://www.arte.tv/hbbtvv2/services/web/index.php/EMAC/teasers/subcategory/' .. id .. '/' .. language ,nil)

				if data then
				    	for  page, title, teaser in data:gmatch('{"id":".-.-"programId":"(.-)",.-"title":"(.-)",.-"shortDescription":"(.-)",.-}')  do -- Version neu
--				    	for  page, title, teaser in data:gmatch('{"programId":"(.-)",.-"title":"(.-)",.-"teaserText":"(.-)",.-}')  do -- Version Old
						if title then
							add_stream(conv_str(title), page , conv_str(teaser) ) 
				        	end
				    	end
				else
				    return nil
				end
			select_playitem()
		end
	end
end

local epg = ""
local title = ""

function select_playitem()
	print("select_playitem:")

	local m = neutrino.CMenuWidget("", arte_concert, 2*neutrino.MENU_WIDTH)
	
	m:setWidgetType(neutrino.WIDGET_TYPE_EXTENDED)
	m:enableShrinkMenu()

	local item = nil
  	for i,r in  ipairs(p) do
		item = neutrino.ClistBoxItem(r.title)
		item:setHint(r.epg)
		--item:setHintIcon(arte_concert)

		m:addItem(item)
  	end

	if pmid < 0 then
		pmid = 0
	end

	m:setSelected(pmid)

	m:exec(null, "")

	local vPlay = neutrino.CMoviePlayerGui()

	pmid = m:getSelected() + 1

	if pmid >= 0 then
		local page = func[p[pmid].access](p[pmid].url)

		if page then --- https://api.arte.tv/api/player/v1/config/de/088439-000-A?lifeCycle=1
			local js_page = getdata('https://api.arte.tv/api/player/v1/config/'.. language .. '/'.. page .. '?lifeCycle=1',nil) -- apicall
		        if js_page ~= nil then
				local jnTab = json:decode(js_page)
				local video_url = nil

				if jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.HTTPS_SQ_1 then
					video_url = jnTab.videoJsonPlayer.VSR.HTTPS_SQ_1.url
				elseif jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.HLS_XQ_1 then 
					video_url = jnTab.videoJsonPlayer.VSR.HLS_XQ_1.url
				elseif jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.HTTP_MP4_SQ_1 then
					video_url = jnTab.videoJsonPlayer.VSR.HTTP_MP4_SQ_1.url
				elseif jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.HTTP_SQ_1 then
					video_url = jnTab.videoJsonPlayer.VSR.HTTP_SQ_1.url
				elseif jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.HLS_SQ_1 then
					video_url = jnTab.videoJsonPlayer.VSR.HLS_SQ_1.url
				elseif jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.HLS_XQ_1 then
					video_url = jnTab.videoJsonPlayer.VSR.HLS_XQ_1.url
				elseif jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.HLS_EQ_1 then
					video_url = jnTab.videoJsonPlayer.VSR.HLS_EQ_1.url
				elseif jnTab.videoJsonPlayer.VSR and jnTab.videoJsonPlayer.VSR.RTMP_SQ_1 then
					video_url = jnTab.videoJsonPlayer.VSR.RTMP_SQ_1.streamer .. jnTab.videoJsonPlayer.VSR.RTMP_SQ_1.url
				end

				title = p[pmid].title 
				if jnTab.videoJsonPlayer.subtitle then
					title = (title .. " - " .. jnTab.videoJsonPlayer.subtitle)
				end

				if title and video_url then
					if jnTab.videoJsonPlayer.VDE then
						epg = jnTab.videoJsonPlayer.VDE 
		                                if language == "fr" then
							  epg = epg .. '\n\nTemps de lecture (mn.) : ' .. jnTab.videoJsonPlayer.VDU 
						elseif language == "de" then
							  epg = epg .. '\n\nSpieldauer (Min.) : ' .. jnTab.videoJsonPlayer.VDU
		                                elseif language == "en" then
							  epg = epg .. '\n\nPlaying time (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "es" then
							  epg = epg .. '\n\nDuración (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "pl" then
							  epg = epg .. '\n\nCzas (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "it" then
							  epg = epg .. '\n\nDurata (min.) : ' .. jnTab.videoJsonPlayer.VDU
		                                end

	 				elseif jnTab.videoJsonPlayer.V7T then
						epg = jnTab.videoJsonPlayer.V7T 
		                                if language == "fr" then
							   epg = epg .. '\n\nTemps de lecture (mn.) : ' .. jnTab.videoJsonPlayer.VDU 
						elseif language == "de" then
							   epg = epg .. '\n\nSpieldauer (Min.) : ' .. jnTab.videoJsonPlayer.VDU
		                                elseif language == "en" then
							   epg = epg .. '\n\nPlaying time (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "es" then
							   epg = epg .. '\n\nDuración (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "pl" then
							   epg = epg .. '\n\nCzas (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "it" then
							   epg = epg .. '\n\nDurata (min.) : ' .. jnTab.videoJsonPlayer.VDU
		                                 end
	 				else
						epg = p[pmid].epg 
		                                if language == "fr" then
							    epg = epg .. '\n\nTemps de lecture (mn.) : ' .. jnTab.videoJsonPlayer.VDU 
						elseif language == "de" then
								 epg = epg .. '\n\nSpieldauer (Min.) : ' .. jnTab.videoJsonPlayer.VDU
		                                elseif language == "en" then
								 epg = epg .. '\n\nPlaying time (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "es" then
								 epg = epg .. '\n\nDuración (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "pl" then
								 epg = epg .. '\n\nCzas (min.) : ' .. jnTab.videoJsonPlayer.VDU
						elseif language == "it" then
								 epg = epg .. '\n\nDurata (min.) : ' .. jnTab.videoJsonPlayer.VDU
		                                end
					end

					vPlay:addToPlaylist(conv_url(video_url), conv_str(title), conv_str(epg))
					vPlay:exec(null, "")
				end

				epg = ""
				title = ""
			end
		end		
	end

	if m:getExitPressed() ~= true then
		select_playitem()
	end
end

function selectmenu()
	local item = nil
	sm = neutrino.CMenuWidget("Arte Konzerte", arte_concert)
	
	for i,v in  ipairs(subs) do
		item = neutrino.ClistBoxItem(v[2], true, "", nil, v[1])

		sm:addItem(item)
	end

	if selected_sm < 0 then
		selected_sm = 0
	end

	sm:setSelected(selected_sm)

	sm:exec(null, "")

	selected_sm = sm:getSelected()
	actionKey = sm:getActionKey()

	if selected_sm >= 0 then
		fill_playlist(actionKey)
	end

	if sm:getExitPressed() ~= true then
		selectmenu()
	end
end

function main()
	init()
	func={
		[stream]=function (x) return x end,
	}
	selectmenu()
	os.execute("rm /tmp/lua*.png");
end

main()


