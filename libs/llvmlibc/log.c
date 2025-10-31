// Natural log using series: ln(1+x) = x - x²/2 + x³/3 - x⁴/4 + ...
// Valid for |x| < 1, so we use ln(a) = ln(m * 2^e) = ln(m) + e*ln(2)

static const double LN2 = 0.69314718055994530942;

double log(double x) {
    if (x <= 0) return -1.0 / 0.0; // -inf
    if (x == 1.0) return 0.0;

    // Extract mantissa and exponent
    int exp = 0;
    while (x >= 2.0) { x /= 2.0; exp++; }
    while (x < 1.0) { x *= 2.0; exp--; }

    // Now x in [1, 2), compute ln(x) via ln(1 + (x-1))
    double z = x - 1.0;
    double result = 0.0;
    double term = z;

    for (int i = 1; i <= 15; i++) {
        result += (i % 2 ? term / i : -term / i);
        term *= z;
    }

    return result + exp * LN2;
}

float logf(float x) { return (float)log(x); }
