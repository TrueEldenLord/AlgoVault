# LeetCode DB — Expansion & Code Testing Design

**Date:** 2026-10-09
**Author:** Alexis Sanchez
**Project:** leetcode-database (C++ · Linked List · Merge Sort)

---

## Overview

Three feature areas added to the existing C++ HTTP server + NeetCode dark UI:

1. **Problem set expansion** — grow from 86 → ~150 problems (NeetCode 150), smarter Add modal
2. **SQLite description cache** — C++ backend caches problem metadata fetched on demand
3. **Full split-panel code testing** — Monaco editor + Judge0 execution, all languages, in-app

Architecture choice: **Option C — Frontend-heavy + C++ stays lean**
C++ gains SQLite for caching. Monaco editor and Judge0 calls live in the frontend JS. Judge0 API key is stored server-side (`.env` file, read via `getenv()`) and proxied so it never reaches the browser.

---

## Section 1 — Problem Set Expansion

**Seed data:** Expand `main.cpp` seed from 86 → ~150 problems by adding NeetCode 150 entries.
Two new categories added: **Tries** and **Advanced Graphs**.
Same insert pattern as existing seed — hardcoded `db.insert({...})` calls.

**Smarter Add Modal:**
When the user types a problem number and tabs/blurs the field, the frontend calls `GET /api/problems/:id/meta`. If a description exists (cached or freshly fetched), the title field auto-populates. User can override. Difficulty and topic still selected manually.

---

## Section 2 — SQLite Description Cache (C++ Backend)

**Dependency:** `sqlite3` amalgamation (`sqlite3.c` + `sqlite3.h`) added to project root — single-file, no system install needed. Makefile updated to compile it.

**Schema — `cache.db`:**
```sql
CREATE TABLE IF NOT EXISTS problem_meta (
  id              INTEGER PRIMARY KEY,
  title           TEXT,
  description_html TEXT,
  examples_json   TEXT,   -- JSON array of {input, output, explanation?}
  constraints     TEXT,
  fetched_at      INTEGER -- Unix timestamp
);
```

**New C++ endpoints:**

| Method | Path | Behavior |
|--------|------|----------|
| `GET` | `/api/problems/:id/meta` | Check SQLite → hit alfa-leetcode-api if missing → cache → return JSON |
| `GET` | `/api/languages` | Fetch Judge0 language list once, cache in-memory (std::vector), return JSON array |
| `POST` | `/api/run` | Proxy to Judge0 with server-side API key; return stdout/stderr/status/time/memory |

**External APIs:**
- **alfa-leetcode-api** (free, public, no key) — source for descriptions and examples
- **Judge0 via RapidAPI** (free tier: 50 req/day) — code execution

**API key storage:** `.env` file in project root (gitignored), one line: `JUDGE0_KEY=xxxxx`. `main.cpp` parses this file at startup with a simple `key=value` reader (reads line by line, splits on `=`) and stores the value in a static string. Falls back to the `JUDGE0_KEY` OS environment variable if the file is absent.

---

## Section 3 — Split-Panel Problem View (Frontend)

Clicking a problem title opens a **full-screen overlay** (replaces direct LeetCode navigation).

**Layout:**
- **Left panel (40%)** — Problem details:
  - Title + difficulty badge
  - "Open on LeetCode ↗" link (existing slug-based URL)
  - Description (rendered HTML from cache)
  - Examples with input/output blocks
  - Constraints section
- **Right panel (60%)** — Code environment:
  - Language selector (populated from `/api/languages`, defaults to C++)
  - Monaco editor (loaded from CDN — no npm/build step)
  - **Run** button — executes against fetched examples
  - **Submit** button — runs all examples, shows aggregate result
  - Results strip at bottom: per-test pass/fail, stdout, stderr, runtime, memory

**State preservation:** Closing the overlay restores the problem list at its previous scroll position, with filters intact.

**Theme:** Consistent with existing dark UI — `--bg: #0f0f0f`, `--accent: #2cbb5d`, cyan for runtime info.

---

## Section 4 — Test Cases & Judge0 Execution Flow

**Test cases source:** `examples_json` field from SQLite cache (populated from alfa-leetcode-api).
Each entry: `{ "input": "...", "output": "...", "explanation": "..." }`.

**Run flow:**
1. Frontend sends `POST /api/run` with `{ code, language_id, stdin: example.input }`
2. C++ proxies to Judge0 RapidAPI endpoint with stored key
3. Response `{ stdout, stderr, status, time, memory }` returned to frontend
4. Frontend trims and compares `stdout` to `example.output` → green ✓ / red ✗ per case

**Submit flow:** Iterates all examples sequentially, collects results, shows summary (e.g. "3 / 3 test cases passed").

**Language list:** `GET /api/languages` proxies Judge0's `/languages` endpoint on first call, caches result in a `static std::string` in memory for the process lifetime. Frontend populates the language dropdown from this.

**Rate limit indicator:** Results strip displays remaining Judge0 quota (returned in Judge0 response headers) as a small muted line: "Judge0: 47 requests remaining today."

---

## Files Changed

| File | Change |
|------|--------|
| `main.cpp` | Add SQLite init, new endpoints (`/meta`, `/run`, `/languages`), `.env` reader, expanded seed data |
| `sqlite3.c` | New — amalgamation source |
| `sqlite3.h` | New — amalgamation header |
| `Makefile` | Add `sqlite3.o` compilation, link flags |
| `frontend/index.html` | Split-panel overlay, Monaco CDN, language selector, results strip, smarter Add modal |
| `.env` | New (gitignored) — `JUDGE0_KEY=...` |
| `.gitignore` | New — ignore `.env` and `cache.db` |

---

## Out of Scope

- User accounts / saved solutions (no auth)
- Custom test case input (only fetched examples used)
- Offline execution (Judge0 requires internet)
- Persistent solve streaks or statistics beyond existing progress bar
