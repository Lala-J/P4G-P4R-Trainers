# Game mechanics: social links, stats, deadlines, endings, NG+

> Research notes compiled while planning the trainer (November 2026), from public mods,
> cheat tables, wikis and read-only inspection of the Steam build (P4G.exe v1.0.5,
> PE timestamp 0x64074B8B). "Your exe" / "your install" means that build. Anything not
> marked as verified must be checked against the game before the trainer relies on it;
> verified facts end up in `src/dll/Engine/Addresses.h` with a provenance tag.

I've gathered enough for most of what you asked, but several primary sources blocked direct fetching. The Megami Tensei Fandom wiki returned 402, megatenwiki.com and GameFAQs returned 403, Steam guides returned 429, and archive mirrors were refused. Anything I cite to those sites comes from search-engine snippets, not the full page. Most of the data below is from RPG Site's per-link pages, hxchector.com's walkthrough, Samurai Gamers and a handful of articles. "⚠" marks single-source, conflicting or unverified points.

---

## 1. Social Links (P4G)

### General mechanics
- **Hidden points.** Each rank needs a hidden point total, and the totals vary by link (not verified). Surplus points do not roll over. Once the total is met, the **next hangout** gives the rank-up. Points earned during a rank-up scene count toward the following rank.
- **Bonuses to link points.** A Persona of the matching arcana gives **1.51x**. A day-off invite gives **1.21x**. Matching arcana plus "Top of the Class" stack to **2.01x**, which implies Top of the Class is about 1.33x (from a GameFAQs guide, seen via snippet only).
- **Links that rank up the moment a condition is met, with no hangout:**
  - Fool, Star and Judgement rank up by date or story event.
  - Hermit (Fox) ranks up per completed Ema quest.
  - Empress (Margaret) ranks up per delivered Persona.
  - Jester/Hunger ranks 7–10 come from story events.
- **Extra hangouts with no rank-up are required** between certain ranks: Marie 5→6 and 9→10, and Shu 8→9.
- **Rainy days** block most daytime links. Dojima and Nanako (nights) and Velvet Room links are the exceptions.
- **Global blackouts:**
  - 12/3–12/7: links locked; quests and links resume 12/8.
  - 1/2: no links.
  - Third term (from 1/10): links resume.
  - **2/5 is the last day to finish links and quests.**
- **Rank 10** gives a key item that unlocks fusion of that arcana's ultimate Persona, plus the maximum fusion EXP bonus.
  - For party members, the Persona also evolves.
  - Golden adds third-tier forms (e.g. Takehaya Susano-o, Haraedo-no-Okami). These come from hanging out with that member after 1/10 with their link already maxed.
  - Some ranks also teach party-member skills, e.g. Chie learns Rebellion at R2 and Bufula at R7→8.
- **Reversal.** There is no P3-style jealousy reversal. Only a few choices cause a rank-down:
  - Ai: R2→3 ("That was downright cruel…") and R4→5 ("Not gonna happen!").
  - Naoto: R4→5 ("You're on your own").
  - Ai's temporary-lovers route can **break permanently** at R8 ("We need time apart.").

### All Social Links

