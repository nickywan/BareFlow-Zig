// exp(x) using Taylor: exp(x) ≈ 1 + x + x²/2! + x³/3! + ... (10 terms)

double exp(double x) {
    double result = 1.0;
    double term = 1.0;

    for (int i = 1; i <= 10; i++) {
        term *= x / i;
        result += term;
    }
    return result;
}

float expf(float x) { return (float)exp(x); }
