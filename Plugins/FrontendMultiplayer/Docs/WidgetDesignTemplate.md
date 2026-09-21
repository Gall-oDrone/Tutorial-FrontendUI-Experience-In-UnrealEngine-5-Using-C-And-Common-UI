# FrontendMultiplayer — Widget design template

Authoring spec for the seven multiplayer Blueprints. Every convention here was taken
from the existing FrontendUI screens (`WBP_CAW_MainMenu`, `WBP_CAW_StoryScreen`,
`WBP_CAW_OptionScreen`, `WBP_DetailsView_Options`, `WBP_ListEntry_InvalidRow`,
`WBP_Template_Layout`) rather than from general CommonUI convention.

## Two corrections to note first

**The `Border` → `Overlay` → `Background Blur` wrapper is not universal.**
`WBP_CAW_MainMenu` and `WBP_CAW_StoryScreen` use `WBP_Template_Layout` as their direct
root with no wrapper at all. Only `WBP_CAW_OptionScreen` has it. The rule is that the
blur wrapper belongs on reading-dense screens where the level behind should be
de-emphasized, and stays off button menus where the level should remain visible.

**Blueprint widget names and C++ bind names follow different conventions.**
Main Menu names its buttons `Button_Story`, `Button_Options`, `Button_Credit`,
`Button_Quit`, because it has no C++ parent constraining them. The type-prefixed
`CommonButton_*` / `CommonTextBlock_*` scheme applies to `BindWidget` properties
specifically — which is where `WBP_DetailsView_Options` shows `CommonTextBlock_Title`,
`CommonRichText_Description`, and the rest.

## Naming

| Scope | Rule | Example |
|---|---|---|
| Widget bound to a C++ `BindWidget` | Exact match required, type-prefixed | `CommonButton_Create` |
| Widget in a Blueprint-only screen | Free, use `Button_*` / `Text_*` | `Button_Host` |
| Screen asset | `WBP_CAW_<Name>` | `WBP_CAW_HostSessionScreen` |
| List row asset | `WBP_ListEntry_<Name>` | `WBP_ListEntry_Session` |
| Reusable non-screen module | `WBP_<Kind>_<Name>` | `WBP_Text_EmptyState` |

Widgets marked `[BIND]` below are `BindWidget` properties, not `BindWidgetOptional`.
A missing or misnamed one is a Blueprint compile error, so those names are exact.

## Reuse inventory

Reuse unchanged: `WBP_Template_Layout`, `Common Bound Action Bar`,
`WBP_Button_BoundAction`, `WBP_Text_ButtonDescription`, `WBP_Button_Default`,
`SizeBox_ListEntry`, `Style_Text_Default`, `Style_Text_ListEntry_Default`,
`Style_Text_OptionsDetailsView_Title`, `Style_Button_Clear`, `Style_Button_Clear_Menu`.

Create two new reusable modules, both parented to `User Widget` the way
`WBP_Text_ButtonDescription` is:

- **`WBP_Text_EmptyState`** — `Overlay` → `CommonTextBlock_EmptyStateMessage`, with the
  message exposed as a variable. Used by the server browser and the leaderboard.
- **`WBP_Form_LabeledField`** — `Vertical Box` → `CommonTextBlock_FieldLabel` +
  `NamedSlot_FieldInputExtendPoint`. Host Session uses it twice, and it keeps label and
  input spacing consistent as more forms appear.

Bind targets must live in the screen itself, so put the real input widget into the named
slot from the screen rather than inside the module.

## 1. `WBP_CAW_MultiplayerScreen`

Parent class `Widget Activatable Base`. There is deliberately no C++ class — the hub only
shows buttons and pushes screens, which is exactly what Main Menu does with no native code.