| Arcana | Character | Start (date + condition) | How it ranks | Romance choice at | Rank 10 item → ultimate Persona |
|---|---|---|---|---|---|
| Fool | Investigation Team | 4/17, automatic | **Story/date only:** R2 4/30, R3 5/18, R4 6/6, R5+R6 7/10, R7 7/27, R8 10/6, R9 11/6, R10 12/3 | – | Loki |
| Magician | Yosuke | 4/16, automatic | Hangout | – | Buddy's Bandage → Mada |
| Priestess | Yukiko | 5/17, automatic | Hangout; Courage 4 and 3 gate some choices | R8→9 | Shrine Charm → Scathach |
| Empress | Margaret | Velvet Room from 5/19–20 with **Knowledge 3 (Expert)** | Persona requests (below) | – | Spiral Brooch → Isis |
| Emperor | Kanji | From 6/9–6/11: talk to the girl at the Classroom Bldg 2F stairs, then Kanji | Hangout | – | Cute Strap → Odin |
| Hierophant | Dojima | 5/6, automatic | Nights when he is home. Rank-ups need Expression 2 (R1→2), 3 (R3→4), 4 (R4→5) ⚠ | – | Coffee Mug → Kohryu |
| Lovers | Rise | 7/23, automatic | Hangout | R7→8 ("Hold her.") | Signed Photo → Ishtar |
| Chariot | Chie | 4/18, automatic | Hangout | R8→9 | Wristband → Futsunushi |
| Justice | Nanako | ~5/3, automatic | Nights when Dojima is away; Expression and Understanding gates ⚠. **Hard cutoff 10/31** | – | Family Photo → Sraosha |
| Hermit | Fox | 5/5, shrine | Ema quests #7–15, one rank each. #13 needs quest #20; #15 needs 6 scooter trips | – | Gratitude Ema → Ongyo-Ki; TV-world SP cost falls 60→15 yen/SP |
| Fortune | Naoto | From 10/21 (hxchector: earliest 10/24 ⚠). Needs **Knowledge 5 + Courage 5** (Man in Black near Aiya, then Naoto) | Hangout | Flag 1 at R5→6 + flag 2 at R7→8 → confession at R7→8 | Detective Badge → Norn |
| Strength | Kou (Basketball) / Daisuke (Soccer) | From 4/19: choose a sports club, which is **exclusive** | Club days | – | Letter to Kou / Spike Brush → Zaou-Gongen. **R4 unlocks Ai** |
| Hanged Man | Naoki | From 6/8, talk to him on several days in a row (earliest ~6/20) | Hangout | – | Junes Receipt → Attis |
| Death | Hisano | After **Devil R4**; Sundays/holidays only | R6→7 needs the **Husband's Letter** key item from Daidara | – | Old Fountain Pen → Mahakala |
| Temperance | Eri | Daycare job (Understanding 2 "Kindly" per most guides ⚠), worked twice | Job days | – | Clover Bookmark → Vishnu |
| Devil | Sayoko | Hospital job from 5/25, **Diligence 3**, worked twice | Job nights | – | Hospital ID → Beelzebub |
| Tower | Shu | Tutor job from 5/25, **Understanding 5** (several guides agree) | Job nights | – | Test Results → Shiva |
| Star | Teddie | 6/24, story | **Story only:** R2/R4/R9 on dungeon clears, others on dates (R10 on 12/5 on the good path) | – | Helel |
| Moon | Ai | After Strength R4 + **Courage 3**; skip class with her | Hangout | Temporary lovers at R5→6 (ends badly); real romance at R9→10 ("Accept it.") | Compact → Sandalphon |
| Sun | Yumi (Drama) / Ayane (Music) | From 4/25: choose a culture club, which is **exclusive** | Club days | Yumi R9→10; Ayane R7→8 | Annotated Script / Handmade Ticket → Asura |
| Judgement | Seekers of Truth (Investigation Team) | 12/3, good path only | **Story only:** R1 12/3 … R8 12/7; R9+R10 on clearing Magatsu Inaba | – | Lucifer |
| **Aeon** (Golden-only) | Marie | Velvet Room from 4/18 | Hangout | R8→9 ("Because I love you." + hug) | Bamboo Comb → Kaguya |
| **Jester → Hunger** (Golden-only) | Adachi | From 5/13 at Junes; only on specific dates | Hangouts to R6, then story | – | Magatsu-Izanagi |

For Judgement, RPG Site names the character as "Igor". hxchector names it the Seekers of Truth; I consider RPG Site wrong here.

### Aeon (Marie) in detail
- **Start.** Velvet Room → "Check on Dwellers" → "Listen to Marie's request".
- **"Taking her out".** After that she waits **outside the Velvet Room door** (Central Shopping District South) and you take her on outings:
  - R1: school area, plus a meal with Yosuke
  - R3: the city
  - R6: comb shopping (Tatsumi Textiles, Marukyu Tofu or Junes)
  - R7: antique and art shops
  - R10: your room
- **Availability.** Wed/Sat/Sun/holidays per RPG Site and walkthroughwizard; Mon/Sat/Sun per thegamehaus ⚠. Never on rainy days, and usually not while an investigation is active.
- **Gates:**
  - R4→5 needs Rise on the team, so 7/24 or later.
  - One extra outing is needed after R5 and after R9.
  - Guides warn of "dead zones" where hangouts don't rank her up, tied to story progress.
