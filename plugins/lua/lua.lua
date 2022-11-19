--[[
neutrinoHD2 lua sample plugin
]]

local selected = 0

-- CMessageBox
function messageBox()
	title = "lua: CMessageBox"
	msg = "neutrino lua:\n testing lua CMessageBox\n"
	mBox = neutrino.CMessageBox(title, msg)
	mBox:exec()
end

-- CHelpBox
function helpBox()
	hbox = neutrino.CHelpBox()
	hbox:addLine("neutrino: lua")
	hbox:addSeparator()
	hbox:addLine("first test")
	hbox:addLine("testing CHelpBox ;-)\n")
	hbox:show("lua: CHelpBox")
end

-- CHintBox
function hintBox()
	hint = neutrino.CHintBox("lua: CHintBox","neutrino lua:\n first test\ntesting CHintBox\ndas ist alles ;-)")
	hint:exec(10)
end

-- CInfoBox
function infoBox()
	info = neutrino.CInfoBox()
	info:setTitle("lua: CInfoBox")
	info:setText("neutrino lua:\nfirst test\ntesting CHintBox ;-)\n")
	info:exec()
end

-- CStringInput
function stringInput()
	local title = "lua: CStringInputSMS"
	local value = "neutrino lua:"
	local input = neutrino.CStringInputSMS(title, vale)
	input:exec(None, "")
end

-- CAudioPlayerGui
function audioPlayer()
	local fileBrowser = neutrino.CFileBrowser()
	local fileFilter = neutrino.CFileFilter()
	
	config = neutrino.CConfigFile('\t')

	config:loadConfig(neutrino.CONFIGDIR .. "/neutrino.conf")

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

	local player = neutrino.CAudioPlayerGui()
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
	local fileBrowser = neutrino.CFileBrowser()
	local fileFilter = neutrino.CFileFilter()
	
	config = neutrino.CConfigFile('\t')

	config:loadConfig(neutrino.CONFIGDIR .. "/neutrino.conf")

	local PATH = config:getString("network_nfs_picturedir")

	fileFilter:addFilter("jpeg")
	fileFilter:addFilter("jpg")
	fileFilter:addFilter("png")
	fileFilter:addFilter("bmp")

	fileBrowser.Multi_Select = false
	fileBrowser.Dirs_Selectable = false
	fileBrowser.Filter = fileFilter

	local player = neutrino.CPictureViewerGui()
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
	local fileBrowser = neutrino.CFileBrowser()
	local fileFilter = neutrino.CFileFilter()

	config = neutrino.CConfigFile('\t')

	config:loadConfig(neutrino.CONFIGDIR .. "/neutrino.conf")

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

	local player = neutrino.CMoviePlayerGui()
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

-- exec_actionKey
function exec(id, msg, actionKey)
	print("lua sample: exec: actionKey: (" .. actionKey ..")")
	
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
	elseif msg == neutrino.RC_info then
		infoBox()
	elseif id == 0 then
		messageBox()
	elseif id == 1 then
		helpBox()
	elseif id == 2 then
		hintBox()
	elseif id == 3 then
		infoBox()
	elseif id == 4 then
		stringInput()
	elseif id == 5 then
		audioPlayer()
	elseif id == 6 then
		pictureViewer()
	elseif id == 7 then
		moviePlayer()
	end
end

