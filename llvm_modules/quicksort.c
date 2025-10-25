// ============================================================================
// Quicksort with Hybrid Strategy - Branch-Heavy Module for PGO
// ============================================================================
// This module is PERFECT for PGO because:
// - Highly unpredictable branches (partition pivot decisions)
// - Recursive calls with varying depth
// - Conditional strategy switching (quicksort vs insertion sort)
// - Branch mispredictions are common without profiling
//
// PGO helps with:
// - Branch prediction hints for partition comparisons
// - Inline decisions for small array handling
// - Tail recursion optimization
// - Call graph optimization
// ============================================================================

#define ARRAY_SIZE 64
#define INSERTION_SORT_THRESHOLD 8

// Pseudo-random number generator for reproducible data
static unsigned int rng_state = 42;

static int pseudo_rand(void) {
    rng_state = rng_state * 1103515245 + 12345;
    return (int)((rng_state / 65536) % 32768);
}

// Insertion sort for small subarrays (< threshold)
static void insertion_sort(int* arr, int left, int right) {
    for (int i = left + 1; i <= right; i++) {
        int key = arr[i];
        int j = i - 1;

        // This inner loop has unpredictable branches
        while (j >= left && arr[j] > key) {
            arr[j + 1] = arr[j];
            j--;
        }
        arr[j + 1] = key;
    }
}

// Three-way comparison function (returns -1, 0, or 1)
static int compare(int a, int b) {
    if (a < b) return -1;
    if (a > b) return 1;
    return 0;
}

// Median-of-three pivot selection
static int median_of_three(int* arr, int left, int right) {
    int mid = left + (right - left) / 2;

    // Complex conditional logic - perfect for PGO branch hints
    if (compare(arr[left], arr[mid]) > 0) {
        int temp = arr[left];
        arr[left] = arr[mid];
        arr[mid] = temp;
    }

    if (compare(arr[mid], arr[right]) > 0) {
        int temp = arr[mid];
        arr[mid] = arr[right];
        arr[right] = temp;
    }

    if (compare(arr[left], arr[mid]) > 0) {
        int temp = arr[left];
        arr[left] = arr[mid];
        arr[mid] = temp;
    }

    return mid;
}

// Partition with median-of-three pivot
static int partition(int* arr, int left, int right) {
    // Select pivot using median-of-three
    int pivot_idx = median_of_three(arr, left, right);
    int pivot = arr[pivot_idx];

    // Move pivot to end
    int temp = arr[pivot_idx];
    arr[pivot_idx] = arr[right];
    arr[right] = temp;

    int i = left - 1;

    // Main partition loop - highly unpredictable branches
    for (int j = left; j < right; j++) {
        // This comparison is the HOT PATH
        // PGO can provide branch prediction hints here
        if (compare(arr[j], pivot) <= 0) {
            i++;
            int swap = arr[i];
            arr[i] = arr[j];
            arr[j] = swap;
        }
    }

    // Move pivot to final position
    int swap = arr[i + 1];
    arr[i + 1] = arr[right];
    arr[right] = swap;

    return i + 1;
}

// Hybrid quicksort with insertion sort fallback
static void quicksort_hybrid(int* arr, int left, int right) {
    // Base case
    if (left >= right) {
        return;
    }

    // CRITICAL BRANCH: Use insertion sort for small subarrays
    // PGO helps predict which branch is taken more often
    int size = right - left + 1;
    if (size <= INSERTION_SORT_THRESHOLD) {
        insertion_sort(arr, left, right);
        return;
    }

    // Partition and recurse
    int pivot_idx = partition(arr, left, right);

    // Calculate subarray sizes for potential optimization
    int left_size = pivot_idx - left;
    int right_size = right - pivot_idx;

    // COMPLEX BRANCHING: Recurse on smaller partition first
    // This reduces stack depth but creates unpredictable branches
    if (left_size < right_size) {
        quicksort_hybrid(arr, left, pivot_idx - 1);
        quicksort_hybrid(arr, pivot_idx + 1, right);
    } else {
        quicksort_hybrid(arr, pivot_idx + 1, right);
        quicksort_hybrid(arr, left, pivot_idx - 1);
    }
}

// Verify array is sorted (for correctness check)
static int is_sorted(int* arr, int size) {
    for (int i = 1; i < size; i++) {
        if (arr[i] < arr[i - 1]) {
            return 0;  // Not sorted
        }
    }
    return 1;  // Sorted
}

// Entry point - will be called 3000+ times for HOT classification
int compute(void) {
    static int arr[ARRAY_SIZE];

    // Generate pseudo-random data with specific patterns
    // Mix of ascending, descending, and random to stress different branches
    int pattern = (rng_state / 1000) % 3;

    if (pattern == 0) {
        // Ascending (best case for quicksort)
        for (int i = 0; i < ARRAY_SIZE; i++) {
            arr[i] = i;
        }
    } else if (pattern == 1) {
        // Descending (worst case without median-of-three)
        for (int i = 0; i < ARRAY_SIZE; i++) {
            arr[i] = ARRAY_SIZE - i;
        }
    } else {
        // Random (average case)
        for (int i = 0; i < ARRAY_SIZE; i++) {
            arr[i] = pseudo_rand() % 100;
        }
    }

    // Perform sort
    quicksort_hybrid(arr, 0, ARRAY_SIZE - 1);

    // Verify correctness
    int sorted = is_sorted(arr, ARRAY_SIZE);

    // Return checksum: sum of first 4 elements + sorted flag
    int checksum = arr[0] + arr[1] + arr[2] + arr[3];
    return (checksum * 1000) + sorted;
}
