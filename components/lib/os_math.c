
#include "../../os_def.h"
#include "os_math.h"

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
    int kd = 0;
    while (os_fabsf(item) > sinf_precision) {
        sum += item;
        k += 1;
        kd = 2 * k;
        item = (-1) * item * rad * rad / ((kd - 2) * (kd - 1));
    }
    return sum;
}

float os_cosf(float rad)
{
#define cosf_precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = 1;
    int k = 0;
    int kd = 0;
    while (os_fabsf(item) > cosf_precision) {
        sum += item;
        k += 1;
        kd = 2 * k;
        item = (-1) * item * rad * rad / ((kd - 1) * kd);
    }
    return sum;
}

float os_tanf(float rad)
{
    float c = os_cosf(rad);
    OS_ASSERT(c != 0);
    return os_sinf(rad) / c;
}

float os_asinf(float x)
{
#define asinf_precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = x;
    int k = 0;
    int kd = 0;
    while (os_fabsf(item) > asinf_precision) {
        sum += item;
        k += 1;
        kd = 2 * k;
        item = item * x * x * (kd - 1) * (kd - 1) /
                (kd * (kd + 1)) ;
    }
    return sum;
}

float os_acosf(float x)
{
    return OS_PI_DIV_2 - os_asinf(x);
}

float os_atanf(float x)
{
#define atanf_precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = x;
    int k = 0;
    int kd = 0;
    while (os_fabsf(item) > atanf_precision) {
        sum += item;
        k += 1;
        kd = 2 * k;
        item = (-1) * item * x * x * (kd - 1) / (kd + 1) ;
    }
    return sum;
}

float os_sqrtf(float num)
{
    int i;
    float x2, y;
    const float threehalfs = 1.5F;

    x2 = num * 0.5F;
    y  = num;
    i  = * ( int * ) &y;
    i  = 0x5f375a86 - ( i >> 1 );
    y  = * ( float * ) &i;
    y  = y * ( threehalfs - ( x2 * y * y ) );
    //y  = y * ( threehalfs - ( x2 * y * y ) );
    //y  = y * ( threehalfs - ( x2 * y * y ) );
    return num * y;
}