- **Rank 10 deadline.** Rank 10 must be reached **before Magatsu Inaba is cleared** (Ameno-sagiri defeated); the link freezes after that. Guides give 12/22, 12/23, 12/24 or "end of December". **Treat 12/22 as safe.**
- **Hollow Forest:**
  1. Marie at R10.
  2. On **1/2**, visit Margaret and choose to search for Marie. Margaret then calls on Wednesdays.
  3. Ski trip 2/11–2/12.
  4. **2/13 is the only day** to clear it (after school).
  - Missing it is **not** a game over. Marie is simply absent from the epilogue extras.
  - Bosses: Gorgeous King (4F), Heaven's Giant (7F), Marie-san then Kusumi-no-Okami (10F).
  - ⚠ Two guides say your inventory is taken on entry and returned at the staging area or on completion. SegmentNext also mentions SP penalties and money not leaving the dungeon.
  - Ski trip extras and the post-credits epilogue require R10 plus a Hollow Forest clear.

### Jester → Hunger (Adachi)
- Must be at **R6 by bedtime on 11/1**, otherwise the link locks for the rest of the game.
- R7: automatic after clearing Heaven. R8: automatic on 12/3.
- On 12/5 you must choose to tell your friends.
- Then you must **meet Adachi alone**, on 12/7 per hxchector and jusgameguide, or 12/5 per the wiki snippet ⚠.
  - This converts Jester to **Hunger**, renumbered 0 → XI. Hunger is the Golden arcana from the Thoth "Lust" card.
  - If you bring the party, or skip the meeting, the link freezes at Jester R8.
- Hunger R9: after defeating Ameno-sagiri. Hunger R10: automatic on 3/20 on the True Ending path.
- A **Jester** R10 happens only on the Accomplice bad-ending path.
- Required rank before 12/7 is contested: R7 (hxchector) vs R8 vs "R6 by 11/1" ⚠.

### Empress (Margaret) requests

Each rank is earned by bringing the requested Persona with the listed skill:

| Rank | Persona (arcana) | Skill | Approx. level |
|---|---|---|---|
| 2 | Ippon-Datara | Sukukaja | 17 |
| 3 | Matador | Mahama | 24 |
| 4 | Gdon | Rampage | 31 |
| 5 | Neko Shogun | Bufula | 32 (needs Cross fusion) |
| 6 | Black Frost | Auto-Sukukaja | 38 |
| 7 | Yatagarasu | Megido | 40 |
| 8 | Yatsufusa | Mediarama | (not stated) |
| 9 | Ganesha | Tetrakarn | 50 |
| 10 | Trumpeter | Mind Charge | 67 |

Rank 10 gives the Spiral Brooch (→ Isis) and unlocks the Deep Blue Clothes costume.

### Lovers and consequences
- **Up to 7 girlfriends at once:** Chie, Yukiko, Rise, Naoto, Ai, Yumi *or* Ayane, and Marie. There are no male romance options.
- **Christmas Eve (12/24):** you pick one girlfriend, and she gives an accessory (Rise gives the Silver Bangle). With no girlfriend you get a party with Yosuke, Kanji and Teddie and receive the Macho Medal.
  - Clearing Magatsu Inaba by about 12/22 skips to 12/23 and Christmas plays out. Clearing it on 12/24 skips to 12/31.
- **1/1:** a date with one girlfriend of your choice.
- **Valentine's Day (2/14), Golden-only:** you must turn down each other girlfriend face-to-face. Fans report this is purely emotional, with no link penalty. Ai, Yumi and Marie reportedly don't react.

---

## 2. Story and dungeon timeline

