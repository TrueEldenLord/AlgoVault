# Run/Submit Fix — Smart Starter Code with Example Pre-fill

**Date:** 2026-10-10
**Author:** Alexis Sanchez
**Status:** Approved

---

## Problem

Piston runs raw source code with no test harness. LeetCode problems expect a `Solution` class/function called by a driver. Currently, clicking Run sends the user's Solution stub to Piston, which produces no output because there's no `main()` calling it. Users must manually add a call at the bottom of their code.

## Goal

When a user opens a problem, the starter code already includes a runnable test call pre-filled with the first example's input. For Python and JavaScript, the call is fully executable with zero edits. For compiled languages (C++, Java), a `main()` stub with the example input embedded as a comment lets users fill in types once and run.

---

## Architecture

**Scope:** `frontend/index.html` only. No server changes, no C++ changes.

**Three changes:**

1. `getStarterCode(langId, name, examples = [])` — add optional `examples` param; embed example data when present.
2. Dirty flag — track whether the user has edited the editor; prevent overwriting their work.
3. Call sites — `openProblem()` and `onLangChange()` pass `currentExamples` to `getStarterCode`.

---

## Example Data

`examples[0].input` is the raw `exampleTestcases` string from the alfa-leetcode-api — one function argument per line:
```
[2,7,11,15]
9
```
`examples[0].output` is the expected result string (e.g. `[0,1]`).

---

## Per-Language Starter Code

### Python / Python3 — fully runnable

Uses `ast.literal_eval` to parse each input line into a Python value, then spreads them as positional args into the Solution method. Works for integers, floats, strings, lists, and nested lists — covers the vast majority of NeetCode 150 problems.

```python
class Solution:
    def twoSum(self, nums, target):
        pass

# ── Example test ──────────────────────────────────────────────
import ast as _ast
_args = [_ast.literal_eval(l) for l in "[2,7,11,15]\n9".strip().split('\n')]
print(Solution().twoSum(*_args))  # expected: [0,1]
```

### JavaScript — fully runnable

Uses `JSON.parse` to parse each input line. Same spread approach.

```javascript
var twoSum = function(nums, target) {

};

// ── Example test ──────────────────────────────────────────────
const _args = `[2,7,11,15]\n9`.trim().split('\n').map(l => JSON.parse(l));
console.log(twoSum(..._args));  // expected: [0,1]
```

### C++ — stub with example as comment

Piston requires `#include` and a `main()` to produce output. The starter embeds a `main()` stub with the raw example lines as comments. User fills in the correct types and calls their function.

```cpp
#include <bits/stdc++.h>
using namespace std;

class Solution {
public:
    // TODO: add params matching LeetCode signature
    int twoSum() {

    }
};

// ── Example test ──────────────────────────────────────────────
// Input:
//   [2,7,11,15]
//   9
// Expected: [0,1]
int main() {
    Solution s;
    // auto result = s.twoSum({2,7,11,15}, 9);
    // cout << result << endl;
    return 0;
}
```

### Java — stub with example as comment

```java
class Solution {
    // TODO: add params matching LeetCode signature
    public void twoSum() {

    }

    // ── Example test ──────────────────────────────────────────────
    // Input:
    //   [2,7,11,15]
    //   9
    // Expected: [0,1]
    public static void main(String[] args) {
        Solution s = new Solution();
        // System.out.println(s.twoSum({2,7,11,15}, 9));
    }
}
```

### TypeScript, Go, Rust, others — example as comment

A comment block with the raw input lines and expected output is appended to the existing starters.

---

## Dirty Flag

```js
let editorDirty = false;
```

- Set to `true` on Monaco's `onDidChangeModelContent` event.
- Reset to `false` whenever `monacoEditor.setValue(...)` is called programmatically (before the setValue call).
- On `openProblem()`: always reset dirty to `false` (new problem, fresh start).
- On `onLangChange()`: only regenerate starter code if `!editorDirty`.
- After meta loads in `openProblem()`: update the editor with example-enriched starter code **only if `!editorDirty`**. Meta fetches are async (can take 1–2 s); if the user starts typing before the fetch returns, their work is preserved.

---

## Call Site Changes

### `openProblem(id, name)`

Current flow:
1. Set initial starter code (no examples yet)
2. Fetch meta async
3. Set `currentExamples`

New flow:
1. Reset `editorDirty = false`
2. Set initial starter code (no examples — same as before)
3. Fetch meta async
4. Set `currentExamples`
5. **Update editor with example-enriched starter code if `!editorDirty`**

### `onLangChange()`

```js
if (currentProblemId && !editorDirty) {
    monacoEditor.setValue(getStarterCode(langId, name, currentExamples));
}
```

---

## `getStarterCode` Signature

```js
function getStarterCode(langId, name, examples = []) { ... }
```

Helper to extract example data:
```js
const ex     = examples[0] || {};
const input  = (ex.input  || '').trim();   // "nums = [2,7,11,15]\n9" or "[2,7,11,15]\n9"
const output = (ex.output || '').trim();   // "[0,1]"
const lines  = input ? input.split('\n') : [];
```

---

## Edge Cases

- **No examples loaded yet** (meta fetch pending or failed): `examples` is `[]`, starter code falls back to current behavior — no test harness block appended.
- **`ast.literal_eval` / `JSON.parse` parse failure at runtime**: These are user-visible errors in the output panel, not silent failures. The user sees the traceback and knows to adjust the call.
- **Problems with complex input formats** (e.g. graph adjacency lists as `[[2],[1]]`): `ast.literal_eval` handles nested lists correctly. Edge cases like `null` → Python's `None` differ between JSON and Python literals, but this affects a small minority of problems.
- **Dirty flag on language switch**: If the user has typed in C++ and switches to Python, the editor is NOT reset — their code is preserved.

---

## Files Changed

| File | Change |
|---|---|
| `frontend/index.html` | `getStarterCode` signature + body, dirty flag setup, `openProblem` post-meta update, `onLangChange` guard |

No other files change.