-- CWidget
function testCWidget()
	local ret = neutrino.RETURN_REPAINT

	local testWidget = neutrino.CWidget()
	testWidget:setMenuPosition(neutrino.MENU_POSITION_LEFT)
	
	local listBox = neutrino.ClistBox()

	listBox:setTitle("lua: CWidget|ClistBox")
	listBox:enablePaintHead()
	listBox:enablePaintDate()
	listBox:enablePaintFoot()
	listBox:enableShrinkMenu()

	--listBox:setWidgetMode(neutrino.MODE_LISTBOX)
	listBox:setWidgetType(neutrino.WIDGET_TYPE_CLASSIC)

	-- CMessageBox
	item1 = neutrino.CMenuForwarder("CMessageBox", true, "", null, "msgBox")
	item1:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino.CMenuForwarder("CHelpBox")
	item2:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")
	item2:setActionKey(null, "helpBox")

	-- CHintBox
	item3 = neutrino.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")
	item3:setActionKey(null, "hintBox")

	-- CInfoBox
	item4 = neutrino.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")
	item4:setActionKey(null, "infoBox")

	-- CStringInput
	local data = ""
	item5 = neutrino.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")
	item5:setActionKey(null, "cStringInput")

	-- CAudioPlayerGui
	item6 = neutrino.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")
	item6:setActionKey(null, "audioPlayer")

	-- CPictureViewerGui
	item7 = neutrino.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")
	item7:setActionKey(null, "pictureViewer")

	-- CMoviePlayerGui
	item8 = neutrino.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
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

	if selected < 0 then
		selected = 0
	end

	listBox:setSelected(selected)

	testWidget:addWidgetItem(listBox)
	testWidget:addKey(neutrino.RC_info, null, "info")

	ret = testWidget:exec(null, "")

	selected = listBox:getSelected()
	local key = testWidget:getKey()
	local actionKey = testWidget:getActionKey()

	exec(selected, key, actionKey)

	if testWidget:getExitPressed() ~= true and ret == neutrino.RETURN_REPAINT then
		testCWidget()
	end

	return ret
end

-- CMenuWidget
function testCMenuWidget()
	local ret = neutrino.RETURN_REPAINT

	local listBoxWidget = neutrino.CMenuWidget("lua: CMenuWidget")
	listBoxWidget:setWidgetType(neutrino.WIDGET_TYPE_STANDARD)
	listBoxWidget:setWidgetMode(neutrino.MODE_MENU)
	listBoxWidget:enablePaintItemInfo(70)
	listBoxWidget:enableShrinkMenu()
	listBoxWidget:setMenuPosition(neutrino.MENU_POSITION_RIGHT)

	-- CMessageBox
	item1 = neutrino.CMenuForwarder("CMessageBox", true, "", self, "msgBox")
	item1:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino.CMenuForwarder("CHelpBox")
	item2:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")

	-- CHintBox
	item3 = neutrino.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")

	-- CInfoBox
	item4 = neutrino.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")

	-- CStringInput
	item5 = neutrino.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")

	-- CAudioPlayerGui
	item6 = neutrino.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")

	-- CPictureViewerGui
	item7 = neutrino.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")

	-- CMoviePlayerGui
	item8 = neutrino.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item8:setHint("testing CMoviePlayerGui")
	item8:setInfo1("testing CMoviePlayerGui")

	listBoxWidget:addItem(item1)
	listBoxWidget:addItem(item2)
	listBoxWidget:addItem(item3)
	listBoxWidget:addItem(item4)
	listBoxWidget:addItem(item5)
	listBoxWidget:addItem(item6)
	listBoxWidget:addItem(item7)
	listBoxWidget:addItem(item8)

	if selected < 0 then
		selected = 0
	end

	listBoxWidget:setSelected(selected)

	listBoxWidget:addKey(neutrino.RC_info)

	ret = listBoxWidget:exec(null, "")

	selected = listBoxWidget:getSelected()
	local key = listBoxWidget:getKey()
	local actionKey = listBoxWidget:getActionKey()
	

	exec(selected, key, actionKey)
		
	if listBoxWidget:getExitPressed() ~= true and ret == neutrino.RETURN_REPAINT then
		testCMenuWidget()
	end

	return ret
end

