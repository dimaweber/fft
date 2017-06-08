#include "../qcustomplot/qcustomplot.h"

#include <iostream>
#include <iomanip>
#include <vector>

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_complex_math.h>

#include <QApplication>
#include <QDialog>
#include <QDateTime>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QTabWidget>
#include <QVariant>

int main(int argc, char *argv[])
{
    size_t n = 8192;
    double T = 4;

    std::vector<double> last_rate(n);
    QVector<double> last_rate_orig(n);

    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "conn");
    db.setHostName("192.168.10.4");
    db.setDatabaseName("trade");
    db.setUserName("trader");
    db.setPassword("traderpassword");
    if (!db.open())
    {
        std::cout << qPrintable(db.lastError().text()) << std::endl;
        return 1;
    }

    QString query("Select last_rate, time from rates where currency='usd' and goods='btc' order by time desc limit %1");
    QSqlQuery sql(db);
    if (!sql.exec(query.arg(n)))
    {
        std::cout << qPrintable(sql.lastError().text()) << std::endl;
        return 1;
    }

    int index=0;
    std::vector<QDateTime> times(n);
    QVector<double> timestamp(n);
    QVector<QString>  timeLabel(n);

    double minRate = 1e20;
    double maxRate = 1e-20;
    while(sql.next())
    {
        QDateTime time = sql.value(1).toDateTime();
        double rate = sql.value(0).toDouble();
        std::cout << qPrintable(time.toString()) << " " << rate << std::endl;
        last_rate[n - index-1] = rate;
        times[n-index-1] = time;
        timestamp[n-index-1] = time.toTime_t();
        timeLabel[n-index-1] = time.toString(Qt::ISODate);
        minRate = qMin(minRate, rate);
        maxRate = qMax(maxRate, rate);
        index++;
    }
    last_rate_orig = QVector<double>::fromStdVector(last_rate);
    T = times[0].secsTo(times[n-1]);

