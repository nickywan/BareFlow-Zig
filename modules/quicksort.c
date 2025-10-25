// ============================================================================
// Quicksort Benchmark - Tests branch prediction and recursion optimization
// ============================================================================
#include "../kernel/module_loader.h"

static const module_header_t __attribute__((section(".module_header"))) module_header = {
    .magic = MODULE_MAGIC,
    .name = "quicksort",
    .entry_point = (void*)0,  // Relative offset (filled by linker)
    .code_size = 0,
    .version = 1
};

// Partition function for quicksort
static int partition(int* arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;

    for (int j = low; j < high; j++) {
        if (arr[j] < pivot) {
            i++;
            // Swap arr[i] and arr[j]
            int temp = arr[i];
            arr[i] = arr[j];
            arr[j] = temp;
        }
    }

    // Swap arr[i+1] and arr[high] (pivot)
    int temp = arr[i + 1];
    arr[i + 1] = arr[high];
    arr[high] = temp;

    return i + 1;
}

// Recursive quicksort
static void quicksort_impl(int* arr, int low, int high) {
    if (low < high) {
        int pi = partition(arr, low, high);
        quicksort_impl(arr, low, pi - 1);
        quicksort_impl(arr, pi + 1, high);
    }
}

// Benchmark entry point
__attribute__((section(".text.entry")))
int quicksort_benchmark(void) {
    // Initialize array with pseudo-random values (deterministic)
    static int data[128] = {1};  // Force .data section
    unsigned int seed = 12345;

    for (int i = 0; i < 128; i++) {
        seed = seed * 1103515245 + 12345;  // Linear congruential generator
        data[i] = (int)(seed % 1000);
    }

    // Sort the array 5 times for better profiling
    for (int iter = 0; iter < 5; iter++) {
        quicksort_impl(data, 0, 127);

        // Shuffle a bit between iterations
        for (int i = 0; i < 10; i++) {
            int idx1 = (seed >> 4) % 128;
            int idx2 = (seed >> 12) % 128;
            int temp = data[idx1];
            data[idx1] = data[idx2];
            data[idx2] = temp;
            seed = seed * 1103515245 + 12345;
        }
    }

    // Return checksum
    int checksum = 0;
    for (int i = 0; i < 128; i++) {
        checksum ^= data[i];
    }
    return checksum;
}
