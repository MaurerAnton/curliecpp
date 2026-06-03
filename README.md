# curliecpp — Curl with HTTPie-style Output (C++ port of curlie)

A C++ port of [curlie](https://github.com/rs/curlie) — wraps `curl` and colorizes the JSON response and HTTP headers for readability.

## Why curliecpp?

The original [curlie](https://github.com/rs/curlie) requires Go plus dozens of modules. curliecpp compiles with a single `make` using C++17 and `curl` (the binary, not libcurl).

## Quick Start

```bash
make
./curliecpp https://api.github.com/repos/MaurerAnton/tomogichi
./curliecpp --curl https://example.com   # Show equivalent curl command
```

## Features

- Passes all arguments through to `curl` (runs the system `curl` binary)
- Pretty-prints JSON responses with ANSI color syntax highlighting
- Colorizes HTTP response headers (status line, header names, values)
- `--pretty` flag to force colored output even when piped
- `--curl` flag to print the equivalent curl command being executed
- Auto-detects terminal width for proper formatting

## How It Works

curliecpp runs `curl -i <args>` to capture headers + body, then post-processes the output. It does **not** link against libcurl — it shells out to the `curl` binary, so any curl flag works.

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make, `curl` in PATH
