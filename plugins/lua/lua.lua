--[[
neutrinoHD2 lua sample plugin
]]

local selected = 0

-- CMessageBox
function messageBox()
	title = "lua: CMessageBox"
	msg = "neutrino lua:\n testing lua CMessageBox\n"
	mBox = neutrino2.CMessageBox(title, msg)
	mBox:exec()
end

-- CHelpBox
function helpBox()
	title = "lua: CHelpBox"
	hbox = neutrino2.CHelpBox(title)
	hbox:addLine("neutrino: lua")
--	hbox:addSeparator()
	hbox:addLine("first test")
	hbox:addLine("testing CHelpBox ;-)\n")
	hbox:show("lua: CHelpBox")
end

-- CHintBox
function hintBox()
	hint = neutrino2.CHintBox("lua: CHintBox","neutrino lua:\n first test\ntesting CHintBox\ndas ist alles ;-)")
	hint:exec(10)
end

-- CInfoBox
function infoBox()
	info = neutrino2.CInfoBox()
	info:setTitle("lua: CInfoBox")
	info:setText("neutrino lua:\nfirst test\ntesting CHintBox ;-)\n")
	info:exec()
end

-- CStringInput
function stringInput()
	local title = "lua: CStringInputSMS"
	local value = "neutrino lua:"
	local input = neutrino2.CStringInputSMS(title, vale)
	input:exec(None, "")
end

-- CAudioPlayerGui
function audioPlayer()
	local fileBrowser = neutrino2.CFileBrowser()
	local fileFilter = neutrino2.CFileFilter()
	
	config = neutrino2.CConfigFile('\t')

	config:loadConfig(neutrino2.CONFIGDIR .. "/neutrino2.conf")

	local PATH = config:getString("network_nfs_audioplayerdir")

	fileFilter:addFilter("cdr")
	fileFilter:addFilter("mp3")
	fileFilter:addFilter("m2a")
	fileFilter:addFilter("mpa")
	fileFilter:addFilter("mp2")
	fileFilter:addFilter("ogg")
	fileFilter:addFilter("wav")
	fileFilter:addFilter("flac")
	fileFilter:addFilter("aac")
	fileFilter:addFilter("dts")
	fileFilter:addFilter("m4a")

	fileBrowser.Multi_Select = false
	fileBrowser.Dirs_Selectable = false
	fileBrowser.Filter = fileFilter

	local player = neutrino2.CAudioPlayerGui()
	local file = nil

	repeat
		fileBrowser:exec(PATH)

		PATH = fileBrowser:getCurrentDir()
		file = fileBrowser:getSelectedFile()

		if file ~= null then
			player:addToPlaylist(file)
			player:exec(None, "")
		end
	until fileBrowser:getExitPressed() == true
end

-- CPictureViewerGui
function pictureViewer()
	local fileBrowser = neutrino2.CFileBrowser()
	local fileFilter = neutrino2.CFileFilter()
	
	config = neutrino2.CConfigFile('\t')

	config:loadConfig(neutrino2.CONFIGDIR .. "/neutrino2.conf")

	local PATH = config:getString("network_nfs_picturedir")

	fileFilter:addFilter("jpeg")
	fileFilter:addFilter("jpg")
	fileFilter:addFilter("png")
	fileFilter:addFilter("bmp")

	fileBrowser.Multi_Select = false
	fileBrowser.Dirs_Selectable = false
	fileBrowser.Filter = fileFilter

	local player = neutrino2.CPictureViewerGui()
	local file = nil

	repeat
		fileBrowser:exec(PATH)
		PATH = fileBrowser:getCurrentDir()
		file = fileBrowser:getSelectedFile()

		if file ~= null then
			player:addToPlaylist(file)
			player:exec(None, "")
		end
	until fileBrowser:getExitPressed() == true
end

