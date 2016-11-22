TARGET = style_tests
CONFIG += console warn_on
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += ../../3party/protobuf/src

ROOT_DIR = ../..
DEPENDENCIES = map traffic indexer platform geometry coding base expat protobuf stats_client

macx-*: LIBS *= "-framework IOKit"

include($$ROOT_DIR/common.pri)

QT *= core

macx-* {
  QT *= gui widgets # needed for QApplication with event loop, to test async events (downloader, etc.)
  LIBS *= "-framework IOKit" "-framework QuartzCore" "-framework Cocoa" "-framework SystemConfiguration"
}
win32*|linux* {
  QT *= network
}

SOURCES += \
  ../../testing/testingmain.cpp \
  classificator_tests.cpp \
  dashes_test.cpp \
  style_symbols_consistency_test.cpp \

HEADERS += \
  helpers.hpp \
