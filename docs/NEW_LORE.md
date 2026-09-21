# NEW LORE — The Charter of Zero

> **Directive:** all remakes are grounded in the original games' lore and stories.
> **Exception:** *kingdom-hearts-zero* builds **new lore around Zero as needed**.
> This document is the ledger of that exception — what is borrowed, what is forged, and the laws that keep the two from colliding.

---

## 1. What is borrowed (the scaffold — never rewritten)

Kingdom Hearts canon may be *invoked* as setting; it is never *replotted*.

- **Worlds** — Destiny Islands, Traverse Town, and the margins of other KH1-era worlds are visited **as scenery and condition, not as theirs to command**. Zero does not save them; he passes through, and they close behind him.
- **The enemy taxonomy** — Heartless swarm; Nobodies linger; the Door to Darkness is real. Zero belongs to none of these ranks, which is the entire point.
- **The Keyblade** — inherited weapons of the Keyblade War. Oblivion and Oathkeeper are real artifacts; Zero carries their fusion.
- **The legacy figures** — Terra-Xehanort hints, the old master experiments, the Organisational red herrings. These are *lamps*, not *pilots*.

## 2. The new mythos around Zero

### Zero — the Liminal
Zero is **not a Nobody, not a Heartless, and not a Keyblade wielder** — he is the *slot in the Inheritance ledger that was never filled*:

- After the Keyblade War, a master attempted the impossible: to forge **one balanced keyblade** that kept light and dark as equals, so no wielder would ever have to choose. The attempt was a failure — the blade refused to hold both natures and *broke before it was given*.
- Zero is what remains of that refusal: the **Keyblade that never happened**, walking.
- Where a Heartless is a heart lost and a Nobody is a body self-divided, Zero is **an inheritance without a giver** — a creature of doors, thresholds, and edges. Worlds touch him and *wobble*.

### The Door
The Door to Darkness of canon exists — but Zero's Door is **between** worlds, not at their end:

- The Doorways only wobble in the presence of the liminal. Zero walks through places closed to everyone else because he is *made of the gap*.
- **Law of Doors:** the Door is both setting and metaphor — every act ends at a threshold, and every threshold ends a person.

### The Twilight Keyblade
Forged from **Oblivion and Oathkeeper** fused at the moment of Zero's waking:

- It is the only keyblade that **changes shape** — dusk-metal during quiet, dawn-metal near a Door.
- It remembers both wielders it was cut from; when it hums near a sealed Keyhole, Zero hears *two* voices arguing.

### The Last Forge
New to this mythos: **the Master of the Last Forge** — the failed balancer, now a ghost in the margins:

- He appears only on the terminals and at the edges of Doors, teaching in fragments.
- His sin is not the broken blade — it is that he never told anyone *why* balance mattered.

### The Observer
New to this mythos: **a figure in a black coat** who watches Zero from rooftops and terminals:

- The GDD's hint stands — "Xehanort did not always wear two coats." The Observer is **a Xehanort of another world-line**, studying the failed key the way a gardener studies a mutation.
- He never fights. He *documents*. That is more frightening.

### The Motes
New to this mythos: **the residue of broken Inheritance** — the shape the Forge's failure leaves on the world:

- Wherever a Door wobbles, small light-particles drift: half-born keyblades that never got a hand. They neither help nor harm; they simply *remember the attempt*.
- Zero can feel them the way a sailor feels weather.

## 3. Laws of the exception

1. **Invoke, never revise.** Any element marked KH-canon appears as-is; the story around it is new.
2. **Attach to liminality.** All new lore must serve Zero's state — door, threshold, edge, inheritance-without-giver. If a new idea does not attach, cut it.
3. **One observer, one ghost, one door.** The new cast is deliberately small: the Master (past), the Observer (present), the Door (place and metaphor). Everything else is borrowed.
4. **Worlds close behind him.** Each world Zero enters is a borrowed story he cannot keep; the terminal logs record what he *remembers* of it.
5. **The Twilight Keyblade changes.** It is the only blade that does — the signature of a liminal wielder.

## 4. Where this lore already lives

