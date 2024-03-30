
float os_fabs(float fnum)
{
    *((int *)&fnum) &= 0x7FFFFFFF;
    return fnum;
}

double os_abs(double dnum)
{
    *(((int *)&dnum) + 1) &= 0x7FFFFFFF;
    return dnum;
}

float os_sinf(float rad)
{
#define precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = rad;
    int k = 1;
    while (os_fabs(item) > precision) {
        sum += item;
        k += 1;
        item = (-1) * item * rad * rad / (2 * (k - 1) * (2 * k - 1));
    }
    return sum;
}

float os_cosf(float rad)
{
#define precision (1e-5)
    // Based on Taylor formula
    float sum = 0, item = 1;
    int k = 0;
    while (os_fabs(item) > precision) {
        sum += item;
        k += 1;
        item = (-1) * item * rad * rad / ((2 * k - 1) * 2 * k);
    }
    return sum;
}
