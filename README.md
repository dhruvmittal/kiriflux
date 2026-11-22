# Kiriflux

A desktop-first RSS reader for [Miniflux](https://miniflux.app/), built with KDE Kirigami and Qt 6.

![Kiriflux three-pane interface](screenshots/kiriflux-overview.png)

## Features

### Three-Pane Interface
A familiar, productivity-focused layout with a sidebar for feeds and categories, a central article list, and a full article reading pane powered by QtWebEngine.

### Miniflux Backend Integration
- **Two-way sync** — read status and starred articles stay in sync with your Miniflux server.
- **Full content fetching** — download the original article text directly from the source via the Miniflux API.
- **Automatic background sync** with a configurable interval.

### Smart Filtering
- Filter articles by **Unread**, **Starred**, or **All**.
- Filters are session-persistent — articles don't vanish the moment you read them. The list only re-filters on explicit sync or context change.
- Filter preference is saved between app launches.

### Keyboard Shortcuts
| Key     | Action                          |
| ------- | ------------------------------- |
| `Space` | Scroll article view down        |
| `j`     | Next article                    |
| `k`     | Previous article                |
| `s`     | Toggle star on active article   |
| `d`     | Download full original text     |

### Additional Features
- Configurable title line count (1–3 lines) for the article list.
- Per-feed and per-category article views with dynamic titles.
- Mark all as read per feed/category.
- Smooth column resizing with no text clipping.

---

## Building

### Prerequisites

Kiriflux requires:
- **Qt 6** (Core, Gui, Quick, QuickControls2, Widgets, Sql, WebEngineQuick)
- **KDE Frameworks 6** (Kirigami, KConfig, KI18n, KCoreAddons, KDBusAddons)
- **CMake** ≥ 3.20 and **Ninja** (recommended)
- **Extra CMake Modules (ECM)**

---

### Option 1: Nix (Recommended)

The project includes a `flake.nix` with all dependencies pinned.

```bash
# Run directly
nix run github:gnuslashdhruv/kiriflux

# Or enter a development shell
nix develop
cmake -B build -G Ninja
cmake --build build
./build/src/kiriflux
```

### Option 2: Flatpak (via Nix)

A Flatpak bundle can be generated from the flake. This produces a self-contained `.flatpak` file targeting the **KDE Platform 6.10** runtime.

```bash
nix build .#flatpak
flatpak install --user result/org.kde.kiriflux.flatpak
flatpak run org.kde.kiriflux
```

### Option 3: Arch Linux

Install the dependencies using `pacman`.

```bash
sudo pacman -S qt6-base qt6-declarative qt6-webengine qt6-svg \
  kirigami ki18n kconfig kcoreaddons kdbusaddons \
  qqc2-desktop-style extra-cmake-modules cmake ninja
```

Then build:
```bash
cmake -B build -G Ninja
cmake --build build
sudo cmake --install build
```

---

## Configuration

On first launch, open **Settings** and provide:
1. Your Miniflux instance URL (e.g. `https://miniflux.example.com`).
2. Your API key (generated from Miniflux → Settings → API Keys).

Settings are stored via KConfig in `~/.config/kirifluxrc`.

---

## Running Tests

```bash
cmake -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build --target SyncTest
./build/tests/SyncTest
```

> **Note:** Tests require a live Miniflux instance with valid API credentials configured in `kirifluxrc`.

---

## Technology Stack

| Component    | Technology                     |
| ------------ | ------------------------------ |
| UI Framework | KDE Kirigami / Qt Quick        |
| Backend      | C++ 20, Qt 6                   |
| Article View | QtWebEngine                    |
| Database     | SQLite (via Qt SQL)            |
| Build System | CMake + Ninja                  |
| Packaging    | Nix Flakes, nix2flatpak        |
| API          | Miniflux REST API v1           |

## License

GPL-3.0-or-later