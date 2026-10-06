# Unreal Boss Fight (UE 5.7, C++ + GAS)

A multiplayer MMO-style raid encounter in which **every damaging attack is a projected AoE**, for both the boss and the players. You play a two-handed-sword warrior whose main survival tool is a dodge with i-frames. The boss is **Kharzul, the Ashen Warden**. It has three phases, a threat table, soft and hard enrage, interruptible casts and summoned adds, and every attack is a replicated ground telegraph.

> **Status:** the code was written against the UE 5.7 APIs but has **not been compiled yet**. No Unreal Engine was available in the authoring environment. Expect to fix a few small compile errors on the first build.

## Quick start

1. Right-click `BossArena.uproject` > **Generate Visual Studio project files**, then build `BossArenaEditor` (Development Editor).
2. Open the project. On the first editor launch, `Content/Python/init_unreal.py` runs `setup_boss_arena.run()` once. It creates:
   - `/Game/BossArena/Materials/M_Telegraph`: translucent, unlit, with `Color` and `Opacity` parameters.
   - `/Game/BossArena/Data/DT_BossAoE`: imported from `Content/BossArena/Data/BossAoE.csv`.
   - `/Game/BossArena/Blueprints/BP_Warrior`, `BP_Boss`, `BP_BossAdd`, `BP_BossArenaGameMode`.
   - `/Game/BossArena/Animation/DA_WarriorAnimSet`: an empty montage set.
   - `/Game/Maps/BossArena`: the circular blockout, sun/sky/fog, and 8 player starts.

   To rerun it by hand: Python console > `import setup_boss_arena; setup_boss_arena.run(force=True)`.
3. **Multiplayer test:** Play dropdown > Number of Players **2** (or more) > Net Mode **Play As Listen Server**, or **Play As Client** for a dedicated server. Press Play.

Everything also works without the generated assets. The game mode spawns a default arena and boss when the map is empty, and telegraphs fall back to the engine basic-shape material.

## Controls

| Input | Ability | Shape |
|---|---|---|
| WASD / Mouse | Move / camera | |
| Space | Jump | |
| LMB (hold to chain) | Cleaving Combo, 3 hits | Frontal cone x3 (100°, 120°, 160°) |
| Q | Whirlwind (25 rage) | 360° circle, 3 pulses |
| E | Shockwave (15 rage) **interrupts** | Forward line/column |
| R | Leaping Slam | Gap-closer jump, landing circle |
| F | Ground Rend (20 rage) | Lingering rectangle, damage over time |
| T | Taunting Shout | Radial; burst threat + 4 s fixate |
| X | Execute Slam (30 rage) | Circle; only when boss < 20% HP |
| **Left Shift** | **Dodge** (3 s CD) | Dash in input direction (backstep with no input), 0.45 s i-frames |
| RMB (hold) | Block | 50% mitigation, *not* immunity |

Console commands (non-shipping builds, routed to the server): `StartFight`, `ResetFight`, `SetBossHealth 0.65`, `SkipEncounterTime 475` (soft enrage) / `595` (hard enrage), `ToggleHoldToAim` (press = preview the AoE, release = cast).

## Architecture

```
Source/BossArena
├─ BossArenaGameplayTags      native tags (State.Invulnerable, Damage.Unblockable, Ability.*, Cooldown.*)
├─ BossArenaTypes             FAoEShape, EBossAoEShape, FBossAoEShapeRow (data-table row)
├─ BossArenaCharacterBase     ACharacter + ASC + team + death + combat text (warrior, boss, adds)
├─ Attributes/                BaseSet (Health, MaxHealth, IncomingDamage meta) -> Warrior (Rage, Armor, Strength) / Boss (Damage, EnrageTimer)
├─ GAS/                       native GameplayEffects (damage, rage, dodge i-frames, block, cooldowns)
├─ AoE/                       shape math + server overlap resolution, procedural telegraph mesh, telegraph actor, persistent zone
├─ Warrior/                   AWarriorCharacter, UWarriorAnimInstance, UWarriorAnimSet, Abilities/GA_*
├─ Boss/                      ABossCharacter, ABossAddCharacter, UThreatComponent, UBossPhaseComponent, AI controller, Abilities/
├─ Game/                      GameMode (encounter flow), GameState (boss ref, raid warnings), PlayerController (debug), GameInstance
├─ Arena/                     ABossArenaBlockout (floor, ring wall, LOS pillars, spawn points)
└─ UI/                        ABossArenaHUD (canvas HUD)
```

### Damage pipeline (all AoE, server-authoritative)
1. An ability (warrior or boss) calls `UBossArenaAoELibrary::GatherTargetsInShape` **on the server only**. It runs a pawn overlap sphere for the broad phase, then an exact 2D test for the circle, cone, donut, line, rectangle or rotating sweep, with capsule-radius tolerance, a height check and optional LOS against `WorldStatic`.
2. `ApplyAoEDamage` applies `UGE_BossArenaDamage` (SetByCaller `Data.Damage` into the `IncomingDamage` meta attribute).
3. `UBossArenaAttributeSetBase::PreGameplayEffectExecute` **discards the damage when the target has `State.Invulnerable`**, unless the spec carries `Damage.Unblockable`. The target then shows "IMMUNE".
4. `PostGameplayEffectExecute` applies armor and block mitigation, reduces Health, generates rage and threat, and handles death.

No damage is ever calculated on a client. Clients only predict their own ability activation, montages and the faint aim indicator.

