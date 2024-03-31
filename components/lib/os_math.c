
#include "../../os_def.h"

int os_abs(int n)
{
    return (n ^ (n >> 31)) - (n >> 31);
}

float os_fabsf(float fnum)
{
    *((int *)&fnum) &= 0x7FFFFFFF;
    return fnum;
}

double os_fabs(double dnum)
{
    *(((int *)&dnum) + 1) &= 0x7FFFFFFF;
    return dnum;
}

float os_sinf(float rad)
{
#define sinf_precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = rad;
    int k = 1;
    while (os_fabsf(item) > sinf_precision) {
        sum += item;
        k += 1;
        item = (-1) * item * rad * rad / (2 * (k - 1) * (2 * k - 1));
    }
    return sum;
}

float os_cosf(float rad)
{
#define cosf_precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = 1;
    int k = 0;
    while (os_fabsf(item) > cosf_precision) {
        sum += item;
        k += 1;
        item = (-1) * item * rad * rad / ((2 * k - 1) * 2 * k);
    }
    return sum;
}

float os_tanf(float rad)
{
    float c = os_cosf(rad);
    OS_ASSERT(c != 0);
    return os_sinf(rad) / c;
}

float os_atanf(float x)
{
#define atanf_precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = x;
    int k = 0;
    while (os_fabsf(item) > atanf_precision) {
        sum += item;
        k += 1;
        item = (-1) * item * x * x * (2 * k - 1) / (2 * k + 1) ;
    }
    return sum;
}
