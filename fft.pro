TEMPLATE=app

QT += sql widgets gui charts concurrent

SOURCES += main.cpp \
    fft_view.cpp

LIBS += -lgsl -lgslcblas


win32 {
    INCLUDEPATH += c:/local/gsl/include
    LIBS += -Lc:/local/gsl/lib
}

HEADERS += \
    fft_view.h \
    types.h
