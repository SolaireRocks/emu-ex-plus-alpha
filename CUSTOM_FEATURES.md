# Custom Snes9x EX+ features: how to use them

This build installs as a separate app, **Snes9x EX+ Custom** (`com.explusalpha.Snes9xPlus.custom`), so it can sit
next to the Play Store version. It keeps its own settings, so copy save files over if you want to keep them.


All of these work with the **on-screen touch controls**. Most new actions can also be mapped to a physical
controller under *Key/Gamepad Input Setup*, in the "In-Emulation Actions" category.

## Adding the new buttons

*Options menu ➔ On-screen Input Setup ➔ Add New Button Group* now offers:

- **Fast-forward Speeds (Hold)** / **(Toggle)**: three speed buttons labeled with their speeds (default 2x, 4x, 8x).
- **Screenshot/Instant Replay/Record**: screenshot, save the last N seconds (CLIP), and start/stop recording (REC).
- **Rolling Save/Load**: save a rolling state now, or load the newest one.
- **Macro Buttons 1-4**: gamepad-style buttons labeled M1–M4.
- **Fast-forward Speed Buttons**: the same speeds as large gamepad-style buttons.

Any app action (fast-forward speeds, macros, save/load, etc.) can also go into a regular gamepad button group:
*edit the group ➔ tap a button ➔ Action*, then scroll to "In-Emulation Actions".

## Simultaneous presses (combo buttons)

- *Edit a gamepad button group ➔ Add Combo Button (e.g. A + B)*: pick the first key, then the second.
- Or edit any existing button and use *Add Key To Combo* (up to 3 keys, e.g. A+B+X).
- Combo buttons show their keys as a label (e.g. "A+B") and can also be Turbo or Toggle.

## Multiple fast-forward speeds

Set the speeds in *Options ➔ Extras ➔ Fast-forward Speeds*. Speed 1 is the original fast-forward button.
An active speed button turns red.

## Turbo buttons

Turbo buttons now get a bright orange outline, a stronger tint, and a small "T" badge.
Switch back with *Options ➔ Extras ➔ Turbo Button Style ➔ Classic*.

## Per-game control layouts

With a game loaded, open *On-screen Input Setup* and turn on **Use Separate Layout For This Game**.
It starts as a copy of your current layout. From then on, button changes only affect that game.
Turning it off returns to the shared layout. *System Actions ➔ Reset Saved Options* also removes it.

## Rolling save states

Turned on by default: every 3 minutes of play, a state is saved, keeping the newest 5 and overwriting the oldest.
Change the count and interval in *Options ➔ Extras*. Load them from *System Actions ➔ Rolling Save States*,
or with the "Load Newest Rolling State" button. Files are saved next to your normal states as `<game>.rollingNN`.

## Screenshots

- **Double-tap the game image** to take a screenshot. Turn this off in *Options ➔ Extras* or On-screen Input Setup.
- **Higher resolution**: *Options ➔ Extras ➔ Screenshot Resolution* (2x–8x, sharp pixel scaling),
  plus an optional 4:3 stretch.

## Video recording (game screen only)

Only the emulated picture and sound are recorded, without on-screen controls or menus.
Start or stop with the REC button, or from *System Actions ➔ Start Video Recording*.
Videos are saved as MP4 in the screenshots folder, under `recordings`.
Resolution is set in *Options ➔ Extras ➔ Video Resolution* (default 2x).

## Instant replay

The CLIP button saves the last 30 seconds of gameplay as an MP4. The length is set in *Options ➔ Extras*.
By default, the replay buffer only runs while a CLIP button is in your on-screen layout,
so it doesn't use battery otherwise. Set *Instant Replay Buffer ➔ Always* to keep it running
(for example, if you trigger it from a physical controller).

## Macros

*System Actions ➔ Macros* has 8 macro slots. For each one you can:

- **Record From Gameplay**: go back to the game, play the inputs, then open the menu
  (or press a "Start/Stop Macro Recording" button) to finish.
- **Edit steps by hand**: each step chooses which buttons are held and for how many frames (60 frames = 1 second).
- **Repeat While Active**: the macro loops until you tap its button again.

Play a macro with its M1–M8 button.
