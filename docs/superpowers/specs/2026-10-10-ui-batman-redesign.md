# UI Redesign — Batman 90s Theme + LeetCode Layout

**Date:** 2026-10-10
**Author:** Alexis Sanchez
**Status:** Approved

---

## Goal

Replace the current green-on-black color scheme with a Batman Animated Series–inspired palette (dark navy, blue-gray, gold accent) and add LeetCode-style tabs to the problem overlay. All changes are in `frontend/index.html` only.

---

## Scope

- CSS variable swap → Batman palette
- Problem overlay left panel gains Description / Details tabs
- Overlay top bar simplified (LeetCode link moves into Details tab)
- Component polish: badges, table rows, buttons, modal, toast

No server changes. No new dependencies.

---

## Color Palette

| Variable | Value | Role |
|---|---|---|
| `--bg` | `#0d0f1a` | Page background — deep navy-black |
| `--surface` | `#141828` | Cards, panels |
| `--surface2` | `#1c2235` | Table header, inputs, editor toolbar |
| `--border` | `#2a3050` | All borders — blue-tinted dark |
| `--text` | `#e8edf5` | Body text — cool off-white |
| `--muted` | `#6b7a99` | Secondary text — blue-gray |
| `--muted2` | `#9aaac5` | Tertiary text — lighter blue-gray |
| `--accent` | `#f5c542` | Batman gold — primary interactive color |
| `--easy` | `#4da6ff` | Easy difficulty — Batman blue |
| `--medium` | `#f0a030` | Medium difficulty — amber |
| `--hard` | `#ff4040` | Hard difficulty — villain red |
| `--radius` | `10px` | Unchanged |

### Derived color uses
- Solved row background: `rgba(245,197,66,.05)` (gold tint, replaces green tint)
- Solved row hover: `rgba(245,197,66,.09)`
- Active category/sidebar: `rgba(245,197,66,.1)` bg, gold text, gold border
- Table row hover: `rgba(100,130,200,.06)` (cool blue tint)
- Check button done state: gold background + border
- Progress bar fill: `var(--accent)`
- Progress big number: `var(--accent)`

---

## Problem Overlay — Tab System

### Structure

The left panel (`.problem-desc`) gains a tab bar at the top with two tabs:

**Tab 1: Description** (default active)
- Problem title as `<h1>`
- Full problem HTML content (unchanged)

**Tab 2: Details**
- Difficulty badge (using new palette colors)
- Topics as clickable chips (same `setTopic()` behavior)
- Problem number
- "Open on LeetCode" styled button (moved from overlay top bar)

### Top bar simplification

Remove the `overlay-lc-link` anchor from `.overlay-topbar`. The top bar becomes: back button + problem title only. Cleaner, less crowded.

### Tab switching

JS state: `let overlayTab = 'description';` — toggled on tab click.

CSS: `.tab-pane` with `display:none` / `display:block` based on active tab. Tab buttons get an active underline using `border-bottom: 2px solid var(--accent)`.

No new API calls. Details tab content is built from two existing sources:
- `id`, `difficulty`, `topics` — from the local `problems` array (`problems.find(p => p.id === id)`) at the time `openProblem` is called
- `title` — from the meta API response (falls back to `name` if meta unavailable)
- `lcUrl` — computed as `'https://leetcode.com/problems/' + slug(name) + '/'`

All of this is stored in `currentProblemMeta` when `openProblem` fires.

---

## Component Changes

### Navbar
- Brand icon background: `var(--accent)` (gold)
- Brand icon SVG stroke: `#0d0f1a` (dark, for contrast on gold)
- `navbar-tag` border color: `var(--border)` (already correct with new var)

### Progress card
- `progress-bar-fill` background: `var(--accent)` → already uses `--accent`, automatic
- `progress-big` color: `var(--accent)` → already uses `--accent`, automatic
- Diff stat colors update automatically via `--easy` / `--medium` / `--hard` vars

### Table
- `tbody tr:hover` background: `rgba(100,130,200,.06)` (blue tint replaces white)
- `tbody tr.solved` background: `rgba(245,197,66,.05)`
- `tbody tr.solved:hover` background: `rgba(245,197,66,.09)`
- `.check-btn.done` background + border: `var(--accent)`
- `.topic-tag:hover` color: `var(--accent)` → already uses `--accent`, automatic

### Difficulty badges
- `.diff-badge.Easy` color: `var(--easy)`, background: `rgba(77,166,255,.12)`
- `.diff-badge.Medium` color: `var(--medium)`, background: `rgba(240,160,48,.12)`
- `.diff-badge.Hard` color: `var(--hard)`, background: `rgba(255,64,64,.12)`

### Editor toolbar buttons
- `.btn-submit` (editor): background `var(--accent)`, color `#0d0f1a`
- `.btn-run`: border `var(--border)`, hover border `#4a5a80`
- `lang-select` focus: border-color `var(--accent)` (add focus style)

### Modal
- `.form-ctrl:focus` border-color: `var(--accent)` (replaces `#555`)
- `.btn-submit` (modal): background `var(--accent)`, color `#0d0f1a` → automatic via `--accent`

### Toast
- `.toast.ok` left border: `var(--accent)` (replaces `var(--easy)`)

### Sidebar
- `.cat-item.active` background: `rgba(245,197,66,.1)`, color: `var(--accent)`, border: `rgba(245,197,66,.2)` → automatic via `--accent`

### Filter pills
- `.pill.active` (All): background `var(--text)`, color `var(--bg)` — unchanged
- `.pill.active.easy/medium/hard` → update colors to new palette vars

---

## New JS State

Add one variable to store problem metadata for the Details tab:

```js
let currentProblemMeta = null; // { title, difficulty, topics, lcUrl }
```

Populated in `openProblem()` after meta loads, alongside `currentExamples`.

---

## Files Changed

| File | Change |
|---|---|
| `frontend/index.html` | CSS variables, tab HTML/CSS/JS, overlay topbar, component styles, `currentProblemMeta` var, `openProblem` update |

No other files change.
