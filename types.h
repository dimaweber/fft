#ifndef TYPES_H
#define TYPES_H

#include <QVector>

#include <gsl/gsl_complex_math.h>

struct DataSet
{
    DataSet(size_t length)
        :vec(length, QPointF(0,0))
    {
        maxX = 1e-10;
        minX = 1e20;
        minY = 1e20;
        maxY = 1e-10;
    }

    void setX(size_t index, double x)
    {
        vec[index].setX(x);
        minX = qMin(minX, x);
        maxX = qMax(maxX, x);
    }

    void setY(size_t index, double y)
    {
        vec[index].setY(y);
        minY = qMin(minY, y);
        maxY = qMax(maxY, y);
    }

    size_t size() const
    {
        return vec.size();
    }

    QVector<QPointF> vec;

    QVector<double> y() const
    {
        QVector<double> ret(size());
        for(size_t index=0; index<size(); index++)
        {
            ret[index] = vec[index].y();
        }
        return ret;
    }

    double minX;
    double minY;
    double maxX;
    double maxY;
};

using Spectre = QVector<gsl_complex>;

#endif // TYPES_H
