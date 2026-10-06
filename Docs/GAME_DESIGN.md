# Game design

*How to Spearfish* (working title) is a first-person co-op game for one or two players. One player
free-dives with a speargun while the other runs a small restaurant on the boat above. They work from
different information and have to talk. Every night the roles swap.

## Pillars

1. **Two jobs, one crew.** Diving and cooking are both complete games, and neither player is the support
   act. The fun is in the hand-offs: what the kitchen needs, what the sea offers, how long the air lasts.
2. **The sea is alive and indifferent.** Fish behave like animals, not targets. They school, spook, hide,
   hunt each other and fight the line. A good catch is earned by approach and timing, not a scanner.
3. **Readable without a HUD layer of truth.** The diver reads an analog air gauge and a depth gauge,
   watches the water and listens to the radio. There is no sonar, fish finder or dive computer.
4. **Every day is a small story.** A morning plan, a dive, a dinner rush, a debrief at bedtime and a swap.
   Rare events and rumors give each day a hook.

## The day

| Time (game hours) | Phase | What happens |
|---|---|---|
| 06:00 | Morning | Wake up in the bunks (roles may have swapped), plan, buy gear at the island dock, sail out. |
| 08:00 | Service | While the boat is anchored and the sign says OPEN, guests board and order. |
| 18:00 | Closing | No new guests. Finish the open tickets. |
| 17:30 onward | Bunks open | Players may go to bed. The day ends when every connected player is in a bunk. |
| 20:30 | Night | Remaining guests leave (half penalty). It is time to sleep. |
| 02:00 | Pass out | Anyone still awake passes out: a fee, and a diver still in the water loses the bag. |

One game hour is 75 real seconds, so a full day lasts about 20 to 25 minutes, and night runs at double
speed. The night transition fades out and shows the **day summary**, which lists:
- dishes served and takings;
- tips and guests lost;
- fish caught, finds sold and new species;
- expenses and the change in reputation;
- fish spoiled overnight.

It then swaps the roles, sails to a newly plotted region if one was set, restocks the sea, rolls the
day's events and saves.

## Roles and the nightly swap

- **Co-op:** exactly one Diver and one Chef. Every night the roles swap, so today's Diver cooks tomorrow.
- **Solo:** the player is always the Diver, and an **auto-chef** runs the restaurant.
  - It opens during service while the boat is anchored, starts dishes as soon as the cooler allows and
    cooks each step at an upgradeable skill level (68% base).
  - It serves, and radios what the kitchen is missing ("Kitchen: still need 2 x Bluestripe Snapper").
  - Solo players are never missing half the game; they get the radio side of it.
- **Partner leaves mid-day:** the remaining player becomes the Diver and the auto-chef takes the kitchen,
  so the game never stalls.
- **Partner (re)joins:** they take whichever role is free, and the next night swaps fairly. A newcomer
  who joined at night dives tomorrow, and the host cooks.
- Roles are kept per player (by online ID) in the save, so a returning partner resumes their rotation.

These rules live in `Rules/RoleRules.*` and are covered by the native tests.

## Diving

**Air**
- Air is drawn from the tank. Consumption rises with depth, effort and sprinting, and when stung.
- Going below the wetsuit's comfortable depth adds a penalty for every metre past it.
- At the surface you breathe freely and the tank does not drain.
- When the tank runs dry you hold your breath for about 8 seconds, then **black out**:
  - the dive bag sinks into the blue and is lost;
  - you wake up on deck after a short rescue that costs a fee.

**Gauges**
- The **air gauge** is a bar with quarter ticks and no numbers; you read it like a pressure needle.
- The **depth gauge** shows metres and the current depth zone's name ("Reef Slope").

**Movement**
- Swimming is full 3D in the direction you look. Fins set speed and acceleration.
- Sprinting is a fin kick that burns air.
- Buoyancy keeps you gently floating near the surface and slightly sinking in the deep.
- The water surface is a ceiling. You climb out at the boat ladder or wade ashore at the island.

**Depth zones and hazards**
- Each region defines its depth zones. Each zone sets an air multiplier and a catch value multiplier,
  so deeper fish are worth more.
- Stinging drifters (jellyfish) and cornered rays sting.
- Morays and sharks bite, which costs air.
- Swimming out of bounds meets a strong current that pushes you back.

**Dive bag**
- Limited fish slots (big fish take 2 or 3), loot slots and a weight limit.
- A loaded bag slows you down.
- Climbing the boat ladder empties the bag into the cooler, sells the loot and refills the tank.

**Visible gear**
- The equipped kit is shown on the diver's body: suit colour, mask, fins, tank, light and speargun.

## Speargun and line