- `docs/GDD.md` — canon hooks this charter extends (liminal arc, the failed experiment, the two-coat hint, Twilight Limit).
- `docs/KH_CANON_LIST.md` — the borrowed scaffold, catalogued.
- `data/worlds/*.json` — KH1-era worlds as scaffold, authored for Zero (resonance, thresholds, urchins naming him "Zero").
- `data/combat/*.json` — Nobodies, Dimensional Shamblers, the liminal's combat profile.
- `assets/blender/char_zero.glb` — the forged Nobody-shadow of the spiky-haired archetype (see Seele ledger, `char_zero`).
- `assets/blender/zones/kh-zero-zones.json` — *Drowned Destiny Shore*, *Traverse Door*, *The Door Sanctum* — the Doors of Act 1.
## 5. Inheritance by Witness — the keyblade gate

Zero cannot wield a keyblade at all until after the second story mission. He is
not denied by weakness; he is denied by *unwitnessedness*: the Inheritance is
given, and he has not yet seen its shape.

When the second door (The Traverse Door) is sealed, the flashback lands —
`data/dialogue/flashback_sora.json`. On a drowned shore, a boy called Sora draws
a Keyblade in the sand, crooked and enough. Every failed Inheritance in the
world leans toward the drawing. It is the first keyblade ever *chosen* rather
than forged — and Zero *sees* it.

Seeing it is the act. The master gate (`master_gate` in
`data/combat/keyblades.json`) opens at mission two; the Twilight Keyblade
answers at his side. Until that moment the engine refuses every keyblade: the
twin red sabres are the only weapon his hands will close on.

**Rule 6 (new, amending): the Inheritance is a witness, not a gift.** A
keyblade is not handed down; it is *seen* and answered. Zero's flashback is the
only Inheritance in the story that happens to a Nobody — which is the point:
what is seen cannot be erased.

## 6. The Bazaar Between Doors — the moogle stall

A striped tent in Traverse Town that is always exactly where Zero turns around
(`data/crafting/moogle_stall.json`). The stallkeeper has no name and answers to
no world; it purchases the motes the dark sheds and forges them.

**The economy of motes.** The dark pays in pieces of itself. Every defeated
Heartless and every Dimensional Shambler settles its account on death
(rolled in `CombatEngine::on_victory`, world- and arc-timed):

- **Heartless** shed common motes — `dusk_shard`, `heart_fragment`,
  `nobody_thread` — wherever their world is young. Rare motes come from the
  specific worlds they haunt.
- **Shamblers** carry the rare ores of the worlds they guard: `stolen_song`
  from Yssora's tide, `first_light` from the Archon alone. Arc gating means a
  boss you defeat early cannot shower you with endgame ore - the story has to
  catch up for the drop to open.

**The forge** (`data/crafting/recipes.json`, 24 blueprints). Every keyblade in
the game can be forged — including the **Ultima Weapon** (arc 3: the grin's wax
and the bells' metal) and the **Ultima Keyseal** (arc 5: First Light from the
Archon's own body). Drive forms and command refinements are also sold:
`shadow_overdrive`, `ultima_drive`, `twilight_form`, and four deck refinements
that open command slots. The stall refuses blueprints the story has not
unlocked yet, and refuses to take munny twice for a blade already owned.

**Rule 7 (new, amending): nothing is found, everything is finished.** The
Ultima Weapon is not a reward hidden in a chest; it is the forge's answer to
what Zero has seen. "The philosopher's stone is forged one step at a time."

## 7. The Echo — a second breath of the drowned shore

When Zero lets a full gauge of the dark's patience slack, he can step back
and let *someone else* stand — a resonance of the boy who drew a keyblade
in the sand. The Echo is not a person and not a memory; it is the fight's
second breath, built fresh every battle and surrendered to the dark when
it unravels. It has no name, saves nothing, and owns nothing — it answers
the drowned shore, and the shore is only ever one swing away.

**Rule 8 (new, minor): the cast is one; the Echo is a resonance, not a
member.** It never persists, it never levels, and it is indistinguishable
from Zero's own will at the moment of swapping. The one-observer doctrine
stands; the observer simply changes which shadow the light falls on.