-- CMoviePlayerGui
function moviePlayer()
	local fileBrowser = neutrino2.CFileBrowser()
	local fileFilter = neutrino2.CFileFilter()

	config = neutrino2.CConfigFile('\t')

	config:loadConfig(neutrino2.CONFIGDIR .. "/neutrino2.conf")

	local PATH = config:getString("network_nfs_recordingdir")

	fileFilter:addFilter("ts")
	fileFilter:addFilter("mpg")
	fileFilter:addFilter("mpeg")
	fileFilter:addFilter("divx")
	fileFilter:addFilter("avi")
	fileFilter:addFilter("mkv")
	fileFilter:addFilter("asf")
	fileFilter:addFilter("aiff")
	fileFilter:addFilter("m2p")
	fileFilter:addFilter("mpv")
	fileFilter:addFilter("m2ts")
	fileFilter:addFilter("vob")
	fileFilter:addFilter("mp4")
	fileFilter:addFilter("mov")	
	fileFilter:addFilter("flv")	
	fileFilter:addFilter("dat")
	fileFilter:addFilter("trp")
	fileFilter:addFilter("vdr")
	fileFilter:addFilter("mts")
	fileFilter:addFilter("wmv")
	fileFilter:addFilter("wav")
	fileFilter:addFilter("flac")
	fileFilter:addFilter("mp3")
	fileFilter:addFilter("wma")
	fileFilter:addFilter("ogg")

	fileBrowser.Multi_Select = false
	fileBrowser.Dirs_Selectable = false
	fileBrowser.Filter = fileFilter

	local player = neutrino2.CMoviePlayerGui()
	local file = nil

	repeat
		fileBrowser:exec(PATH)
		PATH = fileBrowser:getCurrentDir()
		file = fileBrowser:getSelectedFile()

		if file ~= null then
			player:addToPlaylist(file)
			player:exec(None, "")
		end
	until fileBrowser:getExitPressed() == true
end

-- ClistBox
function testClistBox()
	local listBox = neutrino2.ClistBox(340, 60, 600,600)
	listBox:enablePaintHead()
	listBox:setTitle("lua: ClistBox", neutrino2.NEUTRINO_ICON_MOVIE)
	listBox:enablePaintDate()
	listBox:enablePaintFoot()
	listBox:enableShrinkMenu()

	-- CMessageBox
	item1 = neutrino2.CMenuForwarder("CMessageBox", true, "", self, "msgBox")
	item1:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino2.CMenuForwarder("CHelpBox", false)
	item2:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")
	item2:setActionKey(null, "helpBox")

	-- CHintBox
	item3 = neutrino2.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")
	item3:setActionKey(null, "hintBox")

	-- CInfoBox
	item4 = neutrino2.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")
	item4:setActionKey(null, "infoBox")

	-- CStringInput
	item5 = neutrino2.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")

	-- CAudioPlayerGui
	item6 = neutrino2.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")
	item6:setActionKey(null, "audioPlayer")

	-- CPictureViewerGui
	item7 = neutrino2.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")
	item7:setActionKey(null, "pictureViewer")

	-- CMoviePlayerGui
	item8 = neutrino2.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item8:setHint("testing CMoviePlayerGui")
	item8:setInfo1("testing CMoviePlayerGui")
	item8:setActionKey(null, "moviePlayer")

	listBox:addItem(item1)
	listBox:addItem(item2)
	listBox:addItem(item3)
	listBox:addItem(item4)
	listBox:addItem(item5)
	listBox:addItem(item6)
	listBox:addItem(item7)
	listBox:addItem(item8)
	
	listBox:addKey(neutrino2.CRCInput_RC_info, self, "infoBox")
	
	listBox:exec()
	
	local actionKey = listBox:getActionKey()
	
	print("actionKey: " .. actionKey)
	
	if actionKey == "msgBox" then
		messageBox()
	elseif actionKey == "helpBox" then
		helpBox()
	elseif actionKey == "hintBox" then
		hintBox()
	elseif actionKey == "cStringInput" then
		stringInput()
	elseif actionKey == "audioPlayer" then
		audioPlayer()
	elseif actionKey == "pictureViewer" then
		pictureViewer()
	elseif actionKey == "moviePlayer" then
		moviePlayer()
	elseif actionKey == "infoBox" then
		infoBox()
	end
	
	if listBox:getExitPressed() ~= true then
		testClistBox()
	end
end

