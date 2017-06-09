#include "fft_view.h"

#include <QChart>
#include <QTabWidget>

FFT_View::FFT_View(QWidget *parent)
    : QWidget(parent),
      pTab (new QTabWidget(this)),
      valuePen(Qt::brown),
      restoredPen(Qt::magenta),
      filterPen (Qt::black),
      brush(QColor(255,50,30,20)),
      filteredBrush(QColor(255,30,50,20))
{
      restoredPen.setWidth(2);
      filterPen.setStyle(Qt::DashLine);

      pSignalChart = new QtCharts::QChart();
      pSignalChartView = new QtCharts::QChartView(this);
      pFourierTransformationChart = new QtCharts::QChart();
      pFourierTransformationView = new QtCharts::QChartView(this);

      originalSeries = new QtCharts::QLineSeries(this);
      restoredSeries = new QtCharts::QLineSeries(this);
      transformationBarSet = new QtCharts::QBarSet("Spectre", this);
      filteredBarSet = new QtCharts::QBarSet("Filtered Spectre", this);
      fourierBars = new QtCharts::QBarSeries(this);

      fourierBars->append(transformationBarSet);
      fourierBars->append(filteredBarSet);

      axisY3 = new QtCharts::QCategoryAxis(pFourierTransformationChart);

      axisY3->append("Out", amplitude_filter_value);
      axisY3->append("In", 1000);
      axisY3->setGridLinePen(filterPen);
      axisX3->append("In", frequency_filter_value);
      axisX3->append("Out", 100000);
      axisX3->setGridLinePen(filterPen);


      pSignalChartView->setRenderHint(QPainter::Antialiasing);
      pFourierTransformationView->setRenderHint(QPainter::Antialiasing);
      pSignalChartView->setChart(pSignalChart);
      pFourierTransformationView->setChart(pFourierTransformationChart);

      pTab->addTab(pSignalChartView, "QtChart: original signal");
      pTab->addTab(pFourierTransformationView, "QtChart: fourier transformation");
      pTab->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

      transformationBarSet->setBrush(brush);
      transformationBarSet->setPen(valuePen);
      filteredBarSet->setPen(restoredPen);
      filteredBarSet->setBrush(filteredBrush);

      pFourierTransformationChart->addSeries(fourierBars);
      pFourierTransformationChart->addAxis(axisY3, Qt::AlignLeft);
      pFourierTransformationChart->addAxis(axisX3, Qt::AlignBottom);
      fourierBars->attachAxis(axisX3);
      fourierBars->attachAxis(axisY3);


      QtCharts::QValueAxis* pSpectreXAxis = new QtCharts::QValueAxis(pFourierTransformationChart);
      pSpectreXAxis->setTitleText("Frequency");
      pSpectreXAxis->setLabelFormat("%d");
  //    pSpectreXAxis->setTickCount(fft_result.size() / 1000);
  //    pSpectreXAxis->setMinorTickCount(10);
      pSpectreXAxis->setLabelsAngle(60);
      pFourierTransformationChart->addAxis(pSpectreXAxis, Qt::AlignBottom);
      fourierBars->attachAxis(pSpectreXAxis);

      QtCharts::QValueAxis* pSpectreYAxis = new QtCharts::QValueAxis(pFourierTransformationChart);
  //    pSpectreYAxis->setBase(10);
  //    pSpectreYAxis->setRange(-10,10);
      pSpectreYAxis->setTitleText("Amplitude");
      pSpectreYAxis->setLabelFormat("%g");
      pSpectreYAxis->setMinorTickCount(-1);
      pFourierTransformationChart->addAxis(pSpectreYAxis, Qt::AlignLeft);
      fourierBars->attachAxis(pSpectreYAxis);




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
      pChartYAxis->setRange(data.minY, data.maxY);
      pSignalChart->addAxis(pChartYAxis, Qt::AlignLeft);
      originalSeries->attachAxis(pChartYAxis);
      restoredSeries->attachAxis(pChartYAxis);
      pSignalChart->setTitle("Original signal");
}

void FFT_View::setSignalData(const DataSet &data)
{
    originalSeries->replace(data.vec);

    originalSeries->setName("Signal");
    originalSeries->setPen(valuePen);
    originalSeries->setBrush(brush);
    pSignalChart->addSeries(originalSeries);

}

void FFT_View::setRestoredSignal(const QVector<double>& restoredSignal)
{
    QVector<QPointF> restoredPoints = data.vec;
    for (size_t k =0; k<n; k++)
    {
        restoredPoints[k].setY( restoredSignal[k]);
    }
    restoredSeries->replace(restoredPoints);

    restoredSeries->setPen(restoredPen);
    restoredSeries->setName("Filtered/Restored signal");
    pSignalChart->addSeries(restoredSeries);

}

vois FFT_View::setFilteredSpectre(const Spectre &spectre)
{
    filteredBarSet->append(0);
    for (size_t k=1; k<n/2; k++)
    {
        double ampl = gsl_complex_abs(spectre[k]) * 2/n;
        filteredBarSet->append(ampl);
    }
}

void FFT_View::setSpectre(const QVector<gsl_complex> spectre)
{
    transformationBarSet->clear();
    transformationBarSet->append(0);
    for (size_t k = 1; k < n/2; k++)
    {
        double amplitude = gsl_complex_abs(signalSpectre[k]) * 2 / n;
        transformationBarSet->append(amplitude);
    }
}
