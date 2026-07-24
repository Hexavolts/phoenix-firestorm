# Firestorm Codebase Guide

This is an orientation map for the repo, written for someone who knows C++ syntax but has never navigated a codebase this size. It is **not** an exhaustive file-by-file index — `indra/newview` alone has ~1,700 `.cpp`/`.h` files and 685 UI-layout XML files in the English skin alone, so a literal per-file listing would be enormous and go stale immediately. Instead this explains the *organizing structure* — the naming conventions and directory purposes — so that once you understand the pattern, you can find or place any given file yourself.

Firestorm is a fork of the open-source **Second Life viewer** (Linden Lab). That parentage explains almost every naming quirk below: most of the code was written by Linden Lab (LL) and Firestorm adds features on top of it, which is why you'll constantly see two parallel naming families.

## The core naming convention

| Prefix | Meaning |
|---|---|
| `ll*` (e.g. `llfloaterpreference.cpp`) | Original Linden Lab code, upstream from the official SL viewer. Modifying these requires wrapping your change in `// <FS:yourtag> ... // </FS>` comments per `CONTRIBUTING.md`, preserving the original LL code in a comment so future upstream merges can be reconciled. |
| `fs*` (e.g. `fskeywords.cpp`, `fsfloaterassetblacklist.cpp`) | Firestorm-original code with no upstream LL equivalent. No comment-tagging required — it's already "ours." |
| `NACL*` (2 files) | Legacy naming from the "Emerald"/"Nightcrawler" viewer lineage that Firestorm inherited some anti-spam code from. Rare; don't worry about it. |

You'll see this same split in class names, floaters, panels, and settings keys throughout the codebase.

## Top-level repository layout

| Path | Purpose |
|---|---|
| `indra/` | All source code. Everything else at the root is build tooling or docs. |
| `doc/` | Build instructions per OS (`building_windows.md` etc.) and feature test plans. |
| `scripts/` | Build-time helper scripts: `configure_firestorm.sh` (the shell script `autobuild configure` actually runs), message/template verification tools, translation tooling, packaging formatters. |
| `fsutils/` | Firestorm-specific Python helper scripts (dependency downloading, avatar LAD parsing, translation file auditing). |
| `autobuild.xml` | Declares every third-party binary package (Boost, OpenSSL, FMOD, etc.) autobuild downloads during `configure`, with version/hash/URL per platform. |
| `build.sh`, `build_target.sh`, `buildscripts_support_functions` | Entry points invoked by autobuild/CI to drive the actual compiler invocation per platform. |
| `requirements.txt` | Python packages needed to install `autobuild` itself (see `doc/building_windows.md`). |
| `CONTRIBUTING.md` | PR guidelines — most importantly the `<FS:tag>` comment convention mentioned above. |

## The `indra/` library layers

The viewer is built as a stack of static libraries, each with its own `CMakeLists.txt`, culminating in `indra/newview` (the actual application). Roughly bottom-up:

**Foundation utilities**
- `llcommon/` — the bedrock: string utilities, threading primitives, smart pointers, the logging macros (`LL_INFOS`, `LL_WARNS`, `LL_ERRS`), `LLSD` (Linden's dynamic-typed data format — you'll see this everywhere, it's how settings/config/network data gets represented), `LLSingleton` (the singleton base class used throughout, including in the auto-file feature we just built), `llregex.h` (safe regex wrappers).
- `llmath/` — vectors, matrices, quaternions, camera math, bounding boxes.
- `llxml/` — XML parsing (`llxmlnode`/`llxmltree`), and critically **`llcontrol.cpp`**, which is the entire settings/preferences engine: it's what reads `app_settings/settings.xml` into the `gSavedSettings` global object you call `.getBOOL()`/`.getString()` on everywhere.
- `llfilesystem/` — cross-platform directory/path utilities (`lldir.cpp`, the source of `gDirUtilp` and constants like `LL_PATH_PER_SL_ACCOUNT` used for per-account settings files) and the local texture disk cache.
- `llcorehttp/` — the HTTP client library used for talking to simulator capabilities and web services.
- `llmessage/` — the UDP networking protocol layer to simulators (message templates, asset transfers, avatar name cache, cache name lookups).

**Content/data types**
- `llinventory/` — shared inventory data structures (`LLInventoryType`, `LLFolderType`, item/category types) — these are the types both the viewer and simulator agree on.
- `llimage/`, `llimagej2coj/`, `llkdu/` — image codecs; JPEG2000 is SL's texture format, `llimagej2coj` is the open-source (OpenJPEG-based) codec, `llkdu` is the commercial Kakadu codec used in official (non `_open`) builds.
- `llcharacter/`, `llappearance/` — avatar skeleton, animation (BVH loading, motion states), and appearance/wearables system.
- `llprimitive/` — in-world object/primitive data (materials, GLTF material support, legacy object types).
- `llmeshoptimizer/` — mesh LOD generation.

**Rendering & platform**
- `llrender/` — OpenGL abstraction: shaders, fonts (FreeType), cubemaps, render pipeline primitives.
- `llwindow/` — platform windowing/input abstraction (Win32, macOS, SDL, headless variants) — keyboard, cursor, DX hardware detection.
- `llaudio/` — sound engine abstraction over FMOD Studio or OpenAL.
- `llwebrtc/` — voice chat transport.
- `llplugin/`, `media_plugins/` — the out-of-process plugin architecture for embedded web media (CEF/Chromium Embedded Framework, GStreamer, libvlc) — this is how "media on a prim" and the built-in web browser work.
- `llphysicsextensionsos/` — stub/open-source physics shape extension (the commercial Havok physics equivalent isn't in this open build).

**UI toolkit**
- `llui/` — the base UI widget toolkit: buttons, scroll lists, accordions, the base `LLFloater`/`LLPanel` classes everything else derives from. This is *generic* UI plumbing, not any specific window.

**Everything else**
- `viewer_components/` — small standalone components (e.g. `login/` — login screen logic — kept separate from the giant `newview` tree).
- `llcrashlogger/`, `win_crash_logger/`, `mac_crash_logger/`, `linux_crash_logger/` — the separate crash-reporting helper executables.
- `test/`, `integration_tests/` — test harnesses.
- `tools/` — packaging manifests (what files ship in an installer).
- `Version` — the file that stores the current version number string.

## `indra/newview/` — the viewer application itself

This is where nearly all feature work happens (it's where we added the inventory auto-file feature). Structure:

**Flat `.cpp`/`.h` files (the bulk of it, ~1,700 files)** — organized by C++ class, not by feature folder. Some load-bearing entry points to know by name:
- `llappviewer.cpp` — application startup/shutdown lifecycle, the actual `main()`-adjacent driver.
- `llstartup.cpp` — the login/world-connection state machine.
- `llagent.cpp` — the logged-in user's avatar/camera/movement state.
- `llviewerwindow.cpp` — the main application window and top-level UI container.
- `llviewercontrol.cpp/h` — declares `gSavedSettings` / `gSavedPerAccountSettings` (the globals backing `app_settings/settings.xml` and `settings_per_account.xml`).
- `llviewermessage.cpp` / `llimprocessing.cpp` — inbound message handling from the simulator: chat, IMs, inventory offers, group notices. This is where we hooked in the auto-file regex matching.
- `llinventorymodel.cpp/h` — the client-side inventory tree (folders/items), lookup and mutation APIs (`findCategoryByName`, `createNewCategory`, `changeItemParent`).

Within those flat files, two naming families matter for finding UI code:
- **Floaters** (`llfloater*.cpp` / `fsfloater*.cpp`, 177 files) — a "floater" is SL/Firestorm's term for a separate window/dialog (Preferences, Inventory, a chat window, etc.). Each is a C++ class controlling one XUI layout file.
- **Panels** (`llpanel*.cpp` / `fspanel*.cpp`, 93 files) — a reusable embedded UI section that lives inside a floater (e.g. each Preferences tab is a panel embedded in the one Preferences floater).

**Subdirectories:**

| Directory | Purpose |
|---|---|
| `app_settings/` | Non-code configuration data: `settings.xml` (every global preference — type, default, persistence flag, tooltip comment), `settings_per_account.xml` (same, but scoped per logged-in account — inventory/account-specific things like our new `FSInventoryAutoFileEnabled` live here), plus `keywords.ini`, `key_bindings.xml`, `foldertypes.xml`, `grids.xml` (known OpenSim grids), `autoreplace.xml` (default Auto-Replace word lists), and similar data-only files. |
| `skins/default/` | The XUI presentation layer — see below. |
| `character/` | Avatar skeleton/animation definition files (not code). |
| `fonts/`, `icons/`, `res/`, `res-sdl/`, `vmp_icons/` | Binary/visual assets. |
| `gltf/` | GLTF material/PBR support code and assets. |
| `fs_resources/` | Firestorm-specific bundled resources. |
| `installers/` | Per-platform installer scripting (NSIS for Windows, etc.). |
| `tests/` | Unit tests for newview code. |
| `*.lproj/` (English, German, French, ...) | macOS-specific localized resource bundles (Info.plist strings etc.) — not related to the XUI translation files below, this is a Mac packaging convention. |

### The XUI system (`skins/default/`)

This is the *declarative UI layer* — floaters and panels are laid out in XML, not hardcoded in C++. Understanding this is essential for any UI change:

- `skins/default/xui/en/` (685 files) — every floater and panel layout as XML, in US English. File naming mirrors the C++ class: `floater_preferences.xml` pairs with `LLFloaterPreference` (`llfloaterpreference.cpp`), `panel_preferences_privacy.xml` is the Privacy tab panel we added the auto-file checkbox to.
- `skins/default/xui/<lang>/` (de, fr, ja, ru, zh, pt, ...) — translated copies of the same XML files, for localization. When you add a new user-facing string, it only needs adding to the `en/` version; translators handle the rest separately.
- `skins/default/colors.xml` — the named color palette UI elements reference.
- `skins/default/textures/` — UI image assets (icons, button skins).
- A control in an XUI file connects to C++ two ways: `control_name="SomeKey"` binds directly to a `gSavedSettings`/`gSavedPerAccountSettings` value with zero C++ code needed (this is how our new checkbox works), while `commit_callback.function="Some.Name"` binds a button/control to a C++ callback registered via `mCommitCallbackRegistrar.add(...)` (this is how our "Edit rules..." button opens the new floater).

### The settings system, concretely

1. A key is declared in `app_settings/settings.xml` (global) or `settings_per_account.xml` (per logged-in account) — this declares its type, default value, and whether it persists across sessions.
2. C++ reads/writes it via the globals `gSavedSettings` / `gSavedPerAccountSettings` (both `LLControlGroup` instances, from `llxml/llcontrol.cpp`), e.g. `gSavedPerAccountSettings.getBOOL("FSInventoryAutoFileEnabled")`.
3. An XUI checkbox/control can bind straight to a key via `control_name="KeyName"` with no C++ glue at all.

### Floaters, concretely

1. Define the class (`FSFloaterWhatever : public LLFloater`) in a `.h`/`.cpp` pair in `indra/newview/`.
2. Write its layout as `skins/default/xui/en/floater_whatever.xml`.
3. Register the pairing in `llviewerfloaterreg.cpp`: `LLFloaterReg::add("internal_name", "floater_whatever.xml", (LLFloaterBuildFunc)&LLFloaterReg::build<FSFloaterWhatever>);`.
4. Open it from anywhere with `LLFloaterReg::showInstance("internal_name")`.
5. Add the new `.cpp`/`.h` files to `indra/newview/CMakeLists.txt` (both the source list and header list) — **this step is the single most common reason a new file silently doesn't compile in**; CMake won't discover it until this is done and `autobuild configure` (or VS's CMake reconfigure) is rerun.

## Worked example: the inventory auto-file feature

Since we just built a feature end-to-end, here's how it maps onto everything above — a concrete tour of the pattern:

| What | Where | Why there |
|---|---|---|
| Settings key `FSInventoryAutoFileEnabled` | `app_settings/settings_per_account.xml` | Per-account, since inventory folders are account-specific |
| Rule-list model singleton `FSInventoryAutoFile` | `fsinventoryautofile.h/.cpp` (new `fs*` file) | Firestorm-original code, no upstream equivalent, hence `fs` prefix and no `<FS:tag>` comments needed |
| Settings floater `FSFloaterInventoryAutoFile` | `fsfloaterinventoryautofile.h/.cpp` + `skins/default/xui/en/floater_fs_inventory_autofile.xml` | Class + matching XUI layout pair, same as any other floater |
| Floater registration | `llviewerfloaterreg.cpp` | Where every floater's internal name ↔ XML ↔ C++ class triple is wired up |
| Checkbox + "Edit rules..." button | `skins/default/xui/en/panel_preferences_privacy.xml` | The Privacy preferences tab, since it already held related inventory-offer settings |
| Button → floater wiring | `llfloaterpreference.h/.cpp` | Where the Preferences floater registers its buttons' `commit_callback.function` names to C++ methods |
| Actual regex hook into inbound offers | `llimprocessing.cpp` (existing `ll*` file) | This is upstream LL code, so our insertions are wrapped in `<FS:tag>` comments per `CONTRIBUTING.md` |
| New files registered for compilation | `CMakeLists.txt` | Without this, none of the above compiles in at all |

## Where to start for common changes

| I want to... | Look at... |
|---|---|
| Add a new preference (checkbox/slider/etc.) | Add a key to `app_settings/settings.xml` or `settings_per_account.xml`, then a matching control in the relevant `skins/default/xui/en/panel_preferences_*.xml` |
| Add a whole new window | Follow the "Floaters, concretely" steps above; `llfloaterautoreplacesettings.*` + `floater_autoreplace.xml` is a small, clean example to copy from |
| React to a chat message / IM / inventory offer | `llviewermessage.cpp`, `llimprocessing.cpp` |
| Change how inventory folders/items are organized | `llinventorymodel.cpp/h` (in `indra/llinventory` for shared types, `indra/newview` for the live client-side tree) |
| Change avatar rendering/animation | `llcharacter/`, `llappearance/`, and `llvoavatar*.cpp` in `newview` |
| Change a keybinding | `app_settings/key_bindings.xml` |
| Add/modify a translatable string | The `en/` XUI file first; translation files under other `xui/<lang>/` folders are a separate localization workflow |
