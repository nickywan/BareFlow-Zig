// ============================================================================
// BAREFLOW MODULE - Regex DFA Matcher
// ============================================================================
// Purpose: DFA-based pattern matching (branch prediction stress test)
// Pattern: "ab*c" on synthetic input
// ============================================================================

// Simple DFA states for pattern: ab*c
typedef enum {
    STATE_START,
    STATE_A,
    STATE_B,
    STATE_C,
    STATE_FAIL
} dfa_state_t;

// DFA transition function
static dfa_state_t dfa_transition(dfa_state_t state, char input) {
    switch (state) {
        case STATE_START:
            return (input == 'a') ? STATE_A : STATE_FAIL;
        case STATE_A:
            if (input == 'b') return STATE_B;
            if (input == 'c') return STATE_C;
            return STATE_FAIL;
        case STATE_B:
            if (input == 'b') return STATE_B;
            if (input == 'c') return STATE_C;
            return STATE_FAIL;
        case STATE_C:
            return STATE_FAIL;  // Final state
        case STATE_FAIL:
            return STATE_FAIL;
    }
    return STATE_FAIL;
}

// Match pattern in input string
static int dfa_match(const char* input) {
    dfa_state_t state = STATE_START;

    for (int i = 0; input[i] != '\0'; i++) {
        state = dfa_transition(state, input[i]);
        if (state == STATE_C) {
            return 1;  // Match found
        }
        if (state == STATE_FAIL) {
            return 0;  // No match
        }
    }

    return (state == STATE_C) ? 1 : 0;
}

// Module entry point
int module_regex_dfa(void) {
    // Test strings (predictable pattern)
    const char* tests[] = {
        "abc",           // Match
        "abbc",          // Match
        "abbbc",         // Match
        "ac",            // No match
        "abcd",          // No match
        "xabc",          // No match
        "abbbbbbbc",     // Match (long b sequence)
        "abbbbbbbbbbc",  // Match (very long b sequence)
    };

    int matches = 0;
    int total = 8;

    // Run tests 100 times (stress branch predictor)
    for (int iter = 0; iter < 100; iter++) {
        for (int i = 0; i < total; i++) {
            if (dfa_match(tests[i])) {
                matches++;
            }
        }
    }

    // Expected: 5 matches * 100 iterations = 500
    return matches;
}
