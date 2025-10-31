// cos(x) = sin(x + π/2)
extern double sin(double x);
static const double PI_2 = 1.57079632679489661923;

double cos(double x) {
    return sin(x + PI_2);
}

float cosf(float x) { return (float)cos(x); }
