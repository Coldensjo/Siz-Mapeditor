# Full dark mode design

## Goal

Provide a complete Windows dark-mode experience for the Editor application. New
installations start in Dark mode. Users select Dark, Light, or System in
Preferences, and applying the change rethemes the running editor immediately.

Map and sprite rendering are not part of the UI theme and must not change.
MapServer is out of scope.

## Chosen approach

Use a centralized `ThemeManager` rather than relying solely on wxWidgets'
appearance API or rebuilding the main frame. The manager combines:

1. wxWidgets' Windows appearance support for native controls and the title bar.
2. A semantic editor palette for AUI dock chrome and project-owned paint code.
3. Recursive restyling of the main frame's existing descendants, followed by
   AUI update, layout, and repaint.

This preserves open maps, pane placement, and all transient editor state while
making the setting take effect immediately.

## Settings and startup

- Add an integer UI setting representing `Dark`, `Light`, or `System`.
- Its default is `Dark`; existing settings files without the key receive the
  same default.
- Apply the saved appearance before constructing the `MainFrame` so all newly
  created controls inherit the selected mode.
- On Preferences Apply, attempt the appearance change first. Persist the new
  setting only after it succeeds. If Windows or wxWidgets cannot make the
  transition, leave the current theme and setting intact and report the
  failure.

## Theme application

`ThemeManager` owns the active mode and semantic colors for window backgrounds,
surfaces, controls, borders, normal and muted text, selection, hover, and
disabled state. It exposes palette accessors rather than leaking fixed RGB
values.

On a successful runtime change it:

1. Requests the selected wxWidgets appearance and configures the Windows title
   bar.
2. Applies native dark/light treatment and semantic foreground/background
   colors to the main frame and each existing descendant.
3. Updates AUI dock art, AUI toolbars, and AUI notebook art, then calls the
   manager update/layout/refresh sequence.
4. Refreshes menus, popup menus, the status bar, and active windows so the
   change is visible without reopening them.

Every newly created editor dialog and child control is initialized from the
same manager, so it matches the current mode.

## UI coverage

The theme covers the title bar and frame; menu and popup menus; status bar;
toolbars; AUI panes, captions, and tabs; palette panels; welcome dialog;
preferences; existing dialogs; and later-opened dialogs. The implementation
will replace hard-coded black, white, blue, and gray colors in editor-owned
paint paths with palette colors while retaining semantic colors such as error
red and selection highlights.

The map viewport, minimap data, sprites, brush overlays, and game rendering
retain their current behavior and colors.

## Preferences UI

Add a `Theme:` choice to the existing Interface page with the values `Dark`,
`Light`, and `System`. It reflects the persisted setting when opened. Apply and
OK use the same apply path; Cancel makes no appearance or settings change.

## Verification

Before implementation, add test coverage for mode parsing/defaulting,
persistence, and palette selection. Test each test first in its failing state.

After implementation, manually verify on Windows that a running Editor can
switch Dark -> Light -> System while a map, palette, Preferences window, popup
menu, and representative dialog are open. Confirm that controls remain usable,
open maps and AUI layout are unchanged, and the map viewport is unchanged.

Finish with an x64 Release build of `vcproj/Editor.sln`; confirm the expected
executables are emitted in the repository root. Increment
`__RME_SUBVERSION__` because this changes Editor behavior.
