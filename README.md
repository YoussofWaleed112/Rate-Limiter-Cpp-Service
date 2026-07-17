# C++ Rate Limiter Service — README

A token-bucket rate limiter exposed over HTTP, built with `httplib` and
`nlohmann::json`.

## Project structure

```
.
├── main.cpp                    (API layer — HTTP routes)
├── rate_limiter_service.hpp/.cpp  (storage layer — Rate_Limiter class)
├── bucket.hpp/.cpp             (logic layer — Bucket class)
└── include/
    ├── httplib.h
    └── json.hpp
```

## What each class does

### `Bucket` (bucket.hpp / bucket.cpp)

Represents the rate-limit state for **one single key**. One `Bucket`
instance is created per issued API key.

| Function | What it does |
|---|---|
| `Bucket(capacity, refill_rate)` | Constructs a bucket starting full, with the given max capacity and refill speed (tokens/second). |
| `check_and_consume_limit()` | The core operation: refills the bucket based on elapsed time, then checks if at least 1 token is available. If yes, consumes one and returns `allowed = true`. If no, returns `allowed = false` plus a `retry_after_ms` estimate. This is thread-safe — protected by the bucket's own internal mutex, so two requests for the *same* key can never race and over-allow. |
| `get_limit()` | Read-only: refills based on elapsed time and reports the current state, without consuming a token. Used for status checks. |
| `configure(new_capacity, new_refill_rate)` | Updates this bucket's capacity and refill rate (used when a client reconfigures their limits). |
| `refill()` *(private)* | Internal helper: computes how much time has passed since the last refill and adds the corresponding number of tokens, capped at capacity. |

### `Rate_Limiter` (rate_limiter_service.hpp / rate_limiter_service.cpp)

Owns **all** the buckets — one per key — in an in-memory map, and handles
key generation. This is the layer the API handlers talk to; they never
touch a `Bucket` directly.

| Function | What it does |
|---|---|
| `Rate_Limiter(default_capacity, default_refill_rate)` | Sets the default limits handed to any newly generated key. |
| `generate_key()` | Creates a new random key (3 letters + 3 digits), creates a `Bucket` for it with the default limits, stores it in the map, and returns the key string. |
| `check_key(key)` | Returns `true`/`false` for whether a given key exists in the map at all — used to reject unknown keys before doing any rate-limit work. |
| `get_bucket_and_consume(key)` | Looks up the bucket for a key and calls its `check_and_consume_limit()` — this is what actually evaluates and consumes quota. |
| `check_bucket(key)` | Looks up the bucket for a key and calls its `get_limit()` — a read-only status peek, no consumption. |
| `set_configuration(key, new_capacity, new_refill_rate)` | Looks up the bucket for a key and calls its `configure(...)` to change its limits. Returns `false` if the key doesn't exist. |

### `main.cpp` (API layer)

Sets up the HTTP server and wires four routes to the `Rate_Limiter`
methods above. Parses incoming JSON, validates required fields, and turns
the result of each `Rate_Limiter` call into an HTTP status code and JSON
response.

## Build

From the project root, with `include/httplib.h` and `include/json.hpp` in
place:

```bash
g++ -std=c++17 -O2 -pthread main.cpp rate_limiter_service.cpp bucket.cpp -o rate_limiter_service -I.
```

All three `.cpp` files must be listed together, since the project is split
across multiple source files.

## Run

```bash
./rate_limiter_service
```

You should see:
```
Welcome to the Rate Limiter Service API
```

The server is now listening on `http://localhost:8080`. Leave this
terminal running, and use a **second terminal** for the commands below.

## Commands to try

Windows PowerShell treats `curl` as an alias for `Invoke-WebRequest`, which
uses different parameters than real curl. Both options are given below —
use whichever you prefer, but stick to one so the syntax stays consistent.

### 1. `POST /Request-Key` — get a new key

**curl.exe:**
```powershell
curl.exe -X POST http://localhost:8080/Request-Key
```

**PowerShell native:**
```powershell
Invoke-RestMethod -Uri http://localhost:8080/Request-Key -Method Post
```

Response:
```json
{"message":"API key generated , hello there","your_key":"hww830","limit":5.0,"requests_per_second":1.0}
```

Copy the value of `your_key` — every command below uses `hww830` as a
placeholder; replace it with your actual generated key.

### 2. `POST /check` — evaluate and consume one unit of quota

**curl.exe:**
```powershell
curl.exe -X POST http://localhost:8080/check -H "Content-Type: application/json" -d "{\"api_key\": \"hww830\"}"
```

**PowerShell native:**
```powershell
Invoke-RestMethod -Uri http://localhost:8080/check -Method Post -ContentType "application/json" -Body '{"api_key": "hww830"}'
```

Run this repeatedly — the first few calls (up to the configured capacity)
return 200 with decreasing `Remaining`; once exhausted you get 429 with a
`retry_after_ms`.

### 3. `GET /status/key` — read-only status check

The key is passed as a **query parameter**, not in the URL path or body:

**curl.exe:**
```powershell
curl.exe "http://localhost:8080/status/key?api_key=hww830"
```

**PowerShell native:**
```powershell
Invoke-RestMethod -Uri "http://localhost:8080/status/key?api_key=hww830"
```

### 4. `POST /configure` — change a key's limits

**curl.exe:**
```powershell
curl.exe -X POST http://localhost:8080/configure -H "Content-Type: application/json" -d "{\"api_key\": \"hww830\", \"capacity\": 10, \"refill_rate\": 2}"
```

**PowerShell native:**
```powershell
Invoke-RestMethod -Uri http://localhost:8080/configure -Method Post -ContentType "application/json" -Body '{"api_key": "hww830", "capacity": 10, "refill_rate": 2}'
```

## Stopping the server

Go to the terminal running the server and press **Ctrl+C**.
