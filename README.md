# curliecpp — Curl with HTTPie-style Output (C++ port of curlie)

A zero-dependency C++ port of [curlie](https://github.com/rs/curlie) — a frontend to curl that adds the readability of HTTPie output without sacrificing curl's flexibility.

## Why curliecpp?

The original [curlie](https://github.com/rs/curlie) requires the Go toolchain plus dozens of modules. curliecpp compiles with a single `make` using only C++17 and standard Linux headers.

## Quick Start

```bash
make
./curliecpp https://api.example.com/data
```

## Features

- Human-readable HTTP request/response output
- Syntax-highlighted JSON responses
- Full curl command-line compatibility (all flags pass through)
- Method shortcuts (GET, POST, PUT, DELETE)
- Request headers display
- Response timing
- Raw output mode

## Build

```bash
make
```
Requires: GCC 10+ or Clang 12+, GNU Make, libcurl
