#ifndef EVEVARIANCE_H
#define EVEVARIANCE_H


class eveVariance
{
public:
    eveVariance();
    void Clear();
    void Push(double x);
    int NumDataValues() const;
    double Mean() const;
    double Variance() const;
    double StandardDeviation() const;

private:
    int count;
    double oldM;
    double newM;
    double oldS;
    double newS;
};

#endif // EVEVARIANCE_H
