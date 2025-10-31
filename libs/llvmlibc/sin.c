// Minimal sin implementation using Taylor series
// sin(x) ≈ x - x³/3! + x⁵/5! - x⁷/7! (5 terms)

static double fabs(double x) { return x < 0 ? -x : x; }
static const double PI = 3.14159265358979323846;

// Reduce x to [-π, π]
static double reduce_range(double x) {
    while (x > PI) x -= 2 * PI;
    while (x < -PI) x += 2 * PI;
    return x;
}

double sin(double x) {
    x = reduce_range(x);
    double x2 = x * x;
    double result = x;
    double term = x;

    term *= -x2 / (2 * 3); result += term;      // -x³/6
    term *= -x2 / (4 * 5); result += term;      // +x⁵/120
    term *= -x2 / (6 * 7); result += term;      // -x⁷/5040

    return result;
}

float sinf(float x) { return (float)sin(x); }
