# GameUI — History

Notes kept for the record; nothing here is needed to use the plugin.

## Moving content in (Phase 1 of the plugin split, v0.3)

Do every move in the **Content Browser**, then right-click the source folder →
**Fix Up Redirectors**, then commit. Never move `.uasset` files with git or Explorer.

1. `UI/Input/IA_*`, `IMC_Default` → `GameUI/Content/Input`
2. `T_StudioLogo`, `WBP_CAW_LogoScreen` → `GameUI/Content/Brand`
   (startup movies **stay** in `Content/Movies` — MoviePlayer only reads that folder)
3. `WBP_CAW_MainMenu`, `WBP_CAW_StoryScreen`, `WBP_CAW_CreditScreen` → `GameUI/Content/UI/Screens`
4. `BP_FrontendController`, `BP_FrontendGameMode`, `WBP_CUW_PrimaryLayout` → `GameUI/Content/Core`
5. Update `Config/DefaultGame.ini` `FrontendWidgetMap` paths (`/FrontendUI/...` → `/GameUI/...`)
   and `DefaultEngine.ini` GameMode / startup-movie references. PIE after every step.
