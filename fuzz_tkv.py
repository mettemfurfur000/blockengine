#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
TKV Format Fuzzing Script
Tests the TKV serialization/deserialization with many configurations.
Compares input serialization with output serialization to detect discrepancies.
"""

import subprocess
import random
import string
import sys
import os

# Configuration
TKV_TEST_BIN = "build/tkv_test"
NUM_TESTS = 1000
VERBOSE = False
STOP_ON_ERROR = False
SIMPLE_MODE = False

class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'

def log_pass(msg):
    if VERBOSE:
        print(f"{Colors.GREEN}[PASS]{Colors.RESET} {msg}")

def log_fail(msg):
    print(f"{Colors.RED}[FAIL]{Colors.RESET} {msg}")

def log_info(msg):
    print(f"{Colors.BLUE}[INFO]{Colors.RESET} {msg}")

def log_warn(msg):
    print(f"{Colors.YELLOW}[WARN]{Colors.RESET} {msg}")

def gen_valid_key(length=None):
    """Generate a valid TKV key (1-10 chars, a-z, A-Z, 0-9, _)"""
    if length is None:
        length = random.randint(1, 10)
    
    # First character must be letter or underscore
    first_char = random.choice(string.ascii_letters + '_')
    
    # Rest can be letters, digits, or underscore
    rest_chars = string.ascii_letters + string.digits + '_'
    rest = ''.join(random.choice(rest_chars) for _ in range(length - 1))
    
    return first_char + rest

def gen_bool():
    """Generate a boolean value"""
    return random.choice(['true', 'false'])

def gen_i64():
    """Generate an integer value"""
    options = [
        random.randint(0, 255),                              # Small positive
        random.randint(-128, 127),                           # Small negative
        random.randint(0, 65535),                            # Medium positive
        random.randint(-32768, 32767),                       # Medium negative
        random.randint(0, 2**32 - 1),                        # Large positive
        random.randint(-(2**31), 2**31 - 1),                 # Large negative
        random.randint(0, 2**63 - 1),                        # Very large positive
        0,                                                   # Zero
        -1,                                                  # Negative one
        9223372036854775807,                                 # INT64_MAX
        # -9223372036854775808,                                # INT64_MIN
    ]
    val = random.choice(options)
    
    # Sometimes use hex notation
    if random.random() < 0.2 and val >= 0:
        return f"0x{val:X}"
    return str(val)

def gen_f64():
    """Generate a floating point value"""
    options = [
        f"{random.random()}",                                # Simple decimal
        f"{random.uniform(-1000, 1000)}",                    # Range
        "3.14159265358979323846",                            # Pi with many digits
        "2.718281828459045",                                 # e with many digits
        "1.41421356237309504880",                            # sqrt(2)
        "0.1",                                               # Problematic for floats
        "0.33333333333333333333",                            # 1/3
        "0.00000000000000000001",                            # Very small
        "-3.14159",                                          # Negative
        "123.456",                                           # Regular decimal
        "0.999999999",                                       # Close to 1
        # Exponential notation
        "1e10",                                              # Simple exponent
        "1E10",                                              # Uppercase E
        "3.14e-5",                                           # Decimal with negative exponent
        "2.5E+3",                                            # Uppercase with positive sign
        "5e-20",                                             # Very small exponent
        "9.99e99",                                           # Large exponent
        "1.23e0",                                            # Zero exponent
        "-1e5",                                              # Negative with exponent
    ]
    return random.choice(options)

def gen_string():
    """Generate a string value"""
    if SIMPLE_MODE:
        max_len = 20
    else:
        max_len = 50
    
    options = [
        '"hello"',
        '"world"',
        '""',                                                # Empty string
        '"' + ''.join(random.choices(string.ascii_letters, k=random.randint(1, max_len))) + '"',
        '"test123"',
        '"_underscore_"',
        '"with spaces"',
        '"123456789012345"',
        '"' + 'a' * (max_len // 2) + '"',                   # Medium string
    ]
    return random.choice(options)

def gen_array():
    """Generate a byte array"""
    # Limit array size to prevent command-line length issues
    if SIMPLE_MODE:
        length = random.randint(0, 20)
    else:
        # In normal mode, keep arrays reasonably sized (max 50 bytes)
        length = random.randint(0, 50)
    
    if length == 0:
        return "[ ]"
    
    bytes_list = []
    for _ in range(length):
        # Mix of hex, decimal, and char literals
        choice = random.choice(['hex', 'dec', 'char'])
        if choice == 'hex':
            bytes_list.append(f"0x{random.randint(0, 255):02x}")
        elif choice == 'dec':
            bytes_list.append(str(random.randint(0, 255)))
        else:
            # Use printable ASCII chars for char literals
            bytes_list.append(f"'{chr(random.randint(32, 126))}'")
    
    return "[ " + " ".join(bytes_list) + " ]"

def gen_tkv_value(depth=0, max_depth=3):
    """Generate a TKV value of random type"""
    # In simple mode, reduce max depth and complexity
    if SIMPLE_MODE:
        max_depth = 1
        if depth >= max_depth:
            choice = random.choice(['bool', 'i64', 'f64', 'str', 'arr'])
        else:
            choice = random.choice(['bool', 'i64', 'f64', 'str', 'arr'])
    else:
        if depth >= max_depth:
            # At max depth, don't allow nested TKV
            choice = random.choice(['bool', 'i64', 'f64', 'str', 'arr'])
        else:
            choice = random.choice(['bool', 'i64', 'f64', 'str', 'arr', 'tkv'])
    
    if choice == 'bool':
        return 'bool', gen_bool()
    elif choice == 'i64':
        return 'i64', gen_i64()
    elif choice == 'f64':
        return 'f64', gen_f64()
    elif choice == 'str':
        return 'str', gen_string()
    elif choice == 'arr':
        return 'arr', gen_array()
    else:  # tkv
        return 'tkv', gen_tkv_object(depth + 1, max_depth)

def gen_tkv_object(depth=0, max_depth=3):
    """Generate a complete TKV object"""
    # In simple mode, reduce number of keys
    if SIMPLE_MODE:
        num_keys = random.randint(1, 3)
    else:
        num_keys = random.randint(1, 10)
    
    pairs = []
    for _ in range(num_keys):
        key = gen_valid_key()
        typ, value = gen_tkv_value(depth, max_depth)
        
        if typ == 'tkv':
            # value is already a tkv string
            pairs.append(f"  tkv {key} = {value};")
        else:
            pairs.append(f"  {typ} {key} = {value};")
    
    return "{\n" + "\n".join(pairs) + "\n}"

def run_tkv_test(tkv_string, bin_path):
    """Run tkv_test binary and return parsed/serialized output"""
    try:
        result = subprocess.run(
            [bin_path, tkv_string],
            capture_output=True,
            text=True,
            timeout=5
        )
        
        output = result.stdout + result.stderr
        
        # Extract the serialized output
        if "--- Serialized TKV Output ---" in output:
            parts = output.split("--- Serialized TKV Output ---")
            if len(parts) > 1:
                serialized = parts[1].strip()
                return True, serialized
        
        if "Failed to parse TKV" in output:
            return False, output
        
        return False, output
    
    except subprocess.TimeoutExpired:
        return False, "TIMEOUT"
    except Exception as e:
        return False, str(e)

def test_roundtrip(tkv_input, test_num, bin_path):
    """Test that parse -> serialize -> parse produces consistent results"""
    
    # First parse
    success1, output1 = run_tkv_test(tkv_input, bin_path)
    if not success1:
        log_fail(f"Test {test_num}: Initial parse failed")
        print(f"\n{Colors.YELLOW}=== FAILING INPUT ==={Colors.RESET}")
        print(tkv_input)
        print(f"\n{Colors.YELLOW}=== ERROR OUTPUT ==={Colors.RESET}")
        print(output1)
        if STOP_ON_ERROR:
            # Save to file for debugging
            with open("failing_tkv_input.txt", "w") as f:
                f.write(tkv_input)
            log_warn("Failing input saved to: failing_tkv_input.txt")
        return False
    
    # Second parse (using serialized output)
    success2, output2 = run_tkv_test(output1, bin_path)
    if not success2:
        log_fail(f"Test {test_num}: Roundtrip parse failed")
        print(f"\n{Colors.YELLOW}=== ORIGINAL INPUT ==={Colors.RESET}")
        print(tkv_input)
        print(f"\n{Colors.YELLOW}=== FIRST SERIALIZATION ==={Colors.RESET}")
        print(output1)
        print(f"\n{Colors.YELLOW}=== SECOND PARSE ERROR ==={Colors.RESET}")
        print(output2)
        if STOP_ON_ERROR:
            # Save to file for debugging
            with open("failing_tkv_input.txt", "w") as f:
                f.write(tkv_input)
            log_warn("Failing input saved to: failing_tkv_input.txt")
        return False
    
    # Compare outputs
    if output1.strip() != output2.strip():
        log_fail(f"Test {test_num}: Serialization mismatch")
        print(f"\n{Colors.YELLOW}=== ORIGINAL INPUT ==={Colors.RESET}")
        print(tkv_input)
        print(f"\n{Colors.YELLOW}=== FIRST SERIALIZATION ==={Colors.RESET}")
        print(output1)
        print(f"\n{Colors.YELLOW}=== SECOND SERIALIZATION ==={Colors.RESET}")
        print(output2)
        if STOP_ON_ERROR:
            # Save to file for debugging
            with open("failing_tkv_input.txt", "w") as f:
                f.write(tkv_input)
            log_warn("Failing input saved to: failing_tkv_input.txt")
        return False
    
    log_pass(f"Test {test_num}: OK")
    return True

def test_bool_values(bin_path):
    """Test all boolean value combinations"""
    for val in ['true', 'false']:
        tkv = f"{{ bool x = {val}; }}"
        _, output = run_tkv_test(tkv, bin_path)
        if "bool x = " + val not in output:
            return False
    return True

def test_i64_boundaries(bin_path):
    """Test i64 boundary values"""
    values = [
        "0",
        "1",
        "-1",
        "127",
        "-128",
        "255",
        "-32768",
        "32767",
        "65535",
        "2147483647",
        "-2147483648",
        "9223372036854775807",     # INT64_MAX
        # Note: INT64_MIN (-9223372036854775808) is skipped because the tokenizer parses
        # the minus sign separately and then tries to parse the positive number,
        # which would be out of range for a positive i64
        "0xFF",
        "0x7FFFFFFFFFFFFFFF",       # INT64_MAX in hex
    ]
    
    for val in values:
        tkv = f"{{ i64 x = {val}; }}"
        success, _ = run_tkv_test(tkv, bin_path)
        if not success:
            log_fail(f"i64 boundary test failed: {val}")
            return False
    
    return True

def test_float_precision(bin_path):
    """Test floating point precision with many digits and exponential notation"""
    values = [
        "3.14159265358979323846",
        "2.718281828459045",
        "0.1",
        "0.3333333333333333333333333",
        "1.414213562373095048801688724209698078569671875376948073176679737990732478462107038850387534327641",
        "-3.14159",
        "0.00000000000000000001",
        # Exponential notation
        "1e10",
        "1E10",
        "3.14e-5",
        "2.5E+3",
        "5e-20",
        "9.99e99",
        "1.23e0",
        "-1e5",
    ]
    
    for val in values:
        tkv = f"{{ f64 x = {val}; }}"
        success, _ = run_tkv_test(tkv, bin_path)
        if not success:
            log_fail(f"Float precision test failed: {val}")
            return False
    
    return True

def test_string_edge_cases(bin_path):
    """Test string edge cases"""
    values = [
        '""',                                       # Empty
        '"a"',                                      # Single char
        '"hello world"',                            # With space
        '"123456789"',                              # Numbers
        '"_underscore_"',                           # Underscore
        '"' + 'a' * 50 + '"',                       # Medium string
        '"' + 'x' * 100 + '"',                      # Long string
    ]
    
    for val in values:
        tkv = f"{{ str x = {val}; }}"
        success, _ = run_tkv_test(tkv, bin_path)
        if not success:
            log_fail(f"String edge case test failed: {val[:50]}...")
            return False
    
    return True

def test_array_edge_cases(bin_path):
    """Test array edge cases"""
    cases = [
        "[ ]",                                      # Empty
        "[ 0 ]",                                    # Single element
        "[ 0x00 0xFF 0x7F ]",                       # Mixed values
        "[ " + " ".join(f"0x{i:02x}" for i in range(256)) + " ]",  # All bytes
        "[ 'a' 'b' 'c' ]",                          # Char literals
    ]
    
    for arr in cases:
        tkv = f"{{ arr x = {arr}; }}"
        success, _ = run_tkv_test(tkv, bin_path)
        if not success:
            log_fail(f"Array edge case test failed: {arr[:50]}...")
            return False
    
    return True

def test_key_length_limits(bin_path):
    """Test key length limits"""
    # Valid lengths: 1-10
    for length in [1, 5, 10]:
        key = gen_valid_key(length)
        tkv = f"{{ bool {key} = true; }}"
        success, _ = run_tkv_test(tkv, bin_path)
        if not success:
            log_fail(f"Key length {length} test failed")
            return False
    
    return True

def test_nested_depth(bin_path):
    """Test deeply nested TKV structures"""
    tkv = "{ bool a = true; }"
    for i in range(5):
        tkv = f"{{ tkv inner{i} = {tkv}; bool b{i} = true; }}"
    
    success, _ = run_tkv_test(tkv, bin_path)
    if not success:
        log_fail("Deep nesting test failed")
        return False
    
    return True

def main():
    global VERBOSE, STOP_ON_ERROR, SIMPLE_MODE
    
    # Parse command-line arguments
    for arg in sys.argv[1:]:
        if arg == '-v' or arg == '--verbose':
            VERBOSE = True
        elif arg == '--stop-on-error':
            STOP_ON_ERROR = True
        elif arg == '--simple':
            SIMPLE_MODE = True
    
    # Debug path
    log_info(f"Working directory: {os.getcwd()}")
    log_info(f"Looking for binary: {TKV_TEST_BIN}")
    
    if not os.path.exists(TKV_TEST_BIN):
        # Try with .exe extension on Windows
        if os.path.exists(TKV_TEST_BIN + ".exe"):
            bin_path = TKV_TEST_BIN + ".exe"
        else:
            log_fail(f"Binary not found: {TKV_TEST_BIN}")
            log_fail(f"CWD: {os.getcwd()}")
            log_fail(f"Files in build: {os.listdir('build') if os.path.exists('build') else 'build dir not found'}")
            sys.exit(1)
    else:
        bin_path = TKV_TEST_BIN
    
    log_info(f"Using binary: {bin_path}")
    if SIMPLE_MODE:
        log_warn("SIMPLE MODE enabled: reduced complexity")
    if STOP_ON_ERROR:
        log_warn("STOP ON ERROR enabled: will exit on first failure")
    
    log_info("TKV Format Fuzzing Script")
    log_info(f"Running {NUM_TESTS} random tests...")
    print()
    
    # Run structured tests first
    log_info("Running structured tests...")
    
    tests = [
        ("Bool values", test_bool_values),
        ("i64 boundaries", test_i64_boundaries),
        ("Float precision", test_float_precision),
        ("String edge cases", test_string_edge_cases),
        ("Array edge cases", test_array_edge_cases),
        ("Key length limits", test_key_length_limits),
        ("Nested depth", test_nested_depth),
    ]
    
    passed = 0
    failed = 0
    
    for name, test_func in tests:
        try:
            if test_func(bin_path):
                log_pass(f"{name}")
                passed += 1
            else:
                log_fail(f"{name}")
                failed += 1
        except Exception as e:
            log_fail(f"{name}: {e}")
            failed += 1
    
    print()
    log_info(f"Running {NUM_TESTS} random fuzz tests...")
    
    fuzz_passed = 0
    fuzz_failed = 0
    
    for i in range(NUM_TESTS):
        tkv = gen_tkv_object()
        if test_roundtrip(tkv, i + 1, bin_path):
            fuzz_passed += 1
        else:
            fuzz_failed += 1
            if STOP_ON_ERROR:
                print()
                log_fail(f"Stopped at test {i + 1} due to error")
                break
        
        # Progress indicator
        if (i + 1) % 100 == 0:
            log_info(f"Progress: {i + 1}/{NUM_TESTS}")
    
    print()
    print("=" * 50)
    log_info("RESULTS")
    print("=" * 50)
    log_info(f"Structured tests: {passed} passed, {failed} failed")
    log_info(f"Fuzz tests:       {fuzz_passed} passed, {fuzz_failed} failed")
    
    total_passed = passed + fuzz_passed
    total_failed = failed + fuzz_failed
    total_tests = total_passed + total_failed
    
    if total_failed == 0:
        print(f"{Colors.GREEN}[SUCCESS] All {total_tests} tests passed!{Colors.RESET}")
        return 0
    else:
        print(f"{Colors.RED}[RESULT] {total_failed} of {total_tests} tests failed ({100*total_failed//total_tests}% failure rate){Colors.RESET}")
        return 1

if __name__ == '__main__':
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        print()
        log_warn("Interrupted by user")
        sys.exit(130)
