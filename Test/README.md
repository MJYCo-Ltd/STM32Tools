# Host validation

Use a native GCC or Clang toolchain, CMake/CTest 3.22+ and Ninja:

```sh
cmake -S Test -B build/host -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/host --parallel
ctest --test-dir build/host --output-on-failure --no-tests=error --timeout 300
```

Run the same entry with Release as well. `cmake/HostTests.cmake` undefines
`NDEBUG` on host test executables so assertions remain effective in both modes.
The helper rejects a test executable without a same-name `add_test`, recursively
including subdirectories. Call its finalizer after declaring ALL test targets.

The formerly unregistered `32ModulesTest`, `storage_test` and `ml307_http_test`
are now part of the normal CTest inventory. New binding examples compile against
the real public headers, and `api_contract_test` checks current signatures.
`host_registration_contract` uses disposable miniature CMake projects to verify
Release assertions and rejection of direct/nested orphan test executables.
Those fixtures test infrastructure, not product behavior.

For ASan/UBSan, configure a separate build with GCC/Clang, e.g.:

```sh
cmake -S Test -B build/host-sanitize -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Debug "-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all" "-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined"
cmake --build build/host-sanitize --parallel
ctest --test-dir build/host-sanitize --output-on-failure --no-tests=error --timeout 300
```

The Agriculture repository supplies `tools/run_host_tests.py` to run this entire
library suite AND the product suite with logs, inventories and JUnit summaries.
Its lock manifest selects the exact library commit. No private signing key is
needed for ordinary host tests; the product-signed image integration test is
explicitly skipped when its external signed artifact is absent.

Passing host tests is not an ARM link, an OTA/signing validation, or an on-board
acceptance test. Do not describe fixture tests or an explicitly selected subset
as a full product-suite run.