| Victim | Story trigger | TV access from | **Last day (game over that evening)** | Dungeon |
|---|---|---|---|---|
| Yukiko | Midnight Channel 4/15–16 | 4/17 (forced Shadow Chie fight; clearable from 4/18) | **4/29** | Yukiko's Castle |
| Kanji | Kidnapped after the 5/14–17 events; clues 5/18 | 5/19 | **6/4** | Steamy Bathhouse |
| Rise | Watch Midnight Channel from 6/21; missing **6/24** (one calendar says 6/23 ⚠) | 6/25 | **7/9** | Marukyu Striptease |
| Mitsuo | Escapes into the TV 7/27; clues 7/27–29 | 7/29 | **8/12** | Void Quest |
| Naoto | Kidnap window from 9/14; Midnight Channel 9/15 | ~9/16–17 | **10/5** | Secret Laboratory |
| Nanako | Kidnapped 11/5 | 11/6 | **11/20** | Heaven |
| Adachi | Identified 12/5 | 12/8 | **12/24** (hxchector says 12/26 but also calls 12/24 the last day ⚠; use 12/24) | Magatsu Inaba |
| Marie | 1/2 Margaret, then ski trip | 2/13 only | 2/13 (no game over) | Hollow Forest |
| Izanami | 3/20 True Ending chain | 3/20 only | 3/20 | Yomotsu Hirasaka |

- **Missed deadline.** A bad-end scene plays and the game is over (videos label these Bad Ending 2 = Kanji, 3 = Rise, 4 = Mitsuo). The game offers a restart from **about one week earlier**. Two sources say this, but I couldn't confirm the exact restart date for each victim ⚠.
- **The 4/15 Twisted Shopping District** is accessible on that day only.
- **Midnight Channel and fog.** The Midnight Channel airs only on rainy nights at midnight. Fog follows a run of rain, and each deadline falls on a fog day.
- **TV world access** is daytime/after school only. It is blocked on story days such as 11/5, 12/3–12/7 and the trip days.

### December decision (12/3, hospital)
1. Refuse to push Namatame into the TV, both when Yosuke pushes and on the confirmation prompt. Picking "Push the bastard in" then "Do it already" gives **Bad Ending: The Price of Revenge**.
2. Pick, in order: (any) → "Wait a second here…" → "We're missing something." → "Their true feelings." → "Something's been bothering me." → "We're missing something…" → "Calm the hell down!"
3. **12/5:** name **Tohru Adachi**. Wrong picks or running out of attempts (3 per one source) give the dead-end bad ending. Then choose to **tell your friends**. Staying silent gives the **Accomplice ending** (Golden-only), which skips to 3/20.
4. Clear Magatsu Inaba by 12/24.

### Normal vs True ending (3/20)
1. Greet everyone whose link you maxed.
2. At "Return home?" answer **No**. Answering Yes gives the Normal Ending.
3. Junes food court: "I'm not finished yet" → Yes → "There's something missing".
4. Riverbank: talk to Dojima and Nanako.
5. Velvet Room: Igor gives the **Orb of Sight**. RPG Site calls it the "Orb of Clarity".
6. Moel gas station attendant, pick in order: "Isn't this where I first…?" → "Do you know Namatame?" → "Did those two come here?" → "Did you do anything to them?" → "What did you do to me?" → "Did you give me the power?" → any.
7. Clear **Yomotsu Hirasaka** the same day. The bosses are Izanami, then Izanami-no-Okami.

Marie and the Hollow Forest add the post-credits extras and are **not** required for the True Ending.

---

## 3. Social stats

| Rank | Knowledge | Courage | Diligence | Understanding | Expression |
|---|---|---|---|---|---|
| 1 | Aware 0 | Average 0 | Callow 0 | Basic 0 | Rough 0 |
| 2 | Informed 30 | Reliable 16 | Persistent 16 | Kindly 16 | Eloquent 13 |
| 3 | Expert 80 | Brave 40 | Strong 40 | Generous 40 | Persuasive 33 |
| 4 | Professor 150 | Daring 80 | Thorough 80 ⚠ | Motherly 80 | Touching 53 |
| 5 | Sage 240 | Heroic 140 | Rock Solid 130 ⚠ | Saintly 140 | Enthralling 85 |

- **Diligence conflict.** MegaTen Wiki gives "Thorough" and Rock Solid at 130. hxchector and RPG Site name rank 4 "Persuasive", and hxchector puts Rock Solid at 140.
- **Values are hidden.** The game shows them only as music-note icons, so all thresholds are fan-derived.

**What each stat gates:**
- **Knowledge:**
  - 2: Translator job
  - 3: Margaret
  - 5: Naoto start
  - Exam results also trigger link bonuses and Nanako's accessories
