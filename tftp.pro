TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
        tftp_client.cpp \

HEADERS += \
        tftp_client.h \

INCLUDEPATH += $${PWD}/udp_socket/

win32 {
    QMAKE_CXXFLAGS += -std=gnu++11

    SOURCES += $${PWD}/udp_socket/win_udp_socket.cpp
    LIBS += -lws2_32
}
unix {
    QMAKE_CXXFLAGS += -std=c++11
    SOURCES += $${PWD}/udp_socket/unix_udp_socket.cpp
}

# optimization
QMAKE_CXXFLAGS_RELEASE -= -O
QMAKE_CXXFLAGS_RELEASE -= -O1
QMAKE_CXXFLAGS_RELEASE -= -O2
QMAKE_CXXFLAGS_RELEASE += -O3

QMAKE_CXXFLAGS_DEBUG -= -O1
QMAKE_CXXFLAGS_DEBUG -= -O2
QMAKE_CXXFLAGS_DEBUG -= -O3
QMAKE_CXXFLAGS_DEBUG += -O
