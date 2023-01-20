/* math.c - log, pow, expdev */

#include <stdio.h>
#include <conf.h>
#include <math.h>

double pow(double x, int y)
{
    if (!y)
        return 1;
    if (!x)
        return 0;

    return x * pow(x, y - 1);
}

double log(double x)
{
    int i = 1;
    double log_val = 0;
    for(i = 1;i <= TAYLOR_N;i++)
    {
        log_val += (pow(-1, i - 1) * pow(x - 1, i)) / i;
    }
    return log_val;
}

double expdev() {
    double dummy;
    do
        dummy= (double) rand() / RAND_MAX;
    while (dummy == 0.0);
    return -log(dummy) / LAMBDA;
}