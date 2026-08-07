# Kingdom Hearts 0: Door to Darkness — Game Design Document

**Project:** kingdom-hearts-zero  
**Working Title:** Kingdom Hearts 0: Door to Darkness  
**Target Feel:** KH1 + KH2 shell with PS2-era shaders, terminal adventure pacing, and an original pre-KH1 protagonist  
**Status:** Design draft

---

## 1. Core Concept

You are **Zero**, the unknown keyblade wielder. Nobody knows where you came from. You wear a Nobody cloak, wield twin red blades, and carry a Twilight Keyblade fused from Oblivion and Oathkeeper. You operate in the edges of the known worlds before Sora awakens.

Your mission: close 17 Doors to Darkness across 5 Acts before the Realm of Darkness overflows. Each Door hides a **Shambler** — an eldritch Keyblade-corrupted boss. The final Door opens onto the **Shambler Archon**.

The game is a love letter to PS2-era Kingdom Hearts wrapped in a terminal adventure UI, Tron world-select hub, and command-deck combat.

---

## 2. Setting and Tone

- **Era:** Between Dark Road and Kingdom Hearts 1. The Keyblade War is over. Worlds are fragmenting. The Door to Darkness is opening.
- **Protagonist:** Zero — amnesiac keyblade wielder. Voiced internally through terminal logs.
- **Visual Style:** PS2-era shaders on a modern engine. Low-poly worlds, bloom-heavy skies, fixed-angle cameras, jittery HUDs.
- **Hub:** Tron-inspired world-select terminal. Worlds are nodes on a grid. Loading = travel through a data stream.
- **Terminal Adventures:** Between acts, terminal windows deliver lore, keyblade diagnostics, Nobody intel, and cryptic Terra-Xehanort hints.

---

## 3. Protagonist

| Attribute | Detail |
|------|---------|
| Name | Zero |
| Role | Unknown Keyblade Wielder |
| Appearance | Nobody cloak, masked face, twin red blades sheathed at hips |
| Keyblade | Twilight Keyblade — Oblivion + Oathkeeper fusion |
| Motivation | Close Doors to Darkness; recover memory fragments |
| Quirk | Speaks through terminal only; body language in-world |
| Arc | Discovers they are a failed Keyblade Inheritance recipient, not Nobody, not Heartless, but "liminal" |

### Twilight Keyblade
- Fused from **Oblivion** (darkness-leaning) and **Oathkeeper** (light-leaning).
- Shifts form based on **Drive State**.
- Can seal Doors to Darkness when fully tuned.
- lore hook: A master tried to make a balanced keyblade after the Keyblade War and failed — Zero inherited the failed experiment.

---

## 4. Story — 5 Acts

### Act 1: Signal Lost
- Zero wakes on a fractured Destiny Island shore.
- Terminal boots up. First door detected in Traverse Town.
- Learn basics: movement, lock-on, drive shift, command deck.

### Act 2: Clockwork Abyss
- Tron world-select hub unlocks.
- Shambler #1–#6 appear across Disney and original worlds.
- Nobody sightings increase. A figure in a black coat watches from rooftops.
- Hint: "Terra's keyblade once tasted this frequency."

### Act 3: Twilight Synthesis
- Oblivion and Oathkeeper fragments converge.
- Twilight Keyblade achieves fusion.
- Shambler #7–#12.
- Terminal decrypts a memory fragment: a ceremony, a crown, a fall.
- Hint: "Xehanort did not always wear two coats."

### Act 4: Door to Darkness
- The main Door opens in the former Hollow Bastion castle.
- Shambler #13–#17.
- Zero discovers the Doors are not keeping darkness out — they are keeping something in.
- Hint: "The Master of Masters left a page blank."

### Act 5: The Shambler Archon
- Final Door. Archon battle in three phases.
- Post-credits terminal boot: a new keyblade signature appears on the grid.
- Hook for sequel: the Terra-Xehanort connection.

---

## 5. Shambler Bosses (17 Total)

Shamblers are Keyblade-corrupted heartless/nobodies merged with Door energy. They have biomechanical and eldritch traits.

1. Shambler of Rust — Traverse Town (Act 2)
2. Shambler of Steam — Agrabah (Act 2)
3. Shambler of gears — Wonderland (Act 2)
4. Shambler of wires — Monstro (Act 2)
5. Shambler of mirrors — Atlantica (Act 2)
6. Shambler of canals — Halloween Town (Act 2)
7. Shambler of clockwork — Neverland (Act 3)
8. Shambler of skin — Port Royal (Act 3)
9. Shambler of eyes — Olympus Coliseum (Act 3)
10. Shambler of teeth — Beast's Castle (Act 3)
11. Shambler of breath — The Land of Dragons (Act 3)
12. Shambler of silence — Space Paranoids (Act 3)
13. Shambler of echoes — Twilight Town (Act 4)
14. Shambler of hunger — The World That Never Was (Act 4)
15. Shambler of memory — Destiny Islands (Act 4)
16. Shambler of regret — Hollow Bastion (Act 4)
17. Shambler of origin — The Door (Act 4)

