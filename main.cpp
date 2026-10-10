// ============================================================
//  LeetCode Problem Database — HTTP Server + NeetCode Web UI
//  Alexis Sanchez | UHCL CS | Resume Portfolio Project
//
//  Data Structure : Singly-Linked List with Merge Sort
//  Server         : cpp-httplib (header-only)
//  Frontend       : frontend/index.html  →  http://localhost:8080
// ============================================================

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wunused-result"
#include "httplib.h"
#pragma GCC diagnostic pop

#include "LinkedList.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <string>
#include <vector>
#include <ctime>
#include "sqlite3.h"

// ── Shared state ─────────────────────────────────────────────
static LinkedList db;
static std::mutex db_mutex;

static sqlite3*    g_cacheDb    = nullptr;
static std::string g_languagesCache;

// ── JSON helpers ─────────────────────────────────────────────

static std::string escJson(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (char c : s) {
        if      (c == '"')  out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else                out += c;
    }
    return out;
}

static std::string toSlug(const std::string& name) {
    std::string s;
    for (unsigned char c : name) {
        if (std::isalnum(c)) s += (char)std::tolower(c);
        else if (c == ' ' || c == '-') s += '-';
    }
    std::string out;
    bool prevDash = false;
    for (char c : s) {
        if (c == '-') { if (!prevDash) out += c; prevDash = true; }
        else          { out += c; prevDash = false; }
    }
    while (!out.empty() && out.back() == '-') out.pop_back();
    return out;
}

static std::string httpGet(const std::string& url) {
    std::string cmd = "curl -s --max-time 10 \"" + url + "\"";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) return "";
    std::ostringstream ss;
    char buf[4096];
    while (fgets(buf, sizeof(buf), p)) ss << buf;
    pclose(p);
    return ss.str();
}

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

static std::string toJson(const Problem& p) {
    std::string topicsArr = "[";
    for (size_t i = 0; i < p.topics.size(); ++i) {
        if (i) topicsArr += ',';
        topicsArr += '"' + escJson(p.topics[i]) + '"';
    }
    topicsArr += ']';

    return "{\"id\":"           + std::to_string(p.id) +
           ",\"name\":\""       + escJson(p.name) + "\"" +
           ",\"difficulty\":\"" + p.difficulty + "\"" +
           ",\"solved\":"       + (p.solved ? "true" : "false") +
           ",\"topics\":"       + topicsArr + "}";
}

static std::string allToJson() {
    std::string arr = "[";
    Node* cur = db.getHead();
    bool first = true;
    while (cur) {
        if (!first) arr += ',';
        arr += toJson(cur->data);
        first = false;
        cur = cur->next;
    }
    return arr + "]";
}

// ── Request body parsers ──────────────────────────────────────

static std::string jStr(const std::string& body, const std::string& key) {
    const std::string search = "\"" + key + "\":\"";
    auto pos = body.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    auto end = body.find('"', pos);
    return (end == std::string::npos) ? "" : body.substr(pos, end - pos);
}

static std::string jStrRaw(const std::string& body, const std::string& key) {
    const std::string search = "\"" + key + "\":\"";
    auto pos = body.find(search);
    if (pos == std::string::npos) return "";
    pos += search.size();
    std::string result;
    while (pos < body.size()) {
        if (body[pos] == '\\' && pos + 1 < body.size()) {
            char next = body[pos + 1];
            if      (next == '"')  { result += '"';  pos += 2; }
            else if (next == '\\') { result += '\\'; pos += 2; }
            else if (next == 'n')  { result += '\n'; pos += 2; }
            else if (next == 'r')  { result += '\r'; pos += 2; }
            else if (next == 't')  { result += '\t'; pos += 2; }
            else                   { result += body[pos++]; }
        } else if (body[pos] == '"') {
            break;
        } else {
            result += body[pos++];
        }
    }
    return result;
}

static int jInt(const std::string& body, const std::string& key) {
    const std::string search = "\"" + key + "\":";
    auto pos = body.find(search);
    if (pos == std::string::npos) return -1;
    pos += search.size();
    try { return std::stoi(body.substr(pos)); }
    catch (...) { return -1; }
}

