# pointing/pointing.pri --
#
# Initial software
# Authors: Nicolas Roussel
# Copyright © INRIA

#message("Using pointing.pri")

CONFIG += link_prl

INCLUDEPATH += $$POINTING
DEPENDPATH  += $$POINTING/pointing

include(pointing-common.pri)

macx {
  LIBS += -L$$POINTING/pointing -lpointing
  POST_TARGETDEPS += $$POINTING/pointing/libpointing.a
}

unix:!macx {
  LIBS += -L$$POINTING/pointing -lpointing
  POST_TARGETDEPS += $$POINTING/pointing/libpointing.a
}

windows {

  win32-msvc* {
    CONFIG(debug, debug|release){
      LIBS += -L$$POINTING/pointing/debug -lpointing
      PRE_TARGETDEPS += $$POINTING/pointing/debug/pointing.lib
    }else{
      LIBS += -L$$POINTING/pointing/release -lpointing
      PRE_TARGETDEPS += $$POINTING/pointing/release/pointing.lib
    }
  } else {
    CONFIG(debug, debug|release){
      LIBS += -L$$POINTING/pointing/debug -lpointing
      PRE_TARGETDEPS += $$POINTING/pointing/debug/libpointing.a
    }else{
      LIBS += -L$$POINTING/pointing/release -lpointing
      PRE_TARGETDEPS += $$POINTING/pointing/release/libpointing.a
    }
  }
}
