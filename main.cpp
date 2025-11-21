// Compiler: g++ 13.3.0

#include <iostream>
#include <numeric>
#include <vector>
#include <random>
#include <execution>
#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <algorithm>

using namespace std;
using namespace chrono;

vector<double> generate_vector(size_t size) {
    vector<double> random_numbers(size);
    mt19937 gen(random_device{}()); 
    uniform_real_distribution dist(0.0, 100.0);
    
    for (size_t i = 0; i < size; i++) {
        random_numbers[i] = dist(gen);
    }

    return random_numbers;
}

auto measure_time(auto&& f) {
    auto start = high_resolution_clock::now();
    f(); 
    auto end = high_resolution_clock::now();
    auto tdiff = end - start;
    return tdiff;
}

double custom_algoritm(vector<double>& vector1, vector<double>& vector2, unsigned int k) {    
    const size_t total_size = vector1.size();
    size_t chunk_size = (total_size + k - 1) / k;

    vector<thread> threads;
    vector<double> partial_results(k, 0.0);

    for (size_t i = 0; i < k; ++i) {
        size_t start_index = i * chunk_size;
        if (start_index >= total_size) break;
        size_t end_index = min(start_index + chunk_size, total_size);

        threads.emplace_back([&, i, start_index, end_index]() {
            partial_results[i] = transform_reduce(execution::seq,
                vector1.begin() + start_index, vector1.begin() + end_index,
                vector2.begin() + start_index,
                0.0,
                plus<>(),
                multiplies<>()
            );
        });
    }

    for (auto& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    double final_result = accumulate(partial_results.begin(), partial_results.end(), 0.0);
    return final_result;
}

void test_standart_library(vector<double>& vector1, vector<double>& vector2) {
    cout << "---- Standart library algorithm ----" << endl;
    
    vector<pair<string, double>> times;

    cout << left << setw(20) << "Policy" << setw(20) << "Time (ns)" << endl;
    cout << left << setw(20) << "no policy: ";
    auto t_no = measure_time([&vector1, &vector2]() {
        transform_reduce(vector1.begin(), vector1.end(), vector2.begin(), 0.0, plus<>(), multiplies<>());
    });
    cout << left << setw(20) << t_no << endl;
    times.push_back({"no policy", t_no.count()});

    cout << left << setw(20) << "seq: ";
    auto t_seq = measure_time([&vector1, &vector2]() {
        transform_reduce(execution::seq, vector1.begin(), vector1.end(), vector2.begin(), 0.0, plus<>(), multiplies<>());
    });
    cout << left << setw(20) << t_seq << endl;
    times.push_back({"seq", t_seq.count()});
    

    cout << left << setw(20) << "par: ";
    auto t_par = measure_time([&vector1, &vector2]() {
        transform_reduce(execution::par, vector1.begin(), vector1.end(), vector2.begin(), 0.0, plus<>(), multiplies<>());
    });
    cout << left << setw(20) << t_par << endl;
    times.push_back({"par", t_par.count()});
    

    cout << left << setw(20) << "par_unseq: ";
    auto t_par_unseq = measure_time([&vector1, &vector2]() {
        transform_reduce(execution::par_unseq, vector1.begin(), vector1.end(), vector2.begin(), 0.0, plus<>(), multiplies<>());
    });
    cout << setw(20) << t_par_unseq << endl;
    times.push_back({"par_unseq", t_par_unseq.count()});

    auto min_it = min_element(times.begin(), times.end(),
                              [](const auto& a, const auto& b){ return a.second < b.second; });

    cout << "The best policy is: " << min_it->first
         << " (" << fixed << min_it->second << " ns)\n";
}

void test_custom_algorithm(vector<double>& vector1, vector<double>& vector2, const unsigned int hardware_threads) {
    cout << "---- Custom algorithm ----" << endl;    
    cout << left << setw(8) << "K" 
         << setw(18) << "Time (ms)" 
         << setw(18) << "Speed-up (x)"
         << endl;

    unsigned int k_to_test = hardware_threads * 3;
    unsigned int best_k = 1;
    auto best_time = nanoseconds::max();
    double base_time = 0.0;

    for (unsigned int k = 1; k <= k_to_test; ++k) {
        auto t = measure_time([&vector1, &vector2, &k](){
            custom_algoritm(vector1, vector2, k);
        });
        double time_sec = t.count() / 1e6;
        if (k == 1) base_time = time_sec;
        double speedup = base_time / time_sec;

        cout << setw(8) << k 
             << setw(18) << fixed << setprecision(6) << time_sec
             << setw(18) << fixed << setprecision(2) << speedup
             << endl;

        if (t < best_time) {
            best_time = t;
            best_k = k;
        }
    }

    cout << "=== The fastest output when K = " << best_k << " ===" << endl << endl;
    
    if (best_k == hardware_threads)
        cout << "The best K ≈ number of hardware threads — optimal CPU utilization." << endl << endl;
    else if (best_k < hardware_threads)
        cout << "The best K is less than the number of threads — creating additional threads is inefficient." << endl << endl;
    else
        cout << "The best K is greater than the number of threads — the CPU is overloaded, efficiency decreases." << endl << endl;
}

void experiment(size_t size, unsigned int hardware_threads) {
    vector<double> vector1 = generate_vector(size);
    vector<double> vector2 = generate_vector(size);
    test_standart_library(vector1, vector2);
    test_custom_algorithm(vector1, vector2, hardware_threads);
}

int main() {
    #ifdef OPT_LEVEL
    std::cout << "Compiled with optimization level: -O" << OPT_LEVEL << std::endl;
    #endif

    const unsigned int hardware_threads = thread::hardware_concurrency();
    cout << "Number of hardware threads of the processor: " << hardware_threads << endl;

    cout << "================== TRANSFORM_REDUCE ==================" << endl;
    
    vector<size_t> test_sizes = {100'000, 1'000'000, 10'000'000, 50'000'000, 100'000'000};
    for (long long size : test_sizes) {
        cout << "==== Starting the experiment for vectors of " << size << " elements! ====" << endl;
        experiment(size, hardware_threads);
    }

    cout << "======================================================" << endl;
    cout << "As seen from the table, the execution time decreases as K increases "
        "up to the number of hardware threads (" << hardware_threads << "), "
        "after which it grows due to the overhead of thread creation and synchronization."
     << endl;

    return 0;
}