- **Courage:**
  - 3: Ai
  - 5: Naoto start
  - Also: shrine at night, the Shichiri Beach scooter trip, Rise R6→7 and Chie R6→7 choices, Yukiko choices (Courage 3 and 4), Dojima R7→8
- **Diligence:**
  - 2: Origami job
  - 3: Hospital job (Sayoko)
  - 5: a Shu R1→2 choice
  - Also: number of fishing attempts
- **Understanding:**
  - 2: Daycare job (Eri) ⚠
  - 5: Tutor job (Shu), plus the Rise R5→6, Sayoko R4→5 and Naoki R8→9 choices
  - Also: some Nanako ranks
- **Expression:**
  - Dojima rank-ups (2/3/4) and Nanako rank-ups
  - Naoki R3→4 choices (Courage 4 / Expression 4)

---

## 4. Personas and fusion

- **Count.** About **206** fusible/compendium Personas, or **226** including party-member Personas ⚠. hxchector says 206 but its tables list 226.
- **24 arcana:** Fool, Magician, Priestess, Empress, Emperor, Hierophant, Lovers, Chariot, Justice, Hermit, Fortune, Strength, Hanged Man, Death, Temperance, Devil, Tower, Star, Moon, Sun, Judgement, **Aeon**, **Jester/Hunger** (one slot that changes name), and **World**. World holds only Izanagi-no-Okami.
- **Fusion types:**
  - Normal (2) and Triangle (3) from the start.
  - Cross (4), Pentagon (5) and Hexagon (6) from **7/10**, usable from about 7/12 (hxchector). RPG Site says Cross is available "from 7/20" ⚠.
  - Dodecagon (12) is for Izanagi-no-Okami only.
- **Level cap.** You can't fuse a result above the protagonist's level.
- **Stock.** 6 slots at the start, rising to 12 by late game. The growth trigger isn't confirmed. All 12 slots carry into NG+.
- **Inheritance.** Golden lets you **choose** inherited skills. The number of slots depends on how many unique skills the parents have; vanilla P4 used 1 slot (fewer than 6 unique) up to 5 (24 or more). I haven't confirmed that the same formula applies in P4G ⚠.
- **Link fusion bonus**, per MegaTen Wiki (R8 and R9 are my extrapolation ⚠):

| Rank | Bonus |
|---|---|
| 1 | 1 level + 10% |
| 2 | 1 level + 50% |
| 3 | 2 levels |
| 4 | 2 levels + 50% |
| 5 | 3 levels |
| 6 | 3 levels + 50% |
| 7 | 4 levels |
| 8 | ~4 levels + 50% (extrapolated) |
| 9 | ~5 levels (extrapolated) |
| 10 | 5 levels + 10% |

- **Fusion Forecast** (Golden-only, run by Margaret). It shows today's and tomorrow's forecast. Effects include bonus EXP (1.1–1.5x), stat boosts, skill changes, or a fixed skill learned when specific materials or arcana are used.
- **Fusion accidents.** The result switches to a random arcana at a similar level; one calculator gives a 1/64 chance (unverified). I couldn't confirm whether accidents only happen on forecast days.
- **Compendium.** Personas are registered on fusion or on Shuffle Time acquisition, and can be re-summoned for yen (cost scales with level and stats).
  - Discount items are earned at 25/50/75/100% registration (hxchector) ⚠.
  - Skill cards (Golden-only) can be registered with Marie and bought again.
- **Ultimate Personas.** Fusing an arcana's ultimate requires that arcana's link at rank 10 (or its key item); see the table in §1.
  - **Kaguya** (Aeon, Lv74 ⚠) and **Magatsu-Izanagi** (Jester/Hunger, Lv77) are normal triangle results gated by a maxed link.
  - **Izanagi-no-Okami** (World, Lv91):
    - NG+ only, after a True Ending clear.
    - 12 ingredients: Izanagi, Legion, Sandman, Nata Taishi, Girimehkala, **Norn** (so Fortune at R10 is required), Okuninushi, Orthrus, Kartikeya, Mithra, Tzitzimitl, Cu Chulainn.
    - No link EXP bonus, no inheritance, cannot be registered in the compendium.

---

## 5. New Game Plus

