#ifndef QT_CHARTS
#include "qcustomplot/qcustomplot.h"
#endif

#include <iostream>
#include <iomanip>
#include <vector>

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_complex_math.h>

#include <QApplication>
#include <QAreaSeries>
#include <QBarSeries>
#include <QBarSet>
#include <QChart>
#include <QChartView>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QDialog>
#include <QLineSeries>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTabWidget>
#include <QValueAxis>
#include <QVariant>

//#define USE_SINE
//#define QTCHARTS

int main(int argc, char *argv[])
{
    size_t n = 8192;// * 32;
    double filter_value=.4;

    QApplication app(argc, argv);
    QDialog w;

    w.show();
    w.setLayout(new QVBoxLayout);

    QTabWidget* pTab = new QTabWidget(&w);
    w.layout()->addWidget(pTab);

    QPen valuePen(QColor(qrand() % 255,qrand() % 255, qrand() % 255, 128));
    QPen restoredPen(QColor(qrand() % 255,qrand() % 255, qrand() % 255));
    restoredPen.setWidth(2);

    QBrush brush(QColor(255,50,30,20));
    QBrush filteredBrush(QColor(255,30,50,20));

#ifdef QTCHARTS
    QtCharts::QChart* pSignalChart = new QtCharts::QChart();
    QtCharts::QChartView* pSignalChartView = new QtCharts::QChartView(pTab);
    QtCharts::QChart* pFourierTransformationChart = new QtCharts::QChart();
    QtCharts::QChartView* pFourierTransformationView = new QtCharts::QChartView(pTab);

    QtCharts::QLineSeries* originalSeries = new QtCharts::QLineSeries(pTab);
    QtCharts::QLineSeries* restoredSeries = new QtCharts::QLineSeries(pTab);
    QtCharts::QBarSet* transformationBarSet = new QtCharts::QBarSet("Spectre", pTab);
    QtCharts::QBarSet* filteredBarSet = new QtCharts::QBarSet("Filtered Spectre", pTab);
    QtCharts::QBarSeries* fourierBars = new QtCharts::QBarSeries(pTab);
    pTab->addTab(pSignalChartView, "QtChart: original signal");
    pTab->addTab(pFourierTransformationView, "QtChart: fourier transformation");

    //    pSignalChart->legend()->hide();
    pSignalChartView->setRenderHint(QPainter::Antialiasing);
    pFourierTransformationView->setRenderHint(QPainter::Antialiasing);
    pSignalChartView->setChart(pSignalChart);
    pFourierTransformationView->setChart(pFourierTransformationChart);
#else
    QCustomPlot* transformPlot = new QCustomPlot(pTab);
    QCustomPlot* originalPlot = new QCustomPlot(pTab);
    QCustomPlot* filteredPlot = new  QCustomPlot(pTab);
    pTab->addTab(originalPlot,  "Original signal");
    pTab->addTab(transformPlot, "Fourier transform");
    pTab->addTab(filteredPlot,  "Filtered transformation");

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
    // plot->xAxis->setAutoTicks(false);
    originalPlot->xAxis->setAutoTickLabels(false);
    originalPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    originalPlot->addGraph();
    originalPlot->graph(0)->setLineStyle(QCPGraph::lsLine);
    originalPlot->graph(0)->setPen(valuePen);
    originalPlot->graph(0)->setBrush(brush);

    originalPlot->addGraph();
    originalPlot->graph(1)->setLineStyle(QCPGraph::lsLine);
    originalPlot->graph(1)->setPen(restoredPen);
    originalPlot->graph(1)->setBrush(filteredBrush);

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
    QCPBars* bars = new QCPBars(transformPlot->xAxis, transformPlot->yAxis);
    transformPlot->addPlottable(bars);
    bars->setWidth(1.0 / (15*n));
    bars->setPen(valuePen);
    bars->setBrush(brush);
    // plot->xAxis->setAutoTicks(false);
    // plot->xAxis->setAutoTickLabels(false);
    transformPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    filteredPlot->plotLayout()->insertRow(0);
    filteredPlot->plotLayout()->addElement(0,0,new QCPPlotTitle(filteredPlot, "Filtered spectre"));
    filteredPlot->xAxis->setTickLabelRotation(60);
    filteredPlot->xAxis->setSubTickCount(0);
    filteredPlot->xAxis->setTickLength(0, 4);
    filteredPlot->xAxis->grid()->setVisible(true);
    filteredPlot->yAxis->setPadding(5);
    filteredPlot->xAxis->setLabel("Frequency");
    filteredPlot->yAxis->setLabel("Amplitude");
    filteredPlot->yAxis->setSubTickCount(0);
    filteredPlot->setAntialiasedElements(QCP::aeAll);
    QCPBars* filteredBars = new QCPBars(filteredPlot->xAxis, filteredPlot->yAxis);
    filteredPlot->addPlottable(filteredBars);
    filteredBars->setPen(valuePen);
    filteredBars->setBrush(brush);
    // plot->xAxis->setAutoTicks(false);
    // plot->xAxis->setAutoTickLabels(false);
    filteredPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);

    QVector<double> freq(n/2);
    QVector<double> ampl(n/2);
    QVector<QString> label(n/2);