```
[WBP_CAW_MultiplayerScreen]
└─ WBP_Template_Layout
   ├─ NamedSlot_BoundActionExtendPoint
   │  └─ Common Bound Action Bar
   ├─ NamedSlot_DescriptionExtendPoint
   │  └─ WBP_Text_ButtonDescription
   └─ NamedSlot_LeftButtonsExtendPoint
      └─ Grid Panel
         ├─ Button_Host          (WBP_Button_Default)
         ├─ Button_ServerBrowser (WBP_Button_Default)
         ├─ Button_Leaderboard   (WBP_Button_Default)
         └─ Button_APIDebug      (WBP_Button_Default)
```

- Class defaults: tick **Is Back Handler**.
- Per button, set `Button Display Text` and `Button Description Text`. The latter is what
  populates the description slot on hover.
- Bind `Button_APIDebug` Visibility to `ShouldShowAPIDebugPanel`.
- Implement `BP Get Desired Focus Target` returning `Button_Host`.
- Each click calls `Push Soft Widget` with stack tag `Frontend.WidgetStack.Frontend` and
  the matching `Frontend.Widget.*` tag.

## 2. `WBP_CAW_HostSessionScreen`

Parent class `Widget_HostSessionScreen`. A form, so it takes the blur wrapper.

```
[WBP_CAW_HostSessionScreen]
└─ Border
   └─ Overlay
      ├─ Background Blur
      └─ WBP_Template_Layout
         ├─ NamedSlot_BoundActionExtendPoint
         │  └─ Common Bound Action Bar
         ├─ NamedSlot_DescriptionExtendPoint
         │  └─ WBP_Text_ButtonDescription
         └─ NamedSlot_CenterExtendPoint
            └─ Vertical Box
               ├─ CommonTextBlock_ScreenTitle      (Style_Text_OptionsDetailsView_Title)
               ├─ WBP_Form_LabeledField            label "Session Name"
               │  └─ NamedSlot_FieldInputExtendPoint
               │     └─ EditableTextBox_SessionName   [BIND]
               ├─ WBP_Form_LabeledField            label "Max Players"
               │  └─ NamedSlot_FieldInputExtendPoint
               │     └─ SpinBox_MaxPlayers            [BIND]
               └─ CommonButton_Create                 [BIND] (WBP_Button_Default)
```

- `SpinBox_MaxPlayers`: Min 2, Max 8, Delta 1, with the slider range matching.
- `BP Get Desired Focus Target` → `EditableTextBox_SessionName`.

## 3. `WBP_CAW_ServerBrowserScreen`

Parent class `Widget_ServerBrowserScreen`. A list screen, so it follows Options most
closely, including putting the list view in `NamedSlot_MainLeftExtendPoint`.

```
[WBP_CAW_ServerBrowserScreen]
└─ Border
   └─ Overlay
      ├─ Background Blur
      └─ WBP_Template_Layout
         ├─ NamedSlot_BoundActionExtendPoint
         │  └─ Common Bound Action Bar
         └─ NamedSlot_MainLeftExtendPoint
            └─ Overlay
               ├─ CommonListView_Sessions          [BIND] (Common List View)
               └─ WBP_Text_EmptyState              "No sessions found"
```

- On `CommonListView_Sessions`: **Entry Widget Class** `WBP_ListEntry_Session`,
  **Num Designer Preview Entries** 5 (what Options uses, and how you see rows at design time).
- Bind the empty-state Visibility to the list's item count being zero.
- `BP Get Desired Focus Target` → `CommonListView_Sessions`.
- There is no Refresh button. Refresh is a bound action; set the **Refresh Action** handle
  under "Frontend Server Browser Screen" in class defaults once the data table row exists.

Note the list view is a plain `Common List View`, not `WBP`-wrapped and not
`UFrontendCommonListView`. The Frontend one resolves rows through a
`DataAsset_DataListEntryMapping` and casts every item to `UListDataObject_Base`, which
would assert on session rows.

## 4. `WBP_ListEntry_Session`

Parent class `Widget_ListEntry_Session`. Root is the shared size box, the same component
`WBP_ListEntry_InvalidRow` uses.

