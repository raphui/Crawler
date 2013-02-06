TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c

unix:!macx:!symbian: LIBS += -lcurl -ltidy