### Dodge (`UGA_Dodge`)
- Local-predicted. The owning client sends its input direction as target data, and the server **sanitizes it** (horizontal and normalized; falls back to a backstep).
- `UGE_DodgeInvulnerability` (0.45 s, grants `State.Invulnerable` + `State.Dodging`) is applied by the ability. The server's copy is authoritative because the damage check in step 3 above runs on the server's ASC.
- Displacement uses `AbilityTask_ApplyRootMotionConstantForce`: 650 uu over 0.35 s, network-smoothed by CMC root-motion sources. The animation is a directional montage from `DA_WarriorAnimSet` (Forward/Back/Left/Right).
- The 3 s cooldown comes from a SetByCaller-duration GE (`Cooldown.Warrior.Dodge`). The HUD shows it as a highlighted hotbar slot with a sweep.
- Dodging cancels attack wind-ups and block. It cannot be used during a Leap Slam.

**Tuning rationale:** dodgeable telegraphs give at least 1.4 s of lead time (adds) and usually 1.8–3 s (boss). Rotating beams tick every 0.25 s, so a 0.45 s i-frame window always covers at least one tick when you dodge *through* a beam. The donut's safe centre (380 uu) and the meteor radius (450 uu) are both smaller than the dodge distance, so one dodge clears them.

### Boss
- `UBossAoEGameplayAbility` (ServerOnly) handles cast time, a replicated `AAoETelegraphActor` per placement, the cast bar and raid warning, then overlap damage when the cast ends. When `ActiveDuration > 0` it ticks damage instead, which is how the rotating beam works. Parameters come from the `DT_BossAoE` row `RowName`. If the row is missing, the C++ `DefaultParams` are used.
- Kit (`Boss/Abilities/BossKitAbilities`): Cone Swipe, Meteor (top threat), Meteor Rain (each player), Donut "Cinder Ring", Rotating "Searing Beams", **interruptible** "Searing Nova", Summon Adds, Phase Transition, Hard Enrage.
- `UBossPhaseComponent` uses three phases with thresholds at 100% / **70%** / **40%**, each with its own kit and global cooldown. Entering a phase cancels the current cast and fires a room-wide **LOS-checked** burst: dodge it or hide behind a pillar.
- `UThreatComponent`: damage adds threat, and Taunting Shout jumps the taunter above the current top threat and fixates the boss for 4 s. Targeted AoEs go on `GetTopThreatActor()`, and the boss faces and chases that target.
- Enrage: **soft at 8:00** (×1.3 damage via the `Damage` attribute, ×1.2 AoE size) and **hard at 10:00**. Hard enrage casts `Annihilation` repeatedly. It is flagged `bUnblockable`, so **dodge i-frames, block and armor do not save you**.
- Adds (`ABossAddCharacter`) only have small AoEs: Exploding Puddle, which leaves a lingering damage zone, and Ember Charge, a line telegraph followed by a dash.

### Networking summary
| System | Authority | Replication |
|---|---|---|
| Damage, death | Server (GE execution) | Attributes (ASC), `bIsDead` |
| Dodge i-frames | Server-applied GE; client copy is prediction only | GE/tags via ASC (Mixed mode for players) |
| Threat | Server | `ThreatTable` (replicated array) |
| Telegraphs | Server spawns | Actor params + server time; clients animate locally |
| Cast bar, phase, enrage | Server | Replicated structs/flags on the boss |
| Raid warnings | Server | Reliable multicast on GameState |

## Art and animation hookup (editor work)

The C++ is asset-independent and uses greybox capsules and engine shapes. To add the real warrior:

1. **Skeletal mesh:** in `BP_Warrior`, set *Mesh* to your character and add a socket `hand_r` (or change `SwordSocketName`) in the right hand. Assign your two-handed sword mesh to the `Sword` component. The C++ attaches it to the socket. The greybox body hides automatically once a skeletal mesh is set.
2. **Montages:** fill `DA_WarriorAnimSet` with montages for Cleave1/2/3, Whirlwind, LeapSlam, Shockwave, GroundRend, TauntShout, Execute, Dodge Forward/Backward/Left/Right, Block and Death, all using the `DefaultSlot` (UpperBody blend is fine for swings). Then set `AnimSet` on `BP_Warrior`. Abilities run on timers, so they work without montages.
3. **Animation Blueprint:** create `ABP_Warrior` on your skeleton with **parent class `WarriorAnimInstance`**. It exposes `GroundSpeed`, `Direction`, `bShouldMove`, `bIsFalling`, `bIsBlocking`, `bIsDodging`, `bIsLeaping` and `bIsDead`. Suggested state machine `Locomotion`:
   - `Idle` (two-handed idle) ↔ `Run` (blendspace on GroundSpeed/Direction) when `bShouldMove`.
   - `Jump/Fall` while `bIsFalling && !bIsLeaping`.
   - `Block` loop pose layered on the upper body while `bIsBlocking`.
   - `Death` when `bIsDead`.
   - Then `Slot 'DefaultSlot'` → Output for the ability and dodge montages.
4. **Niagara:** set `ImpactEffect` in `DT_BossAoE` rows. Telegraph actors play it on impact for every client.
5. **Boss/adds:** set meshes in `BP_Boss` / `BP_BossAdd`. To use BP adds, set `AddClass` on `GA_Boss_SummonAdds`, or create a BP child of it.

## Tuning without C++
- Boss AoE shapes, timings, damage, flags and colors: `Content/BossArena/Data/BossAoE.csv`. Reimport it into `DT_BossAoE`, or edit the table directly.
- Warrior abilities: create a Blueprint child of any `GA_*` and edit the `Warrior|AoE` / `Warrior|Cost` categories. Then change the class in the `Hotbar` array on `BP_Warrior`.
- Phases: the `Phases` array on `BP_Boss` → `Phases` component sets thresholds, kits, transition ability and global cooldown.
- Enrage: `Boss|Enrage` properties on `BP_Boss`.