-- ClistBox
function testClistBox()
	local ret = neutrino.RETURN_REPAINT

	local listBox = neutrino.ClistBox()
	listBox:enablePaintHead()
	listBox:setTitle("lua: ClistBox", neutrino.NEUTRINO_ICON_MOVIE)
	listBox:enablePaintDate()
	listBox:enablePaintFoot()
	listBox:enableShrinkMenu()

	-- CMessageBox
	item1 = neutrino.CMenuForwarder("CMessageBox", true, "", self, "msgBox")
	item1:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino.CMenuForwarder("CHelpBox")
	item2:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")

	-- CHintBox
	item3 = neutrino.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")

	-- CInfoBox
	item4 = neutrino.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")

	-- CStringInput
	item5 = neutrino.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")

	-- CAudioPlayerGui
	item6 = neutrino.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")

	-- CPictureViewerGui
	item7 = neutrino.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")

	-- CMoviePlayerGui
	item8 = neutrino.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino.DATADIR .. "/icons/features.png")
	item8:setHint("testing CMoviePlayerGui")
	item8:setInfo1("testing CMoviePlayerGui")

	listBox:addItem(item1)
	listBox:addItem(item2)
	listBox:addItem(item3)
	listBox:addItem(item4)
	listBox:addItem(item5)
	listBox:addItem(item6)
	listBox:addItem(item7)
	listBox:addItem(item8)
	
	local m = neutrino.CWidget()
	m:setMenuPosition(neutrino.MENU_POSITION_CENTER)

	m:addWidgetItem(listBox)
	m:addKey(neutrino.RC_info)
	
	if selected < 0 then
		selected = 0
	end

	listBox:setSelected(selected)

	ret = m:exec(null, "")
	
	local selected = listBox:getSelected()
	local actionKey = listBox:getActionKey()
		
	exec(selected, key, actionKey)
	
	if m:getExitPressed() ~= true then
		testClistBox()
	end

	return ret
end

