# LeetCode DB Expansion & Code Testing — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Expand the problem set to NeetCode 150 and add a full split-panel in-app code testing experience with Monaco editor and Judge0 execution.

**Architecture:** C++ server gains SQLite (amalgamation, no install) for caching problem descriptions fetched on-demand from alfa-leetcode-api. Judge0 API calls are proxied through C++ to keep the API key server-side. Monaco editor, language selector, and all execution UI live in the existing single-page `frontend/index.html`.

**Tech Stack:** C++17, cpp-httplib (existing), SQLite3 amalgamation (single-file), Monaco Editor 0.44 (CDN), Judge0 CE via RapidAPI (free tier), system `curl` (popen), alfa-leetcode-api (public, no key)

## Global Constraints

- C++ standard: C++17 (`-std=c++17`)
- No npm, no build step — frontend is pure HTML/JS, CDN only
- Server port 8080; existing `/api/problems` endpoints must not break
- SQLite amalgamation: single-file compile, no system SQLite install
- Judge0 API key stored in `.env` at project root, never sent to browser
- Run from project root: `./leetcode_db`
- Theme: dark `#0f0f0f` bg, `#2cbb5d` accent — all new UI matches existing style
- `-Wshadow` and `-Wunused-result` suppressions already in httplib include; don't add new suppression

---

## File Map

| File | Action | Responsibility |
|------|--------|----------------|
| `sqlite3.c` | Create | SQLite amalgamation — compiled as C |
| `sqlite3.h` | Create | SQLite amalgamation header |
| `main.cpp` | Modify | .env parser, SQLite init, 3 new endpoints, expanded seed |
| `Makefile` | Modify | Add sqlite3.o rule, CC variable, -ldl link flag |
| `frontend/index.html` | Modify | Split-panel overlay, Monaco, Run/Submit, smarter modal |
| `.env` | Create | `JUDGE0_KEY=your_key_here` (gitignored) |
| `.gitignore` | Create | Ignore `.env`, `cache.db`, object files, binary |

---

### Task 1: Project Scaffolding

**Files:**
- Create: `sqlite3.c`, `sqlite3.h`
- Create: `.env`, `.gitignore`
- Modify: `Makefile`

**Interfaces:**
- Produces: compilable project with SQLite; `.env` present for key storage

- [ ] **Step 1: Download SQLite amalgamation**

```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
curl -L "https://www.sqlite.org/2024/sqlite-amalgamation-3460100.zip" -o sqlite-amalg.zip
unzip -o sqlite-amalg.zip
cp sqlite-amalgamation-3460100/sqlite3.c .
cp sqlite-amalgamation-3460100/sqlite3.h .
rm -rf sqlite-amalgamation-3460100 sqlite-amalg.zip
```

Expected: `sqlite3.c` (~250 KB) and `sqlite3.h` appear in project root.

- [ ] **Step 2: Replace Makefile**

Current Makefile uses a generic `%.o` rule. Replace entirely with:

```makefile
CXX      = g++
CC       = gcc
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread
CFLAGS   = -O2 -DSQLITE_THREADSAFE=1
TARGET   = leetcode_db
OBJS     = main.o LinkedList.o sqlite3.o

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) -ldl

main.o: main.cpp LinkedList.h Problem.h sqlite3.h httplib.h
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

LinkedList.o: LinkedList.cpp LinkedList.h Problem.h
	$(CXX) $(CXXFLAGS) -c LinkedList.cpp -o LinkedList.o

sqlite3.o: sqlite3.c
	$(CC) $(CFLAGS) -c sqlite3.c -o sqlite3.o

clean:
	rm -f $(OBJS) $(TARGET) cache.db

.PHONY: clean
```

- [ ] **Step 3: Create .gitignore**

```
.env
cache.db
*.o
leetcode_db
sqlite-amalg.zip
sqlite-amalgamation-*/
```

- [ ] **Step 4: Create .env**

```
JUDGE0_KEY=your_rapidapi_key_here
```

Get a free key at RapidAPI → search "Judge0 CE" → subscribe to free plan (50 req/day).

- [ ] **Step 5: Verify compilation**

```bash
make clean && make
```

Expected: compiles without errors (sqlite3.c takes ~5 s); `leetcode_db` binary produced.

- [ ] **Step 6: Commit**

```bash
git add sqlite3.c sqlite3.h Makefile .gitignore
git commit -m "chore: add sqlite3 amalgamation and update Makefile for SQLite"
```

---

### Task 2: SQLite Init + .env Reader in C++

**Files:**
- Modify: `main.cpp` — add `#include "sqlite3.h"`, two globals, `initEnv()`, `initDb()`, call both in `main()`

**Interfaces:**
- Produces:
  - `static sqlite3* g_cacheDb` — open DB handle used by all cache functions
  - `static std::string g_judge0Key` — key loaded from `.env` or OS env
  - `void initEnv()` — populates `g_judge0Key`
  - `void initDb()` — opens `cache.db`, creates `problem_meta` table

- [ ] **Step 1: Add sqlite3 include and globals**

In `main.cpp`, after the existing `#include <vector>` line, add:

```cpp
#include "sqlite3.h"

static sqlite3*    g_cacheDb    = nullptr;
static std::string g_judge0Key;
static std::string g_languagesCache; // populated on first /api/languages call
```

- [ ] **Step 2: Add initEnv() after the readFile() function**

```cpp
static void initEnv() {
    std::ifstream f(".env");
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        if (key == "JUDGE0_KEY") g_judge0Key = val;
    }
    // Fallback to OS environment variable
    if (g_judge0Key.empty()) {
        const char* env = std::getenv("JUDGE0_KEY");
        if (env) g_judge0Key = env;
    }
}
```

- [ ] **Step 3: Add initDb() after initEnv()**