#endif

    pTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);



    double T = 4;

    std::vector<double> last_rate(n);
    QVector<double> last_rate_orig(n);
    QVector<double> timestamp(n);
    QVector<QString>  timeLabel(n);
    double minRate = 1e20;
    double maxRate = 1e-20;

#ifndef USE_SINE
    QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL", "conn");
    db.setHostName("192.168.10.4");
    db.setDatabaseName("trade");
    db.setUserName("trader");
    db.setPassword("traderpassword");
    if (!db.open())
    {
        std::cerr << qPrintable(db.lastError().text()) << std::endl;
        return 1;
    }

    QString query("select * from (Select last_rate, time from rates where currency='usd' and goods='btc' order by time desc limit %1) A order by time asc");
    QSqlQuery sql(db);
    if (!sql.exec(query.arg(n)))
    {
        std::cerr << qPrintable(sql.lastError().text()) << std::endl;
        return 1;
    }

    int index=0;

    std::vector<QDateTime> times(n);
    while(sql.next())
    {
        QDateTime time = sql.value(1).toDateTime();
        double rate = sql.value(0).toDouble();
        last_rate[index] = rate;
        minRate = qMin(minRate, rate);
        maxRate = qMax(maxRate, rate);
#ifdef QTCHARTS
        originalSeries->append(QPointF(time.toMSecsSinceEpoch(), rate));
#else
        times[index] = time;
        timestamp[index] = time.toTime_t();
        timeLabel[index] = time.toString(Qt::ISODate);
#endif
        index++;
    }
    T = times[0].secsTo(times[n-1]);
#else
    QDateTime now = QDateTime::currentDateTime();
    for (size_t k = 0; k < n; k++)
    {
        double t = (T / n) * k;
        double rate = sin(10 * 2 * M_PI * t) + 0.5*sin(5 * 2 * M_PI * t + M_PI_4) + 0.25 * sin(2.5 * 2 * M_PI * t - M_PI_4)+ /*qrand() % 100/ 100.0 **/ sin(qrand() % 4 *2*M_PI*t);
        last_rate[k] = rate;
        minRate = qMin(minRate, rate);
        maxRate = qMax(maxRate, rate);
#ifdef QTCHARTS
        originalSeries->append(now.addMSecs(t*1000).toMSecsSinceEpoch(), rate);
#else
        timestamp[k] = k;
        timeLabel[k] = QString::number(T / n * k);
#endif
    }
#endif

    last_rate_orig = QVector<double>::fromStdVector(last_rate);
    int res = gsl_fft_real_radix2_transform(last_rate.data(), 1, n);
    if (res != GSL_SUCCESS)
    {
        std::cerr << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
        return 1;
    }

    std::vector<double> complex(n*2);

    double maxAmpl = 0;
    res = gsl_fft_halfcomplex_radix2_unpack(last_rate.data(), complex.data(), 1, n);
    if (res != GSL_SUCCESS)
    {
        std::cerr << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
        return 1;
    }

    std::vector<gsl_complex> fft_result(n/2);
    GSL_SET_COMPLEX(&fft_result[0], complex[0], complex[1]);
#ifdef QTCHARTS
    transformationBarSet->append(1);
#else
    label[0] = "";
    freq[0] = 0;
    ampl[0] = 1;
#endif
    for (size_t k = 1; k < n/2; k++)
    {
        GSL_SET_COMPLEX(&fft_result[k], complex[2 * k], complex[2 * k + 1]);
        double amplitude = gsl_complex_abs(fft_result[k]) * 2 / n;
        if (gsl_fcmp(amplitude+1, 1, 1e-9) == 1)
        {
            maxAmpl = qMax(maxAmpl, amplitude);
#ifdef QTCHARTS
            transformationBarSet->append(amplitude);
#else
            double frequency = k/T;
            freq[k] = frequency;
            ampl[k] = amplitude;
            label[k] = QString::number(1/frequency, 'f', 2);
#endif
        }
    }

    QVector<double> filtered_ampl(n/2);
    double maxFilteredAmpl = 1e-10;
    size_t maxFilteredIndex = 1;

#ifdef QTCHARTS
    filteredBarSet->append(1);
#else
    filtered_ampl[0] = 1;
