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
	listBox:setMenuPosition(neutrino.MENU_POSITION_LEFT)

	--listBox:setWidgetMode(neutrino.MODE_LISTBOX)
	listBox:setWidgetType(neutrino.WIDGET_TYPE_CLASSIC)

	-- CMessageBox
	item1 = neutrino.CMenuForwarder("CMessageBox", true, "", null, "msgBox")
	item1:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino.CMenuForwarder("CHelpBox")
	item2:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")
	item2:setActionKey(null, "helpBox")

	-- CHintBox
	item3 = neutrino.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")
	item3:setActionKey(null, "hintBox")

	-- CInfoBox
	item4 = neutrino.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")
	item4:setActionKey(null, "infoBox")

	-- CStringInput
	local data = ""
	item5 = neutrino.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")
	item5:setActionKey(null, "cStringInput")

	-- CAudioPlayerGui
	item6 = neutrino.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")
	item6:setActionKey(null, "audioPlayer")

	-- CPictureViewerGui
	item7 = neutrino.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")
	item7:setActionKey(null, "pictureViewer")

	-- CMoviePlayerGui
	item8 = neutrino.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
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

	testWidget:addItem(listBox)
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

	-- CMessageBox
	item1 = neutrino.CMenuForwarder("CMessageBox", true, "", self, "msgBox")
	item1:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino.CMenuForwarder("CHelpBox")
	item2:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")

	-- CHintBox
	item3 = neutrino.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")

	-- CInfoBox
	item4 = neutrino.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")

	-- CStringInput
	item5 = neutrino.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")

	-- CAudioPlayerGui
	item6 = neutrino.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")

	-- CPictureViewerGui
	item7 = neutrino.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")

	-- CMoviePlayerGui
	item8 = neutrino.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
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
	listBox:setMenuPosition(neutrino.MENU_POSITION_CENTER)

	-- CMessageBox
	item1 = neutrino.CMenuForwarder("CMessageBox", true, "", self, "msgBox")
	item1:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item1:setHint("testing CMessageBox")
	item1:setInfo1("testing CMessageBox")

	-- CHelpBox
	item2 = neutrino.CMenuForwarder("CHelpBox")
	item2:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item2:setHint("testing CHelpBox")
	item2:setInfo1("testing CHelpBox")

	-- CHintBox
	item3 = neutrino.CMenuForwarder("CHintBox")
	item3:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item3:setHint("testing CHintBox")
	item3:setInfo1("testing CHintBox")

	-- CInfoBox
	item4 = neutrino.CMenuForwarder("CInfoBox")
	item4:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item4:setHint("testing CInfoBox")
	item4:setInfo1("testing CInfoBox")

	-- CStringInput
	item5 = neutrino.CMenuForwarder("CStringInput", false)
	item5:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item5:setHint("testing CStringInput")
	item5:setInfo1("testing CStringInput")

	-- CAudioPlayerGui
	item6 = neutrino.CMenuForwarder("CAudioPlayerGui")
	item6:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item6:setHint("testing CAudioPlayerGui")
	item6:setInfo1("testing CAudioPlayerGui")

	-- CPictureViewerGui
	item7 = neutrino.CMenuForwarder("CPictureViewerGui")
	item7:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
	item7:setHint("testing CPictureViewerGui")
	item7:setInfo1("testing CPictureViewerGui")

	-- CMoviePlayerGui
	item8 = neutrino.CMenuForwarder("CMoviePlayerGui")
	item8:setHintIcon(neutrino.DATADIR .. "/neutrino/icons/features.png")
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
	--m:setMenuPosition(neutrino.MENU_POSITION_CENTER)

	m:addItem(listBox)
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

