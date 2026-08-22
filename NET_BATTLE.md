# LAN Net Battle

`NetBattleMap` is an isolated two-player test mode. Its map-name prefix selects
`ANetBattleGameMode`, so the existing single-player map and game mode are not changed.

## PIE test

1. Close Unreal Editor and build `MyPokemonDemoEditor` once after adding the C++ classes.
2. Open `/Game/NetBattle/NetBattleMap`.
3. In Play settings, set `Number of Players` to `2`.
4. Set `Net Mode` to `Play As Listen Server` and start PIE.
5. Focus each window and submit one action per player.

Controls:

- `A`: submit the active Pokemon's attack.
- `1`, `2`: switch to that roster slot.

## Two-machine LAN test

Both machines must run the same packaged build and be on the same LAN.

On the host, open the console and run:

```text
open /Game/NetBattle/NetBattleMap?listen
```

On the second machine, replace the address with the host's IPv4 address:

```text
open 192.168.1.100
```

The server owns action validation, switch resolution, speed order, damage, fainting,
and victory. Clients only submit actions through Server RPCs. Roster HP, active slot,
turn phase, turn number, and battle messages are replicated back to both clients.

## Presentation integration order

Keep the existing single-player Turn Manager out of `NetBattleMap`. It may still be
used as a reference for animation graphs, but it must not calculate multiplayer
damage or choose the next phase.

1. Create `BP_NetBattlePresenter` based on `ANetBattlePresenter` and place one in
   `NetBattleMap`.
2. Copy the old trainer battle arena actors, lighting, post process, and environment
   into `NetBattleMap`. Use the presenter's `BattleCamera`, `LocalSpawnPoint`, and
   `OpponentSpawnPoint` to frame the battle.
3. Create visual-only Pokemon Blueprint children for multiplayer. Add
   `NetBattleVisualInterface` in Class Settings and map its events to the existing
   attack, hit, faint, and switch animations.
4. In the presenter's `VisualDefinitions`, map the exact display names `Pikachu`
   and `Turtle` to those visual Blueprint classes.
5. Duplicate `WBP_BattleUI` as `WBP_NetBattleUI`. Its move button calls
   `SubmitMove`; party buttons call `SubmitSwitch` with roster indices 0, 1, or 2.
6. Bind the widget to `OnBattleStateChanged` for phase/button state and to
   `OnPresenterCue` for battle text, damage numbers, camera shake, and HP animation.

`Roster.CurrentHP` is authoritative replicated state. During `ResolvingTurn`, do not
snap the displayed health bar directly to it. For an `Attack` cue, animate from the
currently displayed value to `Cue.TargetHPAfter` and show `Cue.Damage`. When the
phase returns to `ChoosingActions`, reconcile the bar with `Roster.CurrentHP`.

### Visual interface event mapping

- `InitializeNetBattleVisual`: cache owner/roster information and apply the initial
  Pokemon state.
- `RefreshNetBattleVisual`: update non-animated state such as authoritative HP.
- `PlayNetAttack`: reuse the old attack animation.
- `PlayNetHit`: reuse hit animation, damage number, and camera shake.
- `PlayNetFaint`: reuse the faint animation.
- `PlayNetSwitchOut` / `PlayNetSwitchIn`: reuse switch effects. The presenter changes
  actor visibility automatically.

Battle cues are replicated and locally queued by `SuggestedDuration`. The server
keeps the phase at `ResolvingTurn` for the same total duration, so action buttons
must only be enabled while the phase is `ChoosingActions` and the local player has
not submitted an action.
