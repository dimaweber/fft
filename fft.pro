TEMPLATE=app

QT += sql widgets gui printsupport

SOURCES += main.cpp \
qcustomplot/qcustomplot.cpp

LIBS += -lgsl -lgslcblas

HEADERS += qcustomplot/qcustomplot.h

win32 {
    INCLUDEPATH += c:/local/gsl/include
    LIBS += -Lc:/local/gsl/lib
}
