#ifndef FFT_VIEW_H
#define FFT_VIEW_H

#include "types.h"

#include <QBrush>
#include <QPen>
#include <QWidget>

class QTabWidget;
namespace  QtCharts {
    class QChart;
    class QChartView;
    class QLineSeries;
    class QBarSet;
    class QBarSeries;
    class QCategoryAxis;
}

struct DataSet;

class FFT_View : public QWidget
{
    Q_OBJECT
    QTabWidget* pTab;
    QPen valuePen; // TODO rename to signalPen
    QPen restoredPen;
    QPen filterPen;
    QBrush brush;
    QBrush filteredBrush;

    QtCharts::QChart* pSignalChart;
    QtCharts::QChartView* pSignalChartView;
    QtCharts::QChart* pFourierTransformationChart;
    QtCharts::QChartView* pFourierTransformationView;

    QtCharts::QLineSeries* originalSeries;
    QtCharts::QLineSeries* restoredSeries;
    QtCharts::QBarSet* transformationBarSet;
    QtCharts::QBarSet* filteredBarSet;
    QtCharts::QBarSeries* fourierBars;

    QtCharts::QCategoryAxis *axisY3;
    QtCharts::QCategoryAxis *axisX3;


public:
    explicit FFT_View(QWidget *parent = nullptr);

signals:

public slots:
    void setSignalData(const DataSet& data);
    void setSpectre(const Spectre& spectre);
    void setFilteredSpectre(const Spectre& spectre);
    void setRestoredSignal(const QVector<double> &);
};

#endif // FFT_VIEW_H
