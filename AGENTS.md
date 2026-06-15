# AI agent instructions — building Siz Mapeditor

This is a **Windows-only** C++ project. There is no CMake, Makefile, or cross-platform build. Use the Visual Studio solution under `vcproj/`.

## Quick reference

| Item | Value |
|------|-------|
| Solution | `vcproj/Editor.sln` |
| Projects | `Editor` (GUI), `MapServer` (console) |
| Platform | **x64 only** (Win32 solution configs map to x64) |
| Preferred config | **Release \| x64** |
| Toolset | MSVC **v143** (Visual Studio 2022) |
| C++ standard | C++17 |
| Dependency manager | **vcpkg** manifest mode (`vcpkg.json` at repo root) |
| Outputs | `Editor_x64.exe`, `MapServer_x64.exe` in **repo root** |
| Intermediate files | `vcproj/Project/x64/{Debug\|Release}/{Editor\|MapServer}/` |

## Prerequisites

1. **Windows 10+**
2. **Visual Studio 2022** (or newer) with:
   - Desktop development with C++
   - MSVC v143 toolset
   - Windows 10/11 SDK
3. **[vcpkg](https://vcpkg.io/)** installed and integrated with Visual Studio (manifest mode).

Set `VCPKG_ROOT` to your vcpkg clone if it is not already set, for example:

```powershell
$env:VCPKG_ROOT = "E:\vcpkg"   # adjust to your path
```

The `.vcxproj` files enable manifest mode (`VcpkgEnableManifest=true`). Dependencies from `vcpkg.json` are installed automatically on the first build. No separate `vcpkg install` step is required, but you may pre-install manually:

```powershell
cd C:\path\to\SizMapeditor
& "$env:VCPKG_ROOT\vcpkg.exe" install --triplet x64-windows
```

### vcpkg dependencies

Declared in `vcpkg.json`: wxwidgets, freeglut, asio, nlohmann-json, fmt, libarchive, boost-spirit, boost-asio, tomlplusplus.

## Build — Visual Studio IDE

1. Open `vcproj/Editor.sln`.
2. Set configuration to **Release** and platform to **x64**.
3. Build Solution (`Ctrl+Shift+B`) — builds both `Editor` and `MapServer`.
4. Confirm outputs exist in the repo root:
   - `Editor_x64.exe`
   - `MapServer_x64.exe`

To build a single target: right-click **Editor** or **MapServer** → **Build**.

## Build — command line (preferred for agents)

**Always build through `vcproj\Editor.sln`, never by pointing MSBuild at a `.vcxproj` file directly.** The projects set `OutDir` to `$(SolutionDir)..\`, which resolves to the **repo root** only when `SolutionDir` comes from the solution (i.e. `vcproj\`). Building `vcproj\Project\Editor.vcxproj` alone leaves `SolutionDir` unset and drops `Editor_x64.exe` into `vcproj\` instead.

From the **repo root**, find MSBuild with `vswhere`, then build the solution:

```powershell
cd C:\path\to\SizMapeditor   # repo root — required

$msbuild = & "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe" `
  -latest -requires Microsoft.Component.MSBuild `
  -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1

& $msbuild "vcproj\Editor.sln" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
```

A successful build ends with lines like (paths must be under the **repo root**, not `vcproj\`):

```text
Editor.vcxproj -> C:\path\to\SizMapeditor\Editor_x64.exe
MapServer.vcxproj -> C:\path\to\SizMapeditor\MapServer_x64.exe
```

If the log shows `...\vcproj\Editor_x64.exe`, the build was invoked incorrectly — use `vcproj\Editor.sln` as shown above.

### Build one project only

Still use the solution file; pass `/t:` to limit the build target:

```powershell
& $msbuild "vcproj\Editor.sln" /t:Editor /p:Configuration=Release /p:Platform=x64 /m /v:minimal
& $msbuild "vcproj\Editor.sln" /t:MapServer /p:Configuration=Release /p:Platform=x64 /m /v:minimal
```

Do **not** run MSBuild on `vcproj\Project\Editor.vcxproj` or `vcproj\Project\MapServer.vcxproj` directly.

### Debug build

```powershell
& $msbuild "vcproj\Editor.sln" /p:Configuration=Debug /p:Platform=x64 /m /v:minimal
```

Debug links against `freeglutd.lib` and uses vcpkg Debug libraries.

### Clean rebuild

```powershell
& $msbuild "vcproj\Editor.sln" /t:Clean /p:Configuration=Release /p:Platform=x64
& $msbuild "vcproj\Editor.sln" /p:Configuration=Release /p:Platform=x64 /m /v:minimal
```

## Source layout (for compile context)

- `source/` — all C++ sources and headers
- `vcproj/Project/*.vcxproj` — MSBuild project files (reference `source/` via relative paths)
- `data/` — editor metadata (tilesets, brushes, etc.); must stay next to the executables at runtime
- `vcpkg.json` — dependency manifest (repo root)

Both projects share most of `source/`. `MapServer` is a console app (`SubSystem=Console`); `Editor` is a Windows GUI app (`SubSystem=Windows`, wxWidgets).

Precompiled header: `main.h` (included as `PrecompiledHeaderFile` in both projects).

## Editor version

The editor version lives in `source/definitions.h`:

```cpp
#define __RME_VERSION_MAJOR__ 2
#define __RME_VERSION_MINOR__ 0
#define __RME_SUBVERSION__ 0
```

**Whenever you change Editor behavior** (C++ under `source/` that affects the GUI app, or Editor-only project files), **increment the version** in the same change:

| Change kind | Bump |
|-------------|------|
| Bug fix, small tweak, or incremental feature | `__RME_SUBVERSION__` (+1) |
| Notable new feature or user-visible milestone | `__RME_VERSION_MINOR__` (+1), reset `__RME_SUBVERSION__` to `0` |
| Breaking change or major rewrite | `__RME_VERSION_MAJOR__` (+1), reset minor and subversion to `0` |

Do **not** bump the editor version for MapServer-only changes, `data/` metadata edits, docs, or build/config-only work unless the user asks.

`__LIVE_NET_VERSION__` in the same file is separate — bump it only when the live collaboration packet format changes incompatibly (see comment in `definitions.h`).

The version appears in the welcome dialog, About window, status bar, exported map headers, and live-client handshake (`__RME_VERSION_ID__` is derived automatically from the three version macros).

## Verify after code changes

When you modify C++ code, **run a build** before considering the task done. Check:

1. Exit code `0` from MSBuild.
2. Expected `.exe` updated in repo root.
3. No new compiler errors in the build log.
4. If Editor behavior changed, `source/definitions.h` version was incremented (see **Editor version** above).

For verbose errors:

```powershell
& $msbuild "vcproj\Editor.sln" /p:Configuration=Release /p:Platform=x64 /m /v:normal
```

Build logs are also written under `vcproj/Project/x64/{Configuration}/{Editor|MapServer}/`.

## Troubleshooting

| Problem | What to try |
|---------|-------------|
| `MSBuild` not found | Install VS 2022 C++ workload, or use `vswhere` path above |
| vcpkg / missing headers (`wx/`, `boost/`, etc.) | Set `VCPKG_ROOT`; rebuild so manifest install runs; or run `vcpkg install --triplet x64-windows` from repo root |
| Wrong platform | Use **x64**, not Win32 |
| LTCG / incremental link warnings | Usually non-fatal; a full rebuild often clears them |
| `error C3859: PCH` / PCH issues | Clean then rebuild; ensure `main.h` is unchanged in incompatible ways across TUs |
| Link errors for `archive`, `freeglut`, wx libs | vcpkg triplet must be `x64-windows`; Debug needs Debug vcpkg libs |
| `.exe` lands in `vcproj\` instead of repo root | Build `vcproj\Editor.sln`, not `vcproj\Project\*.vcxproj`; run MSBuild from repo root |

Hard-coded fallback library paths in `.vcxproj` (e.g. `C:\vcpkg\...`) are developer-specific. The build should work through **vcpkg manifest integration** when `VCPKG_ROOT` and VS vcpkg integration are configured correctly.

## Do not

- Attempt to build on Linux/macOS (Windows-only).
- Add CMake or other build systems unless explicitly requested.
- Commit build artifacts: `*.exe`, `*.pdb`, `*.obj`, `vcproj/Project/x64/`, `.vs/`.
- Expect `data/` alone to provide Tibia client sprites; runtime needs separate client asset paths (see `README.md`).

## More documentation

- `README.md` — features, MapServer usage, client assets, settings
- `mapserver.cfg.example` — MapServer configuration template