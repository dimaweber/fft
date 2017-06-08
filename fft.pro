TEMPLATE=app

QT += sql widgets gui printsupport

SOURCES += ConsoleApplication1/ConsoleApplication1.cpp \
qcustomplot/qcustomplot.cpp

LIBS += -lgsl -lgslcblas

HEADERS += qcustomplot/qcustomplot.h
