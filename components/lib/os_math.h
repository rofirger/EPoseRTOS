
#ifndef _OS_MATH_H_
#define _OS_MATH_H_

#define OS_PI_F         (3.1415265f)
#define OS_PI_DIV_2     (1.5707963f)
#define OS_PI_DIV_180   (0.0174533f)
#define OS_180_DIV_PI   (57.295780f)
#define OS_DEG2RAD(deg) ((deg) * OS_PI_DIV_180)
#define OS_RAD2DEG(rad) ((rad) * OS_180_DIV_PI)

int os_abs(int n);
float os_fabsf(float fnum);
double os_fabs(double dnum);
float os_sinf(float rad);
float os_cosf(float rad);
float os_tanf(float rad);
float os_asinf(float x);
float os_acosf(float x);
float os_atanf(float x);

#endif /* _OS_MATH_H_ */