static bool jBool(const std::string& body, const std::string& key) {
    const std::string search = "\"" + key + "\":";
    auto pos = body.find(search);
    if (pos == std::string::npos) return false;
    pos += search.size();
    return body.size() >= pos + 4 && body.substr(pos, 4) == "true";
}

static std::vector<std::string> jStrArray(const std::string& body, const std::string& key) {
    std::vector<std::string> result;
    const std::string search = "\"" + key + "\":[";
    auto pos = body.find(search);
    if (pos == std::string::npos) return result;
    pos += search.size();
    while (pos < body.size() && body[pos] != ']') {
        if (body[pos] == '"') {
            auto end = body.find('"', pos + 1);
            if (end == std::string::npos) break;
            result.push_back(body.substr(pos + 1, end - pos - 1));
            pos = end + 1;
        } else {
            ++pos;
        }
    }
    return result;
}

// ── File helper ───────────────────────────────────────────────

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

// ── SQLite init ───────────────────────────────────────────────

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

// ── CORS ──────────────────────────────────────────────────────

static void cors(httplib::Response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, DELETE, PATCH, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

// ── Seed data (NeetCode 75) ───────────────────────────────────

static void seed() {
    // Arrays & Hashing
    db.insert({1,   "Two Sum",                              "Easy",   false, {"Arrays & Hashing"}});
    db.insert({49,  "Group Anagrams",                       "Medium", false, {"Arrays & Hashing"}});
    db.insert({128, "Longest Consecutive Sequence",         "Medium", false, {"Arrays & Hashing"}});
    db.insert({217, "Contains Duplicate",                   "Easy",   false, {"Arrays & Hashing"}});
    db.insert({238, "Product of Array Except Self",         "Medium", false, {"Arrays & Hashing"}});
    db.insert({242, "Valid Anagram",                        "Easy",   false, {"Arrays & Hashing"}});
    db.insert({347, "Top K Frequent Elements",              "Medium", false, {"Arrays & Hashing"}});
    db.insert({36,  "Valid Sudoku",                         "Medium", false, {"Arrays & Hashing"}});

    // Two Pointers
    db.insert({11,  "Container With Most Water",            "Medium", false, {"Two Pointers"}});
    db.insert({15,  "3Sum",                                 "Medium", false, {"Two Pointers"}});
    db.insert({42,  "Trapping Rain Water",                  "Hard",   false, {"Two Pointers"}});
    db.insert({125, "Valid Palindrome",                     "Easy",   false, {"Two Pointers"}});
    db.insert({167, "Two Sum II - Input Array Is Sorted",   "Medium", false, {"Two Pointers"}});

    // Sliding Window
    db.insert({3,   "Longest Substring Without Repeating Characters", "Medium", false, {"Sliding Window"}});
    db.insert({76,  "Minimum Window Substring",             "Hard",   false, {"Sliding Window"}});
    db.insert({121, "Best Time to Buy and Sell Stock",      "Easy",   false, {"Sliding Window"}});
    db.insert({424, "Longest Repeating Character Replacement", "Medium", false, {"Sliding Window"}});
    db.insert({239, "Sliding Window Maximum",               "Hard",   false, {"Sliding Window"}});
    db.insert({567, "Permutation in String",                "Medium", false, {"Sliding Window"}});

    // Stack
    db.insert({20,  "Valid Parentheses",                    "Easy",   false, {"Stack"}});
    db.insert({22,  "Generate Parentheses",                 "Medium", false, {"Stack"}});
    db.insert({84,  "Largest Rectangle in Histogram",       "Hard",   false, {"Stack"}});
    db.insert({150, "Evaluate Reverse Polish Notation",     "Medium", false, {"Stack"}});
    db.insert({155, "Min Stack",                            "Medium", false, {"Stack"}});
    db.insert({739, "Daily Temperatures",                   "Medium", false, {"Stack"}});
    db.insert({853, "Car Fleet",                            "Medium", false, {"Stack"}});

    // Binary Search
    db.insert({4,   "Median of Two Sorted Arrays",          "Hard",   false, {"Binary Search"}});
    db.insert({33,  "Search in Rotated Sorted Array",       "Medium", false, {"Binary Search"}});
    db.insert({153, "Find Minimum in Rotated Sorted Array", "Medium", false, {"Binary Search"}});
    db.insert({704, "Binary Search",                        "Easy",   false, {"Binary Search"}});
    db.insert({875, "Koko Eating Bananas",                  "Medium", false, {"Binary Search"}});
    db.insert({74,  "Search a 2D Matrix",                   "Medium", false, {"Binary Search"}});
    db.insert({981, "Time Based Key-Value Store",           "Medium", false, {"Binary Search"}});

    // Linked List
    db.insert({19,  "Remove Nth Node From End of List",     "Medium", false, {"Linked List"}});
    db.insert({21,  "Merge Two Sorted Lists",               "Easy",   false, {"Linked List"}});
    db.insert({23,  "Merge K Sorted Lists",                 "Hard",   false, {"Linked List"}});
    db.insert({141, "Linked List Cycle",                    "Easy",   false, {"Linked List"}});
    db.insert({143, "Reorder List",                         "Medium", false, {"Linked List"}});
    db.insert({206, "Reverse Linked List",                  "Easy",   false, {"Linked List"}});
    db.insert({2,   "Add Two Numbers",                      "Medium", false, {"Linked List"}});
    db.insert({25,  "Reverse Nodes in K-Group",             "Hard",   false, {"Linked List"}});
    db.insert({138, "Copy List with Random Pointer",        "Medium", false, {"Linked List"}});
    db.insert({146, "LRU Cache",                            "Medium", false, {"Linked List"}});
    db.insert({287, "Find the Duplicate Number",            "Medium", false, {"Linked List"}});

    // Trees
    db.insert({98,  "Validate Binary Search Tree",          "Medium", false, {"Trees"}});
    db.insert({100, "Same Tree",                            "Easy",   false, {"Trees"}});
    db.insert({102, "Binary Tree Level Order Traversal",    "Medium", false, {"Trees"}});
    db.insert({104, "Maximum Depth of Binary Tree",         "Easy",   false, {"Trees"}});
    db.insert({105, "Construct Binary Tree from Preorder and Inorder Traversal", "Medium", false, {"Trees"}});
    db.insert({110, "Balanced Binary Tree",                 "Easy",   false, {"Trees"}});
    db.insert({124, "Binary Tree Maximum Path Sum",         "Hard",   false, {"Trees"}});
    db.insert({199, "Binary Tree Right Side View",          "Medium", false, {"Trees"}});
    db.insert({226, "Invert Binary Tree",                   "Easy",   false, {"Trees"}});
    db.insert({230, "Kth Smallest Element in a BST",        "Medium", false, {"Trees"}});
    db.insert({235, "Lowest Common Ancestor of a BST",      "Medium", false, {"Trees"}});
    db.insert({297, "Serialize and Deserialize Binary Tree","Hard",   false, {"Trees"}});
    db.insert({543, "Diameter of Binary Tree",              "Easy",   false, {"Trees"}});
    db.insert({572, "Subtree of Another Tree",              "Easy",   false, {"Trees"}});
    db.insert({1448,"Count Good Nodes in Binary Tree",      "Medium", false, {"Trees"}});

    // Heap / Priority Queue
    db.insert({215, "Kth Largest Element in an Array",     "Medium", false, {"Heap / Priority Queue"}});
    db.insert({295, "Find Median from Data Stream",         "Hard",   false, {"Heap / Priority Queue"}});
    db.insert({355, "Design Twitter",                       "Medium", false, {"Heap / Priority Queue"}});
    db.insert({973, "K Closest Points to Origin",           "Medium", false, {"Heap / Priority Queue"}});
    db.insert({621, "Task Scheduler",                       "Medium", false, {"Heap / Priority Queue"}});
    db.insert({703, "Kth Largest Element in a Stream",      "Easy",   false, {"Heap / Priority Queue"}});
    db.insert({1046,"Last Stone Weight",                    "Easy",   false, {"Heap / Priority Queue"}});

    // Backtracking
    db.insert({39,  "Combination Sum",                      "Medium", false, {"Backtracking"}});
    db.insert({46,  "Permutations",                         "Medium", false, {"Backtracking"}});
    db.insert({78,  "Subsets",                              "Medium", false, {"Backtracking"}});
    db.insert({79,  "Word Search",                          "Medium", false, {"Backtracking"}});
    db.insert({131, "Palindrome Partitioning",              "Medium", false, {"Backtracking"}});
    db.insert({17,  "Letter Combinations of a Phone Number","Medium", false, {"Backtracking"}});
    db.insert({40,  "Combination Sum II",                   "Medium", false, {"Backtracking"}});
    db.insert({51,  "N-Queens",                             "Hard",   false, {"Backtracking"}});
    db.insert({90,  "Subsets II",                           "Medium", false, {"Backtracking"}});

    // Graphs
    db.insert({133, "Clone Graph",                          "Medium", false, {"Graphs"}});
    db.insert({200, "Number of Islands",                    "Medium", false, {"Graphs"}});
    db.insert({207, "Course Schedule",                      "Medium", false, {"Graphs"}});
    db.insert({210, "Course Schedule II",                   "Medium", false, {"Graphs"}});
    db.insert({417, "Pacific Atlantic Water Flow",          "Medium", false, {"Graphs"}});
    db.insert({695, "Max Area of Island",                   "Medium", false, {"Graphs"}});
    db.insert({130, "Surrounded Regions",                   "Medium", false, {"Graphs"}});
    db.insert({261, "Graph Valid Tree",                     "Medium", false, {"Graphs"}});
    db.insert({286, "Walls and Gates",                      "Medium", false, {"Graphs"}});
    db.insert({323, "Number of Connected Components in an Undirected Graph", "Medium", false, {"Graphs"}});
    db.insert({684, "Redundant Connection",                 "Medium", false, {"Graphs"}});
    db.insert({994, "Rotting Oranges",                      "Medium", false, {"Graphs"}});

    // Tries
    db.insert({208, "Implement Trie (Prefix Tree)",         "Medium", false, {"Tries"}});
    db.insert({211, "Design Add and Search Words Data Structure", "Medium", false, {"Tries"}});
    db.insert({212, "Word Search II",                       "Hard",   false, {"Tries"}});

    // Advanced Graphs
    db.insert({269, "Alien Dictionary",                     "Hard",   false, {"Advanced Graphs"}});
    db.insert({332, "Reconstruct Itinerary",                "Hard",   false, {"Advanced Graphs"}});
    db.insert({743, "Network Delay Time",                   "Medium", false, {"Advanced Graphs"}});
    db.insert({778, "Swim in Rising Water",                 "Hard",   false, {"Advanced Graphs"}});
    db.insert({787, "Cheapest Flights Within K Stops",      "Medium", false, {"Advanced Graphs"}});
    db.insert({1584,"Min Cost to Connect All Points",       "Medium", false, {"Advanced Graphs"}});

    // 1-D Dynamic Programming
    db.insert({55,  "Jump Game",                            "Medium", false, {"1-D DP"}});
    db.insert({70,  "Climbing Stairs",                      "Easy",   false, {"1-D DP"}});
    db.insert({91,  "Decode Ways",                          "Medium", false, {"1-D DP"}});
    db.insert({139, "Word Break",                           "Medium", false, {"1-D DP"}});
    db.insert({198, "House Robber",                         "Medium", false, {"1-D DP"}});
    db.insert({213, "House Robber II",                      "Medium", false, {"1-D DP"}});
    db.insert({300, "Longest Increasing Subsequence",       "Medium", false, {"1-D DP"}});
    db.insert({322, "Coin Change",                          "Medium", false, {"1-D DP"}});
    db.insert({5,   "Longest Palindromic Substring",        "Medium", false, {"1-D DP"}});
    db.insert({152, "Maximum Product Subarray",             "Medium", false, {"1-D DP"}});
    db.insert({416, "Partition Equal Subset Sum",           "Medium", false, {"1-D DP"}});
    db.insert({647, "Palindromic Substrings",               "Medium", false, {"1-D DP"}});
    db.insert({746, "Min Cost Climbing Stairs",             "Easy",   false, {"1-D DP"}});

    // 2-D Dynamic Programming
    db.insert({62,  "Unique Paths",                         "Medium", false, {"2-D DP"}});
    db.insert({72,  "Edit Distance",                        "Medium", false, {"2-D DP"}});
    db.insert({1143,"Longest Common Subsequence",           "Medium", false, {"2-D DP"}});
    db.insert({10,  "Regular Expression Matching",          "Hard",   false, {"2-D DP"}});
    db.insert({97,  "Interleaving String",                  "Medium", false, {"2-D DP"}});
    db.insert({115, "Distinct Subsequences",                "Hard",   false, {"2-D DP"}});
    db.insert({309, "Best Time to Buy and Sell Stock with Cooldown", "Medium", false, {"2-D DP"}});
    db.insert({312, "Burst Balloons",                       "Hard",   false, {"2-D DP"}});
    db.insert({329, "Longest Increasing Path in a Matrix",  "Hard",   false, {"2-D DP"}});
    db.insert({494, "Target Sum",                           "Medium", false, {"2-D DP"}});
    db.insert({518, "Coin Change II",                       "Medium", false, {"2-D DP"}});

    // Greedy
    db.insert({45,  "Jump Game II",                         "Medium", false, {"Greedy"}});
    db.insert({53,  "Maximum Subarray",                     "Medium", false, {"Greedy"}});
    db.insert({134, "Gas Station",                          "Medium", false, {"Greedy"}});
    db.insert({678, "Valid Parenthesis String",             "Medium", false, {"Greedy"}});
    db.insert({763, "Partition Labels",                     "Medium", false, {"Greedy"}});
    db.insert({846, "Hand of Straights",                    "Medium", false, {"Greedy"}});
    db.insert({1899,"Merge Triplets to Form Target Triplet","Medium", false, {"Greedy"}});

    // Intervals
    db.insert({56,  "Merge Intervals",                      "Medium", false, {"Intervals"}});
    db.insert({57,  "Insert Interval",                      "Medium", false, {"Intervals"}});
    db.insert({435, "Non-overlapping Intervals",            "Medium", false, {"Intervals"}});
    db.insert({1851,"Minimum Interval to Include Each Query","Hard",  false, {"Intervals"}});

    // Math & Geometry
    db.insert({48,  "Rotate Image",                         "Medium", false, {"Math & Geometry"}});
    db.insert({54,  "Spiral Matrix",                        "Medium", false, {"Math & Geometry"}});
    db.insert({73,  "Set Matrix Zeroes",                    "Medium", false, {"Math & Geometry"}});
    db.insert({43,  "Multiply Strings",                     "Medium", false, {"Math & Geometry"}});
    db.insert({50,  "Pow(x, n)",                            "Medium", false, {"Math & Geometry"}});
    db.insert({66,  "Plus One",                             "Easy",   false, {"Math & Geometry"}});
    db.insert({202, "Happy Number",                         "Easy",   false, {"Math & Geometry"}});
    db.insert({2013,"Detect Squares",                       "Medium", false, {"Math & Geometry"}});

    // Bit Manipulation
    db.insert({190, "Reverse Bits",                         "Easy",   false, {"Bit Manipulation"}});
    db.insert({191, "Number of 1 Bits",                     "Easy",   false, {"Bit Manipulation"}});
    db.insert({268, "Missing Number",                       "Easy",   false, {"Bit Manipulation"}});
    db.insert({338, "Counting Bits",                        "Easy",   false, {"Bit Manipulation"}});
    db.insert({371, "Sum of Two Integers",                  "Medium", false, {"Bit Manipulation"}});
    db.insert({7,   "Reverse Integer",                      "Medium", false, {"Bit Manipulation"}});
    db.insert({136, "Single Number",                        "Easy",   false, {"Bit Manipulation"}});
}

// ── Entry Point ───────────────────────────────────────────────

int main() {
    initDb();
    seed();

    httplib::Server svr;

    // ── Serve frontend ────────────────────────────────────────
    svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
        std::string html = readFile("frontend/index.html");
        if (html.empty()) {
            res.set_content("frontend/index.html not found. Run ./leetcode_db from the project directory.", "text/plain");
            res.status = 404;
        } else {
            res.set_content(html, "text/html");
        }
    });

    // ── GET /api/problems ─────────────────────────────────────
    svr.Get("/api/problems", [](const httplib::Request&, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(db_mutex);
        cors(res);
        res.set_content(allToJson(), "application/json");
    });

    // ── POST /api/problems ────────────────────────────────────
    svr.Post("/api/problems", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(db_mutex);
        cors(res);

        int id                          = jInt(req.body, "id");
        std::string name                = jStr(req.body, "name");
        std::string diff                = jStr(req.body, "difficulty");
        std::vector<std::string> topics = jStrArray(req.body, "topics");

        if (id < 1 || name.empty() ||
            (diff != "Easy" && diff != "Medium" && diff != "Hard")) {
            res.status = 400;
            res.set_content("{\"error\":\"Invalid fields\"}", "application/json");
            return;
        }

        for (Node* cur = db.getHead(); cur; cur = cur->next) {
            if (cur->data.id == id) {
                res.status = 409;
                res.set_content("{\"error\":\"ID already exists\"}", "application/json");
                return;
            }
        }

        Problem p{id, name, diff, false, topics};
        db.insert(p);
        res.status = 201;
        res.set_content(toJson(p), "application/json");
    });

    // ── DELETE /api/problems/:id ──────────────────────────────
    svr.Delete(R"(/api/problems/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(db_mutex);
        cors(res);
        int id = std::stoi(req.matches[1]);
        if (db.remove(id)) {
            res.set_content("{\"deleted\":true}", "application/json");
        } else {
            res.status = 404;
            res.set_content("{\"error\":\"Not found\"}", "application/json");
        }
    });

    // ── PATCH /api/problems/:id ───────────────────────────────
    svr.Patch(R"(/api/problems/(\d+))", [](const httplib::Request& req, httplib::Response& res) {
        std::lock_guard<std::mutex> lock(db_mutex);
        cors(res);
        int id      = std::stoi(req.matches[1]);
        bool solved = jBool(req.body, "solved");

        for (Node* cur = db.getHead(); cur; cur = cur->next) {
            if (cur->data.id == id) {
                cur->data.solved = solved;
                res.set_content(toJson(cur->data), "application/json");
                return;
            }
        }
        res.status = 404;
        res.set_content("{\"error\":\"Not found\"}", "application/json");
    });

    // ── GET /api/problems/:id/meta ────────────────────────────
    svr.Get("/api/problems/:id/meta", [](const httplib::Request& req, httplib::Response& res) {
        cors(res);
        int id = std::stoi(req.path_params.at("id"));

        std::string cached = metaFromCache(id);
        if (!cached.empty()) {
            res.set_content(cached, "application/json");
            return;
        }

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

        std::string slug = toSlug(name);
        std::string url  = "https://alfa-leetcode-api.onrender.com/select?titleSlug=" + slug;
        std::string body = httpGet(url);

        if (body.empty() || body.find("title") == std::string::npos) {
            res.status = 502;
            res.set_content("{\"error\":\"External API unavailable\"}", "application/json");
            return;
        }

        std::string title       = jStr(body, "title");
        std::string descHtml    = jStrRaw(body, "question");
        std::string constraints = jStr(body, "constraints");
        std::string examples    = jStr(body, "exampleTestcases");
        if (title.empty()) title = name;

        std::string exJson = "[{\"input\":\"" + escJson(examples) + "\",\"output\":\"\"}]";

        metaToCache(id, title, descHtml, exJson, constraints);
        res.set_content(metaFromCache(id), "application/json");
    });

    // ── GET /api/languages ────────────────────────────────────
    svr.Get("/api/languages", [](const httplib::Request&, httplib::Response& res) {
        cors(res);
        if (g_languagesCache.empty())
            g_languagesCache = httpGet("https://emkc.org/api/v2/piston/runtimes");
        if (g_languagesCache.empty())
            g_languagesCache = "[]";
        res.set_content(g_languagesCache, "application/json");
    });

    // ── POST /api/run ─────────────────────────────────────────
    svr.Post("/api/run", [](const httplib::Request& req, httplib::Response& res) {
        cors(res);

        std::string code      = jStrRaw(req.body, "code");
        std::string langId    = jStr(req.body, "language_id");  // format: "cpp/10.2.0"
        std::string stdinData = jStrRaw(req.body, "stdin");

        if (code.empty() || langId.empty()) {
            res.status = 400;
            res.set_content("{\"error\":\"code and language_id required\"}", "application/json");
            return;
        }

        // Split "cpp/10.2.0" into lang="cpp" version="10.2.0"
        auto slash   = langId.find('/');
        std::string lang    = (slash != std::string::npos) ? langId.substr(0, slash) : langId;
        std::string version = (slash != std::string::npos) ? langId.substr(slash + 1) : "*";

        // Write Piston request to temp file to avoid shell quoting issues
        std::string pistonBody =
            "{\"language\":\"" + escJson(lang)    + "\","
            "\"version\":\""   + escJson(version) + "\","
            "\"files\":[{\"content\":\"" + escJson(code) + "\"}],"
            "\"stdin\":\""     + escJson(stdinData) + "\"}";

        const std::string tmp = "/tmp/lc_piston_body.json";
        { std::ofstream f(tmp); f << pistonBody; }

        std::string cmd =
            "curl -s --max-time 30 "
            "-X POST \"https://emkc.org/api/v2/piston/execute\" "
            "-H \"Content-Type: application/json\" "
            "-d @" + tmp;

        FILE* p = popen(cmd.c_str(), "r");
        std::string raw;
        if (p) {
            char buf[4096];
            while (fgets(buf, sizeof(buf), p)) raw += buf;
            pclose(p);
        }

        if (raw.empty()) {
            res.set_content("{\"error\":\"Piston API unavailable\"}", "application/json");
            return;
        }

        // Extract from Piston response. Piston returns:
        // {"language":...,"run":{"stdout":"...","stderr":"...","code":0,...}}
        // Also may have "compile":{"stderr":"..."} on compile errors.
        std::string stdout_val  = jStr(raw, "stdout");
        std::string stderr_val  = jStr(raw, "stderr");
        int         exit_code   = jInt(raw, "code");

        // Check compile errors (Piston puts compiler errors in "compile" object's stderr)
        std::string compile_err;
        auto ce_pos = raw.find("\"compile\"");
        if (ce_pos != std::string::npos) {
            // Find stderr after the compile object
            auto stderr_pos = raw.find("\"stderr\":\"", ce_pos);
            if (stderr_pos != std::string::npos) {
                stderr_pos += 10; // len of "\"stderr\":\""
                auto end = raw.find('"', stderr_pos);
                if (end != std::string::npos)
                    compile_err = raw.substr(stderr_pos, end - stderr_pos);
            }
        }

        int         status_id   = (exit_code == 0) ? 3 : 11;
        std::string status_desc = (exit_code == 0) ? "Accepted" : "Runtime Error";
        if (!compile_err.empty() && stdout_val.empty()) {
            status_id   = 6;
            status_desc = "Compilation Error";
        }

        std::string result =
            "{\"stdout\":\""        + escJson(stdout_val)  + "\","
            "\"stderr\":\""         + escJson(stderr_val)  + "\","
            "\"compile_output\":\"" + escJson(compile_err) + "\","
            "\"status\":{\"id\":"   + std::to_string(status_id) +
            ",\"description\":\""   + status_desc + "\"}}";

        res.set_content(result, "application/json");
    });

    // ── OPTIONS preflight ─────────────────────────────────────
    svr.Options(".*", [](const httplib::Request&, httplib::Response& res) {
        cors(res);
        res.status = 204;
    });

    std::cout << "\033[1;36m"
              << "╔══════════════════════════════════════╗\n"
              << "║     LeetCode Problem Database        ║\n"
              << "║                                      ║\n"
              << "║     http://localhost:8080            ║\n"
              << "╚══════════════════════════════════════╝\n"
              << "\033[0m"
              << "\033[2mCtrl+C to stop.\033[0m\n\n";

    svr.listen("0.0.0.0", 8080);
    return 0;
}