-- CFrameBox
function testCFrameBox()
	local ret = neutrino.RETURN_REPAINT

	local box = neutrino.CBox()
	local fb = neutrino.CFrameBuffer_getInstance()

	box.iX = fb:getScreenX() + 40
	box.iY = fb:getScreenY() + 40
	box.iWidth = fb:getScreenWidth() - 80
	box.iHeight = 60

	local frameBox = neutrino.CFrameBox(box)

	frame1 = neutrino.CFrame()
	frame1:setPosition(box.iX, box.iY, box.iWidth/4, box.iHeight)
	frame1:setTitle("MP3")
	frame1:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame1:setActionKey(null, "audioPlayer")
	frame1:enableBorder()
	frameBox:addFrame(frame1)

	frame2 = neutrino.CFrame()
	frame2:setPosition(box.iX + box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame2:setTitle("PicViewer")
	frame2:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame2:setActionKey(null, "pictureViewer")
	frame2:enableBorder()
	frameBox:addFrame(frame2)

	frame3 = neutrino.CFrame()
	frame3:setPosition(box.iX + 2*box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame3:setTitle("MoviePlayer")
	frame3:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame3:setIconName(neutrino.NEUTRINO_ICON_MOVIE)
	frame3:setOption("spielt Movie Dateien")
	frame3:setActionKey(null, "moviePlayer")
	frame3:enableBorder()
	frameBox:addFrame(frame3)

	frame4 = neutrino.CFrame()
	frame4:setPosition(box.iX + 3*box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame4:setTitle("Beenden")
	frame4:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame4:setActionKey(null, "exit")
	frame4:enableBorder()
	frameBox:addFrame(frame4)

	local m = neutrino.CWidget()
	m:addWidgetItem(frameBox)

	ret = m:exec(null, "")

	local actionKey = m:getActionKey()

	if actionKey == "moviePlayer" then
		print("testCFrameBox: actionKey: moviePlayer")
		moviePlayer()
	elseif actionKey == "audioPlayer" then
		audioPlayer()
	elseif actionKey == "pictureViewer" then
		pictureViewer()
	elseif actionKey == "exit" then
		print("testCFrameBox: actionKey: exit")
		return ret
	end

	if m:getExitPressed() ~= true then
		testCFrameBox()
	end

	return ret
end

-- CFrameBox
function testCFrameBoxRandom()
	local ret = neutrino.RETURN_REPAINT

	local fb = neutrino.CFrameBuffer_getInstance()
	local box = neutrino.CBox()

	box.iX = fb:getScreenX() + 40
	box.iY = fb:getScreenY() + 40
	box.iWidth = fb:getScreenWidth() - 80
	box.iHeight = fb:getScreenHeight() - 80

	local frameBox = neutrino.CFrameBox(box)

	frame1 = neutrino.CFrame()
	frame1:setPosition(box.iX, box.iY + 2, 350, 60)
	frame1:setTitle("MP3")
	frame1:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame1:setActionKey(null, "audioPlayer")
	frame1:enableBorder()
	frameBox:addFrame(frame1)

	frame2 = neutrino.CFrame()
	frame2:setPosition(box.iX, box.iY + 2 + 60, 350, 60)
	frame2:setTitle("PicViewer")
	frame2:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame2:setActionKey(null, "pictureViewer")
	frame2:enableBorder()
	frameBox:addFrame(frame2)

	frame3 = neutrino.CFrame()
	frame3:setPosition(box.iX, box.iY + 2 + 60 + 2 + 60, 350, 60)
	frame3:setTitle("MoviePlayer")
	frame3:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame3:setIconName(neutrino.NEUTRINO_ICON_MOVIE)
	frame3:setOption("spielt Movie Dateien")
	frame3:setActionKey(null, "moviePlayer")
	frame3:enableBorder()
	frameBox:addFrame(frame3)

	frame10 = neutrino.CFrame()
	frame10:setPosition(box.iX, fb:getScreenHeight() - 80 - 60, 350, 60)
	frame10:setTitle("Beenden")
	frame10:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame10:setActionKey(null, "exit")
	frame10:enableBorder()
	frameBox:addFrame(frame10)

	local m = neutrino.CWidget(box)

	m:addWidgetItem(frameBox)

	ret = m:exec(null, "")

	local actionKey = m:getActionKey()

	if actionKey == "moviePlayer" then
		moviePlayer()
	elseif actionKey == "audioPlayer" then
		audioPlayer()
	elseif actionKey == "pictureViewer" then
		pictureViewer()
	elseif actionKey == "exit" then
		return ret
	end

	if m:getExitPressed() ~= true then
		testCFrameBoxRandom()
	end

	return ret
end


-- main
function main()
	local ret = neutrino.RETURN_REPAINT
	local m = neutrino.CMenuWidget("lua sample")

	m:setWidgetMode(neutrino.MODE_MENU)
	m:enableShrinkMenu()
	m:enablePaintItemInfo(70)
	m:setItemInfoMode(neutrino.ITEMINFO_HINT_MODE)

	item1 = neutrino.CMenuForwarder("testCWidget", true, "", null, "listWidget")
	item1:setHint("lua: testing CWidget")

	item2 = neutrino.CMenuForwarder("testCMenuWidget")
	item2:setActionKey(null, "listBoxWidget")
	item2:setHint("lua: testing CMenuWidget")

	item5 = neutrino.CMenuForwarder("testCFrameBox")
	item5:setHint("lua: testing CFrameBox")
	item5:setActionKey(null, "frameBox")

	item6 = neutrino.CMenuForwarder("testActionKey/jumpTarget")
	item6:setActionKey(neutrino.CAudioPlayerSettings(), "jumpTarget")
	item6:setHint("lua: testing testActionKey/jumpTarget")

	item8 = neutrino.CMenuForwarder("testCFrameBox(2)")
	item8:setHint("lua: testing CFrameBoxRandom")
	item8:setActionKey(null, "frameBoxRandom")

	m:addItem(item1)
	m:addItem(item2)
	--m:addItem(item3)
	--m:addItem(item4)
	m:addItem(item5)
	m:addItem(item6)
	--m:addItem(item7)
	m:addItem(item8)

	if selected < 0 then
		selected = 0
	end

	m:setSelected(selected)

	ret = m:exec(None, "")

	selected = m:getSelected() 
	actionKey = m:getActionKey()

	if actionKey == "listWidget" then
		ret = testCWidget()
	elseif actionKey == "frameBox" then
		ret = testCFrameBox()
	elseif actionKey == "jumpTarget" then

	elseif actionKey == "frameBoxRandom" then
		ret = testCFrameBoxRandom()
	end

	if selected >= 0 then
		if selected == 1 then
			ret = testCMenuWidget()
		end
	end
	
	if m:getExitPressed() ~= true and ret == neutrino.RETURN_REPAINT then
		main()
	end

	return ret
end

main()