#endif
    for (size_t k=1; k<n/2; k++)
    {
        if (gsl_fcmp(gsl_complex_abs(fft_result[k]) * 2/n, filter_value, 1e-5) == -1)
        {
            GSL_SET_COMPLEX(&fft_result[k], 0,0);
        }
        else
            maxFilteredIndex = qMax(maxFilteredIndex, k);
        double ampl = gsl_complex_abs(fft_result[k]) * 2/n;
#ifdef QTCHARTS
        filteredBarSet->append(ampl);
#else
        filtered_ampl[k] = ampl;
#endif
        maxFilteredAmpl = qMax(maxFilteredAmpl, ampl);
    }

    QVector<double> halfcomplex_filtered(n);
    halfcomplex_filtered[0] = GSL_REAL(fft_result[0]);
    halfcomplex_filtered[n/2] = GSL_REAL(fft_result[n/2]);
    for (size_t k =1; k<n/2; k++)
    {
        halfcomplex_filtered[k] = GSL_REAL(fft_result[k]);
        halfcomplex_filtered[n-k] = GSL_IMAG(fft_result[k]);
    }

    res = gsl_fft_halfcomplex_radix2_inverse(halfcomplex_filtered.data(), 1, n);
    if (res != GSL_SUCCESS)
    {
        std::cerr << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
        return 1;
    }


#ifdef QTCHARTS
    for (size_t k =0; k<n; k++)
    {
        restoredSeries->append(originalSeries->at(k).x(), halfcomplex_filtered[k]);
    }

    originalSeries->setName("Signal");
    originalSeries->setPen(valuePen);
    originalSeries->setBrush(brush);
    pSignalChart->addSeries(originalSeries);
    restoredSeries->setPen(restoredPen);
    restoredSeries->setName("Filtered/Restored signal");
    pSignalChart->addSeries(restoredSeries);
    fourierBars->append(transformationBarSet);
    fourierBars->append(filteredBarSet);
    transformationBarSet->setBrush(brush);
    transformationBarSet->setPen(valuePen);
    filteredBarSet->setPen(restoredPen);
    filteredBarSet->setBrush(filteredBrush);
    pFourierTransformationChart->addSeries(fourierBars);
    pFourierTransformationChart->createDefaultAxes();

    //    pChart->createDefaultAxes();
    QtCharts::QDateTimeAxis* pChartXAxis = new QtCharts::QDateTimeAxis(pSignalChart);
    pChartXAxis->setTickCount(10);
    pChartXAxis->setFormat("dd MM yyyy hh:mm:ss.zzz");
    pChartXAxis->setTitleText("Time");
    pChartXAxis->setLabelsAngle(60);
    pSignalChart->addAxis(pChartXAxis, Qt::AlignBottom);
    originalSeries->attachAxis(pChartXAxis);
    restoredSeries->attachAxis(pChartXAxis);

    QtCharts::QValueAxis* pChartYAxis = new QtCharts::QValueAxis(pSignalChart);
    pChartYAxis->setLabelFormat("%d");
    pChartYAxis->setTitleText("Signal");
    pChartYAxis->setRange(minRate*.99, maxRate * 1.01);
    pSignalChart->addAxis(pChartYAxis, Qt::AlignLeft);
    originalSeries->attachAxis(pChartYAxis);
    restoredSeries->attachAxis(pChartYAxis);
    pSignalChart->setTitle("Original signal");
#else
    originalPlot->graph(0)->setData(timestamp, last_rate_orig);
    originalPlot->graph(1)->setData(timestamp, halfcomplex_filtered);
    originalPlot->xAxis->setRange(timestamp[0], timestamp[timestamp.size()-1]);
    originalPlot->xAxis->setTickVector(timestamp);
    originalPlot->xAxis->setTickVectorLabels(timeLabel);
    originalPlot->yAxis->setRange(minRate * 0.99, maxRate*1.01);

    bars->setData(freq, ampl);
    transformPlot->xAxis->setRange(freq[0] * .9, freq[freq.size()-1] * 1.01);
    transformPlot->xAxis->setTickVector(freq);
    transformPlot->xAxis->setTickVectorLabels(label);
    transformPlot->yAxis->setRange(0, maxAmpl*1.1);

    filteredBars->setWidth(1.0 / (15*n));
    filteredBars->setData(freq, filtered_ampl);
    filteredPlot->xAxis->setRange(freq[0] * .9, freq[maxFilteredIndex] * 1.01);
    filteredPlot->xAxis->setTickVector(freq);
    filteredPlot->xAxis->setTickVectorLabels(label);
    filteredPlot->yAxis->setRange(0, maxFilteredAmpl*1.1);
#endif
    return app.exec();
    //return EXIT_SUCCESS;
}