-- CFrameBox
function testCFrameBox()
	local box = neutrino2.CBox()
	local fb = neutrino2.CFrameBuffer_getInstance()

	box.iX = fb:getScreenX() + 40
	box.iY = fb:getScreenY() + 40
	box.iWidth = fb:getScreenWidth() - 80
	box.iHeight = 60

	local frameBox = neutrino2.CFrameBox(box)

	frame1 = neutrino2.CFrameItem()
	frame1:setPosition(box.iX, box.iY, box.iWidth/4, box.iHeight)
	frame1:setTitle("MP3")
	frame1:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame1:setActionKey(null, "audioPlayer")
	frame1:setBorderMode()
	frameBox:addFrame(frame1)

	frame2 = neutrino2.CFrameItem()
	frame2:setPosition(box.iX + box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame2:setTitle("PicViewer")
	frame2:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame2:setActionKey(null, "pictureViewer")
	frame2:setBorderMode()
	frameBox:addFrame(frame2)

	frame3 = neutrino2.CFrameItem()
	frame3:setPosition(box.iX + 2*box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame3:setTitle("MoviePlayer")
	frame3:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame3:setIconName(neutrino2.NEUTRINO_ICON_MOVIE)
	frame3:setOption("spielt Movie Dateien")
	frame3:setActionKey(null, "moviePlayer")
	frame3:setBorderMode()
	frameBox:addFrame(frame3)

	frame4 = neutrino2.CFrameItem()
	frame4:setPosition(box.iX + 3*box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame4:setTitle("Beenden")
	frame4:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame4:setActionKey(null, "exit")
	frame4:setBorderMode()
	frameBox:addFrame(frame4)

	frameBox:exec()

	local actionKey = frameBox:getActionKey()
	
	print("actionKey: " .. actionKey)

	if actionKey == "moviePlayer" then
		print("testCFrameBox: actionKey: moviePlayer")
		moviePlayer()
	elseif actionKey == "audioPlayer" then
		audioPlayer()
	elseif actionKey == "pictureViewer" then
		pictureViewer()
	elseif actionKey == "exit" then
		return neutrino2.CMenuTarget_RETURN_EXIT
	end

	if frameBox:getExitPressed() ~= true then
		testCFrameBox()
	end
end

-- CFrameBox
function testCFrameBoxRandom()
	local fb = neutrino2.CFrameBuffer_getInstance()
	local box = neutrino2.CBox()

	box.iX = fb:getScreenX() + 40
	box.iY = fb:getScreenY() + 40
	box.iWidth = fb:getScreenWidth() - 80
	box.iHeight = fb:getScreenHeight() - 80

	local frameBox = neutrino2.CFrameBox(box)

	frame1 = neutrino2.CFrameItem()
	frame1:setPosition(box.iX, box.iY + 2, 350, 60)
	frame1:setTitle("MP3")
	frame1:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame1:setActionKey(null, "audioPlayer")
	frame1:setBorderMode()
	frameBox:addFrame(frame1)

	frame2 = neutrino2.CFrameItem()
	frame2:setPosition(box.iX, box.iY + 2 + 60, 350, 60)
	frame2:setTitle("PicViewer")
	frame2:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame2:setActionKey(null, "pictureViewer")
	frame2:setBorderMode()
	frameBox:addFrame(frame2)

	frame3 = neutrino2.CFrameItem()
	frame3:setPosition(box.iX, box.iY + 2 + 60 + 2 + 60, 350, 60)
	frame3:setTitle("MoviePlayer")
	frame3:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame3:setIconName(neutrino2.NEUTRINO_ICON_MOVIE)
	frame3:setOption("spielt Movie Dateien")
	frame3:setActionKey(null, "moviePlayer")
	frame3:setBorderMode()
	frameBox:addFrame(frame3)

	frame10 = neutrino2.CFrameItem()
	frame10:setPosition(box.iX, fb:getScreenHeight() - 80 - 60, 350, 60)
	frame10:setTitle("Beenden")
	frame10:setHAlign(neutrino2.CComponent_CC_ALIGN_CENTER)
	frame10:setActionKey(null, "exit")
	frame10:setBorderMode()
	frameBox:addFrame(frame10)

	frameBox:exec()

	local actionKey = frameBox:getActionKey()
	
	print("actionKey: " .. actionKey)

	if actionKey == "moviePlayer" then
		moviePlayer()
	elseif actionKey == "audioPlayer" then
		audioPlayer()
	elseif actionKey == "pictureViewer" then
		pictureViewer()
	elseif actionKey == "exit" then
		return neutrino2.CMenuTarget_RETURN_EXIT
	end

	if frameBox:getExitPressed() ~= true then
		testCFrameBoxRandom()
	end
end

-- CWidget
function testCWidget()
	local testWidget = neutrino2.CWidget()
	testWidget:setMenuPosition(neutrino2.CWidget_MENU_POSITION_LEFT)
	
	local listBox = neutrino2.ClistBox(340, 60, 600,600)

	listBox:setTitle("lua: CWidget")
	listBox:enablePaintHead()
	listBox:enablePaintDate()
	listBox:enablePaintFoot()
	listBox:enableShrinkMenu()

	--listBox:setWidgetMode(neutrino2.ClistBox_MODE_LISTBOX)
	listBox:setWidgetType(neutrino2.ClistBox_TYPE_CLASSIC)

	-- CMessageBox
	item1 = neutrino2.CMenuForwarder("CMessageBox", true, "", null, "msgBox")
	item1:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino2.CMenuForwarder("CHelpBox", false)
	item2:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")
	item2:setActionKey(null, "helpBox")

	-- CHintBox
	item3 = neutrino2.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")
	item3:setActionKey(null, "hintBox")

	-- CInfoBox
	item4 = neutrino2.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")
	item4:setActionKey(null, "infoBox")

	-- CStringInput
	local data = ""
	item5 = neutrino2.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")
	item5:setActionKey(null, "cStringInput")

	-- CAudioPlayerGui
	item6 = neutrino2.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")
	item6:setActionKey(null, "audioPlayer")

	-- CPictureViewerGui
	item7 = neutrino2.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")
	item7:setActionKey(null, "pictureViewer")

	-- CMoviePlayerGui
	item8 = neutrino2.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item8:setHint("testing CMoviePlayerGui")
	item8:setInfo1("testing CMoviePlayerGui")
	item8:setActionKey(null, "moviePlayer")
	
	-- testClistBox
	item9 = neutrino2.CMenuForwarder("ClistBox")
	item9:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item9:setHint("testing ClistBox")
	item9:setInfo1("testing ClistBox")
	item9:setActionKey(null, "listbox")
	
	-- testCFrameBox
	item10 = neutrino2.CMenuForwarder("CFrameBox")
	item10:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item10:setHint("testing CFrameBox")
	item10:setInfo1("testing CFrameBox")
	item10:setActionKey(null, "framebox")
	
	-- testCFrameBoxRandom
	item11 = neutrino2.CMenuForwarder("CFrameBox2")
	item11:setHintIcon(neutrino2.DATADIR .. "/icons/features.png")
	item11:setHint("testing CFrameBox")
	item11:setInfo1("testing CFrameBox")
	item11:setActionKey(null, "frameboxrandom")

	listBox:addItem(item1)
	listBox:addItem(item2)
	listBox:addItem(item3)
	listBox:addItem(item4)
	listBox:addItem(item5)
	listBox:addItem(item6)
	listBox:addItem(item7)
	listBox:addItem(item8)
	listBox:addItem(item9)
	listBox:addItem(item10)
	listBox:addItem(item11)

	if selected < 0 then
		selected = 0
	end

	listBox:setSelected(selected)

	testWidget:addCCItem(listBox)
	testWidget:addKey(neutrino2.CRCInput_RC_info, null, "info")

	testWidget:exec(null, "")

	selected = listBox:getSelected()
	local key = testWidget:getKey()
	local actionKey = testWidget:getActionKey()
	
	print("selected: " .. selected)
	print("key: " .. key)
	print("actionKey: " .. actionKey)
	
	if actionKey == "msgBox" then
		messageBox()
	elseif actionKey == "helpBox" then
		helpBox()
	elseif actionKey == "hintBox" then
		hintBox()
	elseif actionKey == "cStringInput" then
		stringInput()
	elseif actionKey == "audioPlayer" then
		audioPlayer()
	elseif actionKey == "pictureViewer" then
		pictureViewer()
	elseif actionKey == "moviePlayer" then
		moviePlayer()
	elseif actionKey == "infoBox" then
		infoBox()
	elseif actionKey == "listbox" then
		testClistBox()
	elseif actionKey == "framebox" then
		testCFrameBox()
	elseif actionKey == "frameboxrandom" then
		testCFrameBoxRandom()
	--elseif key == neutrino2.CRCInput_RC_info then
	--	infoBox()
	elseif actionKey == "info" then
		infoBox()
	--[[
	elseif selected == 0 then
		messageBox()
	elseif selected == 1 then
		helpBox()
	elseif selected == 2 then
		hintBox()
	elseif selected == 3 then
		infoBox()
	elseif selected == 4 then
		stringInput()
	elseif selected == 5 then
		audioPlayer()
	elseif selected == 6 then
		pictureViewer()
	elseif selected == 7 then
		moviePlayer()
	elseif selected == 8 then
		testClistBox()
	elseif selected == 9 then
		testCFrameBox()
	elseif selected == 10 then
		testCFrameBoxRandom()
	]]
	end

	if testWidget:getExitPressed() ~= true then
		testCWidget()
	end
end

-- main
function main()
	testCWidget()
end

main()

