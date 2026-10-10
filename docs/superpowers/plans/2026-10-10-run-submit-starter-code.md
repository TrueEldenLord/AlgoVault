# Run/Submit Fix — Smart Starter Code Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Pre-fill the Monaco editor starter code with the problem's first example input so users can hit Run immediately in Python/JS, or fill in types once in C++/Java.

**Architecture:** Pure frontend change in `frontend/index.html`. Add a `editorDirty` flag + `_settingStarter` suppressor to protect user edits. Rewrite `getStarterCode` to accept an `examples` array and embed example data per language. Update `openProblem` and `onLangChange` to pass `currentExamples` through.

**Tech Stack:** Vanilla JS, Monaco Editor 0.44.0, no build step, no new dependencies.

## Global Constraints

- Only `frontend/index.html` changes — no server, no C++ files
- No new JS dependencies
- Must not overwrite user code once they start typing (`editorDirty` guard)
- Python / Python3: generate fully runnable call using `ast.literal_eval`
- JavaScript: generate fully runnable call using `JSON.parse`
- C++ / Java: generate `main()` stub with example input as comment lines
- TypeScript / Go / Rust / others: append example as comment block only
- Falls back to current behavior (no harness) when `examples` is empty

---

### Task 1: Add dirty flag and programmatic-set suppressor

**Files:**
- Modify: `frontend/index.html` — variables block (~line 996) and Monaco init block (~line 1002)

**Interfaces:**
- Produces: `editorDirty` (bool), `_settingStarter` (bool), `setEditorValue(code)` helper, Monaco `onDidChangeModelContent` listener

- [ ] **Step 1: Add two variables to the variables block**

Find this block in `index.html` (~line 996):
```js
  let currentProblemId = null;
  let currentExamples  = [];
  let lcLanguages      = [];
  let monacoEditor     = null;
  let monacoReady      = false;
```

Replace with:
```js
  let currentProblemId = null;
  let currentExamples  = [];
  let lcLanguages      = [];
  let monacoEditor     = null;
  let monacoReady      = false;
  let editorDirty      = false;   // true once user types; prevents overwriting their work
  let _settingStarter  = false;   // suppresses dirty flag during programmatic setValue
```

- [ ] **Step 2: Add `setEditorValue` helper + Monaco change listener**

Find the Monaco init block (~line 1002):
```js
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
```

Replace with:
```js
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
    monacoEditor.onDidChangeModelContent(function () {
      if (!_settingStarter) editorDirty = true;
    });
    monacoReady = true;
  });

  // Use this instead of monacoEditor.setValue() to avoid triggering editorDirty
  function setEditorValue(code) {
    _settingStarter = true;
    monacoEditor.setValue(code);
    _settingStarter = false;
    editorDirty = false;
  }
```

- [ ] **Step 3: Verify manually**

Start the server: `cd "/Users/Alex/Documents/Resume Projects/leetcode-database" && ./leetcode_db`

Open http://localhost:8080. Open the browser console (F12 → Console).

Type a character in the editor after opening any problem. Then run in the console:
```js
console.log('editorDirty:', editorDirty);       // expected: true
console.log('_settingStarter:', _settingStarter); // expected: false
```

Call `setEditorValue('// hello')` in the console. Expected: editor updates to `// hello` and `editorDirty` is `false` immediately after.

Note: the dirty flag won't visibly guard the language switch yet — that wiring is in Task 3.

- [ ] **Step 4: Commit**

```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
git add frontend/index.html
git commit -m "feat: add editorDirty flag and setEditorValue helper"
```

---

### Task 2: Rewrite `getStarterCode` with per-language example harness

**Files:**
- Modify: `frontend/index.html` — `getStarterCode` function (~line 1082)

**Interfaces:**
- Consumes: `editorDirty` (bool), `_settingStarter` (bool), `setEditorValue(code)` — from Task 1
- Produces: `getStarterCode(langId, name, examples)` — `examples` is `[]` or `[{input: string, output: string}]`

- [ ] **Step 1: Replace `getStarterCode` entirely**

Find the existing function (it starts with `function getStarterCode(langId, name) {` at ~line 1082) and replace the entire function with:

```js
  function getStarterCode(langId, name, examples) {
    examples = examples || [];
    const lang = (langId + '').split('/')[0].toLowerCase();
    const fn   = titleToMethod(name);

    const ex     = examples[0] || {};
    const input  = (ex.input  || '').trim();
    const output = (ex.output || '').trim();
    const lines  = input ? input.split('\n') : [];

    // ── Python / Python3 ─────────────────────────────────────
    if (lang === 'python' || lang === 'python3') {
      const base = 'class Solution:\n    def ' + fn + '(self):\n        pass\n';
      if (!lines.length) return base;
      const inputEsc = input.replace(/\\/g, '\\\\');
      return base +
        '\n# ── Example test ──────────────────────────────────────────────\n' +
        'import ast as _ast\n' +
        '_args = [_ast.literal_eval(l) for l in """' + inputEsc + '""".strip().split(\'\\n\')]\n' +
        'print(Solution().' + fn + '(*_args))  # expected: ' + (output || '?') + '\n';
    }

    // ── JavaScript ───────────────────────────────────────────
    if (lang === 'javascript') {
      const base = 'var ' + fn + ' = function() {\n    \n};\n';
      if (!lines.length) return base;
      const inputEsc = input.replace(/\\/g, '\\\\').replace(/\n/g, '\\n');
      return base +
        '\n// ── Example test ──────────────────────────────────────────────\n' +
        'var _args = "' + inputEsc + '".trim().split("\\n").map(function(l) { return JSON.parse(l); });\n' +
        'console.log(' + fn + '.apply(null, _args));  // expected: ' + (output || '?') + '\n';
    }

    // ── C++ ──────────────────────────────────────────────────
    if (lang === 'cpp') {
      const commentBlock = lines.length
        ? '// ── Example ──────────────────────────────────────────────\n' +
          lines.map(function(l) { return '// ' + l; }).join('\n') + '\n' +
          (output ? '// Expected: ' + output + '\n' : '')
        : '';
      return '#include <bits/stdc++.h>\nusing namespace std;\n\n' +
        'class Solution {\npublic:\n    // TODO: add params matching LeetCode signature\n    int ' + fn + '() {\n        \n    }\n};\n\n' +
        (commentBlock ? commentBlock + '\n' : '') +
        'int main() {\n    Solution s;\n    // auto result = s.' + fn + '(...);\n    // cout << result << endl;\n    return 0;\n}\n';
    }

    // ── Java ─────────────────────────────────────────────────
    if (lang === 'java') {
      const commentBlock = lines.length
        ? '    // ── Example ──────────────────────────────────────────────\n' +
          lines.map(function(l) { return '    // ' + l; }).join('\n') + '\n' +
          (output ? '    // Expected: ' + output + '\n' : '')
        : '';
      return 'class Solution {\n    // TODO: add params matching LeetCode signature\n    public void ' + fn + '() {\n        \n    }\n\n' +
        (commentBlock ? commentBlock + '\n' : '') +
        '    public static void main(String[] args) {\n        Solution s = new Solution();\n        // s.' + fn + '(...);\n    }\n}\n';
    }

    // ── TypeScript ───────────────────────────────────────────
    if (lang === 'typescript') {
      const commentBlock = lines.length
        ? '\n// ── Example ──────────────────────────────────────────────\n' +
          lines.map(function(l) { return '// ' + l; }).join('\n') + '\n' +
          (output ? '// Expected: ' + output + '\n' : '')
        : '';
      return 'function ' + fn + '(/* TODO: params */): void {\n    \n};\n' + commentBlock;
    }

    // ── Go ───────────────────────────────────────────────────
    if (lang === 'go') {
      const commentBlock = lines.length
        ? '\n// ── Example ──────────────────────────────────────────────\n' +
          lines.map(function(l) { return '// ' + l; }).join('\n') + '\n' +
          (output ? '// Expected: ' + output + '\n' : '')
        : '';
      return 'func ' + fn + '() {\n    \n}\n' + commentBlock;
    }

    // ── Rust ─────────────────────────────────────────────────
    if (lang === 'rust') {
      const snakeFn = fn.replace(/([A-Z])/g, '_$1').toLowerCase().replace(/^_/, '');
      const commentBlock = lines.length
        ? '\n// ── Example ──────────────────────────────────────────────\n' +
          lines.map(function(l) { return '// ' + l; }).join('\n') + '\n' +
          (output ? '// Expected: ' + output + '\n' : '')
        : '';
      return 'impl Solution {\n    pub fn ' + snakeFn + '() {\n        \n    }\n}\n' + commentBlock;
    }

    // ── Fallback ─────────────────────────────────────────────
    const commentBlock = lines.length
      ? '\n// ── Example ──────────────────────────────────────────────\n' +
        lines.map(function(l) { return '// ' + l; }).join('\n') + '\n' +
        (output ? '// Expected: ' + output + '\n' : '')
      : '';
    return '// ' + name + '\n// Add params matching the LeetCode signature\n' + commentBlock;
  }
```