- **Carries over:**
  - Money and playtime
  - Social stats
  - Compendium (and its discounts)
  - All 12 Persona slots
  - Skill cards registered with Marie
  - Weapons and armor unlocked at Daidara's shop
  - Costumes
  - Rank-10 key items: four guides say these carry over ⚠
- **Does not carry over:**
  - Character and Persona levels
  - Link ranks
  - Personas in stock
  - Equipment and inventory items
  - Shadow analysis data
- **NG+-gated content:**
  - **Izanagi-no-Okami**: needs a prior True Ending clear plus Lv91.
  - **Margaret superboss** requirements:
    - Empress at R10
    - All six optional dungeon bosses beaten by the end of January
    - True path on 3/20
    - Get the Orb of Sight, re-enter the Velvet Room, and Margaret gives you the **Aqua Invitation**. The fight is on Heaven 10F.
  - Some dialogue options are available early because your stats carried over.
- **Difficulty:**
  - Five presets: Very Easy (about 5x EXP and money, retry battles), Easy, Normal, Hard, and Very Hard ("Risky"; about 0.4x EXP and money, no retry).
  - Custom settings let you change damage taken, damage dealt, EXP, money and the retry rule separately.
  - On Vita, changing or customising difficulty was NG+-only. On PC and later ports it can be changed any time ⚠ (hxchector still says it is locked on a first run).
