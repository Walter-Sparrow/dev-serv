# dev-serv

A minimal C webserver that uses macOS FSEvents for automatically triggering live reload in the browser (via Server-Sent Events, SSE). **Not intended for production**; this is strictly for personal experiments, quick local demos, and recreational development.

## Requirements

- **macOS** (required for the `CoreFoundation` and `CoreServices` frameworks, as well as FSEvents).
- A C compiler (e.g. `clang` or `gcc` that supports Apple frameworks).
- `pthread` library (included by default on macOS).

## Not Production-Ready

This project is **not** designed for production:

- No TLS/SSL support
- No advanced security features
- Not performance-tested or benchmarked
- Minimal error handling

Use at your own risk for local or recreational purposes.

## Building

1. Make sure you have `clang` or another compiler installed (e.g. via Xcode Command Line Tools).
2. Run the provided build script:

   ```bash
   ./build.sh
   ```
