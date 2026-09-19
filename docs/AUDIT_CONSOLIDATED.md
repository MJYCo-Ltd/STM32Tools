# Audit branch integration notice

The audit branch has been reconciled with the engineering and runtime branches.
The current contract is documented in [BRANCH_INTEGRATION.md](BRANCH_INTEGRATION.md)
and [ENGINEERING_CONTRACTS.md](../Inc/ENGINEERING_CONTRACTS.md).

Distinct audit regression coverage remains in `Test/AuditTests.cmake`, including
MQTT boundary checks, queued button edges, Auxiliary errors, HealthMonitor
semantics, CMake target aliases and a non-default UART work budget. Firmware-slot
successful-step progress hooks remain covered by `Test/storage_test.c`.

Superseded choices: there is no implicit F411 memory map; `ReadFlash` is not a
supported API; UART work is bounded globally with round-robin fairness, not per
port; `IOStatistics.h` owns IOInfo and `IOInfo.h` is a compatibility include.
`System/HealthMonitor.h` is the sole HealthMonitor implementation; the incompatible
experimental `Services/HealthMonitor.h` API is not retained. Rebuild consumers.

Old branch-specific test counts and publication statements are not validation
claims for this integrated revision. Use its CI logs and integration report.
