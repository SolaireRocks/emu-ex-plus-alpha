# Custom Snes9x EX+ build (fork of Rakashazi/emu-ex-plus-alpha)

Work happens on branch `custom-features`; only the Snes9x EX+ app (`Snes9x/`) is built.

## Building

There is no local toolchain on the Windows dev machine. Builds run only in GitHub Actions
(`.github/workflows/build.yml`), triggered on push or via "Run workflow":

- arm64-only by default; tick `all_abis` in a manual run for all 4 ABIs.
- CMake is pinned to `~4.3.0`. `imagine/cmake/config.cmake` sets an experimental
  `CMAKE_EXPERIMENTAL_CXX_IMPORT_STD` UUID that only matches CMake 4.3.x, so newer CMake fails at configure.
- The third-party bundle (`imagine/bundle`, includes libc++) is cached by hash; the first run takes much longer.
- Job logs need sign-in, so a failure step posts compiler errors as **annotations**, which the public API can read:
  `curl https://api.github.com/repos/SolaireRocks/emu-ex-plus-alpha/check-runs/<job id>/annotations`
- APK artifact: `Snes9xEXPlus-<sha>`. Pushes to `custom-features` or `master` also update the
  `snes9x-custom-latest` pre-release. Signing uses the `DEBUG_KEYSTORE` secret (base64 debug keystore).

## Layout

- `imagine/` – platform library (gfx, input, Android glue, Java in `src/base/android/imagine-v9`).
- `EmuFramework/` – shared emulator app framework. Nearly all custom features live here.
- `Snes9x/src/main/` – SNES-specific glue (`AppMeta.cc` key/button definitions, `input.cc`).
- C++23 with modules: `.cc` files `#include` framework headers, then `import imagine;`.
  `import std` comes from `defs.hh`. Only include C/POSIX headers directly.

## On-screen controls

- `EmuFramework/src/vcontrols/VController.cc` – touch handling, drawing, config read/write, button labels,
  double-tap screenshot, per-game layouts.
- `VControllerButtonGroup.cc` / `VControllerButton.cc` – button layout, sprite colors, turbo outline ("halo") quads.
- Button labels (`VController::buttonLabel`/`drawLabels`) use the same dim gray as the letters in the button art.
  UI-group fast-forward buttons have no number; `setFastForwardImageScales` sizes the arrow by speed (`VControllerButton::imageScale`).
- `EmuFramework/src/gui/TouchConfigView.cc` – "On-screen Input Setup" menu.
- `KeyInfo` (inputDefs.hh) holds up to 3 key codes plus flags (appCode, turbo, toggle, deviceId).
  Combo buttons are KeyInfos with several system codes. App actions (`AppKeyCode`) are allowed in gamepad groups.

## Custom features (where to look)

| Feature | Files |
|---|---|
| Multiple fast-forward speeds | `AppKeyCode.hh` (fastForwardSpeed2/3 + toggles), `AltSpeedMode::fast2/fast3` in `defs.hh`, `EmuInputView.cc`, `EmuApp.hh` (`fastModeSpeed2/3`) |
| Combo buttons (A+B) | `InputManager::categoryOfKeyCode` (validation), TouchConfigView "Add Combo Button" / "Add Key To Combo" |
| Distinct turbo buttons | `VControllerTurboStyle`, `VControllerButtonGroup::writeHaloSprites`, `VControllerButton::setAlpha` (Outline/Classic option) |
| Macros | `MacroManager.{hh,cc}`, `gui/MacroView.cc`, frame hook in `EmuSystemTask::advanceFrames` |
| Rolling save states | `RollingSaveManager.{hh,cc}` (separate manual/auto file sets via `RollingSaveKind`), `gui/RollingSaveView.cc`, timer start/pause in `EmuSystem.cc` |
| Per-game touch layouts | `VController::{enable,restoreGlobal,applyPending}GameLayout`, session config keys 203/204 |
| Element/button sizes | `VControllerElement::sizePercent`, `VControllerButton::sizePercent`, size bar in `gui/PlaceVControlsView.cc`. Saved after each element when the element list's configId is 1 (`sizedElementsConfigId`); configId 0 is upstream data without sizes. Groups grow their bounds by `overflowPx` for enlarged buttons |
| Add Single Button | TouchConfigView `addSingleButton` item + `addSingleButtonElement` (one-key `InputComponentDesc`: `button` type for gamepad style, `ui` type for icons, icons only for app actions). `buttonLabel`'s `aloneInUIGroup` keeps text on lone fast-forward speed 2/3 icons (rolling save/load icons never have text) |
| Layout export/import | `VController::{export,import}Layout` (config-format file with marker key 216), TouchConfigView items + `onDocumentPicked`, `ApplicationContext::showSystemCreateDocumentPicker(suggestedName)` (Java `createDocument(long, String)`, kept in `imagine/src/base/android/proguard/imagine.cfg`) |
| Double-tap screenshot | `VController::checkDoubleTap` |
| High-res screenshots | `EmuVideo::doScreenshot` (nearest-neighbor scale, optional 4:3) |
| Video recording and instant replay | `MediaRecorder.{hh,cc}` (NDK MediaCodec/MediaMuxer via dlopen), hooks in `EmuVideo::finishFrame`, `EmuAudio::writeFrames`, `EmuSystemTask::advanceFrames` |
| Options UI | `gui/ExtrasOptionView.cc` (Options ➔ Extras), System Actions entries |

Index buffer updates in `imagine/src/gfx/opengl/Buffer.cc` unbind the current VAO first (`bindForUpdate`); otherwise
binding GL_ELEMENT_ARRAY_BUFFER swaps the index buffer of whatever VAO was last bound (this broke the size bar drawing
in PlaceVControlsView, whose uint16 `gridIdxs` got attached to the size bar's VAO).

Custom config keys are numbered 200 and up (`EmuOptions.hh`) so they don't collide with upstream.
New `AppKeyCode` values must stay appended at the end, because codes are saved in config files.
