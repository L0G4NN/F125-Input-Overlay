# Build and test

Run the commands from the repository root. The native target is Windows x64. The portable tests run on Linux with a C++20 compiler and sanitizer support.

```sh
bash scripts/test.sh
ZIG=/path/to/zig-0.14.1 bash scripts/build.sh
python3 scripts/package.py
```

When the host runs under ptrace, use `ASAN_OPTIONS=detect_leaks=0 bash scripts/test.sh`. AddressSanitizer and UndefinedBehaviorSanitizer still run.

The build writes `build/native/input_overlay.dll` and its checksum. Packaging reads the version from `addon/swofty.json` and writes the ZIP and importable `.swoftyaddon` under `dist/`. Import the latter through Swofty on Windows. Keep game files, logs and recordings out of the package.

See [architecture](ARCHITECTURE.md) for callback ownership, [sources](SOURCES.md) for the pinned SDK, and [multi-driver review](MULTI_DRIVER_REVIEW.md#validation) for the full-field replay, allocation benchmark and mutation test. The [game test plan](TEST_PLAN.md) covers launcher import, input behaviour and frame-time measurements.
