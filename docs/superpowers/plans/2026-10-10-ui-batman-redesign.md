# UI Redesign — Batman 90s Theme Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace the green color scheme with a Batman Animated Series palette (dark navy, blue-gray, gold) and add Description/Details tabs to the problem overlay.

**Architecture:** Pure CSS + JS changes in `frontend/index.html`. Task 1 handles all color changes (variables + hardcoded rgba values). Task 2 adds the tab system and overlay cleanup.

**Tech Stack:** Vanilla CSS, Vanilla JS, Monaco Editor 0.44.0 (unchanged).

## Global Constraints

- Only `frontend/index.html` changes — no server, no C++ files
- No new dependencies
- All color values must match the spec exactly
- `--accent: #f5c542` replaces `--accent: #2cbb5d` throughout
- Tabs default to Description on every problem open

---

### Task 1: CSS variable swap and hardcoded color updates

**Files:**
- Modify: `frontend/index.html` — `:root` block and CSS rules with hardcoded colors

**Interfaces:**
- Produces: Batman palette active across all page elements

- [ ] **Step 1: Update `:root` CSS variables**

Find the `:root` block (~line 11):
```css
    :root {
      --bg:       #0f0f0f;
      --surface:  #1a1a1a;
      --surface2: #222222;
      --border:   #2d2d2d;
      --text:     #eff1f6;
      --muted:    #6b7280;
      --muted2:   #9ca3af;
      --easy:     #2cbb5d;
      --medium:   #ffb800;
      --hard:     #ef4743;
      --accent:   #2cbb5d;
      --radius:   10px;
    }
```

Replace with:
```css
    :root {
      --bg:       #0d0f1a;
      --surface:  #141828;
      --surface2: #1c2235;
      --border:   #2a3050;
      --text:     #e8edf5;
      --muted:    #6b7a99;
      --muted2:   #9aaac5;
      --easy:     #4da6ff;
      --medium:   #f0a030;
      --hard:     #ff4040;
      --accent:   #f5c542;
      --radius:   10px;
    }
```

- [ ] **Step 2: Update sidebar active state hardcoded rgba values**

Find:
```css
    .cat-item.active {
      background: rgba(44,187,93,.1);
      color: var(--accent);
      border-color: rgba(44,187,93,.2);
    }
    .cat-item.active .cat-count { background: rgba(44,187,93,.15); color: var(--accent); }
```

Replace with:
```css
    .cat-item.active {
      background: rgba(245,197,66,.1);
      color: var(--accent);
      border-color: rgba(245,197,66,.2);
    }
    .cat-item.active .cat-count { background: rgba(245,197,66,.15); color: var(--accent); }
```

- [ ] **Step 3: Update table row hover and solved row colors**

Find:
```css
    tbody tr { border-top: 1px solid var(--border); transition: background .1s; }
    tbody tr:first-child { border-top: none; }
    tbody tr:hover { background: rgba(255,255,255,.03); }
    tbody tr.solved { background: rgba(44,187,93,.05); }
    tbody tr.solved:hover { background: rgba(44,187,93,.09); }
```

Replace with:
```css
    tbody tr { border-top: 1px solid var(--border); transition: background .1s; }
    tbody tr:first-child { border-top: none; }
    tbody tr:hover { background: rgba(100,130,200,.06); }
    tbody tr.solved { background: rgba(245,197,66,.05); }
    tbody tr.solved:hover { background: rgba(245,197,66,.09); }
```

- [ ] **Step 4: Update check button hover rgba**

Find:
```css
    .check-btn:hover { border-color: var(--accent); background: rgba(44,187,93,.1); }
```

Replace with:
```css
    .check-btn:hover { border-color: var(--accent); background: rgba(245,197,66,.1); }
```

- [ ] **Step 5: Update difficulty badge background colors**

Find:
```css
    .diff-badge.Easy   { color: var(--easy);   background: rgba(44,187,93,.12); }
    .diff-badge.Medium { color: var(--medium);  background: rgba(255,184,0,.12); }
    .diff-badge.Hard   { color: var(--hard);    background: rgba(239,71,67,.12); }
```

Replace with:
```css
    .diff-badge.Easy   { color: var(--easy);   background: rgba(77,166,255,.12); }
    .diff-badge.Medium { color: var(--medium);  background: rgba(240,160,48,.12); }
    .diff-badge.Hard   { color: var(--hard);    background: rgba(255,64,64,.12); }
```

- [ ] **Step 6: Update focus and hover border colors**

Find:
```css
    .search-input:focus { border-color: #555; }
```
Replace with:
```css
    .search-input:focus { border-color: #4a5a80; }
```