- [ ] **Step 2: Verify manually — no examples case**

Rebuild and start the server:
```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
make && ./leetcode_db
```

Open http://localhost:8080. Open any problem before its meta loads (immediately after clicking, before the description appears). Expected: editor shows plain starter code with no example block (same as before).

- [ ] **Step 3: Verify manually — Python with examples**

You'll test the full wiring in Task 3. For now: in the browser console, paste and run this to test the function directly:

```js
// paste into browser console while on http://localhost:8080
const fakeExamples = [{input: '[2,7,11,15]\n9', output: '[0,1]'}];
console.log(getStarterCode('python3/3.10.0', 'Two Sum', fakeExamples));
```

Expected output (printed to console):
```
class Solution:
    def twoSum(self):
        pass

# ── Example test ──────────────────────────────────────────────
import ast as _ast
_args = [_ast.literal_eval(l) for l in """[2,7,11,15]
9""".strip().split('\n')]
print(Solution().twoSum(*_args))  # expected: [0,1]
```

- [ ] **Step 4: Verify manually — JavaScript with examples**

In browser console:
```js
const fakeExamples = [{input: '[2,7,11,15]\n9', output: '[0,1]'}];
console.log(getStarterCode('javascript/18.15.0', 'Two Sum', fakeExamples));
```

Expected:
```
var twoSum = function() {

};

// ── Example test ──────────────────────────────────────────────
var _args = "[2,7,11,15]\n9".trim().split("\n").map(function(l) { return JSON.parse(l); });
console.log(twoSum.apply(null, _args));  // expected: [0,1]
```

- [ ] **Step 5: Verify manually — C++ with examples**

In browser console:
```js
const fakeExamples = [{input: '[2,7,11,15]\n9', output: '[0,1]'}];
console.log(getStarterCode('cpp/10.2.0', 'Two Sum', fakeExamples));
```

Expected:
```
#include <bits/stdc++.h>
using namespace std;

class Solution {
public:
    // TODO: add params matching LeetCode signature
    int twoSum() {

    }
};

// ── Example ──────────────────────────────────────────────
// [2,7,11,15]
// 9
// Expected: [0,1]

int main() {
    Solution s;
    // auto result = s.twoSum(...);
    // cout << result << endl;
    return 0;
}
```

- [ ] **Step 6: Commit**

```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
git add frontend/index.html
git commit -m "feat: rewrite getStarterCode with per-language example harness"
```

---

### Task 3: Wire call sites — `openProblem` and `onLangChange`

**Files:**
- Modify: `frontend/index.html` — `openProblem` (~line 1102) and `onLangChange` (~line 1063)

**Interfaces:**
- Consumes: `setEditorValue(code)` from Task 1, `getStarterCode(langId, name, examples)` from Task 2, `editorDirty` from Task 1, `currentExamples` (existing variable)

- [ ] **Step 1: Update `onLangChange`**

Find the existing `onLangChange` function:
```js
  function onLangChange() {
    if (!monacoReady) return;
    const langId = document.getElementById('langSelect').value;
    monaco.editor.setModelLanguage(monacoEditor.getModel(), getMonacoLang(langId));
    if (currentProblemId) {
      const name = document.getElementById('overlayTitle').textContent;
      monacoEditor.setValue(getStarterCode(langId, name));
    }
  }
```