```
[WBP_ListEntry_Session]
└─ SizeBox_ListEntry
   └─ Horizontal Box
      ├─ CommonText_SessionInfo   [BIND] — Fill, left aligned, Style_Text_ListEntry_Default
      └─ CommonButton_Join        [BIND] — Auto, right aligned, Style_Button_Clear
```

This row intentionally does not inherit `Widget_ListEntry_Base`, so it has no
hover-highlight or gamepad-focus hooks. If CommonUI's baseline selection feedback turns
out not to be enough, the fix is to extract a smaller generic base out of
`Widget_ListEntry_Base` inside FrontendUI — splitting the hover/gamepad behaviour from the
`UListDataObject_Base` specifics — rather than reaching back into the options types here.

## 5. `WBP_CAW_LeaderboardScreen`

Parent class `Widget_LeaderboardScreen`. Reading-dense, so the blur wrapper is on. This is
the server browser's structural twin — same list view in `NamedSlot_MainLeftExtendPoint`,
same empty state, same Refresh-as-bound-action. Build it by duplicating the server browser
and replacing the list view and its entry class.

```
[WBP_CAW_LeaderboardScreen]
└─ Border
   └─ Overlay
      ├─ Background Blur
      └─ WBP_Template_Layout
         ├─ NamedSlot_BoundActionExtendPoint
         │  └─ Common Bound Action Bar
         └─ NamedSlot_MainLeftExtendPoint
            └─ Vertical Box
               ├─ CommonTextBlock_ScreenTitle   (Style_Text_OptionsDetailsView_Title)
               ├─ Horizontal Box                — column headers, Style_Text_Default
               │  ├─ CommonTextBlock_HeaderRank    "#"      — Fixed, matches row width
               │  ├─ CommonTextBlock_HeaderPlayer  "Player" — Fill
               │  └─ CommonTextBlock_HeaderScore   "Score"  — Fixed, matches row width
               └─ Overlay
                  ├─ CommonListView_Leaderboard [BIND] (Common List View)
                  └─ WBP_Text_EmptyState        "No scores yet"
```

- On `CommonListView_Leaderboard`: **Entry Widget Class** `WBP_ListEntry_Leaderboard`,
  **Num Designer Preview Entries** 5.
- The header row is plain Blueprint decoration, not bound to C++. Its three cells must use
  the same widths and padding as the row below or the columns will not line up; that is the
  one thing to get right here.
- Bind the empty-state Visibility to the list's item count being zero.
- `BP Get Desired Focus Target` → `CommonListView_Leaderboard`.
- Refresh is a bound action, not a button. Set the **Refresh Action** handle under
  "Frontend Leaderboard Screen" in class defaults to the same `RefreshAction` row the
  server browser uses.
- Same note as the server browser: use a plain `Common List View`, not
  `UFrontendCommonListView`, which would assert on leaderboard rows.

## 6. `WBP_ListEntry_Leaderboard`

Parent class `Widget_ListEntry_Leaderboard`. Root is the shared size box, as with
`WBP_ListEntry_Session`.

```
[WBP_ListEntry_Leaderboard]
└─ SizeBox_ListEntry
   └─ Horizontal Box
      ├─ CommonText_Rank        [BIND] — Fixed, right aligned, Style_Text_ListEntry_Default
      ├─ CommonText_PlayerName  [BIND] — Fill, left aligned,  Style_Text_ListEntry_Default
      └─ CommonText_Score       [BIND] — Fixed, right aligned, Style_Text_ListEntry_Default
```

Rank and score are right aligned so the digits stack; the name fills the space between
them. Widths must match the header row in the screen above.

The C++ parent fires `BP On Local Player State Changed` after populating the text, once per
row. Implement it to highlight the viewer's own row — recolouring the three text blocks is
enough. It is an event rather than a hard-coded colour in C++ specifically so the highlight
is authored alongside the rest of the styling. Handle the `false` branch too: rows are
recycled as the list scrolls, so a row that is not reset will keep the highlight from
whichever entry it displayed last.

