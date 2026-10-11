# Custom Snes9x EX+ features: how to use them

This build installs as a separate app, **Snes9x EX+ Custom** (`com.explusalpha.Snes9xPlus.custom`), so it can sit
next to the Play Store version. It keeps its own settings, so copy save files over if you want to keep them.


All of these work with the **on-screen touch controls**. Most new actions can also be mapped to a physical
controller under *Key/Gamepad Input Setup*, in the "In-Emulation Actions" category.

## Adding the new buttons

*Options menu ➔ On-screen Input Setup ➔ Add New Button Group* now offers:

- **Fast-forward Speeds (Hold)** / **(Toggle)**: three speed buttons, slowest to fastest. The arrows get bigger with each faster speed (default 2x, 4x, 8x).
- **Screenshot/Instant Replay/Record**: screenshot, save the last N seconds (CLIP), and start/stop recording (REC).
  In this group the buttons show only their icons. A CLIP or REC button added on its own keeps its text label.
- **Rolling Save/Load**: save a rolling state now, or load the newest one.
- **Macro Buttons 1-4**: gamepad-style buttons labeled M1–M4.
- **Fast-forward Speed Buttons**: the same speeds as large gamepad-style buttons.

Any app action (fast-forward speeds, macros, save/load, etc.) can also go into a regular gamepad button group:
*edit the group ➔ tap a button ➔ Action*, then scroll to "In-Emulation Actions".

## Simultaneous presses (combo buttons)

- *Edit a gamepad button group ➔ Add Combo Button (e.g. A + B)*: pick the first key, then the second.
- Or edit any existing button and use *Add Key To Combo* (up to 3 keys, e.g. A+B+X).
- Combo buttons are drawn as a blank button with their keys as a label (e.g. "A+B") and can also be Turbo or Toggle.

## Multiple fast-forward speeds

Set the speeds in *Options ➔ Extras ➔ Fast-forward Speeds*. Speed 1 is the original fast-forward button.
An active speed button turns red.

## Turbo buttons

Turbo buttons now get a bright orange outline and a stronger tint (*Turbo Button Style ➔ Outline*).
Switch back with *Options ➔ Extras ➔ Turbo Button Style ➔ Classic*.

## Resizing button groups and single buttons

In *On-screen Input Setup ➔ Set Button Positions*, tap (don't drag) a button group, the D-pad, or a single button.
It gets a yellow outline and a size bar appears at the bottom of the screen (at the top instead if the bar
would cover what you selected):

- **Group / Button**: tapping a button selects just that button. Tap this to switch between resizing
  the button and its whole group. It's grayed out when you tapped the D-pad or the space around the buttons.
- **-** / **+**: shrink or grow in 5% steps (groups 30–300%, single buttons 30–250%). **Reset** goes back to 100%.
- **Done**, or tapping an empty spot, closes the bar. Dragging still moves things as before.

A resized button stays centered in its spot, so an enlarged one can overlap its neighbors
(raise the group's *Spacing* if that's a problem). Sizes are saved with the layout, including per-game layouts. They apply on top of the overall *Button Size* setting.

## Exporting and importing a layout

*On-screen Input Setup ➔ Export Layout To File* saves the current on-screen layout (all groups, their buttons,
positions, sizes, and the overall button size) to a file you name, e.g. in Downloads or Google Drive.
*Import Layout From File* loads one back and replaces the current layout. If *Use Separate Layout For This Game*
is on, export and import use that game's layout.

## Per-game control layouts

With a game loaded, open *On-screen Input Setup* and turn on **Use Separate Layout For This Game**.
It starts as a copy of your current layout. From then on, button changes only affect that game.
Turning it off returns to the shared layout. *System Actions ➔ Reset Saved Options* also removes it.

## Rolling save states

Turned on by default: every 3 minutes of play, a state is auto saved, keeping the newest 5 and overwriting the oldest.
Turn the timer off with *Options ➔ Extras ➔ Auto Save On A Timer*, and change the count and interval there too.

Manual rolling saves (the save button, or *Save Rolling State Now*) are kept separately from the auto saves,
so an auto save never overwrites one you made yourself. Each set keeps its own newest 5 (or your chosen count).
Load them from *System Actions ➔ Rolling Save States* (listed as "Manual N" / "Auto N", newest first),
or with the "Load Newest Rolling State" button, which loads the newest of either kind.
Files are saved next to your normal states as `<game>.rollingNN` (manual) and `<game>.autorollingNN` (auto).

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