**Guns**
- Pole-sling to pneumatic to railgun. Each has its own power, range, reload time and shaft speed.

**Hits**
- Shots are hit-tested against the fish's body. Head shots are cleaner (better quality) than tail shots.

**Line physics** (`Rules/LineRules.*`)
- The line has a reel with **drag**: a strong fish pulls out line when its pull exceeds the drag, and
  reeling brings it back.
- At full length the line takes stress. **Sustained** overload, or one violent jerk above 1.6 times the
  break force, snaps it.
- The line **wraps** around rocks and wreck parts. A fish that runs around a coral head drags the line
  over it.

**What the line can hold**
- **Fish** fight, tire, and then can be bagged up close, or towed to the ladder and landed into the cooler.
- **The partner** can be clipped on by the other diver, who can then reel them in or tow them. This is
  for fun and rescues, and does no damage.
- **Physics props** (salvage crates, the treasure chest) can be dragged out of the wreck.
- **Rock or wreck:** the shaft sticks, and reeling pulls you toward it. Use it as a grapple against
  currents.

## Fish

There are 20 species across three regions (`Content/Data/Source/Fish.json`). Each species has:
- an **archetype**;
- habitat biomes and a depth range;
- a size range with weight and value;
- an activity window;
- perception, flee and fight parameters;
- a procedural body (body plan, colours, fins).

| Archetype | Behaviour |
|---|---|
| Schooler | Moves as a school, and the whole school bolts when one is startled. |
| Reef | Browses near the bottom and darts into cover. |
| Ambusher | Holds near a den and explodes away when threatened. Grouper-sized fish fight hard. |
| Pelagic | Open water: fast, strong, long runs on the line. |
| Predator | Patrols, hunts smaller fish, and investigates struggling catches. A shark may steal your fish. |
| Drifter | Drifts and stings on contact. |
| Ray | Glides over sand and stings when cornered. |

**Perception**
- Fish notice divers from approach speed, distance and visibility, which the mask and light change.
- Gunshots and struggling fish make noise that spooks nearby fish.

**Legendary fish** appear only through events (the golden *Sunscale Grouper*, *The Old One*). They are
the most valuable ingredients in the game.

**Decorative baitfish schools** are local-only instanced boids. They make the reef feel alive without
network cost.

## Restaurant and cooking

**Guests** arrive while the boat is anchored and the sign says OPEN, up to the number of seats.
- The **Food Critic** has low patience and moves reputation a lot.
- The **Local Fisher** shares **rumors** about today's events.

**Orders**
- Guests pick dishes from the unlocked menu. They are biased toward their favourite tags and toward fish
  that live in the current region.
- Each order has a patience timer. Guests who wait too long walk out, which costs reputation.

**Recipes** (17) need one or more fish, sometimes a specific species and sometimes any fish of a category,
sometimes at a minimum length. Each recipe is a chain of station steps.

**Cooking minigames** (`Rules/CookingRules.*`) are 4 to 8 seconds each, and every one is a different
skill:

| Step | Station | Skill |
|---|---|---|
| Fillet | Cutting board | Press as the knife crosses each cut mark (precision timing). |
| Chop | Cutting board | Press on the beat (rhythm). |
| Season | Spice station | Stop the swinging needle in the zone, twice. |
| Grill | Grill | Flip each side while it is golden (anticipation). |
| Fry | Fryer | Hold to heat and release to cool; keep the oil in the band. |
| Simmer | Stove | The same as Fry with heavier, slower dynamics. |
| Plate | Plating counter | Enter the arrow sequence quickly without slips. |

**Dish quality**
- 70% comes from the step scores and 30% from ingredient quality (shot placement, freshness).
- Upgrades (the lava-stone grill, the spice rack) add a flat bonus.

**Serving** at the pass pays the dish price scaled by quality, plus a tip for speed, and moves reputation.
Order and dish IDs are single-use, so nothing can be paid twice.

**The cooler** holds the catch. Fish lose quality overnight, and fish that are too far gone are thrown out.

## Communication and asymmetric information

**The chef knows the demand**
- The waterproof tablet has these tabs:
  - **Orders:** patience and missing ingredients.
  - **Kitchen:** dishes in progress.
  - **Cooler.**
  - **Diver:** telemetry, with an upgrade.
  - **Comms.**
  - **Restaurant.**
- The chef can always use the tablet. The diver can use it only on deck.

**The diver knows the supply**
- What is down there, how big it is, how much air is left.

**Radio**
- Quick messages on keys 1 to 6 are role-specific. "Need 2 x Snapper (35 cm+)" fills in the top kitchen
  need automatically; "Spotted a Golden Trevally!" names the fish you are looking at.
