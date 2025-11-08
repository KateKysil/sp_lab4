#include <iostream>
#include <cmath>

bool isPrime(long long n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i <= std::sqrt(n); i += 2) {
        if (n % i == 0)
            return false;
    }
    return true;
}

int main() {
    int count = 0;
    long long number = 1;
    long long lastPrime = 2;

    while (count < 1000) {
        number++;
        if (isPrime(number)) {
            lastPrime = number;
            count++;
        }
    }

    std::cout << "1000th prime number: " << lastPrime << std::endl;
    return 0;
}
