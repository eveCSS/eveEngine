#include <math.h>
#include "eveVariance.h"

eveVariance::eveVariance()
{
    Clear();
}

void eveVariance::Clear()
{
    count = 0;
}

void eveVariance::Push(double x)
{
    count++;

    // See Knuth TAOCP vol 2, 3rd edition, page 232
    if (count == 1)
    {
        oldM = newM = x;
        oldS = 0.0;
    }
    else
    {
        newM = oldM + (x - oldM)/count;
        newS = oldS + (x - oldM)*(x - newM);

        // set up for next iteration
        oldM = newM;
        oldS = newS;
    }
}

int eveVariance::NumDataValues() const
{
    return count;
}

double eveVariance::Mean() const
{
    return (count > 0) ? newM : 0.0;
}

double eveVariance::Variance() const
{
    return ( (count > 1) ? newS/(count - 1) : 0.0 );
}

double eveVariance::StandardDeviation() const
{
    return sqrt( Variance() );
}

