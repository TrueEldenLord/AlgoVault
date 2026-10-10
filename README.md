# AlgoVault

A self-hosted LeetCode-style problem browser built in C++. Serves 146 NeetCode 150 problems with a Monaco editor, live code execution, and a SQLite cache — no external database required.

## Tech Stack

- **Backend** — C++ HTTP server (`httplib`), SQLite (amalgamation, compiled in)
- **Frontend** — Vanilla JS, Monaco Editor (CDN)
- **Code Execution** — [Piston API](https://github.com/engineer-man/piston) (self-hostable, no API key)
- **Problem Data** — [alfa-leetcode-api](https://github.com/alfaarghya/alfa-leetcode-api), cached in `cache.db`

## Features

- 146 problems seeded across 18 categories (NeetCode 150 coverage)
- Split-panel overlay with Monaco editor on problem click
- Language dropdown — C++, Python3, Java, JS/TS first; 20+ more under "More Languages"
- Starter code generated per language from problem title
- Run/Submit buttons execute code via Piston
- "Open on LeetCode" link on every problem
- Add Problem modal with title auto-fill from problem number
- SQLite description cache (fetches from alfa-leetcode-api, stores locally)

## Build & Run

```bash
make
./leetcode_db
# open http://localhost:8080
```

Requires `gcc`. The SQLite amalgamation (`sqlite3.c`) is compiled in — no system SQLite needed.

**If code execution fails:** Piston's public API (`emkc.org`) can be rate-limited. Self-host with Docker:

```bash
docker run -d --name piston -p 2000:2000 ghcr.io/engineer-man/piston
```

Then update the Piston URL in `main.cpp`.

## Project Structure

| File | Description |
|------|-------------|
| `main.cpp` | HTTP server, API routes, SQLite init, problem seed |
| `LinkedList.h/cpp` | Linked list with merge sort (underlying data structure) |
| `Problem.h` | `Problem` struct |
| `frontend/index.html` | Single-page UI — problem table, overlay, Monaco editor |
| `sqlite3.c/h` | SQLite amalgamation |
| `httplib.h` | Header-only HTTP server |
| `Makefile` | Build config |