/*
    for (size_t k = 0; k < n; k++)
    {
        double t = (T / n) * k;
        last_rate_orig[k] = sin(10 * 2 * M_PI * t) + 0.5*sin(5 * 2 * M_PI * t) + 0.25 * sin(2.5 * 2 * M_PI * t);
        last_rate[k]      = sin(10 * 2 * M_PI * t) + 0.5*sin(5 * 2 * M_PI * t) + 0.25 * sin(2.5 * 2 * M_PI * t);
    }
*/
    int res = gsl_fft_real_radix2_transform(last_rate.data(), 1, n);
    if (res != GSL_SUCCESS)
    {
        std::cout << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
        return 1;
    }

    std::vector<double> complex(n*2);

    QVector<double> freq(n/2);
    QVector<double> ampl(n/2);
    QVector<QString> label(n/2);
    double maxAmpl = 0;
    res = gsl_fft_halfcomplex_radix2_unpack(last_rate.data(), complex.data(), 1, n);
    if (res != GSL_SUCCESS)
    {
        std::cout << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
        return 1;
    }
    else
    {
        freq[0] = 0;
        ampl[0] = 1;
        label[0] = "";
        for (size_t k = 1; k < n/2; k++)
        {
            gsl_complex c;
            GSL_SET_COMPLEX(&c, complex[2 * k], complex[2 * k + 1]);
            double amplitude = gsl_complex_abs(c) * 2 / n;
            double frequency = k/T;
            if (gsl_fcmp(amplitude+1, 1, 1e-9) == 1)
                std::cout << k << ';'
                          << "     period: " << std::setw(8) << qPrintable(QString::number(1/frequency,'f', 1))  << "sec"
                          << "  amplitude: " << std::setw(8) << qPrintable(QString::number(amplitude,'f', 3))
                          << "      phase: " << std::setw(8) << qPrintable(QString::number(gsl_complex_arg(c),'f', 5)) << std::endl;
            freq[k] = frequency;
            ampl[k] = amplitude;
            label[k] = QString::number(1/frequency, 'f', 2);
            maxAmpl = qMax(maxAmpl, amplitude);
        }
    }

    QApplication app(argc, argv);
    QDialog w;
    //app.setActiveWindow(&w);
    w.show();
    w.setLayout(new QVBoxLayout);

    QTabWidget* pTab = new QTabWidget(&w);
    w.layout()->addWidget(pTab);

    QPen valuePen(QColor(qrand() % 255,qrand() % 255, qrand() % 255));
    QBrush brush(QColor(255,50,30,20));

    QCustomPlot* transformPlot = new QCustomPlot(pTab);
    QCustomPlot* originalPlot = new QCustomPlot(pTab);

    pTab->addTab(originalPlot, "Original signal");
    pTab->addTab(transformPlot, "Furie transform");

    pTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    originalPlot->plotLayout()->insertRow(0);
    originalPlot->plotLayout()->addElement(0,0,new QCPPlotTitle(transformPlot, "Spectre"));
    originalPlot->xAxis->setTickLabelRotation(60);
    originalPlot->xAxis->setSubTickCount(0);
    originalPlot->xAxis->setTickLength(0, 4);
    originalPlot->xAxis->grid()->setVisible(true);
    originalPlot->yAxis->setPadding(5);
    originalPlot->xAxis->setLabel("Time");
    originalPlot->yAxis->setLabel("Value");
    originalPlot->yAxis->setSubTickCount(0);
    originalPlot->setAntialiasedElements(QCP::aeAll);
    originalPlot->addGraph();
    originalPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
    originalPlot->graph(0)->setPen(valuePen);
    originalPlot->graph(0)->setBrush(brush);
    originalPlot->graph(0)->setData(timestamp, last_rate_orig);
    originalPlot->xAxis->setRange(timestamp[0], timestamp[timestamp.size()-1]);
    originalPlot->xAxis->setTickVector(timestamp);
    originalPlot->xAxis->setTickVectorLabels(timeLabel);
    originalPlot->yAxis->setRange(minRate * 0.99, maxRate*1.01);
    // plot->xAxis->setAutoTicks(false);
    originalPlot->xAxis->setAutoTickLabels(false);
    originalPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    transformPlot->plotLayout()->insertRow(0);
    transformPlot->plotLayout()->addElement(0,0,new QCPPlotTitle(transformPlot, "Spectre"));
    transformPlot->xAxis->setTickLabelRotation(60);
    transformPlot->xAxis->setSubTickCount(0);
    transformPlot->xAxis->setTickLength(0, 4);
    transformPlot->xAxis->grid()->setVisible(true);
    transformPlot->yAxis->setPadding(5);
    transformPlot->xAxis->setLabel("Frequency");
    transformPlot->yAxis->setLabel("Amplitude");
    transformPlot->yAxis->setSubTickCount(0);
    transformPlot->setAntialiasedElements(QCP::aeAll);

    QCPAbstractPlottable* plottable = nullptr;

    QCPBars* bars = new QCPBars(transformPlot->xAxis, transformPlot->yAxis);
    plottable = bars;
    transformPlot->addPlottable(plottable);
    bars->setWidth(1.0 / (15*n));



//    plot->graph(0)->setLineStyle(QCPGraph::lsLine);
      plottable->setPen(valuePen);
      plottable->setBrush(brush);

//    plot->graph(0)->setData(freq, ampl);
    bars->setData(freq, ampl);

    transformPlot->xAxis->setRange(freq[0] * .9, freq[freq.size()-1] * 1.01);
    transformPlot->xAxis->setTickVector(freq);
    transformPlot->xAxis->setTickVectorLabels(label);

    transformPlot->yAxis->setRange(0, maxAmpl*1.1);

   // plot->xAxis->setAutoTicks(false);
   // plot->xAxis->setAutoTickLabels(false);

    transformPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    return app.exec();
    //return EXIT_SUCCESS;
}
