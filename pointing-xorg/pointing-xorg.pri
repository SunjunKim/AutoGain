CONFIG += pointing_xorg

pointing_xorg {
  DEFINES += POINTING_XORG
  HEADERS += $$HERE/../pointing-xorg/transferfunctions/XorgFunction.h
  SOURCES += $$HERE/../pointing-xorg/transferfunctions/XorgFunction.cpp
}
