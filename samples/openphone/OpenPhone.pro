SOURCES	+= main.cpp 
unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
FORMS	= mainwindow.ui configuration.ui 
TEMPLATE	=app
CONFIG	+= qt warn_on release
DBFILE	= OpenPhone.db
LANGUAGE	= C++