Find:
```css
    .form-ctrl:focus { border-color: #555; }
```
Replace with:
```css
    .form-ctrl:focus { border-color: var(--accent); }
```

Find:
```css
    .btn-run:hover { border-color: #555; }
```
Replace with:
```css
    .btn-run:hover { border-color: #4a5a80; }
```

- [ ] **Step 7: Update result badge pass color and toast ok border**

Find:
```css
    .result-badge.pass { background: rgba(44,187,93,.15); color: var(--easy); }
```
Replace with:
```css
    .result-badge.pass { background: rgba(77,166,255,.15); color: var(--easy); }
```

Find:
```css
    .toast.ok  { border-left: 3px solid var(--easy); }
```
Replace with:
```css
    .toast.ok  { border-left: 3px solid var(--accent); }
```

- [ ] **Step 8: Update overlay LC link border color**

Find:
```css
    .overlay-lc-link {
      font-size: 12px; color: var(--accent); text-decoration: none;
      display: flex; align-items: center; gap: 4px; flex-shrink: 0;
      padding: 4px 10px; border: 1px solid rgba(44,187,93,.3);
      border-radius: 6px; transition: all .15s;
    }
    .overlay-lc-link:hover { background: rgba(44,187,93,.1); }
```

Replace with:
```css
    .overlay-lc-link {
      font-size: 12px; color: var(--accent); text-decoration: none;
      display: flex; align-items: center; gap: 4px; flex-shrink: 0;
      padding: 4px 10px; border: 1px solid rgba(245,197,66,.3);
      border-radius: 6px; transition: all .15s;
    }
    .overlay-lc-link:hover { background: rgba(245,197,66,.1); }
```

- [ ] **Step 9: Update btn-submit color reference**

There are two `.btn-submit` rules. Both use `color: #0f0f0f`. Update both to match the new bg:

Find (modal submit):
```css
    .btn-submit {
      padding: 8px 20px; background: var(--accent); border: none; border-radius: 7px;
      color: #0f0f0f; font-size: 13px; font-weight: 600; font-family: inherit;
      cursor: pointer; transition: filter .15s;
    }
```
Replace with:
```css
    .btn-submit {
      padding: 8px 20px; background: var(--accent); border: none; border-radius: 7px;
      color: #0d0f1a; font-size: 13px; font-weight: 600; font-family: inherit;
      cursor: pointer; transition: filter .15s;
    }
```

Find (editor submit):
```css
    .btn-submit {
      padding: 5px 14px; border-radius: 6px; font-size: 12px; font-weight: 600;
      font-family: inherit; cursor: pointer; border: none;
      background: var(--accent); color: #0f0f0f; transition: filter .15s;
    }
```
Replace with:
```css
    .btn-submit {
      padding: 5px 14px; border-radius: 6px; font-size: 12px; font-weight: 600;
      font-family: inherit; cursor: pointer; border: none;
      background: var(--accent); color: #0d0f1a; transition: filter .15s;
    }
```

- [ ] **Step 10: Verify visually**

Build and start the server:
```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
make && ./leetcode_db
```

Open http://localhost:8080. Check:
- Page background is dark navy-black (not pure black)
- Navbar brand icon is gold
- Progress bar fill is gold
- "Add Problem" button is gold
- Difficulty badges: Easy = blue, Medium = amber, Hard = red
- Solved row has a faint gold tint
- Category items in sidebar have gold active state
- Filter pills (Easy/Medium/Hard) use the new colors

- [ ] **Step 11: Commit**

```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
git add frontend/index.html
git commit -m "feat: apply Batman Animated Series color palette"
```

---

### Task 2: Description/Details tab system + overlay cleanup

**Files:**
- Modify: `frontend/index.html` — CSS (tab styles + details panel), HTML (overlay structure), JS (tab state + helpers + openProblem update)

**Interfaces:**
- Consumes: Batman palette from Task 1 (tab active color uses `var(--accent)`)
- Produces: `switchTab(tab)`, `buildDetailsPane(meta)`, `currentProblemMeta` variable, `overlayTab` variable

- [ ] **Step 1: Add tab and details panel CSS**

Find the `/* ── Problem Overlay ────────────────────────────────────── */` CSS comment block. Add the following new rules immediately after the last rule in that section (after `.results-summary.has-fail { color: var(--hard); }`):