There is no Join button here, so unlike the session row this one has no interactive
element at all. The same caveat about lacking `Widget_ListEntry_Base`'s hover and gamepad
hooks applies, and the same fix would resolve it for both.

## 7. `WBP_CAW_APIDebugScreen`

Parent class `Widget_APIDebugScreen`. A log wall, so blur on. Push this one to
`Frontend.WidgetStack.Modal` rather than the Frontend stack: it is a panel over whatever
you were already looking at, which is the role Modal already plays for the confirm screen.

```
[WBP_CAW_APIDebugScreen]
└─ Border
   └─ Overlay
      ├─ Background Blur
      └─ WBP_Template_Layout
         ├─ NamedSlot_BoundActionExtendPoint
         │  └─ Common Bound Action Bar
         └─ NamedSlot_CenterExtendPoint
            └─ ScrollBox_DebugLog               [BIND]
               └─ CommonTextBlock_DebugLog      [BIND] (Style_Text_ListEntry_Default)
```

## Bound action bar settings

Copy Main Menu's: Action Button Class `WBP_Button_BoundAction`, Entry Spacing `8.0, 0.0`,
Entry Size Rule `Auto`, Entry Box Type `Horizontal`, Max Element Size `0`.

Back needs no per-screen configuration. `InputData_Default` sets its Default Back Action to
`DT_CommonInputKeyMapping`'s `BackAction` row globally, and every screen's action bar picks
it up. The four C++ screens register that action in `NativeOnInitialized`; the
Blueprint-only hub gets it from the **Is Back Handler** checkbox.

## Data setup

Add all five `Frontend.Widget.*` multiplayer tags to `FrontendWidgetMap` in Project
Settings once the Blueprints exist.

For Refresh, add a second row to `DT_CommonInputKeyMapping` alongside `BackAction`:

| Field | Value |
|---|---|
| Row Name | `RefreshAction` |
| Display Name | Refresh |
| Keyboard Input Type Info → Key | `R` |
| Default Gamepad Input Type Info → Key | a free face button |

One row serves both the server browser and the leaderboard; select it in each screen's
Refresh Action handle. Until it exists the binding is skipped, so the screens still work —
Refresh is simply unavailable.

## Required bind names, by screen

| Blueprint | Required widget names |
|---|---|
| `WBP_CAW_MultiplayerScreen` | none (no C++ parent) |
| `WBP_CAW_HostSessionScreen` | `EditableTextBox_SessionName`, `SpinBox_MaxPlayers`, `CommonButton_Create` |
| `WBP_CAW_ServerBrowserScreen` | `CommonListView_Sessions` |
| `WBP_ListEntry_Session` | `CommonText_SessionInfo`, `CommonButton_Join` |
| `WBP_CAW_LeaderboardScreen` | `CommonListView_Leaderboard` |
| `WBP_ListEntry_Leaderboard` | `CommonText_Rank`, `CommonText_PlayerName`, `CommonText_Score` |
| `WBP_CAW_APIDebugScreen` | `ScrollBox_DebugLog`, `CommonTextBlock_DebugLog` |

## Suggested order

Create the two reusable modules, then `WBP_ListEntry_Session`, then the server browser
(which needs the row asset to exist), then `WBP_ListEntry_Leaderboard` and the leaderboard
(duplicated from the server browser, so build it while that one is fresh), then Host
Session and API Debug, then the hub last since it pushes all of them. Add the
`RefreshAction` data table row before compiling either list screen if you want Refresh
working on the first run.

## One thing to confirm

`CommonButton_Create` and `CommonButton_Join` are typed `UFrontendCommonButtonBase*`, so
the button Blueprint dropped into those slots must derive from that class.
`WBP_Button_Default` is almost certainly fine — Main Menu depends on the
`Button Description Text` field feeding `WBP_Text_ButtonDescription`, and that field only
exists on `UFrontendCommonButtonBase`. Worth confirming its parent class in the editor; if
it turns out to be parented straight to `Common Button Base`, widen the two C++ property
types instead of reparenting the asset.
