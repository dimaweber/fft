#include "../qcustomplot/qcustomplot.h"

#include <iostream>
#include <iomanip>
#include <vector>

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_errno.h>
#include <gsl/gsl_complex_math.h>

#include <QApplication>
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

#define USE_SINE

int main(int argc, char *argv[])
{
    size_t n = 8192;

    QApplication app(argc, argv);
    QDialog w;

    w.show();
    w.setLayout(new QVBoxLayout);

    QTabWidget* pTab = new QTabWidget(&w);
    w.layout()->addWidget(pTab);

    QPen valuePen(QColor(qrand() % 255,qrand() % 255, qrand() % 255));
    QPen restoredPen(QColor(qrand() % 255,qrand() % 255, qrand() % 255));
    QBrush brush(QColor(255,50,30,20));

    QCustomPlot* transformPlot = new QCustomPlot(pTab);
    QCustomPlot* originalPlot = new QCustomPlot(pTab);
    QCustomPlot* filteredPlot = new  QCustomPlot(pTab);

    QtCharts::QChart* pChart = new QtCharts::QChart();
    QtCharts::QChartView* pChartView = new QtCharts::QChartView(pTab);
    QtCharts::QLineSeries* originalSeries = new QtCharts::QLineSeries(pTab);
    QtCharts::QLineSeries* restoredSeries = new QtCharts::QLineSeries(pTab);

    pTab->addTab(originalPlot,  "Original signal");
    pTab->addTab(transformPlot, "Fourier transform");
    pTab->addTab(filteredPlot,  "Filtered transformation");
    pTab->addTab(pChartView, "QtChart: original signal");


    pTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    pChart->legend()->hide();

    pChartView->setRenderHint(QPainter::Antialiasing);
    pChartView->setChart(pChart);

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
    originalPlot->graph(1)->setBrush(brush);

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

    QString query("Select last_rate, time from rates where currency='usd' and goods='btc' order by time desc limit %1");
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
        std::cerr << qPrintable(time.toString()) << " " << rate << std::endl;
        last_rate[n - index-1] = rate;
        times[n-index-1] = time;
        timestamp[n-index-1] = time.toTime_t();
        timeLabel[n-index-1] = time.toString(Qt::ISODate);
        minRate = qMin(minRate, rate);
        maxRate = qMax(maxRate, rate);
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
        timestamp[k] = k;
        timeLabel[k] = QString::number(T / n * k);
        minRate = qMin(minRate, rate);
        maxRate = qMax(maxRate, rate);

        originalSeries->append(now.addMSecs(t*1000).toMSecsSinceEpoch(), rate);
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

    QVector<double> freq(n/2);
    QVector<double> ampl(n/2);
    QVector<QString> label(n/2);
    double maxAmpl = 0;
    res = gsl_fft_halfcomplex_radix2_unpack(last_rate.data(), complex.data(), 1, n);
    if (res != GSL_SUCCESS)
    {
        std::cerr << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
        return 1;
    }

    std::vector<gsl_complex> fft_result(n/2);
    freq[0] = 0;
    ampl[0] = 1;
    GSL_SET_COMPLEX(&fft_result[0], 0, 0);
    label[0] = "";
    for (size_t k = 1; k < n/2; k++)
    {
        GSL_SET_COMPLEX(&fft_result[k], complex[2 * k], complex[2 * k + 1]);
        double amplitude = gsl_complex_abs(fft_result[k]) * 2 / n;
        double frequency = k/T;
        if (gsl_fcmp(amplitude+1, 1, 1e-9) == 1)
        freq[k] = frequency;
        ampl[k] = amplitude;
        label[k] = QString::number(1/frequency, 'f', 2);
        maxAmpl = qMax(maxAmpl, amplitude);
    }


    double filter_value=.1;

    QVector<double> filtered_ampl(n/2);
    double maxFilteredAmpl = 1e-10;
    size_t maxFilteredIndex = 0;
    for (size_t k=0; k<n/2; k++)
    {
        if (gsl_fcmp(gsl_complex_abs(fft_result[k]) * 2/n, filter_value, 1e-5) == -1)
        {
            GSL_SET_COMPLEX(&fft_result[k], 0,0);
        }
        else
            maxFilteredIndex = qMax(maxFilteredIndex, k);
        filtered_ampl[k] = gsl_complex_abs(fft_result[k]) * 2/n;
        maxFilteredAmpl = qMax(maxFilteredAmpl, filtered_ampl[k]);
    }

    QVector<double> halfcomplex_filtered(n);
    for (size_t k =0; k<n/2; k++)
    {
        halfcomplex_filtered[k] = GSL_REAL(fft_result[k]);
        if (k!=0 && k != n/2)
            halfcomplex_filtered[n-k] = GSL_IMAG(fft_result[k]);
    }

    res = gsl_fft_halfcomplex_radix2_inverse(halfcomplex_filtered.data(), 1, n);
    if (res != GSL_SUCCESS)
    {
        std::cerr << "gsl error " << res << ": " << gsl_strerror(res) << std::endl;
        return 1;
    }

    for (size_t k =0; k<n; k++)
    {
        double t = (T / n) * k;
        restoredSeries->append(now.addMSecs(t*1000).toMSecsSinceEpoch(), halfcomplex_filtered[k]);
    }

    originalSeries->setColor(Qt::blue);
    pChart->addSeries(originalSeries);
    restoredSeries->setColor(Qt::green);
    pChart->addSeries(restoredSeries);

    //    pChart->createDefaultAxes();
    QtCharts::QDateTimeAxis* pChartXAxis = new QtCharts::QDateTimeAxis(pChart);
    pChartXAxis->setTickCount(10);
    pChartXAxis->setFormat("dd MM yyyy hh:mm:ss.zzz");
    pChartXAxis->setTitleText("Time");
    pChartXAxis->setLabelsAngle(60);
    pChart->addAxis(pChartXAxis, Qt::AlignBottom);
    originalSeries->attachAxis(pChartXAxis);

    QtCharts::QValueAxis* pChartYAxis = new QtCharts::QValueAxis(pChart);
    pChartYAxis->setLabelFormat("%d");
    pChartYAxis->setTitleText("Signal");
    pChart->addAxis(pChartYAxis, Qt::AlignLeft);
    originalSeries->attachAxis(pChartYAxis);
    pChart->setTitle("Original signal");

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

    filteredBars->setWidth(2.0 / (maxFilteredIndex));
    filteredBars->setData(freq, filtered_ampl);
    filteredPlot->xAxis->setRange(freq[0] * .9, freq[maxFilteredIndex] * 1.01);
    filteredPlot->xAxis->setTickVector(freq);
    filteredPlot->xAxis->setTickVectorLabels(label);
    filteredPlot->yAxis->setRange(0, maxFilteredAmpl*1.1);

    return app.exec();
    //return EXIT_SUCCESS;
}
