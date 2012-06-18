QT       += core gui

HEADERS += \
    game.h \
    item.h \
    button.h \
    aircursor.h

SOURCES += \
    game.cpp \
    main.cpp \
    item.cpp \
    button.cpp \
    aircursor.cpp

INCLUDEPATH += /usr/include/ni
DEPENDPATH += /usr/include/ni

INCLUDEPATH += /usr/include/nite
DEPENDPATH += /usr/include/nite

INCLUDEPATH += /usr/include/opencv
DEPENDPATH += /usr/include/opencv

LIBS += -lOpenNI -lXnVNite_1_5_2 -lXnVHandGenerator_1_5_2
LIBS += -lopencv_core -lopencv_imgproc
