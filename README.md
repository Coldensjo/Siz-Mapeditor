# Siz Mapeditor

Remere's Map Editor fork with a standalone **MapServer** for live collaborative map editing.

## Features

Fork improvements on top of Remere's Map Editor, grouped by area.

### Live collaboration

- Standalone **MapServer** — host a map from the command line while editors connect over the network
- Connect to a live server from the **welcome dialog** on startup
- Session bounds, passwords, auto-save, and client management (`list`, `kick`, `save`) on the server

### Mapping & brushes

- **2×2 brush** for faster area painting
- **Refresh zone** tool
- **Copy raid area** from a selection
- **TVP door settings** on right-click (keyhole number, door level, and related fields)
- Spawns are not placed on top of an existing monster

### Creatures & spawns

- **Search creatures** in the palette
- **Spawn times** shown directly on spawns
- **Export spawn creatures**
- Set `species="unknown"` in `monsterName.xml` to auto-sort monsters into categories in the creature palette

### Items, containers & replace

- Improved **add items to containers** with buckets and favorites
- Improved **replace item(s)** dialog
- **Search unique** items; actions are sorted
- Choose where **monsters, NPCs, and items** are loaded from in preferences

### View & navigation

- **Minimap** shows all floors at once
- **Fishable water** overlay
- **Select house exit** hotkey
- **Additional copy position** format
- Larger **edit text** dialog

### Palette & UI

- Palette toolbar on the **left side**
- Optional **large sprite previews** in palettes (configurable in preferences)
- **Massively improved performance** across the editor

### Workflow & preferences

- **Auto-save on close** (optional)
- **Skip save confirmation on close** (optional)
- **Open a specific map on startup** (optional)

## Build

**Requirements**

- Windows 10+
- Visual Studio 2022 (MSVC v143)
- [vcpkg](https://vcpkg.io/) with manifest mode enabled

**Steps**

1. Clone this repo.
2. Open `vcproj/Editor.sln` in Visual Studio.
3. Select **Release | x64**.
4. Build the solution (builds both `Editor_x64.exe` and `MapServer_x64.exe`).

Dependencies are listed in `vcpkg.json` and are installed automatically when you build.

Built executables are placed in the repo root. Keep the `data/` folder next to them (sprites, clients, etc.).

## MapServer

MapServer hosts a map so multiple editors can edit it together over the network.

### Start the server

```text
MapServer_x64.exe --map path\to\map.otbm [options]
```

**Options**

| Option | Description | Default |
|--------|-------------|---------|
| `--port <number>` | TCP port | `31313` |
| `--password <pass>` | Session password | empty |
| `--name <name>` | Session display name | map name |
| `--autosave <minutes>` | Auto-save interval (`0` = off) | `5` |
| `--x`, `--y`, `--z`, `--radius` | Limit editing to an area around a center tile | whole map |

**Example**

```text
.\MapServer_x64.exe --map data\world.otbm --port 31313 --password secret --name "Thais rebuild"
```

**Example with session bounds** (only tiles within 50 sqm of 1000,1000,7):

```text
.\MapServer_x64.exe --map data\world.otbm --x 1000 --y 1000 --z 7 --radius 50
```

**This is what I use**
```text
.\MapServer_x64.exe --map C:\Servers\Ironcore\data\world\Ironcore.otbm --port 31313 --password 1 --name "Ironcore" --x 2564 --y 1618 --z 7 --radius 350 --autosave 5
```

### Console commands

While the server is running, type:

| Command | Action |
|---------|--------|
| `save` | Save the map now |
| `list` | Show connected clients |
| `kick <name>` | Disconnect a client |
| `exit` | Save pending changes and shut down |

Auto-save creates timestamped backups when the map has changed.

## Connect from the editor

1. Run `Editor_x64.exe`.
2. Go to **Network → Connect to Server...**
3. Enter host, port, display name, and password (if the server uses one).
4. Click **Connect**.

You edit the hosted map in real time with other connected editors.

## Settings

Editor and MapServer read settings from `editor.cfg` in the same folder as the executable. If the file is missing, defaults are used.

Live connection defaults (under the **Network** section):

| Setting | Default | Used for |
|---------|---------|----------|
| `LIVE_HOST` | `127.0.0.1` | Last host used in the connect dialog |
| `LIVE_PORT` | `31313` | Last port used |
| `LIVE_USERNAME` | `User` | Your display name on the server |
| `LIVE_PASSWORD` | empty | Session password |

These are updated when you connect from the editor. MapServer session options (`--port`, `--password`, etc.) are set on the command line when you start the server, not in `editor.cfg`.

You can edit other editor preferences in `editor.cfg` or through the editor's settings UI.

## License

MIT [LICENSE](LICENSE)