```cpp
static void initDb() {
    if (sqlite3_open("cache.db", &g_cacheDb) != SQLITE_OK) {
        std::cerr << "SQLite open failed: " << sqlite3_errmsg(g_cacheDb) << "\n";
        g_cacheDb = nullptr;
        return;
    }
    const char* sql =
        "CREATE TABLE IF NOT EXISTS problem_meta ("
        "  id                INTEGER PRIMARY KEY,"
        "  title             TEXT,"
        "  description_html  TEXT,"
        "  examples_json     TEXT,"
        "  constraints       TEXT,"
        "  fetched_at        INTEGER"
        ");";
    char* err = nullptr;
    sqlite3_exec(g_cacheDb, sql, nullptr, nullptr, &err);
    if (err) { std::cerr << "SQLite schema error: " << err << "\n"; sqlite3_free(err); }
}
```

- [ ] **Step 4: Call initEnv() and initDb() in main()**

In `main()`, before the `seed();` call, add:

```cpp
    initEnv();
    initDb();
```

- [ ] **Step 5: Build and verify**

```bash
make clean && make && ./leetcode_db &
sleep 1
curl -s http://localhost:8080/api/problems | python3 -c "import sys,json; d=json.load(sys.stdin); print('OK:', len(d), 'problems')"
ls -lh cache.db
kill %1
```