```css
    /* ── Overlay tabs ───────────────────────────────────────── */
    .desc-tabs {
      display: flex; border-bottom: 1px solid var(--border);
      margin: -20px -24px 16px; padding: 0 24px;
      flex-shrink: 0;
    }
    .desc-tab {
      background: none; border: none;
      border-bottom: 2px solid transparent;
      color: var(--muted2); font-size: 13px; font-weight: 500;
      font-family: inherit; padding: 10px 14px; cursor: pointer;
      transition: color .15s, border-color .15s; margin-bottom: -1px;
    }
    .desc-tab:hover { color: var(--text); }
    .desc-tab.active { color: var(--accent); border-bottom-color: var(--accent); }

    /* ── Details panel ──────────────────────────────────────── */
    .details-panel { padding: 4px 0; }
    .detail-row {
      display: flex; align-items: flex-start; gap: 12px;
      padding: 12px 0; border-bottom: 1px solid var(--border);
    }
    .detail-row:last-of-type { border-bottom: none; }
    .detail-label {
      font-size: 11px; font-weight: 600; color: var(--muted);
      text-transform: uppercase; letter-spacing: .06em;
      width: 76px; flex-shrink: 0; padding-top: 2px;
    }
    .detail-value { font-size: 13px; color: var(--text); }
    .details-lc-btn {
      display: inline-flex; align-items: center; gap: 6px;
      margin-top: 20px; padding: 8px 16px;
      background: rgba(245,197,66,.1); color: var(--accent);
      border: 1px solid rgba(245,197,66,.3); border-radius: 7px;
      text-decoration: none; font-size: 13px; font-weight: 600;
      transition: background .15s;
    }
    .details-lc-btn:hover { background: rgba(245,197,66,.18); }
```

- [ ] **Step 2: Update overlay HTML structure**

Find the current problem description div inside `<!-- Problem Overlay -->`:
```html
    <div class="problem-desc" id="problemDesc">
      <div class="desc-loading">Loading problem...</div>
    </div>
```

Replace with:
```html
    <div class="problem-desc" id="problemDesc">
      <div class="desc-tabs">
        <button class="desc-tab active" id="tabDesc"    onclick="switchTab('description')">Description</button>
        <button class="desc-tab"        id="tabDetails" onclick="switchTab('details')">Details</button>
      </div>
      <div id="paneDescription">
        <div class="desc-loading">Loading problem...</div>
      </div>
      <div id="paneDetails" style="display:none"></div>
    </div>
```

- [ ] **Step 3: Remove LC link from overlay topbar HTML**

Find in `.overlay-topbar`:
```html
    <a class="overlay-lc-link" id="overlayLcLink" href="#" target="_blank" rel="noopener">
      Open on LeetCode
      <svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
        <path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/>
        <polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/>
      </svg>
    </a>
```

Delete that entire `<a>` element. The topbar now has only the back button and title.

- [ ] **Step 4: Add new JS variables**

Find the variables block:
```js
  let currentProblemId = null;
  let currentExamples  = [];
```

Replace with:
```js
  let currentProblemId   = null;
  let currentExamples    = [];
  let currentProblemMeta = null;
  let overlayTab         = 'description';
```

- [ ] **Step 5: Add `switchTab` and `buildDetailsPane` functions**

Add these two functions immediately before `async function openProblem(id, name)`:

```js
  function switchTab(tab) {
    overlayTab = tab;
    document.getElementById('paneDescription').style.display = tab === 'description' ? 'block' : 'none';
    document.getElementById('paneDetails').style.display     = tab === 'details'     ? 'block' : 'none';
    document.getElementById('tabDesc').classList.toggle('active',    tab === 'description');
    document.getElementById('tabDetails').classList.toggle('active', tab === 'details');
  }

  function buildDetailsPane(meta) {
    const topics = (meta.topics || []).map(function(t) {
      return '<span class="topic-tag" onclick="setTopic(\'' + t + '\')">' + esc(t) + '</span>';
    }).join('');
    return '<div class="details-panel">' +
      '<div class="detail-row">' +
        '<span class="detail-label">Difficulty</span>' +
        '<span class="diff-badge ' + esc(meta.difficulty) + '">' + esc(meta.difficulty) + '</span>' +
      '</div>' +
      '<div class="detail-row">' +
        '<span class="detail-label">Problem</span>' +
        '<span class="detail-value">#' + meta.id + '</span>' +
      '</div>' +
      '<div class="detail-row">' +
        '<span class="detail-label">Topics</span>' +
        '<div class="topic-tags">' + (topics || '<span style="color:var(--muted);font-size:12px">None</span>') + '</div>' +
      '</div>' +
      '<a class="details-lc-btn" href="' + esc(meta.lcUrl) + '" target="_blank" rel="noopener">' +
        'Open on LeetCode' +
        '<svg width="11" height="11" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">' +
          '<path d="M18 13v6a2 2 0 0 1-2 2H5a2 2 0 0 1-2-2V8a2 2 0 0 1 2-2h6"/>' +
          '<polyline points="15 3 21 3 21 9"/><line x1="10" y1="14" x2="21" y2="3"/>' +
        '</svg>' +
      '</a>' +
    '</div>';
  }
```

