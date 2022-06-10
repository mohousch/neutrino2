from neutrino import *

class messageBox(CMessageBox):
	title = "python: CMessageBox"
	msg = "TEST"
	def __init__(self):
		CMessageBox.__init__(self, self.title, self.msg)
		self._exec(-1)

class helpBox(CHelpBox):
	line1 = "python: CHelpBox"
	line2 = "Huhu"
	line3 = "alles gut"
	def __init__(self):
		CHelpBox.__init__(self)
		self.addLine(self.line1)
		self.addLine(self.line2)
		self.addLine(self.line3)
		self.show("python: CHelpBox")

class hintBox(CHintBox):
	title = "python: CHintBox:"
	msg = "alles gut"
	def __init__(self):
		CHintBox.__init__(self, self.title, self.msg)
		self._exec()

class infoBox(CInfoBox):
	msgTitle = "pythonTest: CInfoBox"
	msgText = "first test\ntesting CInfoBox\nthat's all Folk!"
	def __init__(self):
		CInfoBox.__init__(self)
		self.setTitle(self.msgTitle)
		self.setText(self.msgTitle + "\n" + self.msgText)
		self._exec(-1)

class stringInput(CStringInputSMS):
	title = "pythonTest: CStringInputSMS"
	value = ''
	def __init__(self):
		CStringInputSMS.__init__(self, self.title, self.value)
		self._exec(None, "")

class audioPlayer(CFileBrowser):
	config = CConfigFile('\t')

	config.loadConfig(CONFIGDIR + "/neutrino.conf")

	PATH = config.getString("network_nfs_audioplayerdir")

	def __init__(self):
		CFileBrowser.__init__(self)
		fileFilter = CFileFilter()

		fileFilter.addFilter("cdr")
		fileFilter.addFilter("mp3")
		fileFilter.addFilter("m2a")
		fileFilter.addFilter("mpa")
		fileFilter.addFilter("mp2")
		fileFilter.addFilter("ogg")
		fileFilter.addFilter("wav")
		fileFilter.addFilter("flac")
		fileFilter.addFilter("aac")
		fileFilter.addFilter("dts")
		fileFilter.addFilter("m4a")

		self.Multi_Select = False
		self.Dirs_Selectable = False
		self.Filter = fileFilter

		self._exec(self.PATH)

		self.PATH = self.getCurrentDir()

		player = CAudioPlayerGui()

		if self.getSelectedFile() is not None:
			player.addToPlaylist(self.getSelectedFile())
			player._exec(None, "")

		if self.getExitPressed() is not True:
			self.__init__()

class pictureViewer(CFileBrowser):
	config = CConfigFile('\t')

	config.loadConfig(CONFIGDIR + "/neutrino.conf")

	PATH = config.getString("network_nfs_picturedir")

	def __init__(self):
		CFileBrowser.__init__(self)
		fileFilter = CFileFilter()

		fileFilter.addFilter("jpeg")
		fileFilter.addFilter("jpg")
		fileFilter.addFilter("png")
		fileFilter.addFilter("bmp")

		self.Multi_Select = False
		self.Dirs_Selectable = False
		self.Filter = fileFilter
		
		self._exec(self.PATH)

		self.PATH = self.getCurrentDir()

		player = CPictureViewerGui()
	
		if self.getSelectedFile() is not None:
			player.addToPlaylist(self.getSelectedFile())
			player._exec(None, "")
			
		if self.getExitPressed() is not True:
			self.__init__()

class moviePlayer(CFileBrowser):
	config = CConfigFile('\t')

	config.loadConfig(CONFIGDIR + "/neutrino.conf")

	PATH = config.getString("network_nfs_recordingdir")

	def __init__(self):
		CFileBrowser.__init__(self)
		fileFilter = CFileFilter()

		fileFilter.addFilter("ts")
		fileFilter.addFilter("mpg");
		fileFilter.addFilter("mpeg");
		fileFilter.addFilter("divx");
		fileFilter.addFilter("avi");
		fileFilter.addFilter("mkv");
		fileFilter.addFilter("asf");
		fileFilter.addFilter("aiff");
		fileFilter.addFilter("m2p");
		fileFilter.addFilter("mpv");
		fileFilter.addFilter("m2ts");
		fileFilter.addFilter("vob");
		fileFilter.addFilter("mp4");
		fileFilter.addFilter("mov");	
		fileFilter.addFilter("flv");	
		fileFilter.addFilter("dat");
		fileFilter.addFilter("trp");
		fileFilter.addFilter("vdr");
		fileFilter.addFilter("mts");
		fileFilter.addFilter("wmv");
		fileFilter.addFilter("wav");
		fileFilter.addFilter("flac");
		fileFilter.addFilter("mp3");
		fileFilter.addFilter("wma");
		fileFilter.addFilter("ogg");

		self.Multi_Select = False
		self.Dirs_Selectable = False
		self.Filter = fileFilter

		self._exec(self.PATH)

		self.PATH = self.getCurrentDir()

		player = CMoviePlayerGui()
	
		if self.getSelectedFile() is not None:
			player.addToPlaylist(self.getSelectedFile())
			player._exec(None, "")
			
		if self.getExitPressed() is not True:
			self.__init__()