-- CWindow
function testCWindow()
	local ret = neutrino.RETURN_REPAINT

	local fb = neutrino.CFrameBuffer_getInstance()

	box = neutrino.CBox()
	box.iWidth = fb:getScreenWidth(true)
	box.iHeight = fb:getScreenHeight(true)
	box.iX = fb:getScreenX(true)
	box.iY = fb:getScreenY(true)

	local headBox = neutrino.CBox()
	headBox.iWidth = box.iWidth
	headBox.iHeight = 40
	headBox.iX = box.iX
	headBox.iY = box.iY

	local footBox = neutrino.CBox()
	footBox.iWidth = box.iWidth
	footBox.iHeight = 40
	footBox.iX = box.iX
	footBox.iY = box.iY + box.iHeight - footBox.iHeight

	local picBox = neutrino.CBox()
	picBox.iWidth = 200
	picBox.iHeight = 320
	picBox.iX = box.iX + box.iWidth - 10 - picBox.iWidth
	picBox.iY = box.iY + headBox.iHeight + 10

	local listbox = neutrino.CBox()
	listbox.iWidth = 200
	listbox.iHeight = box.iHeight - headBox.iHeight - footBox.iHeight
	listbox.iX = box.iX
	listbox.iY = box.iY + headBox.iHeight
	
	local framebox = neutrino.CBox()
	framebox.iWidth = box.iWidth - listbox.iWidth
	framebox.iHeight = box.iHeight - headBox.iHeight - footBox.iHeight
	framebox.iX = box.iX + listbox.iWidth
	framebox.iY = box.iY + headBox.iHeight

	local textbox = neutrino.CBox()
	textbox.iWidth = 350
	textbox.iHeight = 350
	textbox.iX = box.iX + box.iWidth - 10 - picBox.iWidth - 10 - 100 - 10 - textbox.iWidth
	textbox.iY = box.iY + headBox.iHeight + 10 + 60 + 25

	local pluginBox = neutrino.CBox()
	pluginBox.iWidth = 100
	pluginBox.iHeight = 60
	pluginBox.iX = box.iX + box.iWidth - 10 - picBox.iWidth - 10 - pluginBox.iWidth
	pluginBox.iY = box.iY + headBox.iHeight + 10

	local iconBox = neutrino.CBox()
	iconBox.iWidth = 100
	iconBox.iHeight = 40
	iconBox.iX = box.iX + box.iWidth - 10 - 200 - 10 - 100 - 10 - 350
	iconBox.iY = box.iY + box.iHeight - 10 - 40 - 40

	local frame1Box = neutrino.CBox()
	frame1Box.iWidth = 250
	frame1Box.iHeight = 60
	frame1Box.iX = box.iX + box.iWidth - 10 - 200 - 10 - 100 - 10 - 350 + 10 + 100
	frame1Box.iY = box.iY + box.iHeight - 10 - 40 - 60

	-- head
	head = neutrino.CHeaders(headBox, "lua: ClistBox|CFrameBox", neutrino.NEUTRINO_ICON_MOVIE)
	head:enablePaintDate()

	btn = neutrino.button_label_struct()

	btn.button = neutrino.NEUTRINO_ICON_AUDIO
	btn.localename = ""
	head:setButtons(btn)

	info = neutrino.button_label_struct()

	info.button = neutrino.NEUTRINO_ICON_BUTTON_HELP
	info.localename = ""
	head:setButtons(info)

	-- foot
	foot = neutrino.CFooters(footBox)

	red = neutrino.button_label_struct()
	red.button = neutrino.NEUTRINO_ICON_BUTTON_RED
	red.localename = "audioPlayer"
	foot:setButtons(red, 1)

	green = neutrino.button_label_struct()
	green.button = neutrino.NEUTRINO_ICON_BUTTON_GREEN
	green.localename = "pictureViewer"
	foot:setButtons(green)

	yellow = neutrino.button_label_struct()
	yellow.button = neutrino.NEUTRINO_ICON_BUTTON_YELLOW
	yellow.localename = "Focus"
	foot:setButtons(yellow)

	blue = neutrino.button_label_struct()
	blue.button = neutrino.NEUTRINO_ICON_BUTTON_BLUE
	blue.localename = "InfoBox"
	foot:setButtons(blue)

	-- frame
	frame1 = neutrino.CFrame()
	frame1:setPosition(frame1Box)
	frame1:setTitle("Mediaplayer")
	frame1:setIconName(neutrino.NEUTRINO_ICON_MOVIE)
	frame1:setOption("spielt Media Dateien")
	frame1:setActionKey(null, "frame1")

	-- Icon
	frame2 = neutrino.CFrame()
	frame2:setMode(neutrino.FRAME_ICON)
	frame2:setPosition(iconBox)
	frame2:setTitle("Exit")
	frame2:setIconName(neutrino.NEUTRINO_ICON_BUTTON_RED)
	frame2:setActionKey(null, "exit")

	-- picture
	config = neutrino.CConfigFile('\t')
	config:loadConfig(neutrino.CONFIGDIR .. "/neutrino.conf")
	local PATH = config:getString("network_nfs_recordingdir")

	local mFile = PATH .. "/ProSieben_20121225_201400.ts"

	local m_movieInfo = neutrino.CMovieInfo()

	local movieInfo = m_movieInfo:loadMovieInfo(mFile)

	frame3 = neutrino.CFrame()
	frame3:setMode(neutrino.FRAME_PICTURE)
	frame3:setPosition(picBox)
	frame3:setTitle(movieInfo.epgTitle)
	frame3:setIconName(movieInfo.tfile)
	frame3:setActionKey(null, "frame3")

	--title
	titleFrame = neutrino.CFrame()
	titleFrame:setMode(neutrino.FRAME_LABEL)
	titleFrame:setActive(false)
	titleFrame:setPosition(textbox.iX, box.iY + headBox.iHeight + 10, 350, 40)
	titleFrame:setTitle(movieInfo.epgTitle)
	titleFrame:paintMainFrame(false)
	titleFrame:setActive(false)

	--icon
	iconFrame1= neutrino.CFrame()
	iconFrame1:setMode(neutrino.FRAME_ICON)
	iconFrame1:setPosition(textbox.iX, box.iY + headBox.iHeight + 50, 25, 25)
	iconFrame1:setIconName(neutrino.NEUTRINO_ICON_STAR_ON)
	iconFrame1:paintMainFrame(false)
	iconFrame1:setActive(false)

	iconFrame2= neutrino.CFrame()
	iconFrame2:setMode(neutrino.FRAME_ICON)
	iconFrame2:setPosition(textbox.iX + 25, box.iY + headBox.iHeight + 50, 25, 25)
	iconFrame2:setIconName(neutrino.NEUTRINO_ICON_STAR_ON)
	iconFrame2:paintMainFrame(false)
	iconFrame2:setActive(false)

	iconFrame3= neutrino.CFrame()
	iconFrame3:setMode(neutrino.FRAME_ICON)
	iconFrame3:setPosition(textbox.iX + 25 + 25, box.iY + headBox.iHeight + 50, 25, 25)
	iconFrame3:setIconName(neutrino.NEUTRINO_ICON_STAR_ON)
	iconFrame3:paintMainFrame(false)
	iconFrame3:setActive(false)

	iconFrame4= neutrino.CFrame()
	iconFrame4:setMode(neutrino.FRAME_ICON)
	iconFrame4:setPosition(textbox.iX + 25 +25 + 25, box.iY + headBox.iHeight + 50, 25, 25)
	iconFrame4:setIconName(neutrino.NEUTRINO_ICON_STAR_OFF)
	iconFrame4:paintMainFrame(false)
	iconFrame4:setActive(false)

	-- Text
	frame4 = neutrino.CFrame()
	frame4:setMode(neutrino.FRAME_TEXT)
	frame4:setPosition(textbox)
	--frame4:setBackgroundColor(0xFFAAAA)
	frame4:setTitle(movieInfo.epgInfo1 .. "\n" .. movieInfo.epgInfo2)
	frame4:setActionKey(null, "frame4")
	frame4:paintMainFrame(false)
	frame4:setActive(false)

	-- plugin
	frame5 = neutrino.CFrame()
	frame5:setMode(neutrino.FRAME_PLUGIN)
	frame5:setPosition(pluginBox)
	frame5:setTitle("nfilm")
	frame5:setPlugin("nfilm")
	--frame5:showPluginName()
	--frame5:disableShadow()

	-- vframe
	vframe = neutrino.CFrame()
	vframe:setMode(neutrino.FRAME_VLINE)
	vframe:setPosition(box.iX + listbox.iWidth + 10, box.iY + 50, 5, box.iHeight- 100)

	testFrame = neutrino.CFrameBox(framebox)

	testFrame:addFrame(titleFrame)
	testFrame:addFrame(iconFrame1)
	testFrame:addFrame(iconFrame2)
	testFrame:addFrame(iconFrame3)
	testFrame:addFrame(iconFrame4)
	testFrame:addFrame(frame4)
	testFrame:addFrame(frame5)
	testFrame:addFrame(frame3)
	testFrame:addFrame(frame2)
	testFrame:addFrame(frame1)
	testFrame:addFrame(vframe)

	--listbox
	listBox = neutrino.ClistBox(listbox)
	listBox:enablePaintHead()
	listBox:setTitle("listBox", neutrino.NEUTRINO_ICON_MOVIE)
	listBox:setHeadGradient(neutrino.NOGRADIENT)
	listBox:setOutFocus()

	listBox:addItem(neutrino.CMenuForwarder("back"))
	listBox:addItem(neutrino.CMenuSeparator(neutrino.LINE))
	listBox:addItem(neutrino.ClistBoxItem("item1", true, "", neutrino.CAudioPlayerSettings(), "jumpTarget"))
	listBox:addItem(neutrino.ClistBoxItem("item2"))
	listBox:addItem(neutrino.ClistBoxItem("item3"))
	listBox:addItem(neutrino.CMenuSeparator(neutrino.LINE))
	listBox:addItem(neutrino.ClistBoxItem("item4"))
	listBox:addItem(neutrino.ClistBoxItem("item5"))
	listBox:addItem(neutrino.CMenuSeparator(neutrino.LINE))
	listBox:addItem(neutrino.CMenuSeparator())
	listBox:addItem(neutrino.CMenuSeparator())
	listBox:addItem(neutrino.CMenuSeparator())
	listBox:addItem(neutrino.CMenuSeparator())
	listBox:addItem(neutrino.CMenuSeparator())
	listBox:addItem(neutrino.CMenuSeparator(neutrino.LINE))
	listBox:addItem(neutrino.ClistBoxItem("Exit", true, "", null, "exit"))

	local m = neutrino.CWidget(box)

	if selected < 0 then
		selected = 0
	end
	
	m:setSelected(selected)

	m:addItem(head)
	m:addItem(listBox)
	m:addItem(testFrame)
	m:addItem(foot)

	m:addKey(neutrino.RC_red, null, "audioPlayer")
	m:addKey(neutrino.RC_green, null, "pictureViewer")
	--m:addKey(neutrino.RC_yellow, null, "moviePlayer")
	m:addKey(neutrino.RC_blue, null, "infoBox")
	m:addKey(neutrino.RC_info, null, "infoBox")
	m:addKey(neutrino.RC_audio, null, "infoBox")

	ret = m:exec(null, "")

	local actionKey = m:getActionKey()

	if actionKey == "moviePlayer" then
		print("lua sample: testCWindow(): actionKey: moviePlayer")

		audioPlayer()
	elseif actionKey == "pictureViewer" then
		print("lua sample: testCWindow(): actionKey: pictureViewer")

		pictureViewer()
	elseif actionKey == "audioPlayer" then
		print("lua sample: testCWindow(): actionKey: audioPlayer")
		
		audioPlayer()
	elseif actionKey == "frame1" then
		print("lua sample: testCWindow(): actionKey: frame1")

		moviePlayer()
	elseif actionKey == "exit" then
		print("lua sample: testCWindow(): actionKey: exit")

		return neutrino.RETURN_REPAINT
	elseif actionKey == "frame3" then
		print("lua sample: testCWindow(): actionKey: frame3")

		--funArt()
		movieWidget = neutrino.CMovieInfoWidget()
		movieWidget:setMovie(movieInfo)

		movieWidget:exec(null, "")

	elseif actionKey == "nfilm" then
		print("lua sample: testCWindow(): actionKey: nfilm")
		neutrino.g_PluginList:startPlugin("nfilm")
	elseif actionKey == "infoBox" then
		print("lua sample: testCWindow(): actionKey: infoBox")
		
		infoBox()
	elseif actionKey == "frame4" then
		print("lua sample: testCWindow(): actionKey: frame4")

		neutrino.InfoBox(title, "lua window|widget")
	end

	if m:getExitPressed() ~= true then
		testCWindow()
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
	frame1:enableShadow()
	frameBox:addFrame(frame1)

	frame2 = neutrino.CFrame()
	frame2:setPosition(box.iX + box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame2:setTitle("PicViewer")
	frame2:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame2:setActionKey(null, "pictureViewer")
	frame2:enableShadow()
	frameBox:addFrame(frame2)

	frame3 = neutrino.CFrame()
	frame3:setPosition(box.iX + 2*box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame3:setTitle("MoviePlayer")
	frame3:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame3:setIconName(neutrino.NEUTRINO_ICON_MOVIE)
	frame3:setOption("spielt Movie Dateien")
	frame3:setActionKey(null, "moviePlayer")
	frame3:enableShadow()
	frameBox:addFrame(frame3)

	frame4 = neutrino.CFrame()
	frame4:setPosition(box.iX + 3*box.iWidth/4, box.iY, box.iWidth/4, box.iHeight)
	frame4:setTitle("Beenden")
	frame4:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame4:setActionKey(null, "exit")
	frame4:enableShadow()
	frameBox:addFrame(frame4)

	local m = neutrino.CWidget()
	m:addItem(frameBox)

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

	local box = neutrino.CBox()
	local fb = neutrino.CFrameBuffer_getInstance()

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
	frame1:enableShadow()
	frameBox:addFrame(frame1)

	frame2 = neutrino.CFrame()
	frame2:setPosition(box.iX, box.iY + 2 + 60, 350, 60)
	frame2:setTitle("PicViewer")
	frame2:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame2:setActionKey(null, "pictureViewer")
	frame2:enableShadow()
	frameBox:addFrame(frame2)

	frame3 = neutrino.CFrame()
	frame3:setPosition(box.iX, box.iY + 2 + 60 + 2 + 60, 350, 60)
	frame3:setTitle("MoviePlayer")
	frame3:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame3:setIconName(neutrino.NEUTRINO_ICON_MOVIE)
	frame3:setOption("spielt Movie Dateien")
	frame3:setActionKey(null, "moviePlayer")
	frame3:enableShadow()
	frameBox:addFrame(frame3)

	frame10 = neutrino.CFrame()
	frame10:setPosition(box.iX, fb:getScreenHeight() - 80 - 60, 350, 60)
	frame10:setTitle("Beenden")
	frame10:setHAlign(neutrino.CC_ALIGN_CENTER)
	frame10:setActionKey(null, "exit")
	frame10:enableShadow()
	frameBox:addFrame(frame10)

	local m = neutrino.CWidget(box)

	m:paintMainFrame(true)

	--m:addItem(window)
	m:addItem(frameBox)

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
		testCFrameBoxRandom()
	end

	return ret
end

function movieBrowser()
	local ret = neutrino.RETURN_REPAINT

	local menu = neutrino.CMenuWidget("Movie Browser", neutrino.NEUTRINO_ICON_MOVIE)
	menu:setWidgetType(neutrino.WIDGET_TYPE_FRAME)
	menu:setItemsPerPage(6, 2)
	menu:enablePaintDate()

	-- head
	info = neutrino.button_label_struct()

	info.button = neutrino.NEUTRINO_ICON_BUTTON_HELP
	info.localename = ""
	menu:setHeadButtons(info)

	btn = neutrino.button_label_struct()

	btn.button = neutrino.NEUTRINO_ICON_BUTTON_MUTE_SMALL
	btn.localename = ""
	menu:setHeadButtons(btn)

	-- foot
	btnRed = neutrino.button_label_struct()

	btnRed.button = neutrino.NEUTRINO_ICON_BUTTON_RED
	btnRed.localename = "delete all"
	menu:setFootButtons(btnRed)

	btnGreen = neutrino.button_label_struct()

	btnGreen.button = neutrino.NEUTRINO_ICON_BUTTON_GREEN
	btnGreen.localename = "Add"
	menu:setFootButtons(btnGreen)

	if selected < 0 then
		selected = 0
	end

	menu:setSelected(selected)
	
	local item = nil

	local ret = neutrino.RETURN_REPAINT
	local selected = 0

	-- load movies
	local fileBrowser = neutrino.CFileBrowser()
	local fh = neutrino.CFileHelpers()
	local fileFilter = neutrino.CFileFilter()

	config = neutrino.CConfigFile('\t')

	config:loadConfig(neutrino.CONFIGDIR .. "/neutrino.conf")

	local PATH = config:getString("network_nfs_recordingdir") .. "/"

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

	-- fill items
	fileBrowser.Filter = fileFilter

	local filelist = {}
	filelist = fh:readDir(PATH, fileFilter)

	--print(filelist.Name)

	menu:exec(null, "")

	if menu:getExitPressed() ~= true then
		movieBrowser()
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

	--item3 = neutrino.CMenuForwarder("testClistBox")
	--item3:setHint("lua: testing ClistBox")

	--item4 = neutrino.CMenuForwarder("testCWidget (ClistBox|CFrameBox)")
	--item4:setHint("lua: testing ClistBox|CFrameBox")

	item5 = neutrino.CMenuForwarder("testCFrameBox")
	item5:setHint("lua: testing CFrameBox")
	item5:setActionKey(null, "frameBox")

	item6 = neutrino.CMenuForwarder("testActionKey/jumpTarget")
	item6:setActionKey(neutrino.CAudioPlayerSettings(), "jumpTarget")
	item6:setHint("lua: testing testActionKey/jumpTarget")
	
	--item7 = neutrino.CMenuForwarder("movieBrowser", true, "", self, "movieBrowser")

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

--[[
	elseif actionKey == "movieBrowser" then
		ret = movieBrowser()
]]
	elseif actionKey == "frameBoxRandom" then
		ret = testCFrameBoxRandom()
	end

	if selected >= 0 then
		if selected == 1 then
			ret = testCMenuWidget()
		--elseif selected == 2 then
			--ret = testClistBox()
		--elseif selected == 3 then
			--ret = testCWindow()
		end
	end
	
	if m:getExitPressed() ~= true and ret == neutrino.RETURN_REPAINT then
		main()
	end

	return ret
end

main()





