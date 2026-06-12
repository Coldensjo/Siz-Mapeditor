# Siz Mapeditor

Remere's Map Editor fork with a standalone **MapServer** for live collaborative map editing.

## Features

Fork improvements on top of Remere's Map Editor, grouped by area.

### Live collaboration

- Standalone **MapServer** — host a map from the command line while editors connect over the network
- Connect to a live server from the **welcome dialog** on startup
- Server-side **asset sync** on connect (editors download the host's client files; see [Client assets](#client-assets))
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

**Recommended:** copy `mapserver.cfg.example` to `mapserver.cfg` next to `MapServer_x64.exe`, edit it, then run:

```text
MapServer_x64.exe
```

**Or** pass options on the command line (they override `mapserver.cfg`):

```text
MapServer_x64.exe --map path\to\map.otbm [options]
```

#### mapserver.cfg

| Setting | Description | Default |
|---------|-------------|---------|
| `MAP` | Path to the `.otbm` file to host | *(required)* |
| `PORT` | TCP port | `31313` |
| `PASSWORD` | Session password | empty |
| `NAME` | Session display name | map file name |
| `AUTOSAVE` | Auto-save interval in minutes (`0` = off) | `5` |
| `ASSETS` | Client folder with `Tibia.dat`, `Tibia.spr`, `Tibia.otfi` | search next to exe |
| `ITEMS` | Server folder with `items.otb` and `items.xml` | `<map-parent>/items` if both files exist |
| `MONSTERS` | Folder with monster XML files | `<map-parent>/monster` if it exists |
| `NPCS` | Folder with NPC XML files | `<map-parent>/npc` if it exists |
| `CENTER_X`, `CENTER_Y`, `CENTER_Z`, `RADIUS` | Limit editing to an area (all four required) | whole map |

Example `mapserver.cfg`:

```ini
MAP=C:\Servers\Ironcore\data\world\Ironcore.otbm
PORT=31313
PASSWORD=secret
NAME=Ironcore
AUTOSAVE=1
ASSETS=C:\Servers\Ironcore Client\data\things\800
ITEMS=C:\Servers\Ironcore\data\items
MONSTERS=C:\Servers\Ironcore\data\monster
NPCS=C:\Servers\Ironcore\data\npc
CENTER_X=2564
CENTER_Y=1618
CENTER_Z=7
RADIUS=350
```

#### Command-line options

| Option | Description | Default |
|--------|-------------|---------|
| `--map <file.otbm>` | Map file to host | from `mapserver.cfg` |
| `--port <number>` | TCP port | `31313` |
| `--password <pass>` | Session password | empty |
| `--name <name>` | Session display name | map name |
| `--autosave <minutes>` | Auto-save interval (`0` = off) | `5` |
| `--assets <directory>` | Client folder with `Tibia.dat`, `Tibia.spr`, `Tibia.otfi` | next to `MapServer_x64.exe`, then current directory |
| `--items <directory>` | Server folder with `items.otb` and `items.xml` | from `mapserver.cfg` or auto-detected |
| `--monsters <directory>` | Folder with monster XML files | from `mapserver.cfg` or auto-detected |
| `--npcs <directory>` | Folder with NPC XML files | from `mapserver.cfg` or auto-detected |
| `--x`, `--y`, `--z`, `--radius` | Limit editing to an area around a center tile | whole map |

**Example**

```text
.\MapServer_x64.exe --map data\world.otbm --port 31313 --password secret --name "Thais rebuild"
```

**Example with session bounds** (only tiles within 50 sqm of 1000,1000,7):

```text
.\MapServer_x64.exe --map data\world.otbm --x 1000 --y 1000 --z 7 --radius 50
```

**Example with assets folder:**

```text
.\MapServer_x64.exe --map C:\Path\To\Map.otbm --assets C:\Path\To\assets\ --port 31313 --password secret --name "LiveMap" --x 2564 --y 1618 --z 7 --radius 350 --autosave 5
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

On first connect, the editor downloads the server's client assets (this can take a minute for large `.spr` files). See [Client assets](#client-assets) for paths and security notes.

## Client assets

The editor and MapServer need Tibia client files to display items and sprites. MapServer also **sends these files to connecting editors** during live join so everyone uses the same client version as the hosted map.

### MapServer

Point MapServer at separate folders (see `mapserver.cfg`):

- **ASSETS** — `Tibia.dat`, `Tibia.spr`, `Tibia.otfi`
- **ITEMS** — `items.otb`, `items.xml`
- **MONSTERS** / **NPCS** — creature XML (used for spawn looktypes)

Connecting editors download all of the above from the host during live join.

```text
.\MapServer_x64.exe --map path\to\map.otbm --assets C:\path\to\things\800 --items C:\path\to\items [other options]
```

If you omit paths, MapServer looks next to the executable and in the current working directory. On startup it prints which folders it picked, for example: `[live] Using client assets from ...`, `[live] Using items from ...`, `[live] Using monsters from ...`.

### Editor (offline / normal use)

Configure per-client-version paths in **Preferences** (monsters, NPCs, items, and client graphics directories), or in `editor.cfg` under `ASSETS_DATA_DIRS` in the `[Version]` section.

The `data/` folder shipped with the repo is editor metadata (tilesets, brushes, etc.), not a substitute for your Tibia `.dat` / `.spr` and items files.

### Live asset cache

On first connect to a live server, the editor downloads the server's asset set and caches it on disk (not only in memory):

```text
{Editor_x64.exe folder}\data\user\data\live_cache\{client version}\
├── client\    ← Tibia.dat, Tibia.spr, Tibia.otfi
├── items\     ← items.otb, items.xml
├── monsters\  ← monster XML from the server
└── npcs\      ← NPC XML from the server
```

Later connections reuse the cache if file sizes match; the live log may show **Using cached assets from a previous live session.**

This fork uses **portable storage** (`data\user\data\` next to the executable), not `%AppData%\Remere's Map Editor`.

### Asset sharing warning

**Anyone who can connect to your live server receives a copy of the client assets you host** (including `Tibia.spr`), saved locally in the cache folder above. Treat live access like sharing your sprite archive:

- Use a **strong session password** and only give access to people you trust.
- Assume connected mappers **can keep and reuse** those files outside the editor.
- The cache is **plain, unencrypted** Tibia format — do not rely on hidden paths for protection.
- If your `.spr` is valuable, consider a **stripped asset pack** (only sprites/items your map needs), legal agreements with mappers, or accepting that full client art cannot be fully protected once sent to a client machine.

MapServer's `--assets` path controls **what you send**; it does not encrypt or restrict what recipients can do with downloaded files.

## Settings

The editor reads `editor.cfg` from the same folder as `Editor_x64.exe`. MapServer reads **`mapserver.cfg`** from the same folder as `MapServer_x64.exe`. Copy `mapserver.cfg.example` to get started. If a config file is missing, defaults are used (MapServer still requires `MAP` via config or `--map`).

Live connection defaults in the editor (under the **Network** section of `editor.cfg`):

| Setting | Default | Used for |
|---------|---------|----------|
| `LIVE_HOST` | `127.0.0.1` | Last host used in the connect dialog |
| `LIVE_PORT` | `31313` | Last port used |
| `LIVE_USERNAME` | `User` | Your display name on the server |
| `LIVE_PASSWORD` | empty | Session password |

These are updated when you connect from the editor. MapServer session options are set in `mapserver.cfg` or on the command line when you start the server.

You can edit other editor preferences in `editor.cfg` or through the editor's settings UI.

## License

MIT [LICENSE](LICENSE)