class testMenu(CMenuTarget):
	selected = 0
	listWidget = CMenuWidget("pythonTest: CMenuWidget", NEUTRINO_ICON_MOVIE)

	def __init__(self):
		CMenuTarget.__init__
		self.showMenu()

	def showMenu(self):
		self.listWidget.setWidgetType(WIDGET_TYPE_STANDARD)
		self.listWidget.setWidgetMode(MODE_LISTBOX)
		self.listWidget.enableShrinkMenu()
		self.listWidget.addWidgetType(WIDGET_TYPE_CLASSIC)
		self.listWidget.addWidgetType(WIDGET_TYPE_EXTENDED)
		self.listWidget.addWidgetType(WIDGET_TYPE_FRAME)
		self.listWidget.enablePaintItemInfo(70)

		# messageBox
		item1 = CMenuForwarder("CMessageBox")
		item1.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item1.setHint("testing CMessageBox")
		item1.setInfo1("testing CMessageBox")

		# CHelpBox
		item2 = CMenuForwarder("CHelpBox")
		item2.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item2.setHint("testing CHelpBox")
		item2.setInfo1("testing CHelpBox")

		# CHintBox
		item3 = CMenuForwarder("CHintBox")
		item3.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item3.setHint("testing CHintBox")
		item3.setInfo1("testing CHintBox")

		# CInfoBox
		item4 = CMenuForwarder("CInfoBox")
		item4.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item4.setHint("testing CInfoBox")
		item4.setInfo1("testing CInfoBox")

		# CStringInput
		item5 = CMenuForwarder("CStringInput")
		item5.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item5.setHint("testing CStringInput")
		item5.setInfo1("testing CStringInput")

		# CAudioPlayerGui
		item6 = CMenuForwarder("CAudioPlayerGui")
		item6.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item6.setHint("testing CAudioPlayerGui")
		item6.setInfo1("testing CAudioPlayerGui")

		# CPictureViewerGui
		item7 = CMenuForwarder("CPictureViewerGui")
		item7.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item7.setHint("testing CPictureViewerGui")
		item7.setInfo1("testing CPictureViewerGui")

		# CFileBrowser | CMoviePlayerGui
		item8 = CMenuForwarder("CMoviePlayerGui")
		item8.setItemIcon(DATADIR + "/neutrino/icons/plugin.png")
		item8.setHint("testing CMoviePlayerGui")
		item8.setInfo1("testing CMoviePlayerGui")

		item = CMenuSeparator(LINE)

		self.listWidget.addItem(item1)
		self.listWidget.addItem(item2)
		self.listWidget.addItem(item3)
		self.listWidget.addItem(item4)
		self.listWidget.addItem(item)
		self.listWidget.addItem(item5)
		self.listWidget.addItem(item)
		self.listWidget.addItem(item6)
		self.listWidget.addItem(item7)
		self.listWidget.addItem(item8)

		self.listWidget.addKey(RC_info)

		self.listWidget._exec(None, "")

		self.selected = self.listWidget.getSelected()
		key = self.listWidget.getKey()

		# first handle keys
		if key == RC_info:
			infoBox()

		# handle selected line
		if self.selected == 0:
			messageBox()
		elif self.selected == 1:
			helpBox()
		elif self.selected == 2:
			hintBox()
		elif self.selected == 3:
			infoBox()
		elif self.selected == 5:
			stringInput()
		elif self.selected == 7:
			audioPlayer()
		elif self.selected == 8:
			pictureViewer()
		elif self.selected == 9:
			moviePlayer()

		# exit pressed
		if self.listWidget.getExitPressed() is False:
			self.listWidget.clearItems()
			self.showMenu()

if __name__ == "__main__":
	testMenu()



















