Monocypher 4.0.2, unmodified upstream source and licence.

Source: https://github.com/LoupVaillant/Monocypher/tree/4.0.2

- `src/monocypher.c`, `src/monocypher.h`
- `src/optional/monocypher-ed25519.c`, `src/optional/monocypher-ed25519.h`
- `LICENCE.md`

The OTA verifier uses SHA-512 and Ed25519 as documented at
https://monocypher.org/manual/ed25519. It is linked into the application and required
at candidate completion and installation request. Host corruption tests and the
WiFi firmware build passed on 2026-09-08 before extraction into STM32Tools. The extraction has not been rebuilt; this is not a deployed secure boot chain.
