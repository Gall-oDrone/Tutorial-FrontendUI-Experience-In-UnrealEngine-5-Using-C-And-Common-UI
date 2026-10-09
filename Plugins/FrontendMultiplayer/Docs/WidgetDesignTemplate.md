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

Create three new reusable modules. The first two are parented to `User Widget` the way
`WBP_Text_ButtonDescription` is, and their graph work is in
[EventGraph work, by Blueprint](#eventgraph-work-by-blueprint). The third has a C++ parent
and needs no graph work.

**`WBP_Text_EmptyState`** — used by the server browser and the leaderboard.

```
[WBP_Text_EmptyState]
└─ Overlay
   └─ CommonTextBlock_EmptyStateMessage   (Is Variable)
```

- Overlay Slot: Horizontal Alignment **Center**, Vertical Alignment **Center**. With Fill /
  Fill the text pins to the top-left of whatever list it sits over.
- Appearance → Justification: **Center**, so a message that wraps stays centred.
- Common Text → Style: `Style_Text_Default`, the same style `WBP_Text_ButtonDescription` uses.
- Is Scrolling Enabled can stay on, matching `WBP_Text_ButtonDescription`.

**`WBP_Form_LabeledField`** — Host Session uses it twice, and it keeps label and input
spacing consistent as more forms appear.

```
[WBP_Form_LabeledField]
└─ SizeBox_ListEntry                       — Width/Height Overrides copied from WBP_ListEntry_String
   └─ Horizontal Box
      ├─ CommonTextBlock_FieldLabel        (Is Variable, Style_Text_ListEntry_Default) — Fill, V Center
      └─ Size Box                          Width Override 300 — Auto, V Center
         └─ NamedSlot_FieldInputExtendPoint
```

A horizontal row rather than label-above-input, so labels and inputs form two aligned
columns the way Options rows do. Bind targets must live in the screen itself, so put the
real input widget into the named slot from the screen rather than inside the module.

**`WBP_DetailsView_Session`** — the server browser's right-hand panel. Parent class
`Widget_SessionDetailsView`, the session counterpart of `WBP_DetailsView_Options`; build it
by copying that asset's layout and styles.

```
[WBP_DetailsView_Session]
└─ Scroll Box
   └─ Vertical Box
      ├─ CommonTextBlock_Title             [BIND] (Style_Text_OptionsDetailsView_Title)
      ├─ Size Box
      │  └─ CommonLazyImage_DescriptionImage  (optional bind)
      ├─ CommonRichText_Description        [BIND] (Common Rich Text Block)
      └─ CommonRichText_DisabledReason     [BIND] (Common Rich Text Block)
```

- `CommonLazyImage_DescriptionImage` is optional. When present, C++ shows the session's
  `PreviewImage` and collapses the image when a session has none, which is every session
  for now. Without that, the designer's placeholder brush would show for every session.

- Both rich text blocks: **Text Style Set** `DT_RichTextStyle_OptionsScreen` and
  **Auto Wrap Text** on, as in `WBP_DetailsView_Options`. C++ writes `<Bold>…</>` into the
  description, which only renders if that table has a `Bold` row.
- C++ fills all three: the session name as the title; host, map, players, and ping as the
  description; "This session is full." as the disabled reason when it applies.

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

- Class defaults, under Back: tick **Is Back Handler** and
  **Is Back Action Displayed In Action Bar**.
- Per button, set `Button Display Text` and `Button Description Text`. The latter is what
  populates the description slot on hover.
- Button clicks, the API debug button's visibility, and the focus target are all graph
  work — see [EventGraph work, by Blueprint](#eventgraph-work-by-blueprint). This is the one
  screen whose buttons the graph can reference directly, because it has no C++ parent.

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
         ├─ NamedSlot_TabButtonsExtendPoint
         │  └─ CommonTextBlock_ScreenTitle         "Host Session" (Style_Text_OptionsDetailsView_Title)
         ├─ NamedSlot_MainLeftExtendPoint
         │  └─ Vertical Box
         │     ├─ WBP_Form_LabeledField            label "Session Name"
         │     │  └─ NamedSlot_FieldInputExtendPoint
         │     │     └─ EditableTextBox_SessionName   [BIND]
         │     ├─ WBP_Form_LabeledField            label "Max Players"
         │     │  └─ NamedSlot_FieldInputExtendPoint
         │     │     └─ SpinBox_MaxPlayers            [BIND]
         │     └─ CommonButton_Create                 [BIND] (WBP_Button_Default) — top padding 24
         └─ NamedSlot_MainRightExtendPoint
            └─ Vertical Box
               ├─ Common Text Block                "Host a Session" (Style_Text_OptionsDetailsView_Title)
               └─ Common Text Block                one paragraph (Style_Text_Default, Auto Wrap Text)
```

The title sits where Options' tabs sit, the form takes the left column, and a static
explanation fills the right column where Options shows its details panel. Use a plain
Common Text Block for the paragraph; `<Bold>` markup only renders in a rich text block.

- `SpinBox_MaxPlayers`: Min 2, Max 8, Delta 1, with the slider range matching.
- `EditableTextBox_SessionName` and `SpinBox_MaxPlayers`: Background Color black at
  alpha 0.45 and white text. The engine defaults are a white field with white text.
- Set each `WBP_Form_LabeledField` instance's **Field Label** in Details.
- Focus is handled in C++ (`NativeGetDesiredFocusTarget` returns
  `EditableTextBox_SessionName`), so there is no Blueprint focus step.

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
         ├─ NamedSlot_TabButtonsExtendPoint
         │  └─ CommonTextBlock_ScreenTitle         "Server Browser" (Style_Text_OptionsDetailsView_Title)
         ├─ NamedSlot_MainLeftExtendPoint
         │  └─ Overlay
         │     ├─ CommonListView_Sessions          [BIND] (Common List View)
         │     └─ EmptyState_Sessions              (WBP_Text_EmptyState) "No sessions found"
         └─ NamedSlot_MainRightExtendPoint
            └─ DetailsView_SessionInfo             [BIND] (WBP_DetailsView_Session)
```

- `DetailsView_SessionInfo` is driven entirely from C++ from the list selection, like
  Options' `DetailsView_ListEntryInfo`. Unlike Options, hovering a row selects it, so only one
  row is highlighted at a time. The first row is selected whenever the list arrives.

- On `CommonListView_Sessions`: **Entry Widget Class** `WBP_ListEntry_Session`,
  **Num Designer Preview Entries** 5 (what Options uses, and how you see rows at design time).
- On `EmptyState_Sessions`: tick **Is Variable**, set Visibility to **Collapsed**, and set
  **Empty State Message** to "No sessions found". The graph shows it when the list comes
  back empty — see [EventGraph work, by Blueprint](#eventgraph-work-by-blueprint).
- Focus is handled in C++: the selected row's entry widget, otherwise the list view.
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

This row intentionally does not inherit `Widget_ListEntry_Base`, but it carries the same
hover and selection hook: **On Toggle Entry Widget Highlight State**, fired with the same
rules as Options rows (hovered, or selected). `CommonText_SessionInfo` is
`BlueprintReadOnly` so the graph can restyle it.

C++ also puts `CommonButton_Join` into its selected state while the row is selected, so the
button's style needs a **Selected Text Style** (same as its Hovered one) and Selected brushes
that draw nothing, or Join will not light up with the row. If that style is shared with the
Options tab buttons, duplicate it for Join instead of editing it.

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
                  └─ EmptyState_Leaderboard     (WBP_Text_EmptyState) "No scores yet"
```

- On `CommonListView_Leaderboard`: **Entry Widget Class** `WBP_ListEntry_Leaderboard`,
  **Num Designer Preview Entries** 5.
- The header row is plain Blueprint decoration, not bound to C++. Its three cells must use
  the same widths and padding as the row below or the columns will not line up; that is the
  one thing to get right here.
- On `EmptyState_Leaderboard`: tick **Is Variable**, set Visibility to **Collapsed**, and
  set **Empty State Message** to "No scores yet". Toggled from the graph, as on the server
  browser.
- Focus is handled in C++: the selected row's entry widget, otherwise the list view.
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
row. Implement it to highlight the viewer's own row by swapping the three text blocks
between `Style_Text_ListEntry_Highlight` and `Style_Text_ListEntry_Default` — both already
exist in FrontendUI for exactly this. It is an event rather than a hard-coded colour in C++
specifically so the highlight is authored alongside the rest of the styling. Handle the
`false` branch too: rows are recycled as the list scrolls, so a row that is not reset will
keep the highlight from whichever entry it displayed last. The three text blocks are
`BlueprintReadOnly`, so the graph can reference them.

There is no Join button here, so unlike the session row this one has no interactive
element at all. The same caveat about lacking `Widget_ListEntry_Base`'s hover and gamepad
hooks applies, and the same fix would resolve it for both.

## 7. `WBP_CAW_APIDebugScreen`

Parent class `Widget_APIDebugScreen`. A log wall, so blur on. Push it to
`Frontend.WidgetStack.Frontend` like the other screens, not Modal. Modal is a separate layer,
so the hub would stay drawn underneath with its own action bar and Back would appear twice.
The confirm screen gets away with Modal only because it has no action bar.

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
Blueprint-only hub gets it from the **Is Back Handler** and
**Is Back Action Displayed In Action Bar** checkboxes.

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
| `WBP_CAW_ServerBrowserScreen` | `CommonListView_Sessions`, `DetailsView_SessionInfo` |
| `WBP_DetailsView_Session` | `CommonTextBlock_Title`, `CommonRichText_Description`, `CommonRichText_DisabledReason` |
| `WBP_ListEntry_Session` | `CommonText_SessionInfo`, `CommonButton_Join` |
| `WBP_CAW_LeaderboardScreen` | `CommonListView_Leaderboard` |
| `WBP_ListEntry_Leaderboard` | `CommonText_Rank`, `CommonText_PlayerName`, `CommonText_Score` |
| `WBP_CAW_APIDebugScreen` | `ScrollBox_DebugLog`, `CommonTextBlock_DebugLog` |

## EventGraph work, by Blueprint

Seven of the ten Blueprints (the seven screens and rows, plus the three reusable modules)
need graph work, and all of it is small. Everything else — button clicks on the C++
screens, Back, Refresh, list population, focus — is already done in the C++ parent.

A rule that explains most of this: on the C++-backed screens, the bound widgets are
private `BindWidget` properties, so the graph cannot reference them. That is deliberate
and matches FrontendUI's Options and Confirm screens. Where a Blueprint does need to react,
the C++ parent fires a `BP On …` event instead.

| Blueprint | Graph work |
|---|---|
| `WBP_Text_EmptyState` | Yes — expose the message |
| `WBP_Form_LabeledField` | Yes — expose the label |
| `WBP_CAW_MultiplayerScreen` | Yes — clicks, API debug visibility, focus |
| `WBP_CAW_ServerBrowserScreen` | Yes — empty state |
| `WBP_CAW_LeaderboardScreen` | Yes — empty state |
| `WBP_ListEntry_Leaderboard` | Yes — local-player highlight |
| `WBP_ListEntry_Session` | Yes — hover and selection highlight |
| `WBP_CAW_HostSessionScreen` | None |
| `WBP_CAW_APIDebugScreen` | None |
| `WBP_DetailsView_Session` | None |

### `WBP_Text_EmptyState`

1. My Blueprint → Variables → **+**. Name `EmptyStateMessage`, type **Text**.
2. In its Details: tick **Instance Editable**, set Category to `Frontend Empty State`, and
   give it a default value such as "Empty state message".
3. In the Event Graph, from **Event Pre Construct**: drag in `CommonTextBlock_EmptyStateMessage`
   as a getter, call **Set Text** on it, and plug `EmptyStateMessage` into **In Text**.
4. Delete the disabled **Event Construct** and **Event Tick** nodes.

Use Pre Construct rather than Construct because it also runs in the designer, so the
screens that place this module preview their own message. It is the Blueprint version of
what `UFrontendCommonButtonBase` does with `ButtonDisplayText` in `NativePreConstruct`.

### `WBP_Form_LabeledField`

Same four steps as the empty state: a **Text** variable named `FieldLabel`, Instance
Editable, Category `Frontend Form Field`, applied with **Set Text** on
`CommonTextBlock_FieldLabel` from **Event Pre Construct**.

### `WBP_CAW_MultiplayerScreen`

**Button clicks.** For each button, select it in the hierarchy, and in Details → Events
click **+** next to **On Button Base Clicked**. From that event:

1. Add **Push Soft Widget To Widget Stack**.
2. **Owning Player Controller** ← **Get Owning Player**.
3. **In Soft Widget Class** ← **Get Frontend Soft Widget Class By Tag**, with the screen's tag.
4. **In Widget Stack Tag** ← the stack from the table below.
5. Leave **Focus on Newly Pushed Widget** ticked.

| Button | Widget tag | Stack tag |
|---|---|---|
| `Button_Host` | `Frontend.Widget.HostSessionScreen` | `Frontend.WidgetStack.Frontend` |
| `Button_ServerBrowser` | `Frontend.Widget.ServerBrowserScreen` | `Frontend.WidgetStack.Frontend` |
| `Button_Leaderboard` | `Frontend.Widget.LeaderboardScreen` | `Frontend.WidgetStack.Frontend` |
| `Button_APIDebug` | `Frontend.Widget.APIDebugScreen` | `Frontend.WidgetStack.Frontend` |

Main Menu's button that opens the hub must also use **Push Soft Widget To Widget Stack**
onto `Frontend.WidgetStack.Frontend`, never Create Widget + Add to Viewport. A stack only
draws its top screen; anything on another layer stays visible and doubles the Back button.

The push node takes a soft class, not a tag, which is why the tag goes through
**Get Frontend Soft Widget Class By Tag** first. That function asserts if the tag is not in
`FrontendWidgetMap`, so register all five tags in Project Settings before clicking any of
these in PIE — an unregistered tag is a crash, not a silent no-op.

**API debug button visibility.** From **Event On Initialized**: call
**Should Show API Debug Panel**, feed it into a **Select** node (True → **Visible**, False →
**Collapsed**), and call **Set Visibility** on `Button_APIDebug` with the result. Set it once
here rather than with a Visibility binding: the setting cannot change at runtime, and a
binding re-evaluates every frame.

**Focus target.** My Blueprint → Functions → Override → **BP Get Desired Focus Target**,
and return `Button_Host`.

### `WBP_CAW_ServerBrowserScreen`

1. In the Event Graph, right-click and add **Event BP On Session List Updated**. It has a
   **Num Sessions** pin.
2. Compare **Num Sessions == 0**.
3. Feed that into a **Select** node: True → **Not Hit-Testable (Self Only)**, False →
   **Collapsed**.
4. Call **Set Visibility** on `EmptyState_Sessions` with the result.

"Not Hit-Testable (Self Only)" rather than plain Visible, so the message never swallows a
click meant for the list beneath it — the same visibility FrontendUI's panels use.

### `WBP_CAW_LeaderboardScreen`

Identical to the server browser, using **Event BP On Leaderboard Updated** (pin
**Num Entries**) and `EmptyState_Leaderboard`.

### `WBP_ListEntry_Leaderboard`

1. Add **Event BP On Local Player State Changed**. It has an **Is Local Player** pin.
2. Feed **Is Local Player** into a **Select** node of text style class: True →
   `Style_Text_ListEntry_Highlight`, False → `Style_Text_ListEntry_Default`.
3. Call **Set Style** on each of `CommonText_Rank`, `CommonText_PlayerName`, and
   `CommonText_Score` with that result.

Because the False branch sets the default style explicitly, a recycled row that last showed
the local player is reset correctly.

### `WBP_ListEntry_Session`

1. Add **Event On Toggle Entry Widget Highlight State**. It has a **Should Highlight** pin.
2. Feed it into a **Select** node of text style class: True →
   `Style_Text_ListEntry_Highlight`, False → `Style_Text_ListEntry_Default`.
3. Call **Set Style** on `CommonText_SessionInfo` with the result.

### No graph work

- **`WBP_CAW_HostSessionScreen`** — Create, Back, and focus are all in C++. Only set
  `CommonButton_Create`'s Button Display Text and Button Description Text in Details.
- **`WBP_CAW_APIDebugScreen`** — C++ appends log lines, scrolls, and handles Back.

## Suggested order

Create the three reusable modules, then `WBP_ListEntry_Session`, then the server browser
(which needs the row asset and `WBP_DetailsView_Session` to exist), then `WBP_ListEntry_Leaderboard` and the leaderboard
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