- Yes/No and the less frequent calls are on the tablet's Comms tab.
- **Voice:** push-to-talk on V.

**Upgrades that bridge the gap**, deliberately on the chef's side:
- the *Dive Link Tablet* shows the diver's depth, air and bag on the tablet;
- the *Buoy Tracker* marks the diver's position on the chef's screen.

Neither tells the diver where fish are.

## World and regions

**Regions** are generated deterministically from a seed and terrain parameters:
- an island with a beach and dock, reef ridges and seagrass, kelp, a drop-off, caves and a wreck;
- the same layout on every machine and every visit;
- different loot, fish and events each day.

| Region | Unlock | Character |
|---|---|---|
| Coral Cove | Starter | Warm reef, sand flats, a small wreck, caves. 15 species. |
| Kelp Narrows | 1,200 coins, 2.5 stars, day 5, 8 species caught | Kelp forest, rock walls, colder and deeper. *The Old One* lives here. |
| Wreck Graveyard | 3,000 coins, 3.5 stars, day 9, 14 species caught | Cargo fields and deep hulls, with sharks. |

**Travel**
- Unlock regions at the **chart table** on the boat, then plot a course.
- The boat **sails overnight**, and you wake up in the new region.

**Island hub**
- The dock has the **dive shop** (gear and boat upgrades) and the **journal board**.
- You sail back to shop.

**The boat**
- **Helm:** a driveable boat, with an anchor to drop and raise.
- **Ladder:** with a descent line.
- **Gear locker.**
- **Kitchen:** cooler, cutting board, spice station, grill, fryer, stove, plating counter, pass and open
  sign.
- **Dining tables:** 2 seats to start, expandable to 8.
- **Cabin:** two bunks.

**Daily content**
- About 16 loose finds (shells, coins, bottles, artifacts).
- **Giant clams:** reach in while open for a pearl. They tremble before snapping shut, and a late grab
  costs air.
- **Salvage crates** around the wreck, which you can pull loose with the line and pry open.

## Events and rumors

Each morning the region rolls its event list (`Content/Data/Source/Events.json`, `Rules/EventRules.*`):
- every event has a chance and a first possible day;
- at most two events per day, and one per type;
- the rolls are deterministic per day seed.

| Event | Effect |
|---|---|
| Legendary visitor | A legendary fish spawns in its habitat, such as a wreck or cave. |
| Treasure cache | A storm chest lies in a cave. It is heavy and holds gold. |
| Critic visit | A food critic is guaranteed to come to dinner. |
| Bait ball | A huge school of baitfish with predators circling. |
| Species bloom | A rare species shows up in numbers. |

Gossiping guests reveal the rumor text on the chef's tablet. The chef has to relay it.

## Progression and economy

- **Money** comes from dishes, tips and sold finds. It is spent on:
  - gear: 31 items in 8 slots, 3 to 4 tiers each, shared by the crew;
  - restaurant upgrades: seats, better stations, patience, auto-chef skill, telemetry, tracker;
  - new regions.
- **Reputation** (0 to 5 stars) gates guests, recipes and regions.
- **The journal** records species seen and caught, best sizes and finds. Species count gates regions too.
- **Failure is never terminal.**
  - Blackouts cost the bag and a fee.
  - Passing out costs a fee.
  - Money never goes below zero, and the starter gear is free.
- **Saving:** the host's campaign saves every morning and on quitting.
  - It keeps money, gear, upgrades, regions, the journal, the cooler and role seats.
  - The current day restarts from the morning when you load.
  - Solo and co-op use separate save slots.

## Design decisions

- **No sonar, scanner or dive computer.**
  - Finding fish is about reading the reef and the water. The depth gauge and analog air gauge are the
    only instruments.
  - The boat marker appears only at the surface, where you could actually see the boat.
  - Any future instrument needs a strong design reason and explicit approval.
- **The chef never fishes and the diver never cooks** within a day. The nightly swap is what makes both
  players experts at both jobs over time.
- **Solo is a first-class mode**, not co-op with an empty seat. The auto-chef and radio replace the partner.
- **Not a clone.** The working title is close to *How to Fish*. The mechanics avoid that game's
  identity on purpose (free-diving with a physical line, a floating restaurant, asymmetric co-op roles),
  and the final name should be chosen to avoid confusion.
- **Data first.** Every species, item, recipe, guest, region, find, upgrade and event is data, so balance
  and content changes need no code.

## Open questions for later milestones

- Weather (swell, visibility, storms) and its effect on fish and boat handling.
- Night diving with bioluminescence, using the dive light.
- Crew hiring for the restaurant beyond the apprentice cook.
- Proper online sessions (Steam/EOS invites) instead of direct IP.
