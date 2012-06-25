QT       += core gui

TARGET = DebugView
TEMPLATE = app


SOURCES += main.cpp\
        debugview.cpp \
        ../aircursor.cpp

HEADERS += debugview.h \
        ../aircursor.h

INCLUDEPATH += /usr/include/ni
DEPENDPATH += /usr/include/ni

INCLUDEPATH += /usr/include/nite
DEPENDPATH += /usr/include/nite

INCLUDEPATH += /usr/include/opencv
DEPENDPATH += /usr/include/opencv

INCLUDEPATH += ../
DEPENDPATH += ../

LIBS += -lOpenNI -lXnVNite_1_5_2 -lXnVHandGenerator_1_5_2
LIBS += -lopencv_core -lopencv_imgproc


