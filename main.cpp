#include "fft_view.h"
#include "types.h"

#include <iostream>
#include <iomanip>

#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>
#include <gsl/gsl_errno.h>

#include <QApplication>
#include <QtConcurrent>
#include <QBarSeries>
#include <QBarSet>
#include <QBoxLayout>
#include <QCategoryAxis>
#include <QChartView>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QDialog>
#include <QLineSeries>
#include <QLogValueAxis>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTabWidget>
#include <QValueAxis>
#include <QVariant>

#define USE_SINE



DataSet getData(size_t length)
{
    DataSet data(length);
#ifdef USE_SINE
    quint64 now = QDateTime::currentDateTime().toMSecsSinceEpoch();
    for (size_t k = 0; k < length; k++)
    {
        double t = (1.0 / length) * k;
        double rate = sin(10 * 2 * M_PI * t) + 0.5*sin(5 * 2 * M_PI * t + M_PI_4) + 0.25 * sin(2.5 * 2 * M_PI * t - M_PI_4)
                + sin(100 * 2 * M_PI * t) + qrand() % 100/ 100.0 * sin(qrand() % 4 *2*M_PI*t);
        data.setY(k, rate);
        data.setX(k, now + t*1000);
    }
#else
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
    if (!sql.exec(query.arg(length)))
    {
        std::cerr << qPrintable(sql.lastError().text()) << std::endl;
        return 1;
    }

    int index=0;

    while(sql.next())
    {
        double time = sql.value(1).toDateTime().toMSecsSinceEpoch();
        double rate = sql.value(0).toDouble();
        data.setY(index, rate);
        data.setX(index, time);
        index++;
    }
#endif

    return data;
}

template<int successRetCode>
void throw_if_not(int retCode)
{
    if  (retCode != successRetCode)
        throw retCode;
}

Spectre FFT(QVector<double> values)
{
    int n = values.size();
    int res = gsl_fft_real_radix2_transform(values.data(), 1, n);
    throw_if_not<GSL_SUCCESS> (res);
    
    QVector<double> complex(n*2);
    res = gsl_fft_halfcomplex_radix2_unpack(values.data(), complex.data(), 1, n);
    throw_if_not<GSL_SUCCESS> (res);
    
    Spectre fft_result(n/2);
    GSL_SET_COMPLEX(&fft_result[0], complex[0], complex[1]);
    for (size_t k = 1; k < n/2; k++)
        GSL_SET_COMPLEX(&fft_result[k], complex[2 * k], complex[2 * k + 1]);
    
    return fft_result;
}

void amplitudeFilter(QVector<gsl_complex>& fft_result, double amplitude)
{
    int size = fft_result.size();
    auto filterFunc = [amplitude, size](gsl_complex& complex)
    {
        if (gsl_fcmp(gsl_complex_abs(complex) / size, amplitude, 1e-5) == -1)
            GSL_SET_COMPLEX(&complex, 0,0);
    };

    QtConcurrent::blockingMap(fft_result, filterFunc);
}

void frequencyFilter(QVector<gsl_complex>& fft_result, double frequency)
{
    auto iter = fft_result.begin();
    iter += (int)frequency;
    auto filterFunc = [](gsl_complex& complex)
    {
        GSL_SET_COMPLEX(&complex, 0, 0);
    };
    QtConcurrent::blockingMap(iter, fft_result.end(), filterFunc);
}

QVector<double> restoreSignal(const QVector<gsl_complex>& spectre)
{
    int n = spectre.size() * 2;
    QVector<double> restoredSignal(n);
    restoredSignal[0] = GSL_REAL(spectre.first());
    restoredSignal[n/2] = GSL_REAL(spectre.last());
    for (size_t k =1; k<n/2; k++)
    {
        restoredSignal[k] = GSL_REAL(spectre[k]);
        restoredSignal[n-k] = GSL_IMAG(spectre[k]);
    }

    int res = gsl_fft_halfcomplex_radix2_inverse(restoredSignal.data(), 1, n);
    throw_if_not<GSL_SUCCESS>(res);

    return restoredSignal;
}

int main(int argc, char *argv[])
{
    size_t n = 8192 / 4;// * 32;
    double amplitude_filter_value=.4;
    double frequency_filter_value = 20;

    QApplication app(argc, argv);
    QDialog w;
    FFT_View view;
    w.show();
    w.setLayout(new QVBoxLayout);
    w.layout()->addWidget(&view);


    DataSet data = getData(n);
    view.setSignalData(data);

    QVector<gsl_complex> signalSpectre = FFT(data.y());
    view.setSpectre(signalSpectre);

    amplitudeFilter(signalSpectre, amplitude_filter_value);
    frequencyFilter(signalSpectre, frequency_filter_value);
    view.setFilteredSpectre(signalSpectre);

    QVector<double> restoredSignal = restoreSignal(signalSpectre);
    view.setRestoredSignal(restoredSignal);

    return app.exec();
}