- **System-wide.** TV Listings on the title screen (videos, music, Teddie's quiz) unlock with progress. I found no confirmed list of "clear-data" unlocks beyond that.

---

## 6. Items

- **Equipment.** Weapon, armor and **one** accessory, plus a cosmetic outfit. Rise equips nothing.
  - Character weapons, mostly from my own knowledge (unverified): Protagonist swords, Yosuke dual blades, Chie footwear, Yukiko fans, Kanji plates/chairs, Teddie claws, Naoto guns.
  - The strongest gear comes from **the Reaper**, who appears after opening chests or lingering on a floor. Daidara unlocks new stock when you sell materials.
- **Consumables:** about 41 healing items, 29 battle items, and 3 dungeon items (Goho-M, Vanish Ball, Chest Key).
- **Other categories:** materials (sold to Daidara or used for quests), skill cards, gems (traded from fish), fish and bait, garden produce, boxed lunches and food, books, costumes, and key items. There are 69 quests in total.
- **Max stack** is believed to be **99**; not verified.
- **Dangerous to add or remove:**
  - **Rank-10 items** (Bamboo Comb, Detective Badge, Spiral Brooch, Gratitude Ema, etc.) unlock ultimate fusions.
  - **Husband's Letter** gates Hisano R6→7.
  - **Orb of Sight** and **Aqua Invitation** drive the 3/20 True Ending and the Margaret fight.
  - Quest items and the TV-shopping Genji Beetle (needed for the Sea Guardian fish).
  - Items given at specific story beats.
  - Anything added inside the Hollow Forest may be overwritten when your inventory is restored ⚠.

---

## Notes for trainer warnings
- **Setting a link rank directly skips:**
  - key items
  - party-member skills and evolutions
  - romance flags (Naoto's two flags, Ai's route)
  - prerequisite unlocks (Strength R4 → Ai, Devil R4 → Hisano)
- Raising Jester, Judgement, Star or Fool out of story order, or forcing the Jester→Hunger change, is likely to break story logic.
- **Dates to warn on:**
  - 10/31: Nanako cutoff
  - 11/1: Adachi must be R6
  - 12/22: Marie R10 (safe target)
  - 1/2: Margaret "search for Marie" talk
  - 2/5: last day for links and quests
  - each victim's last day in the §2 table
- **Club choices are exclusive** (soccer or basketball; drama or music). Editing both links will conflict.
- Granting Izanagi-no-Okami or Margaret content on a first playthrough is something the game never allows.

## Sources
- RPG Site: [link index](https://www.rpgsite.net/feature/9850-persona-4-golden-social-link-guide-dialogue-options-lovers-relationship-options-skill-unlocks-and-walkthroughs-for-every-s-link), [Fool/Star/Judgement](https://www.rpgsite.net/feature/9840-persona-4-golden-social-link-unlocks-for-fool-star-and-judgement-investigation-team-teddie-igor), [Marie](https://www.rpgsite.net/feature/9849-persona-4-golden-marie-aeon-social-link-choices-unlock-guide), [Adachi](https://www.rpgsite.net/feature/9830-persona-4-golden-adachi-social-link-choices-unlock-guide), [Margaret](https://www.rpgsite.net/feature/9837-persona-4-golden-fusion-solutions-for-the-empress-social-link-margaret), [Strength](https://www.rpgsite.net/feature/9831-persona-4-golden-athletes-strength-social-link-choices-unlock-guide), [Sun](https://www.rpgsite.net/feature/9826-persona-4-golden-yumi-ayane-sun-social-link-choices-unlock-guide), [Ai](https://www.rpgsite.net/feature/9834-persona-4-golden-ai-ebihara-moon-social-link-choices-unlock-guide), [Rise](https://www.rpgsite.net/feature/9845-persona-4-golden-rise-lovers-social-link-choices-unlock-guide), [Naoto](https://www.rpgsite.net/feature/9846-persona-4-golden-naoto-fortune-social-link-choices-unlock-guide), [Chie](https://www.rpgsite.net/feature/9841-persona-4-golden-chie-chariot-social-link-choices-unlock-guide), [Yukiko](https://www.rpgsite.net/feature/9843-persona-4-golden-yukiko-priestess-social-link-choices-unlock-guide), [Kanji](https://www.rpgsite.net/feature/9844-persona-4-golden-kanji-emperor-social-link-choices-unlock-guide), [Yosuke](https://www.rpgsite.net/feature/9842-persona-4-golden-yosuke-magician-social-link-choices-unlock-guide), [Sayoko](https://www.rpgsite.net/feature/9828-persona-4-golden-sayoko-uehara-devil-social-link-choices-unlock-guide), [Hisano](https://www.rpgsite.net/feature/9835-persona-4-golden-hisano-kuroda-death-social-link-choices-unlock-guide), [Shu](https://www.rpgsite.net/feature/9836-persona-4-golden-shu-nakajima-tower-social-link-choices-unlock-guide), [Eri](https://www.rpgsite.net/feature/9827-persona-4-golden-eri-minami-temperance-social-link-choices-unlock-guide), [Fox](https://www.rpgsite.net/feature/9832-persona-4-golden-fox-hermit-social-link-quest-guide), [Naoki](https://www.rpgsite.net/feature/9829-persona-4-golden-naoki-konishi-hanged-man-social-link-choices-unlock-guide), [Dojima](https://www.rpgsite.net/feature/9839-persona-4-golden-dojima-hierophant-social-link-choices-unlock-guide), [Nanako](https://www.rpgsite.net/feature/9838-persona-4-golden-nanako-justice-social-link-choices-unlock-guide), [Jobs](https://www.rpgsite.net/feature/9822-persona-4-golden-part-time-jobs-every-job-requirements-rewards), [Endings](https://www.rpgsite.net/feature/9824-persona-4-golden-endings-guide-true-ending-requirements-how-to-get-the-canon-end)
- hxchector: [index](https://hxchector.com/persona-4-golden-walkthrough/), [Social Links](https://hxchector.com/persona-4-golden-walkthrough/social-links), [Finding Yukiko](https://www.hxchector.com/persona-4-golden-walkthrough/finding-yukiko/), [Kanji](https://www.hxchector.com/persona-4-golden-walkthrough/finding-kanji/), [Rise](https://www.hxchector.com/persona-4-golden-walkthrough/finding-rise/), [Mitsuo](https://www.hxchector.com/persona-4-golden-walkthrough/the-murderer-confesses/), [Naoto](https://www.hxchector.com/persona-4-golden-walkthrough/finding-naoto/), [Nanako](https://www.hxchector.com/persona-4-golden-walkthrough/case-reopened/), [Seekers of Truth](https://hxchector.com/persona-4-golden-walkthrough/seekers-of-truth), [Magatsu Inaba](https://www.hxchector.com/persona-4-golden-walkthrough/magatsu-inaba/), [End of Journey](https://www.hxchector.com/persona-4-golden-walkthrough/end-of-your-journey/), [Hollow Forest](https://hxchector.com/persona-4-golden-walkthrough/hollow-forest), [Yomotsu Hirasaka](https://www.hxchector.com/persona-4-golden-walkthrough/yomotsu-hirasaka/), [Margaret Battle](https://www.hxchector.com/persona-4-golden-walkthrough/margaret-battle/), [Starting the Game](https://www.hxchector.com/persona-4-golden-walkthrough/starting-the-game/), [Social Qualities](https://www.hxchector.com/persona-4-golden-walkthrough/social-qualities/), [Items](https://www.hxchector.com/persona-4-golden-walkthrough/items/), [Equipment](https://www.hxchector.com/persona-4-golden-walkthrough/equipment/), [Persona Guide](https://www.hxchector.com/persona-4-golden-walkthrough/persona-guide/), [Izanagi-no-Okami](https://www.hxchector.com/persona-4-golden-walkthrough/persona-guide/izanagi-no-okami/), [Magatsu-Izanagi](https://www.hxchector.com/persona-4-golden-walkthrough/persona-guide/magatsu-izanagi/), [Kaguya](https://www.hxchector.com/persona-4-golden-walkthrough/persona-guide/kaguya/)
- Samurai Gamers: [April](https://samurai-gamers.com/persona-4-golden/p4g-april-walkthrough/), [May](https://samurai-gamers.com/persona-4-golden/p4g-may-walkthrough/), [June](https://samurai-gamers.com/persona-4-golden/p4g-june-walkthrough/), [July](https://samurai-gamers.com/persona-4-golden/p4g-july-walkthrough/), [August](https://samurai-gamers.com/persona-4-golden/p4g-august-walkthrough/)
- [ArcanaFuse True Ending](https://arcanafuse.com/p4-golden/guides/true-ending/), [Final Weapon endings](https://finalweapon.net/2023/01/20/persona-4-golden-p4g-endings-guide/), [TheGamer Adachi](https://www.thegamer.com/persona-4-golden-p4g-tohru-adachi-jester-social-guide/), [TheGamer Marie](https://www.thegamer.com/persona-4-golden-p4g-marie-aeon-social-link-romance-guide/), [TheGamer Izanagi-no-Okami](https://www.thegamer.com/persona-4-golden-p4g-izanagi-no-okami-fusion-recipe-guide/), [walkthroughwizard Marie](https://walkthroughwizard.com/all-posts/rpgs/persona-4-golden/persona-4-social-link-guides/persona-4-marie-social-link-guide/), [thegamehaus Marie](https://thegamehaus.com/gaming/persona-4-golden-marie-social-link-guide/2023/01/02/), [SegmentNext Hollow Forest](https://segmentnext.com/persona-4-golden-hollow-forest-walkthrough/), [ScreenRant Hollow Forest](https://screenrant.com/unlock-hollow-forest-persona-4-golden/), [GameRant NG+](https://gamerant.com/persona-4-golden-what-carries-over-new-game-plus/), [CBR NG+](https://www.cbr.com/persona-4-golden-new-game-plus-features/), [SVG difficulty](https://www.svg.com/1171984/persona-4-golden-the-5-difficulty-levels-explained), [SegmentNext fusion](https://segmentnext.com/persona-4/mechanics/fusion)
- Seen via search snippets only (direct fetch blocked): [MegaTen Wiki Social Stats](https://megatenwiki.com/wiki/Social_Stats), [MegaTen Wiki Jester](https://megatenwiki.com/wiki/Jester_(Arcana)), [Fandom Calendar](https://megamitensei.fandom.com/wiki/Calendar/Persona_4), [Fandom Fusion Forecast](https://megamitensei.fandom.com/wiki/Fusion_Forecast), [GameFAQs rank-up base values](https://gamefaqs.gamespot.com/xbox-series-x/370653-persona-4-golden/faqs/81937/social-links-rank-up-events-base-values-and-responses), [GameFAQs missed-rescue thread](https://gamefaqs.gamespot.com/boards/641695-persona-4-golden/64959609), [GameFAQs Valentine's thread](https://gamefaqs.gamespot.com/boards/641695-persona-4-golden/64704323), [Steam fog-dates thread](https://steamcommunity.com/app/1113000/discussions/0/2516897767696288530)
