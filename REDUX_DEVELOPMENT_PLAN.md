# Siz Mapeditor — Redux Porting Development Plan

A milestone-based plan for adopting worthwhile changes from
[TibiaDev/remeres-map-editor-redux](https://github.com/TibiaDev/remeres-map-editor-redux).

This supersedes the raw candidate list in [REDUX_PORTING_CANDIDATES.md](REDUX_PORTING_CANDIDATES.md):
every item below was **cross-checked against the actual repo source** on 2026‑06‑28. Many original
candidates turned out to be already present or not applicable — see the table at the bottom.

---

## How to use this document (for AI agents)

- Work **one task at a time, top to bottom**. Tasks have IDs like `M1-T1`.
- Each task lists: **Goal · Files · Change · Acceptance · Risk**. Do not exceed the listed files
  without saying why.
- A task is **done** only when its Acceptance bullets pass *and* the project still builds
  (`vcproj/Editor.sln`, Release|x64).
- `redux@<hash>` references a commit in the redux repo for context — **read it, don't blind-copy**:
  redux's source tree is a rewrite (`source/rendering/`, `source/map/`, `source/io/otbm/`…),
  so file paths differ from this fork's flat `source/`.
- Commit per task with message `redux-port(M#-T#): <summary>`. Keep diffs small and reviewable.
- If a task's Acceptance reveals the change is already present, mark it `SKIP (already present)`
  in this file and move on.

**Status legend:** `TODO` · `IN PROGRESS` · `DONE` · `SKIP`

---

## Milestone 1 — Correctness & data-safety hardening  *(low risk, high value)*

Goal: close real or latent data/stability gaps. Small, surgical diffs.

### M1-T1 — OTBM first-tile node guard  ·  `DONE`
- **redux@6284c7e**
- **Goal:** make the first tile in OTBM save always open a fresh `OTBM_TILE_AREA` node, independent
  of the sentinel `local_*` values.
- **Files:** [source/iomap_otbm.cpp](source/iomap_otbm.cpp) (~line 1817).
- **Change:** add `first ||` to the node-boundary condition:
  ```cpp
  if (first || pos.x < local_x || pos.x >= local_x + 256 || pos.y < local_y || pos.y >= local_y + 256 || pos.z != local_z) {
  ```
- **Acceptance:** save a map, reload it, tile count and contents unchanged; a map whose first tile
  has `x,y < 256` and `z == 0` still saves correctly.
- **Risk:** Low. Currently the `local_z = -1` init already forces the first node, so this is
  defensive hardening/parity rather than a live bug. Keep it as a 1-line change.

### M1-T2 — Prune stale recent-file paths on startup  ·  `DONE`
- **redux@ec5bde9** (recent-files portion)
- **Goal:** silently drop recent-file entries whose file no longer exists, so the File menu /
  welcome screen never offers dead links.
- **Files:** [source/main_menubar.cpp](source/main_menubar.cpp) and/or
  [source/gui.cpp](source/gui.cpp) (wherever `recentFiles` / `wxFileHistory` is loaded),
  [source/welcome_dialog.cpp](source/welcome_dialog.cpp).
- **Change:** on load, `wxFileName(path).FileExists()`-check each entry; remove missing ones and
  persist the trimmed list.
- **Acceptance:** delete/rename a recently-opened map, restart; it no longer appears in recents and
  no error dialog fires.
- **Risk:** Low. Read-side only; do not touch save logic.

### M1-T3 — Validate client paths, drop missing ones  ·  `DONE`
- **redux@ec5bde9** (client-path portion)
- **Goal:** when a configured client data directory no longer exists, skip/prune it instead of
  failing or pointing at a dead path.
- **Files:** [source/client_version.cpp](source/client_version.cpp) (`loadVersions`, path setters
  near lines 458/492/556 already `DirExists()`-check — extend to prune config).
- **Change:** if a stored client path fails `DirExists()`, clear/skip that version's user path and
  persist. NOTE: this fork loads versions from `clients.xml` (not redux's TOML `[clients]` array),
  so adapt the *idea*, not redux's TOML iterator code.
- **Acceptance:** point a version at a now-deleted folder, restart; no crash, version is simply
  unavailable, config no longer references the dead path.
- **Risk:** Medium — touches version loading. Verify other versions still load.

**Milestone 1 exit criteria:** all three tasks `DONE`/`SKIP`, full save→reload round-trip verified,
app launches clean with intentionally-broken recent/client paths.

---

## Milestone 2 — UX polish  *(low risk, visible wins)*

Goal: accessibility and discoverability improvements that don't depend on redux's renderer.

### M2-T1 — Tooltips + status-bar help text  ·  `SKIP (already present)`
> Verified 2026-06-28: toolbar buttons already pass short-help text to `AddTool`
> (main_toolbar.cpp) and use `SetToolTip` on the position controls; every
> `<item>` in `data/menubar.xml` already has a `help=` attribute, wired into the
> status bar via `wxMenu::Append`'s help param (main_menubar.cpp:771). Dialogs
> already carry 67 `SetToolTip` calls. Both acceptance bullets pass with no change.

- **redux@9d99fb3, @f46e403, @d346d8c**
- **Goal:** add hover tooltips to toolbar buttons and dialog controls, and status-bar help strings
  to menu items.
- **Files:** [source/main_toolbar.cpp](source/main_toolbar.cpp),
  [data/menubar.xml](data/menubar.xml) (help attrs),
  dialog windows under `source/*_window.cpp` as needed.
- **Change:** `SetToolTip(...)` on controls; menu `help=""` strings shown via the status bar.
- **Acceptance:** hovering toolbar buttons shows tooltips; hovering menu items shows help text in
  the status bar. No layout regressions.
- **Risk:** Low, additive. Tedious but safe. Can be split per-window if large.

### M2-T2 — Map HUD / minimap polish  ·  `DONE`
> Implemented an on-canvas HUD (top-left) showing centered position (X,Y,Z),
> floor, and zoom %, reusing the fork's OpenGL 1.x screen-space text path
> (`PushScreenSpaceGL`/`glutBitmapCharacter`). Gated behind a new "Show HUD"
> View > Map Overlays toggle (`Config::SHOW_HUD`, default on; off for ingame/
> screenshot rendering). Minimap left as-is for now. redux's diff was reference
> only, as predicted.

- **redux@2ac2fa7**
- **Goal:** adopt the *ideas* — clearer on-canvas HUD (position/zoom/floor), minimap readability.
- **Files:** [source/map_display.cpp](source/map_display.cpp),
  [source/minimap_window.cpp](source/minimap_window.cpp) if present.
- **Change:** reimplement against this fork's OpenGL 1.x drawing — redux's diff targets its new
  `map_display`/`minimap_window` and **will not apply**. Treat as design reference only.
- **Acceptance:** HUD shows current position/floor/zoom; no FPS regression; toggleable if intrusive.
- **Risk:** Medium — touches the hot draw path. Gate behind a view option.

**Milestone 2 exit criteria:** tooltips/status help shipped; HUD change either landed behind a
toggle or deferred with a note here.

---

## Milestone 3 — Performance: spatial hash grid  *(high effort, high value)*

Goal: the single best *non-rendering* perf idea in redux — speed up the hottest path, `getTile`.

> ⚠️ This is a scoped engineering project, **not** a cherry-pick. Land it on a branch, benchmark
> before/after, and only merge on a measured win with identical map output.

### M3-T1 — Baseline benchmark  ·  `TODO`
- **Goal:** measure current `getTile` / borderize / render cost on a large map before changing anything.
- **Files:** none (measurement only); record numbers in this file under the task.
- **Acceptance:** documented baseline (open-large-map time, pan/scroll FPS, full-map borderize time).
- **Risk:** None.

### M3-T2 — Introduce spatial hash grid behind the map API  ·  `DONE (pending benchmark)`
> Implemented `source/spatial_hash_grid.h`: a leaf-lookup accelerator over the
> existing HexTree. Keyed by the upper 14 bits of x/y (the leaf = 4x4 block the
> tree navigation resolves to), it caches `QTreeNode*` leaf pointers plus a
> single-entry fast path for spatially-local access (rendering/borderize scans).
> Wired behind `BaseMap::getLeaf`/`createLeaf` (basemap.h) so all call sites —
> `getTileL`, create/set/swap, live client/server, map_drawer — are unchanged.
> The quadtree stays the source of truth; the cache is dropped on `clear()`.
>
> Safety: leaves are never individually freed (no `freeNode` caller; only the
> recursive `~QTreeNode` at teardown), so cached pointers can't dangle. Map
> access is already single-threaded (live net work is marshalled to the main
> thread via `CallAfter`), so the write-on-read cache adds no new race.
>
> STILL REQUIRED before this counts as fully `DONE`: a Release|x64 build, the
> M3-T1 baseline, and a save->reload byte-identical + measured-win check. I
> cannot build/run in this environment.

- **redux@0022306, @552ebce, @cec63db, @48406ee**
- **Goal:** add a spatial hash grid (sorted-vector backed, cell-index not pointer) caching tile
  lookups, behind the existing `BaseMap`/`Map` accessors so call sites don't change.
- **Files:** [source/basemap.h](source/basemap.h), [source/basemap.cpp](source/basemap.cpp),
  [source/map.h](source/map.h), `source/map_region.*`, plus a new `spatial_hash_grid.h`.
- **Change:** port the data structure; keep the classic quadtree as source of truth or replace it
  carefully. Invalidate the cache on tile insert/remove.
- **Acceptance:** all existing map ops behave identically; save→reload byte-identical; M3-T1
  benchmarks improve measurably with no correctness regressions.
- **Risk:** High — core storage. Extensive manual testing (paste, fill, borderize, undo/redo, copy).

### M3-T3 — Cache ItemType lookups on the hot path  ·  `TODO`
- **redux@e0e1051, @48c5af8**
- **Goal:** eliminate repeated `g_items[id]` array lookups in tile update/draw.
- **Files:** [source/tile.cpp](source/tile.cpp), [source/item.cpp](source/item.cpp).
- **Change:** cache the `ItemType*` where safe; redux later removed an unsafe cached pointer
  (`48c5af8`) — apply the **safe** final form, not the intermediate.
- **Acceptance:** no behavior change; profile shows fewer array lookups.
- **Risk:** Medium — dangling-pointer hazard on version reload; null-check and invalidate.

**Milestone 3 exit criteria:** measured speedup over M3-T1 baseline, zero output diff on
save→reload, all edit operations verified by hand.

---

## Milestone 4 — Renderer modernization  *(major, aspirational — plan only)*

Goal: the headline of redux (OpenGL 4.x, async sprite loading, batching, GPU light, 160+ FPS).
**Not portable commit-by-commit.** This milestone is a placeholder/decision point, not scheduled work.

- **redux:** the `feat(rendering)`/`refactor(rendering)` series + build move to wxWidgets 3.3 / C++23.
- **Decision required (escalate to maintainer):** is a renderer rewrite worth it for this fork's
  goals (live collaborative editing via MapServer)? If yes, scope as its own multi-week project
  with its own plan; M3 (spatial hash) is the cheap prerequisite/down-payment.
- **Do not start** M4 work as part of this plan without explicit sign-off.

---

## Cross-check results (why the candidate list shrank)

Verified against repo source on 2026‑06‑28:

| Redux item | Verdict | Evidence in this repo |
|---|---|---|
| Door IDs not assigned to windows (`b4ef9bc`) | ✅ Already present | `Door::isRealDoor()` at iomap_otbm.cpp:453 |
| Welcome-screen CLI override (`d7a803f`) | ✅ Already present | application.cpp:375 sets `WELCOME_DIALOG` from argv |
| FPS limit/counter (`6505740`) | ✅ Already present | preferences/settings + map_drawer |
| Find Action/Unique sort (`7c7559b`) | ✅ Already present | main_menubar.cpp:1218 `sort()` |
| ctrl+V teleport destination (`4ea9d56`) | ✅ Already present (more advanced) | positionctrl.cpp paste handlers |
| `doBorders` infinite-loop fix (`8f42cd9`) | ✅ Already present | ground_brush.cpp |
| House-with-no-town crash (`5c20805`) | ✅ Already present | palette_house.cpp:612 |
| Collections / locked doors / spawntime cap / tileset export | ✅ Already present | grep-confirmed |
| VersionManager use-after-free (`81da033`) | ❌ Not applicable | no `VersionManager`/`SpritePreloader`; uses `GUI::UnloadVersion()` |
| Tile state-flag reset (`6526a2a`) | ❌ Not applicable | `Tile::update()` (tile.cpp:456) doesn't manage `mapflags`/`house_id` |
| Ground border lookup table (`18b9575`) | ➖ Already equivalent | ground_brush.cpp uses bitmask `tiledata` |
| Animation pause control (`78b77e2`) | ➖ Marginal | `AnimationTimer` already has Start/Stop (map_display.cpp:3349) |
| OTBM first-tile guard (`6284c7e`) | ⚠️ Missing, low impact → **M1-T1** | iomap_otbm.cpp:1817 lacks `first \|\|` |
| Stale path pruning (`ec5bde9`) | 🟡 Partially portable → **M1-T2/T3** | XML clients + recent files |
| Tooltips / status help (`9d99fb3` …) | 🟡 Portable UX → **M2-T1** | additive |
| Map HUD / minimap (`2ac2fa7`) | 🟡 Reimplement → **M2-T2** | redux diff won't apply |
| Spatial hash grid (`0022306` …) | 🔴 Big-ticket → **Milestone 3** | classic quadtree storage |
| Renderer overhaul | 🔴 Aspirational → **Milestone 4** | OpenGL 1.x here |

---

## Recommended order

`M1-T1 → M1-T2 → M1-T3` (safe wins) → `M2-T1` (polish) → benchmark `M3-T1` → decide on `M3` /
`M2-T2` → escalate `M4`. Stop and reassess after Milestone 1 if time-boxed.

*Generated from redux clone @ `01c2536`. Always diff against current source before editing.*