**Final Boss:** Shambler Archon — multi-phase eldritch entity formed from all 17 door frequencies.

---

## 6. World-Select Hub: Tron Grid

- Top-down grid with glowing nodes.
- Each node is a world; glow intensity = door proximity.
- Travel = "compile" animation.
- Terminal opens from hub via hotkey.
- Visuals: neon grid, monospace text, scanlines.

---

## 7. Terminal Adventures

Terminals are embedded between missions and inside hub.

| Terminal Content | Purpose |
|------|---------|
| Zero's log | Self-discovery, memory fragments |
| Nobody dossier | Intel on cloaked figures, Org XII red herrings |
| Keyblade telemetry | Drive tuning, seal strength |
| Door telemetry | Lock status, corruption rate |
| Terra-Xehanort fragments | Cryptic archival hints |
| Master of Masters echoes | Foreshadowing |
| System messages | Glitches, warnings, seal breaches |

---

## 8. Combat: Command Deck

- **Command Deck** style: assign abilities to deck slots.
- 4 face commands, 2 shortcuts, 1 summon slot, 1 item slot.
- Limit Break = **Twilight Limit** — zero dual-wields Oblivion + Oathkeeper raw.
- Finishers consume Drive Gauge.

### Shadow Drive Form
- Passive speed buff, dark-aligned.
- Keyblade shifts toward Oblivion silhouette.
- Visual: red-black trail, shadowstep dodge.

### Ultima Drive Form
- High-cost, high-damage.
- summons storm of light/dark fragments.
- Visual: dual keyblades split into light shards.

### Twilight Drive Form
- Balanced form.
- Oblivion + Oathkeeper active simultaneously.
- Special: can seal Doors from this form.

### Ultima Keyseal
- Rare consumable/key item.
- Allows Zero to seal one Door without fighting the Shambler.
- Limited supply. Strategic use.

---

## 9. Shader and Visual Direction

- PS2-era shader emulation:
  - Fixed-function pipeline aesthetic
  - Dithering, palette banding, vertex-lit skies
  - Bloom, lens flare, heat haze
- UI:
  - CRT scanlines
  - Monospace fonts
  - Terminal window chrome
  - Glitch effects on damage/death

---

## 10. Terra-Xehanort Hints

These are vague, woven through terminals and world flavor text.

- A keyblade signature that predates Zero's.
- A record of "two apprentices" in a forgotten archive.
- Terminal fragment: "He asked the darkness to keep a door for him."
- Final hint post-credits: the new keyblade signature is **Terra's**.

---

## 11. Progression

| System | Description |
|------|-------------|
| Drive Gauge | Fills on hit/damage taken. Drives consume gauge. |
| Door Seals | Each world has 0–3 doors. Seal = reward + hub unlock. |
| Memory Fragments | Collectible logs. Expand terminal story. |
| Command Deck | Swap via terminal between missions. |
| Keyblade Tuning | Spend fragments to upgrade Twilight Keyblade forms. |
| Ultima Keyseal | Crafted from fragments or found in chests. |

---

## 12. Art References

- KH1/2 HUD layout and color language
- Tron grid geometry
- PS2-era keyblade designs
- Nobody cloak silhouette reference
- Command Deck and Reaction Command layout

---

## 13. Music Direction

- Re-orchestrated KH1/2 motifs
- Terminal tracks: ambient data-stream soundscapes
- Shambler themes: dissonant orchestral + electronic
- Drive themes: metallic percussion + choir

---

## 14. Scope and Milestones

| Milestone | Content |
|------|---------|
| M1 | Hub, terminal, Zero movement, one test world, one Shambler |
| M2 | 3 worlds, 3 Shamblers, command deck, shadow drive |
| M3 | 6 worlds, 6 Shamblers, ultima drive |
| M4 | All 17 Shamblers, Act 4, Archon phase 1 |
| M5 | Act 5, Archon final, post-credits, shader polish |

---

## 15. Technical Notes

- Engine: custom C++ with PS2-era shader pipeline.
- Hub: Tron-style immediate-mode UI.
- Terminal: embedded web-style overlay.
- Combat: command-deck state machine with drive-state interrupts.
- Save: per-world door status, drive unlocks, terminal log progress.

---

## 16. Lore Non-Negotiables

- Zero is not Sora, Riku, or Kairi.
- Zero is not a Nobody.
- Zero is not a Heartless.
- Twilight Keyblade is explicitly Oblivion + Oathkeeper.
- Terra-Xehanort connection is present only as vague foreshadowing.
- The story ends before KH1 begins.

---

## 17. Open Questions

- Exact list of original worlds vs Disney worlds.
- Reaction Command gating per Shambler.
- Summon pool.
- Multiplayer / coop scope.
- Target platforms beyond PC.

---

*Last updated: design draft*