Expected: prints `OK: 86 problems`; `cache.db` file exists (may be 0 bytes — that's fine, table is empty).

- [ ] **Step 6: Commit**

```bash
git add main.cpp
git commit -m "feat: add SQLite init and .env reader to main.cpp"
```

---

### Task 3: Problem Meta Endpoint (`/api/problems/:id/meta`)

**Files:**
- Modify: `main.cpp` — add `toSlug()`, `httpGet()`, `metaFromCache()`, `metaToCache()`, new endpoint

**Interfaces:**
- Consumes: `g_cacheDb` (Task 2), `db` linked list (existing), `escJson()` (existing)
- Produces: `GET /api/problems/:id/meta` → JSON `{ id, title, description_html, examples_json, constraints }`

- [ ] **Step 1: Add toSlug() after escJson()**

```cpp
static std::string toSlug(const std::string& name) {
    std::string s;
    for (unsigned char c : name) {
        if (std::isalnum(c)) s += (char)std::tolower(c);
        else if (c == ' ' || c == '-') s += '-';
    }
    // Collapse duplicate dashes and trim trailing
    std::string out;
    bool prevDash = false;
    for (char c : s) {
        if (c == '-') { if (!prevDash) out += c; prevDash = true; }
        else          { out += c; prevDash = false; }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
}
```

- [ ] **Step 2: Add httpGet() after toSlug()**

```cpp
static std::string httpGet(const std::string& url) {
    // url is constructed from our own slug — safe to use in shell command
    std::string cmd = "curl -s --max-time 10 \"" + url + "\"";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "";
    std::ostringstream ss;
    char buf[4096];
    while (fgets(buf, sizeof(buf), p)) ss << buf;
    pclose(p);
    return ss.str();
}
```

- [ ] **Step 3: Add metaFromCache() after httpGet()**

```cpp
static std::string metaFromCache(int id) {
    if (!g_cacheDb) return "";
    const char* sql =
        "SELECT title, description_html, examples_json, constraints "
        "FROM problem_meta WHERE id = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_cacheDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return "";
    sqlite3_bind_int(stmt, 1, id);
    std::string result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        auto col = [&](int i) -> std::string {
            const unsigned char* v = sqlite3_column_text(stmt, i);
            return v ? reinterpret_cast<const char*>(v) : "";
        };
        std::string exJson = col(2);
        if (exJson.empty()) exJson = "[]";
        result = "{\"id\":"                 + std::to_string(id) +
                 ",\"title\":\""            + escJson(col(0)) + "\"" +
                 ",\"description_html\":\"" + escJson(col(1)) + "\"" +
                 ",\"examples_json\":"      + exJson           +
                 ",\"constraints\":\""      + escJson(col(3)) + "\"}";
    }
    sqlite3_finalize(stmt);
    return result;
}
```

- [ ] **Step 4: Add metaToCache() after metaFromCache()**

```cpp
static void metaToCache(int id, const std::string& title,
                         const std::string& descHtml,
                         const std::string& examplesJson,
                         const std::string& constraints) {
    if (!g_cacheDb) return;
    const char* sql =
        "INSERT OR REPLACE INTO problem_meta "
        "(id, title, description_html, examples_json, constraints, fetched_at) "
        "VALUES (?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(g_cacheDb, sql, -1, &stmt, nullptr) != SQLITE_OK) return;
    sqlite3_bind_int   (stmt, 1, id);
    sqlite3_bind_text  (stmt, 2, title.c_str(),        -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 3, descHtml.c_str(),     -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 4, examplesJson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text  (stmt, 5, constraints.c_str(),  -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64 (stmt, 6, (sqlite3_int64)std::time(nullptr));
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
```

Add `#include <ctime>` to the includes at the top of `main.cpp` (after `#include <vector>`).

- [ ] **Step 5: Add /api/problems/:id/meta endpoint in main()**

In `main()`, after the PATCH endpoint block and before the OPTIONS block, add:

```cpp
    // ── GET /api/problems/:id/meta ────────────────────────────
    svr.Get(R"(/api/problems/(\d+)/meta)", [](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        int id = std::stoi(req.matches[1]);

        // 1. Check SQLite cache first
        std::string cached = metaFromCache(id);
        if (!cached.empty()) {
            res.set_content(cached, "application/json");
            return;
        }

        // 2. Look up problem name in linked list
        std::string name;
        {
            std::lock_guard<std::mutex> lock(db_mutex);
            for (Node* cur = db.getHead(); cur; cur = cur->next) {
                if (cur->data.id == id) { name = cur->data.name; break; }
            }
        }
        if (name.empty()) {
            res.status = 404;
            res.set_content("{\"error\":\"Problem not found\"}", "application/json");
            return;
        }

        // 3. Fetch from alfa-leetcode-api using title slug
        std::string slug = toSlug(name);
        std::string url  = "https://alfa-leetcode-api.onrender.com/select?titleSlug=" + slug;
        std::string body = httpGet(url);

        if (body.empty() || body.find("title") == std::string::npos) {
            res.status = 502;
            res.set_content("{\"error\":\"External API unavailable\"}", "application/json");
            return;
        }

        // 4. Extract fields — jStr stops at first unescaped '"' so HTML fields
        //    with attribute quotes may be truncated; acceptable for this app.
        std::string title       = jStr(body, "title");
        std::string descHtml    = jStr(body, "content");
        std::string constraints = jStr(body, "constraints");
        std::string examples    = jStr(body, "exampleTestcases");
        if (title.empty()) title = name;

        // Wrap raw exampleTestcases string in a single-element JSON array
        std::string exJson = "[{\"input\":\"" + escJson(examples) + "\",\"output\":\"\"}]";

        // 5. Cache and return
        metaToCache(id, title, descHtml, exJson, constraints);
        res.set_content(metaFromCache(id), "application/json");
    });
```

- [ ] **Step 6: Build and test**

```bash
make clean && make && ./leetcode_db &
sleep 1
# First call — fetches from external API (~2-3 s)
curl -s "http://localhost:8080/api/problems/1/meta" | python3 -m json.tool | head -10
# Second call — instant from SQLite cache
curl -s "http://localhost:8080/api/problems/1/meta" | python3 -c "import sys,json; d=json.load(sys.stdin); print('cached title:', d.get('title'))"
kill %1
```

Expected: first call returns JSON with `title`, `description_html`, etc.; second call returns instantly with same data.

- [ ] **Step 7: Commit**

```bash
git add main.cpp
git commit -m "feat: add /api/problems/:id/meta with SQLite caching and alfa-leetcode-api fetch"
```

---

### Task 4: Judge0 Proxy Endpoints

**Files:**
- Modify: `main.cpp` — add `judge0Get()`, `judge0Post()`, `/api/languages`, `/api/run`

**Interfaces:**
- Consumes: `g_judge0Key` (Task 2), `g_languagesCache` (Task 2), `escJson()` (existing)
- Produces:
  - `GET /api/languages` → Judge0 JSON array of `[{ id, name }, ...]`
  - `POST /api/run` body `{ code, language_id, stdin }` → Judge0 JSON `{ stdout, stderr, compile_output, status, time, memory }`

- [ ] **Step 1: Add judge0Get() after httpGet()**

```cpp
static std::string judge0Get(const std::string& path, const std::string& key) {
    std::string cmd =
        "curl -s --max-time 15 "
        "\"https://judge0-ce.p.rapidapi.com" + path + "\" "
        "-H \"X-RapidAPI-Key: " + key + "\" "
        "-H \"X-RapidAPI-Host: judge0-ce.p.rapidapi.com\"";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "[]";
    std::ostringstream ss;
    char buf[4096];
    while (fgets(buf, sizeof(buf), p)) ss << buf;
    pclose(p);
    return ss.str();
}
```

- [ ] **Step 2: Add judge0Post() after judge0Get()**

```cpp
static std::string judge0Post(const std::string& path,
                               const std::string& jsonBody,
                               const std::string& key) {
    // Write body to temp file to avoid shell quoting issues with code content
    const std::string tmp = "/tmp/lc_judge0_body.json";
    { std::ofstream f(tmp); f << jsonBody; }

    std::string cmd =
        "curl -s --max-time 30 "
        "-X POST \"https://judge0-ce.p.rapidapi.com" + path + "\" "
        "-H \"Content-Type: application/json\" "
        "-H \"X-RapidAPI-Key: " + key + "\" "
        "-H \"X-RapidAPI-Host: judge0-ce.p.rapidapi.com\" "
        "-d @" + tmp;

    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "{\"error\":\"curl failed\"}";
    std::ostringstream ss;
    char buf[4096];
    while (fgets(buf, sizeof(buf), p)) ss << buf;
    pclose(p);
    return ss.str();
}
```

- [ ] **Step 3: Add /api/languages endpoint in main()**

After the `/api/problems/:id/meta` endpoint, add:

```cpp
    // ── GET /api/languages ────────────────────────────────────
    svr.Get("/api/languages", [](const httplib::Request&, httplib::Response& res) {
        cors(res);
        if (g_judge0Key.empty()) {
            res.status = 503;
            res.set_content("{\"error\":\"JUDGE0_KEY not configured in .env\"}", "application/json");
            return;
        }
        if (g_languagesCache.empty())
            g_languagesCache = judge0Get("/languages", g_judge0Key);
        res.set_content(g_languagesCache, "application/json");
    });
```

- [ ] **Step 4: Add /api/run endpoint after /api/languages**

```cpp
    // ── POST /api/run ─────────────────────────────────────────
    svr.Post("/api/run", [](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        if (g_judge0Key.empty()) {
            res.status = 503;
            res.set_content("{\"error\":\"JUDGE0_KEY not configured in .env\"}", "application/json");
            return;
        }

        std::string code      = jStr(req.body, "code");
        int         langId    = jInt(req.body, "language_id");
        std::string stdinData = jStr(req.body, "stdin");

        if (code.empty() || langId < 1) {
            res.status = 400;
            res.set_content("{\"error\":\"code and language_id required\"}", "application/json");
            return;
        }

        std::string body =
            "{\"source_code\":\""  + escJson(code)                 + "\","
            "\"language_id\":"     + std::to_string(langId)        + ","
            "\"stdin\":\""         + escJson(stdinData)            + "\"}";

        std::string result = judge0Post(
            "/submissions?base64_encoded=false&wait=true", body, g_judge0Key);
        res.set_content(result, "application/json");
    });
```

- [ ] **Step 5: Build and test**

```bash
make clean && make && ./leetcode_db &
sleep 1

# Test languages endpoint (requires valid JUDGE0_KEY in .env)
curl -s http://localhost:8080/api/languages | python3 -c "import sys,json; d=json.load(sys.stdin); print(f'{len(d)} languages')"

# Test /api/run — Hello World in C++ (language_id 54 = C++ GCC 9.2.0)
curl -s -X POST http://localhost:8080/api/run \
  -H "Content-Type: application/json" \
  -d '{"code":"#include<iostream>\nint main(){std::cout<<\"Hello World\";}","language_id":54,"stdin":""}' \
  | python3 -m json.tool

kill %1
```

Expected: languages returns `~50 languages`; run returns `{"stdout":"Hello World","status":{"id":3,"description":"Accepted"},...}`.

- [ ] **Step 6: Commit**

```bash
git add main.cpp
git commit -m "feat: add /api/languages and /api/run Judge0 proxy endpoints"
```

---

### Task 5: Expand Seed Data to NeetCode 150

**Files:**
- Modify: `main.cpp` — ~62 new `db.insert()` calls; `frontend/index.html` — 2 new CATEGORIES entries + 2 new `<option>` tags

**Interfaces:**
- Produces: ~150 seeded problems across 18 categories (Tries and Advanced Graphs added)

- [ ] **Step 1: Add new inserts to seed() in main.cpp**

In `seed()`, add the following after each existing category block.

**After Arrays & Hashing block:**
```cpp
    db.insert({36,   "Valid Sudoku",                                          "Medium", false, {"Arrays & Hashing"}});
```

**After Two Pointers block:**
```cpp
    db.insert({167,  "Two Sum II - Input Array Is Sorted",                   "Medium", false, {"Two Pointers"}});
```

**After Sliding Window block:**
```cpp
    db.insert({239,  "Sliding Window Maximum",                               "Hard",   false, {"Sliding Window"}});
    db.insert({567,  "Permutation in String",                                "Medium", false, {"Sliding Window"}});
```

**After Stack block:**
```cpp
    db.insert({853,  "Car Fleet",                                            "Medium", false, {"Stack"}});
```

**After Binary Search block:**
```cpp
    db.insert({74,   "Search a 2D Matrix",                                   "Medium", false, {"Binary Search"}});
    db.insert({981,  "Time Based Key-Value Store",                           "Medium", false, {"Binary Search"}});
```

**After Linked List block:**
```cpp
    db.insert({2,    "Add Two Numbers",                                      "Medium", false, {"Linked List"}});
    db.insert({25,   "Reverse Nodes in K-Group",                             "Hard",   false, {"Linked List"}});
    db.insert({138,  "Copy List with Random Pointer",                        "Medium", false, {"Linked List"}});
    db.insert({146,  "LRU Cache",                                            "Medium", false, {"Linked List"}});
    db.insert({287,  "Find the Duplicate Number",                            "Medium", false, {"Linked List"}});
```

**After Trees block:**
```cpp
    db.insert({1448, "Count Good Nodes in Binary Tree",                      "Medium", false, {"Trees"}});
```

**After Heap / Priority Queue block:**
```cpp
    db.insert({621,  "Task Scheduler",                                       "Medium", false, {"Heap / Priority Queue"}});
    db.insert({703,  "Kth Largest Element in a Stream",                      "Easy",   false, {"Heap / Priority Queue"}});
    db.insert({1046, "Last Stone Weight",                                    "Easy",   false, {"Heap / Priority Queue"}});
```

**After Backtracking block:**
```cpp
    db.insert({17,   "Letter Combinations of a Phone Number",                "Medium", false, {"Backtracking"}});
    db.insert({40,   "Combination Sum II",                                   "Medium", false, {"Backtracking"}});
    db.insert({51,   "N-Queens",                                             "Hard",   false, {"Backtracking"}});
    db.insert({90,   "Subsets II",                                           "Medium", false, {"Backtracking"}});
```

**After Graphs block:**
```cpp
    db.insert({130,  "Surrounded Regions",                                   "Medium", false, {"Graphs"}});
    db.insert({261,  "Graph Valid Tree",                                     "Medium", false, {"Graphs"}});
    db.insert({286,  "Walls and Gates",                                      "Medium", false, {"Graphs"}});
    db.insert({323,  "Number of Connected Components in an Undirected Graph","Medium", false, {"Graphs"}});
    db.insert({684,  "Redundant Connection",                                 "Medium", false, {"Graphs"}});
    db.insert({994,  "Rotting Oranges",                                      "Medium", false, {"Graphs"}});

    // Tries
    db.insert({208,  "Implement Trie (Prefix Tree)",                         "Medium", false, {"Tries"}});
    db.insert({211,  "Design Add and Search Words Data Structure",            "Medium", false, {"Tries"}});
    db.insert({212,  "Word Search II",                                       "Hard",   false, {"Tries"}});

    // Advanced Graphs
    db.insert({269,  "Alien Dictionary",                                     "Hard",   false, {"Advanced Graphs"}});
    db.insert({332,  "Reconstruct Itinerary",                                "Hard",   false, {"Advanced Graphs"}});
    db.insert({743,  "Network Delay Time",                                   "Medium", false, {"Advanced Graphs"}});
    db.insert({778,  "Swim in Rising Water",                                 "Hard",   false, {"Advanced Graphs"}});
    db.insert({787,  "Cheapest Flights Within K Stops",                      "Medium", false, {"Advanced Graphs"}});
    db.insert({1584, "Min Cost to Connect All Points",                       "Medium", false, {"Advanced Graphs"}});
```

**After 1-D DP block:**
```cpp
    db.insert({5,    "Longest Palindromic Substring",                        "Medium", false, {"1-D DP"}});
    db.insert({152,  "Maximum Product Subarray",                             "Medium", false, {"1-D DP"}});
    db.insert({416,  "Partition Equal Subset Sum",                           "Medium", false, {"1-D DP"}});
    db.insert({647,  "Palindromic Substrings",                               "Medium", false, {"1-D DP"}});
    db.insert({746,  "Min Cost Climbing Stairs",                             "Easy",   false, {"1-D DP"}});
```

**After 2-D DP block:**
```cpp
    db.insert({10,   "Regular Expression Matching",                          "Hard",   false, {"2-D DP"}});
    db.insert({97,   "Interleaving String",                                  "Medium", false, {"2-D DP"}});
    db.insert({115,  "Distinct Subsequences",                                "Hard",   false, {"2-D DP"}});
    db.insert({309,  "Best Time to Buy and Sell Stock with Cooldown",        "Medium", false, {"2-D DP"}});
    db.insert({312,  "Burst Balloons",                                       "Hard",   false, {"2-D DP"}});
    db.insert({329,  "Longest Increasing Path in a Matrix",                  "Hard",   false, {"2-D DP"}});
    db.insert({494,  "Target Sum",                                           "Medium", false, {"2-D DP"}});
    db.insert({518,  "Coin Change II",                                       "Medium", false, {"2-D DP"}});
```

**After Greedy block:**
```cpp
    db.insert({678,  "Valid Parenthesis String",                             "Medium", false, {"Greedy"}});
    db.insert({763,  "Partition Labels",                                     "Medium", false, {"Greedy"}});
    db.insert({846,  "Hand of Straights",                                    "Medium", false, {"Greedy"}});
    db.insert({1899, "Merge Triplets to Form Target Triplet",                "Medium", false, {"Greedy"}});
```

**After Intervals block:**
```cpp
    db.insert({1851, "Minimum Interval to Include Each Query",               "Hard",   false, {"Intervals"}});
```

**After Math & Geometry block:**
```cpp
    db.insert({43,   "Multiply Strings",                                     "Medium", false, {"Math & Geometry"}});
    db.insert({50,   "Pow(x, n)",                                            "Medium", false, {"Math & Geometry"}});
    db.insert({66,   "Plus One",                                             "Easy",   false, {"Math & Geometry"}});
    db.insert({202,  "Happy Number",                                         "Easy",   false, {"Math & Geometry"}});
    db.insert({2013, "Detect Squares",                                       "Medium", false, {"Math & Geometry"}});
```

**After Bit Manipulation block:**
```cpp
    db.insert({7,    "Reverse Integer",                                      "Medium", false, {"Bit Manipulation"}});
    db.insert({136,  "Single Number",                                        "Easy",   false, {"Bit Manipulation"}});
```

- [ ] **Step 2: Update CATEGORIES array in frontend/index.html**

Find the `const CATEGORIES = [` array in the `<script>` block. After `'Graphs'`, add:

```javascript
    'Tries', 'Advanced Graphs',
```

So it reads: `...'Backtracking','Graphs','Tries','Advanced Graphs','1-D DP',...`

- [ ] **Step 3: Add new options to the topic select in the Add Problem modal**

Find the `<select class="form-ctrl" id="fTopic">` element. After the `<option value="Graphs">Graphs</option>` line, add:

```html
        <option value="Tries">Tries</option>
        <option value="Advanced Graphs">Advanced Graphs</option>
```

- [ ] **Step 4: Build and verify count**

```bash
make clean && make && ./leetcode_db &
sleep 1
curl -s http://localhost:8080/api/problems | python3 -c "import sys,json; d=json.load(sys.stdin); print('Total:', len(d))"
kill %1
```

Expected: prints `Total: 150` (give or take — 86 original + ~64 new).

- [ ] **Step 5: Commit**

```bash
git add main.cpp frontend/index.html
git commit -m "feat: expand seed data to NeetCode 150 with Tries and Advanced Graphs categories"
```

---

### Task 6: Split-Panel Overlay + Monaco Editor

**Files:**
- Modify: `frontend/index.html` — Monaco CDN script tag, overlay CSS, overlay HTML, overlay JS

**Interfaces:**
- Consumes: `GET /api/problems/:id/meta` (Task 3), `GET /api/languages` (Task 4)
- Produces:
  - `openProblem(id, name)` — opens overlay, loads description, loads languages once
  - `closeProblem()` — closes overlay, restores list
  - `window.monacoEditor` — Monaco editor instance
  - `lcLanguages` — array of `{ id, name }` from Judge0

- [ ] **Step 1: Add Monaco CDN script tag**

In `<head>`, after the Google Fonts link tag, add:

```html
  <script src="https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.44.0/min/vs/loader.min.js"></script>
```

- [ ] **Step 2: Add overlay CSS**

Inside the `<style>` block, before the closing `</style>`, add:

```css
    /* ── Problem Overlay ────────────────────────────────────── */
    .problem-overlay {
      position: fixed; inset: 0; z-index: 500;
      background: var(--bg); display: none; flex-direction: column;
    }
    .problem-overlay.open { display: flex; }
    .overlay-topbar {
      height: 48px; flex-shrink: 0;
      background: rgba(15,15,15,0.96); border-bottom: 1px solid var(--border);
      display: flex; align-items: center; padding: 0 16px; gap: 12px;
    }
    .overlay-back {
      background: none; border: none; color: var(--muted2); cursor: pointer;
      display: flex; align-items: center; gap: 6px; font-size: 13px;
      font-family: inherit; padding: 5px 8px; border-radius: 6px; transition: all .15s;
    }
    .overlay-back:hover { color: var(--text); background: var(--surface); }
    .overlay-title { font-weight: 600; font-size: 14px; flex: 1; white-space: nowrap; overflow: hidden; text-overflow: ellipsis; }
    .overlay-lc-link {
      font-size: 12px; color: var(--accent); text-decoration: none;
      display: flex; align-items: center; gap: 4px; flex-shrink: 0;
      padding: 4px 10px; border: 1px solid rgba(44,187,93,.3);
      border-radius: 6px; transition: all .15s;
    }
    .overlay-lc-link:hover { background: rgba(44,187,93,.1); }
    .overlay-body { flex: 1; display: flex; overflow: hidden; }
    .problem-desc {
      width: 40%; flex-shrink: 0; border-right: 1px solid var(--border);
      overflow-y: auto; padding: 20px 24px;
    }
    .problem-desc h1 { font-size: 18px; font-weight: 700; margin-bottom: 12px; }
    .problem-desc-content { font-size: 13.5px; line-height: 1.75; color: var(--muted2); }
    .problem-desc-content p { margin-bottom: 12px; }
    .problem-desc-content pre {
      background: var(--surface2); border: 1px solid var(--border);
      border-radius: 6px; padding: 10px 14px; overflow-x: auto;
      font-size: 12.5px; margin: 10px 0; white-space: pre-wrap;
    }
    .problem-desc-content code {
      background: var(--surface2); border-radius: 4px; padding: 1px 5px; font-size: 12px;
    }
    .desc-loading { color: var(--muted); font-size: 13px; padding: 8px 0; }
    .problem-editor { flex: 1; display: flex; flex-direction: column; overflow: hidden; min-width: 0; }
    .editor-toolbar {
      height: 44px; flex-shrink: 0; border-bottom: 1px solid var(--border);
      background: var(--surface); display: flex; align-items: center; padding: 0 12px; gap: 8px;
    }
    .lang-select {
      background: var(--bg); border: 1px solid var(--border); border-radius: 6px;
      color: var(--text); font-size: 12px; font-family: inherit;
      padding: 4px 28px 4px 10px; cursor: pointer; outline: none; appearance: none;
      background-image: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' width='12' height='12' viewBox='0 0 24 24' fill='none' stroke='%236b7280' stroke-width='2'%3E%3Cpath d='M6 9l6 6 6-6'/%3E%3C/svg%3E");
      background-repeat: no-repeat; background-position: right 8px center; max-width: 200px;
    }
    .editor-spacer { flex: 1; }
    .btn-run {
      padding: 5px 14px; border-radius: 6px; font-size: 12px; font-weight: 600;
      font-family: inherit; cursor: pointer; border: 1px solid var(--border);
      background: var(--surface2); color: var(--text); transition: all .15s;
    }
    .btn-run:hover { border-color: #555; }
    .btn-submit {
      padding: 5px 14px; border-radius: 6px; font-size: 12px; font-weight: 600;
      font-family: inherit; cursor: pointer; border: none;
      background: var(--accent); color: #0f0f0f; transition: filter .15s;
    }
    .btn-submit:hover { filter: brightness(1.1); }
    .btn-run:disabled, .btn-submit:disabled { opacity: .5; cursor: not-allowed; filter: none; }
    #monaco-container { flex: 1; min-height: 0; }
    .results-strip {
      border-top: 1px solid var(--border); background: var(--surface);
      max-height: 220px; overflow-y: auto; padding: 10px 14px; font-size: 12px;
    }
    .result-case {
      display: flex; align-items: flex-start; gap: 10px;
      padding: 8px 0; border-bottom: 1px solid var(--border);
    }
    .result-case:last-child { border-bottom: none; }
    .result-badge {
      flex-shrink: 0; font-weight: 700; font-size: 11px;
      padding: 2px 7px; border-radius: 10px; margin-top: 1px;
    }
    .result-badge.pass { background: rgba(44,187,93,.15); color: var(--easy); }
    .result-badge.fail { background: rgba(239,71,67,.15);  color: var(--hard); }
    .result-badge.err  { background: rgba(255,184,0,.15);  color: var(--medium); }
    .result-detail { flex: 1; min-width: 0; }
    .result-detail pre {
      margin: 2px 0; font-size: 11px; color: var(--muted2);
      white-space: pre-wrap; word-break: break-all;
    }
    .result-meta { font-size: 10px; color: var(--muted); margin-top: 3px; }
    .results-summary { padding: 6px 0 8px; font-weight: 600; font-size: 13px; }
    .results-summary.all-pass { color: var(--easy); }
    .results-summary.has-fail { color: var(--hard); }
```

- [ ] **Step 3: Add overlay HTML**

Just before the `<!-- Modal -->` comment, add:

```html
<!-- Problem Overlay -->
<div class="problem-overlay" id="problemOverlay">
  <div class="overlay-topbar">
    <button class="overlay-back" onclick="closeProblem()">
      <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
        <path d="M19 12H5M12 19l-7-7 7-7"/>
      </svg>
      Problems
    </button>
    <span class="overlay-title" id="overlayTitle"></span>
    <a class="overlay-lc-link" id="overlayLcLink" href="#" target="_blank" rel="noopener">
      Open on LeetCode
      <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
        <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/>
        <polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/>
      </svg>
    </a>
  </div>
  <div class="overlay-body">
    <div class="problem-desc" id="problemDesc">
      <div class="desc-loading">Loading problem...</div>
    </div>
    <div class="problem-editor">
      <div class="editor-toolbar">
        <select class="lang-select" id="langSelect" onchange="onLangChange()">
          <option value="54">C++ (GCC 9.2.0)</option>
        </select>
        <div class="editor-spacer"></div>
        <button class="btn-run"    id="btnRun"    onclick="runCode()">Run</button>
        <button class="btn-submit" id="btnSubmit" onclick="submitCode()">Submit</button>
      </div>
      <div id="monaco-container"></div>
      <div class="results-strip" id="resultsStrip" style="display:none"></div>
    </div>
  </div>
</div>
```

- [ ] **Step 4: Add overlay JS in the script block**

Before the closing `</script>`, add:

```javascript
  // ── Problem Overlay ──────────────────────────────────────────

  let currentProblemId = null;
  let currentExamples  = [];
  let lcLanguages      = [];
  let monacoEditor     = null;
  let monacoReady      = false;
  let judge0RunsUsed   = 0; // client-side counter (Judge0 free tier: 50/day)

  require.config({ paths: { vs: 'https://cdnjs.cloudflare.com/ajax/libs/monaco-editor/0.44.0/min/vs' } });
  require(['vs/editor/editor.main'], function () {
    monacoEditor = monaco.editor.create(document.getElementById('monaco-container'), {
      value: '// Select a problem to start coding\n',
      language: 'cpp',
      theme: 'vs-dark',
      fontSize: 14,
      minimap: { enabled: false },
      scrollBeyondLastLine: false,
      automaticLayout: true,
    });
    monacoReady = true;
  });

  async function loadLanguages() {
    try {
      const r = await fetch('/api/languages');
      if (!r.ok) return;
      lcLanguages = await r.json();
      const sel = document.getElementById('langSelect');
      sel.innerHTML = lcLanguages
        .map(l => `<option value="${l.id}">${esc(l.name)}</option>`)
        .join('');
      // Default to C++ GCC
      const cpp = [...sel.options].find(o => o.text.includes('C++') && o.text.includes('GCC'));
      if (cpp) sel.value = cpp.value;
    } catch {}
  }

  function getMonacoLang(langId) {
    const lang = lcLanguages.find(l => l.id === parseInt(langId));
    if (!lang) return 'plaintext';
    const n = lang.name.toLowerCase();
    if (n.includes('python'))     return 'python';
    if (n.includes('javascript')) return 'javascript';
    if (n.includes('typescript')) return 'typescript';
    if (n.includes('java') && !n.includes('javascript')) return 'java';
    if (n.includes('c++') || n.includes('cpp')) return 'cpp';
    if (n.includes('c#'))   return 'csharp';
    if (n.includes('go'))   return 'go';
    if (n.includes('rust')) return 'rust';
    if (n.includes('ruby')) return 'ruby';
    if (n.includes('swift'))  return 'swift';
    if (n.includes('kotlin')) return 'kotlin';
    return 'plaintext';
  }

  function onLangChange() {
    if (!monacoReady) return;
    const langId = document.getElementById('langSelect').value;
    monaco.editor.setModelLanguage(monacoEditor.getModel(), getMonacoLang(parseInt(langId)));
  }

  async function openProblem(id, name) {
    currentProblemId = id;
    document.getElementById('overlayTitle').textContent = name;
    document.getElementById('overlayLcLink').href = 'https://leetcode.com/problems/' + slug(name) + '/';
    document.getElementById('problemDesc').innerHTML = '<div class="desc-loading">Loading description...</div>';
    document.getElementById('resultsStrip').style.display = 'none';
    document.getElementById('problemOverlay').classList.add('open');
    if (lcLanguages.length === 0) await loadLanguages();
    try {
      const r = await fetch('/api/problems/' + id + '/meta');
      if (!r.ok) throw new Error('not found');
      const meta = await r.json();
      currentExamples = Array.isArray(meta.examples_json) ? meta.examples_json : [];
      document.getElementById('problemDesc').innerHTML =
        '<h1>' + esc(meta.title || name) + '</h1>' +
        '<div class="problem-desc-content">' + (meta.description_html || '<p>No description available.</p>') + '</div>';
    } catch {
      document.getElementById('problemDesc').innerHTML =
        '<div class="desc-loading">Could not load description — ' +
        '<a class="overlay-lc-link" style="display:inline-flex" href="' +
        document.getElementById('overlayLcLink').href +
        '" target="_blank" rel="noopener">open on LeetCode ↗</a></div>';
    }
  }

  function closeProblem() {
    document.getElementById('problemOverlay').classList.remove('open');
    currentProblemId = null;
    currentExamples  = [];
  }
```

- [ ] **Step 5: Change problem title link to open overlay**

In the `render()` function, find this line:
```javascript
<a class="t-name ${p.solved ? 'striked' : ''}" href="https://leetcode.com/problems/${slug(p.name)}/" target="_blank" rel="noopener">${esc(p.name)}</a>
```

Replace it with:
```javascript
<a class="t-name ${p.solved ? 'striked' : ''}" href="#" onclick="openProblem(${p.id}, ${JSON.stringify(p.name)}); return false;">${esc(p.name)}</a>
```

- [ ] **Step 6: Build and test visually**

```bash
make && ./leetcode_db &
sleep 1
open http://localhost:8080
```

Manual checks:
1. Problem list loads — verify ~150 problems, Tries and Advanced Graphs appear in sidebar
2. Click "Two Sum" → full-screen overlay opens
3. Left panel shows "Loading description..." then populates with HTML content
4. Right panel has Monaco editor with C++ syntax highlighting
5. Language dropdown populates with all Judge0 languages (~50)
6. Changing language updates Monaco syntax highlighting
7. Back arrow closes overlay, list is intact

```bash
kill %1
```

- [ ] **Step 7: Commit**

```bash
git add frontend/index.html
git commit -m "feat: add split-panel problem overlay with Monaco editor and language selector"
```

---

### Task 7: Run/Submit Execution + Smarter Add Modal

**Files:**
- Modify: `frontend/index.html` — `runCode()`, `submitCode()`, `buildResultHtml()`, Add modal auto-fill

**Interfaces:**
- Consumes: `POST /api/run` (Task 4), `currentExamples`, `monacoEditor`, `lcLanguages`
- Produces: working Run/Submit buttons with per-test results; Add modal auto-fills title from problem number

- [ ] **Step 1: Add runCode() in the script block after closeProblem()**

```javascript
  async function runCode() {
    if (!monacoReady) return;
    const code   = monacoEditor.getValue();
    const langId = parseInt(document.getElementById('langSelect').value);
    const strip  = document.getElementById('resultsStrip');
    const btnRun = document.getElementById('btnRun');
    const btnSub = document.getElementById('btnSubmit');

    strip.style.display = 'block';
    if (currentExamples.length === 0) {
      strip.innerHTML = '<div class="desc-loading">No test cases cached for this problem yet.</div>';
      return;
    }

    btnRun.disabled = btnSub.disabled = true;
    btnRun.textContent = 'Running...';
    strip.innerHTML = '<div class="desc-loading">Running...</div>';

    try {
      const r = await fetch('/api/run', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ code, language_id: langId, stdin: currentExamples[0].input || '' })
      });
      const data = await r.json();
      judge0RunsUsed++;
      strip.innerHTML = buildResultHtml([{ ex: currentExamples[0], data }], false);
    } catch (e) {
      strip.innerHTML = '<div class="desc-loading">Network error: ' + esc(e.message) + '</div>';
    } finally {
      btnRun.disabled = btnSub.disabled = false;
      btnRun.textContent = 'Run';
    }
  }
```

- [ ] **Step 2: Add submitCode() after runCode()**

```javascript
  async function submitCode() {
    if (!monacoReady) return;
    const code   = monacoEditor.getValue();
    const langId = parseInt(document.getElementById('langSelect').value);
    const strip  = document.getElementById('resultsStrip');
    const btnRun = document.getElementById('btnRun');
    const btnSub = document.getElementById('btnSubmit');

    strip.style.display = 'block';
    if (currentExamples.length === 0) {
      strip.innerHTML = '<div class="desc-loading">No test cases available.</div>';
      return;
    }

    btnRun.disabled = btnSub.disabled = true;
    btnSub.textContent = 'Submitting...';
    strip.innerHTML = '<div class="desc-loading">Running all test cases...</div>';

    const results = [];
    for (const ex of currentExamples) {
      try {
        const r = await fetch('/api/run', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ code, language_id: langId, stdin: ex.input || '' })
        });
        const data = await r.json();
        judge0RunsUsed++;
        results.push({ ex, data });
      } catch (e) {
        results.push({ ex, data: { error: e.message, status: { description: 'Network Error' } } });
      }
    }

    strip.innerHTML = buildResultHtml(results, true);
    btnRun.disabled = btnSub.disabled = false;
    btnSub.textContent = 'Submit';
  }
```

- [ ] **Step 3: Add buildResultHtml() after submitCode()**

```javascript
  function buildResultHtml(results, showSummary) {
    let html = '';
    let passed = 0;

    for (const { ex, data } of results) {
      const stdout   = (data.stdout   || '').trim();
      const expected = (ex.output     || '').trim();
      const statusId = data.status?.id;
      const isPass   = expected ? stdout === expected : statusId === 3;
      const isErr    = statusId && statusId !== 3;
      if (isPass) passed++;

      html += `<div class="result-case">
        <span class="result-badge ${isPass ? 'pass' : isErr ? 'err' : 'fail'}">
          ${isPass ? '✓ Pass' : isErr ? esc(data.status?.description || 'Error') : '✗ Fail'}
        </span>
        <div class="result-detail">
          ${ex.input          ? `<pre>Input: ${esc(ex.input)}</pre>`                                    : ''}
          ${stdout            ? `<pre>Output: ${esc(stdout)}</pre>`                                     : ''}
          ${expected          ? `<pre>Expected: ${esc(expected)}</pre>`                                 : ''}
          ${data.stderr       ? `<pre style="color:var(--hard)">Stderr: ${esc(data.stderr)}</pre>`      : ''}
          ${data.compile_output ? `<pre style="color:var(--medium)">Compile: ${esc(data.compile_output)}</pre>` : ''}
          <div class="result-meta">${data.time ? data.time + 's' : ''}${data.memory ? ' · ' + data.memory + ' KB' : ''} · ${50 - judge0RunsUsed} Judge0 requests remaining today</div>
        </div>
      </div>`;
    }

    const summary = showSummary
      ? `<div class="results-summary ${passed === results.length ? 'all-pass' : 'has-fail'}">${passed} / ${results.length} test cases passed</div>`
      : '';
    return summary + html;
  }
```

- [ ] **Step 4: Add smarter Add modal auto-fill**

In the `<script>` block, after the `load();` call at the bottom, add:

```javascript
  // Auto-fill title when problem # is entered in Add modal
  document.getElementById('fId').addEventListener('blur', async function () {
    const id = parseInt(this.value);
    if (!id || id < 1) return;
    if (document.getElementById('fName').value.trim()) return; // don't overwrite
    try {
      const r = await fetch('/api/problems/' + id + '/meta');
      if (!r.ok) return;
      const meta = await r.json();
      if (meta.title) {
        document.getElementById('fName').value = meta.title;
        if (meta.difficulty &&
            ['Easy','Medium','Hard'].includes(meta.difficulty))
          document.getElementById('fDiff').value = meta.difficulty;
      }
    } catch {}
  });
```

- [ ] **Step 5: Build and full end-to-end test**

```bash
make && ./leetcode_db &
sleep 1
open http://localhost:8080
```

Manual test checklist:
1. Click **"Two Sum"** → overlay opens, description loads
2. Type in Monaco: `#include<iostream>\nint main(){std::cout<<0;}` (wrong answer)
3. Click **Run** → results strip shows `✗ Fail` with output vs expected
4. Fix code to `int main(){std::cout<<0<<" "<<1;}` (for input `[2,7,11,15]\n9`)
5. Click **Submit** → summary shows `X / X test cases passed`
6. Click **Add Problem**, type `15` in Problem #, tab → Title auto-fills "3Sum", difficulty fills "Medium"
7. Close overlay → problem list intact with filters

```bash
kill %1
```

- [ ] **Step 6: Commit**

```bash
git add frontend/index.html
git commit -m "feat: add Run/Submit execution, results strip, and smarter Add modal auto-fill"
```

---

## Done

All 7 tasks complete. Final state:
- ~150 problems seeded (NeetCode 150), sidebar shows 18 categories
- Clicking any problem opens split-panel with live description + Monaco editor
- Run tests against first example; Submit tests all examples
- All Judge0 languages supported; C++ default
- "Open on LeetCode ↗" link on every problem panel
- Add Problem modal auto-fills title + difficulty from problem number
- SQLite cache at `cache.db`; descriptions survive server restarts