Replace with:
```js
  function onLangChange() {
    if (!monacoReady) return;
    const langId = document.getElementById('langSelect').value;
    monaco.editor.setModelLanguage(monacoEditor.getModel(), getMonacoLang(langId));
    if (currentProblemId && !editorDirty) {
      const name = document.getElementById('overlayTitle').textContent;
      setEditorValue(getStarterCode(langId, name, currentExamples));
    }
  }
```

- [ ] **Step 2: Update `openProblem`**

Find the existing `openProblem` function:
```js
  async function openProblem(id, name) {
    currentProblemId = id;
    document.getElementById('overlayTitle').textContent = name;
    document.getElementById('overlayLcLink').href = 'https://leetcode.com/problems/' + slug(name) + '/';
    document.getElementById('problemDesc').innerHTML = '<div class="desc-loading">Loading description...</div>';
    document.getElementById('resultsStrip').style.display = 'none';
    document.getElementById('problemOverlay').classList.add('open');
    if (monacoReady) monacoEditor.setValue(getStarterCode(document.getElementById('langSelect').value, name));
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
```

Replace with:
```js
  async function openProblem(id, name) {
    currentProblemId = id;
    currentExamples  = [];
    editorDirty      = false;
    document.getElementById('overlayTitle').textContent = name;
    document.getElementById('overlayLcLink').href = 'https://leetcode.com/problems/' + slug(name) + '/';
    document.getElementById('problemDesc').innerHTML = '<div class="desc-loading">Loading description...</div>';
    document.getElementById('resultsStrip').style.display = 'none';
    document.getElementById('problemOverlay').classList.add('open');
    if (monacoReady) setEditorValue(getStarterCode(document.getElementById('langSelect').value, name, []));
    if (lcLanguages.length === 0) await loadLanguages();
    try {
      const r = await fetch('/api/problems/' + id + '/meta');
      if (!r.ok) throw new Error('not found');
      const meta = await r.json();
      currentExamples = Array.isArray(meta.examples_json) ? meta.examples_json : [];
      if (monacoReady && !editorDirty) {
        setEditorValue(getStarterCode(document.getElementById('langSelect').value, name, currentExamples));
      }
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
```

- [ ] **Step 3: Verify manually — Python example pre-fill end-to-end**

Make sure the server is running, then open http://localhost:8080.

1. Click **"Two Sum"** to open the overlay.
2. Wait ~2 seconds for the description to load.
3. In the language dropdown, select **Python3** (or whichever Python is listed).

Expected: the editor now shows:
```python
class Solution:
    def twoSum(self):
        pass

# ── Example test ──────────────────────────────────────────────
import ast as _ast
_args = [_ast.literal_eval(l) for l in """[2,7,11,15]
9""".strip().split('\n')]
print(Solution().twoSum(*_args))  # expected: [0,1]
```

- [ ] **Step 4: Verify manually — Run button works for Python**

With the Python starter code visible, fix the method signature so it accepts `*args` or the correct params:
```python
class Solution:
    def twoSum(self, nums, target):
        return [0, 1]  # dummy answer

import ast as _ast
_args = [_ast.literal_eval(l) for l in """[2,7,11,15]
9""".strip().split('\n')]
print(Solution().twoSum(*_args))  # expected: [0,1]
```

Click **Run**. Expected result panel shows output `[0, 1]`.

- [ ] **Step 5: Verify manually — dirty flag protects user code**

1. Click a problem to open the overlay.
2. Wait for description to load.
3. Type any character in the editor.
4. Switch to a different language in the dropdown.

Expected: the editor is NOT reset — your typed content remains.

- [ ] **Step 6: Verify manually — C++ example in main()**

1. Click **"Two Sum"**, wait for description.
2. Select C++ in the dropdown.

Expected: editor shows starter code that includes `#include <bits/stdc++.h>`, the `Solution` class, a comment block with the example input, and a `main()` stub.

- [ ] **Step 7: Commit**

```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
git add frontend/index.html
git commit -m "feat: wire example pre-fill into openProblem and onLangChange"
```