- [ ] **Step 6: Update `openProblem` to populate meta, reset tab, and remove old LC link logic**

Find the current `openProblem` function:
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

Replace with:
```js
  async function openProblem(id, name) {
    currentProblemId   = id;
    currentExamples    = [];
    editorDirty        = false;

    // Build meta from local data immediately (no API needed for Details tab)
    const localP = problems.find(function(p) { return p.id === id; });
    currentProblemMeta = {
      id:         id,
      title:      name,
      difficulty: localP ? localP.difficulty : '',
      topics:     localP ? (localP.topics || []) : [],
      lcUrl:      'https://leetcode.com/problems/' + slug(name) + '/'
    };

    document.getElementById('overlayTitle').textContent = name;
    document.getElementById('resultsStrip').style.display = 'none';
    document.getElementById('problemOverlay').classList.add('open');

    // Reset to Description tab and populate Details immediately
    switchTab('description');
    document.getElementById('paneDescription').innerHTML = '<div class="desc-loading">Loading description...</div>';
    document.getElementById('paneDetails').innerHTML = buildDetailsPane(currentProblemMeta);

    if (monacoReady) setEditorValue(getStarterCode(document.getElementById('langSelect').value, name, []));
    if (lcLanguages.length === 0) await loadLanguages();

    try {
      const r = await fetch('/api/problems/' + id + '/meta');
      if (!r.ok) throw new Error('not found');
      const meta = await r.json();
      currentExamples = Array.isArray(meta.examples_json) ? meta.examples_json : [];
      if (meta.title) currentProblemMeta.title = meta.title;
      document.getElementById('paneDetails').innerHTML = buildDetailsPane(currentProblemMeta);
      if (monacoReady && !editorDirty) {
        setEditorValue(getStarterCode(document.getElementById('langSelect').value, name, currentExamples));
      }
      document.getElementById('paneDescription').innerHTML =
        '<h1>' + esc(meta.title || name) + '</h1>' +
        '<div class="problem-desc-content">' + (meta.description_html || '<p>No description available.</p>') + '</div>';
    } catch {
      const lcUrl = currentProblemMeta.lcUrl;
      document.getElementById('paneDescription').innerHTML =
        '<div class="desc-loading">Could not load description — ' +
        '<a class="details-lc-btn" style="display:inline-flex;margin-top:8px" href="' + esc(lcUrl) +
        '" target="_blank" rel="noopener">open on LeetCode ↗</a></div>';
    }
  }
```

- [ ] **Step 7: Update `closeProblem` to reset new state**

Find:
```js
  function closeProblem() {
    document.getElementById('problemOverlay').classList.remove('open');
    currentProblemId = null;
    currentExamples  = [];
  }
```

Replace with:
```js
  function closeProblem() {
    document.getElementById('problemOverlay').classList.remove('open');
    currentProblemId   = null;
    currentExamples    = [];
    currentProblemMeta = null;
  }
```

- [ ] **Step 8: Verify manually — tabs**

Start the server: `./leetcode_db`

Open http://localhost:8080. Click any problem title. Check:
- Overlay opens with "Description" tab active (gold underline on tab)
- Description tab shows "Loading description..." then problem content after fetch
- Clicking "Details" tab shows: difficulty badge, problem number, topics as chips, "Open on LeetCode" button
- Details tab is populated immediately (doesn't wait for meta fetch)
- Closing and reopening a problem always starts on the Description tab
- The overlay topbar no longer has a cramped LC link — just back button + title

- [ ] **Step 9: Verify manually — full visual check**

With the server running, check all visual elements:
- Overlay tabs use gold active underline
- Details panel LC button is gold-bordered
- Topic chips in Details tab are clickable (they filter the table behind the overlay)
- Difficulty badge in Details tab uses the correct Batman color (blue/amber/red)
- No JavaScript errors in browser console (F12)

- [ ] **Step 10: Commit**

```bash
cd "/Users/Alex/Documents/Resume Projects/leetcode-database"
git add frontend/index.html
git commit -m "feat: add Description/Details tabs and clean up overlay topbar"
```
