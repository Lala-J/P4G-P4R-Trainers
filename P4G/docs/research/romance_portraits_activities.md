# Romance routes, portrait assets and daily activities

> Research notes compiled while planning the trainer (November 2026), from public mods,
> cheat tables, wikis and read-only inspection of the Steam build (P4G.exe v1.0.5,
> PE timestamp 0x64074B8B). "Your exe" / "your install" means that build. Anything not
> marked as verified must be checked against the game before the trainer relies on it;
> verified facts end up in `src/dll/Engine/Addresses.h` with a provenance tag.

I couldn't open megatenwiki.com or GameFAQs FAQs directly; both return 403. So Part A relies on the Fandom wiki (read through its API), RPGSite and hxchector, with GameFAQs only through search snippets. Part B was checked directly against your `data.cpk`, read-only, in memory, with no files written.

## Part A — Romance

"Rank N event" means the scene that raises the link to Rank N, which is Fandom's numbering. RPGSite writes the same event as "R(N-1)→N".

| Character (Arcana) | Start | Days / place | Romance branch: choices that lock it in | Avoid |
|---|---|---|---|---|
| Chie (Chariot) | 4/18, automatic | School roof on weekdays; Daidara/South Shopping District on days off; no rain. Fandom: Mon–Sat. RPGSite: Mon/Tue/Thu/Sat **(they disagree)** | **Rank 9**: "Will you be my girlfriend?" (the alternative "I'm counting on you." = platonic). Rank 10 has a separate romance version with no gate choice | — |
| Yukiko (Priestess) | 5/17, automatic | Mon–Thu + Sun; Classroom 1F, or the bookstore on days off; no rain | **Rank 9**: "I really like you." | "You're my classmate." / "You're my friend." = platonic |
| Rise (Lovers) | 7/23, automatic | Fri/Sat/Sun; 1F hallway, or the tofu shop on days off; no rain | **Rank 8**: "Hold her" | "Stand there" = platonic |
| Naoto (Fortune) | Earliest 10/22. Needs Knowledge 5 + Courage 5: talk to the man in black (North Shopping District, from 10/21), then Naoto. RPGSite says 10/21, hxchector 10/23 | School days only. Fandom: Mon/Tue/Wed/Sat and also on rainy days. RPGSite adds Sun and says no rain **(they disagree)** | Two flags are needed. **Rank 6**: "I'm glad you're a girl." **Rank 8**: "Protect Naoto", then "Because I love you." (this line only appears if both flags are set) | "Your gender doesn't matter.", "Run with Naoto", "Fight back". Rank 5 "You're on your own." reverses the link |
| Ai (Moon) | Talk to her after school (from 4/27) to set a flag, then Strength link Rank 4 + Courage 3. Start is 5/13 at the earliest | Wed/Thu/Fri; Classroom 1F; she invites you to skip class | Real romance is at **Rank 10**: "Accept her". You only get that choice on the Friendship route, so at **Rank 6** pick "......", then "Are you sure about that?" | Rank 6 "I'll be your boyfriend." or "Sounds good." starts the **False Romance**: she breaks up with you at Rank 9 and the link ends as friends. On that route, Rank 9 "We need time apart." **breaks the link permanently**. Reverses: Rank 3 "That was downright cruel...", Rank 5 "Not gonna happen!" |
| Yumi (Sun, Drama Club) | 4/25, join the Drama Club | Mon/Tue/Thu per Fandom and hxchector; Fandom says also in rain. RPGSite: Mon/Tue/Fri **(they disagree)** | **Rank 10**: "Hug her" | "Leave" = platonic |
| Ayane (Sun, Band) | 4/25, join the Band | Same as Yumi | **Rank 8**: "Reveal your feelings for her" | "Change the subject." = platonic |
| Marie (Aeon, Golden only) | 4/18: Velvet Room → "Check on Dwellers" → "Listen to Marie's request" | Wed/Sat/Sun outside the Velvet Room (Rank 1 can also start in rain). Not available while you still need to gather info on a TV victim. Every day from Magatsu Inaba (12/8) to 12/23 | **Rank 9**: "Because I love you.", then "Hug her" | "Don't hug her" turns it back to platonic |

**"Locked in"**
- Picking the Lovers option makes the relationship official at that moment, and the "A Special Lady" trophy pops ("Enter a special relationship").
- Rank 10 then plays its romance version. Both versions give the same max-rank item.
- The branch choice is permanent for that playthrough. No source describes a way to break up.
- The only exceptions are Ai's False Romance, which she ends herself at Rank 9, and the Break option above.

**Marie's special rules**
- **Rise:** RPGSite says Rise must be on the team before Rank 4→5. Fandom just says she can't pass Rank 4 until 7/24. Rise joins around 7/23, so these are effectively the same gate.
- **Extra hangouts:** Rank 6 and Rank 10 each need one more hangout before the rank-up triggers.
- **Deadline:** She must be maxed by 12/23. Her link freezes once Adachi and Ameno-sagiri are beaten (Fandom, TheGamer). hxchector says "end of December" and some sites say 12/3. **Use 12/22** if you also want Christmas Eve.
- **After Rank 10:**
  - She disappears.
  - 1/2: agree to look for her when Margaret asks.
  - Margaret calls with updates on Wednesdays.
  - 2/12, ski trip: the Hollow Forest is found.
  - 2/13: rescue her ("Welcome Back" trophy).
- **Valentine's and epilogue:** She gives you chocolate on 2/14 if her link is maxed, even if you didn't romance her. In the Golden epilogue she says she loves you on TV, but only if she's your girlfriend.

**Yumi/Ayane** is one Sun link. You can only join one culture club, so you only get one of them per playthrough.

**Dating more than one girl** is allowed. Any combination works, and there's no gameplay penalty, but there are scenes about it.

**Holiday and post-romance events:**

| Date | Event |
|---|---|
| 7/25 (Golden) | Say which girl you're looking forward to seeing for +3 points (Chie, Yukiko or Rise) |
| 8/21 (Golden festival) | Go with one girl and wish to get closer for +3. For Chie, Yukiko and Yumi this needs you to be dating (Yumi also works below Rank 3) |
| 12/22 | **Last day to defeat Adachi and still get the Christmas date** |
| 12/23 | Each girlfriend texts an invitation. Decline until the one you want comes up, then accept. The order isn't documented **(uncertain)** |
| 12/24 | Christmas Eve date with one girl and a gift accessory: Chie – Fluffy Muffler, Yukiko – Leather Keychain, Rise – Silver Bangle, Naoto – Handmade Watch, Ai – Rugged Ring, Yumi – Fancy Mini-Car, Ayane – Hand-Knit Mitten. Chie and Yukiko ask to stay the night. **Marie has no Christmas Eve date** |
| 1/1 (Golden) | Shrine visit with a girlfriend. How one is picked if you have several isn't documented **(uncertain)** |
| 1/2 | New Year's greetings from all links, then the snowman with Nanako and Dojima. This is my best guess for your "snow day"; the ski trip on 2/11–12 is the other candidate |
| 2/14 (Golden, good-ending route only) | Pick one girl. Players report sad scenes from rejected Chie, Yukiko, Rise and Naoto, while Ai, Yumi and Marie don't react badly. This comes from forum posts only **(anecdotal)**. Nanako's chocolate scene also plays |
| 3/20 | Goodbye scenes with every maxed link. I didn't confirm what changes for a girlfriend **(unverified)** |
| Golden epilogue | Needs the True Ending plus Marie maxed and the Hollow Forest cleared. The protagonist returns to Inaba that summer |

**Relevant trophies:** A Special Lady, A True Bond (max one link), Bond Maniac (10 links), Legend of Inaba (all links, so Ai must reach Rank 10 and must not be broken), Welcome Back, A Favor for Marie.

## Part B — Bustups in your PC install (verified on your files)

Your install is the 64-bit build. Its `data.cpk` is a **standard CRI CPK**, not the PreApp Partners format that ShrineFox describes. That format, and TGE's `preappfile` tool, belong to the old 32-bit 2020 build.

**The archive**
- The file starts with the `CPK ` magic and an `@UTF` header written by `CPKMC2.49.37`.
- The table of contents sits at 0x800, is not encrypted, and lists 47,178 files.
- Its columns are DirName, FileName, FileSize, ExtractSize, FileOffset, ID and UserString.
- A file's real offset is **FileOffset + 0x800** (the TocOffset).
- When FileSize is smaller than ExtractSize, the file is **CRILAYLA**-compressed:
  - the header holds the uncompressed size and the offset of a raw 0x100-byte prefix;
  - the rest is a backward bitstream.
  - A pure-Python decoder took about 3 seconds per bustup.

**The bustup files**
- Path pattern: `bustup/b{charID}_{expression}_{outfit}.bin`. There are 1,010 of them.
- Outfit 1–6 = summer/winter/mid-winter uniform and casual. Outfit 0 = costume or special sets.
- None are in `data_e.cpk` or the other language CPKs.
- Each `.bin` is an Atlus PAK v2 container (little-endian): a u32 entry count, then for each entry `char name[32]`, a u32 size, and the data.
- Every bustup I checked has two entries:
  - `b3_1_1.tmx`: 1024×1024.
  - `m3_1_1.tmx`: 1024×256, probably the eye-blink frames (Amicitia mentions "blinking sprites"). **Where the game positions it over the portrait is unknown.**

**The TMX texture inside**
- 64-byte header: u16 = 2, u32 length, `TMX0`, palette count 0, pixel format 0 (PSMCT32), no mipmaps. Pixel data starts at +0x40.
- Pixels are 32-bit with no palette, stored linearly and upright.
- I'm assuming the byte order is RGBA. I didn't check colours visually.
- **Alpha runs 0–128**, the PS2 convention, so multiply by 2 and clamp at 255.
- The figure fills roughly the top 750 rows.

**Character IDs** (file counts in brackets): Chie 3 (101), Yukiko 4 (112), Rise 5 (95), Naoto 7 (83), Ai 28 (27), Yumi 30 (32), Ayane 31 (23), Marie 60 (20; `b60_{1–14,20–22,30,31,40}_0`). Amicitia's bustup page has the per-character expression lists, e.g. Chie 6 = blushing, Rise 7 = crying.

**Other portrait sets**
- `commu/bustup/com_kyaraXX_*_s|_w.tmx`: Social Link screen portraits in summer/winter versions. **I haven't confirmed how the XX numbers map to characters.**
- `camp/bustup/c_cha0{1-8}{a,b}.tmx`: party menu portraits.
- Most other textures in the CPK are DDS, but the bustups stay TMX.

**Tools and docs:** CriFsV2Lib / CriFsLib.GUI (the extraction guide lists P4G's `data.cpk`), Amicitia (TMX, PAC/BIN), PAKPack, and Reloaded-II with Persona Essentials. GameBanana has P4G bustup replacement mods, e.g. Pikachu over Yukiko.

## Part C — Daily activities

- **Social Link hangouts (day):** on each character's set days; rain cancels most. Matching-Arcana Personas give bonus points. Reverse and Break choices exist (Ai, Naoto).
- **Evening hangouts (Golden):** from 4/19, in the Central Shopping District at night.
  - Possible when Dojima is away, or any night once you've taken an outside evening job.
  - They give points (+2/+3 depending on your answer) and an item, but never a rank-up.
  - They're tied to specific dates (Chie: 4/20, 5/1, 5/4, 6/1, 6/15, 6/26, 7/3, 8/3, 9/4, 9/11, 10/2 or 10/23, 11/2, 12/11). **Missable.**
- **Scooter (Golden):**
  - **Unlock:** license around 6/8–6/9 through the story (unmissable). Riding raises Courage.
  - **Places:**
    - A few neighbourhood rides unlock Okina City: Chagall Café gives a skill card for your equipped Persona, the 30 Frame cinema boosts Persona stats (Movie Buff trophy), and there's a crane game.
    - Repeated "Long Rides" then unlock Shichiri Beach: fishing (Sea Guardian / Fishing Master trophy), gazing at the sea for a new skill, and swimming for +Courage. The beach is also needed for a Fox quest.
  - **Bike skills:** invite party members waiting at Moel Gas Station after school. Each has 5 bike-only skills; Rise gets an All-Out Attack boost instead. **Unique; missable if you never invite them.**
- **Clubs:**
  - Sports (from 4/19): Basketball (Kou) or Soccer (Daisuke) leads to Strength. hxchector says Tue/Thu/Sat, while Fandom's club table lists Basketball as Mon/Tue/Wed/Sat **(they disagree)**. Strength Rank 4 is needed for Ai.
  - Culture (from 4/25): Drama or Band leads to Sun, Mon/Tue/Thu. You can only pick one.
- **Part-time jobs** (start 4/23; night jobs open 5/25; outside evening jobs also let you go out at night):

| Job | When | Requirement | Gain / link |
|---|---|---|---|
| Envelopes | At home, any day | — | Diligence |
| Origami cranes | At home | Diligence 2 | Understanding, unpaid |
| Translator | At home | — | Expression |
| Day-care | Mon/Fri/Sat daytime | — | Understanding, ¥4,000+; Eri (Temperance) |
| Hospital janitor | Wed/Thu/Fri evenings | Diligence 3 | Courage; Sayoko (Devil), which leads to Hisano (Death) |
| Tutor | Tue/Thu/Sat evenings | Understanding 5 | Knowledge + Expression, ¥10,000; Shu (Tower). **Closes once Shu is maxed** |
| Pub dishwasher (Golden) | Nights | — | Diligence + one other stat |

- **Other time slots:** study, books, fishing, gardening/bug catching, Aiya's Mega Beef Bowl on rainy days, Fox and Margaret requests, TV dungeons.
- **Time-limited, Golden-specific:**
  - Girlfriend calls on Sundays and holidays (movie dates and the like).
  - The 7/25, 8/21, 12/24, 1/1 and 2/14 events from Part A.
  - **From 1/11:** hanging out with maxed party members triggers their third-tier Persona awakening.
  - Marie's Rank 10 by 12/23, which unlocks the Hollow Forest and the epilogue.
  - Adachi defeated by 12/22 for Christmas.

**Sources:**
- Fandom: [Social Link/Chie Satonaka](https://megamitensei.fandom.com/wiki/Social_Link/Chie_Satonaka) (similarly `/Yukiko_Amagi`, `/Rise_Kujikawa`, `/Naoto_Shirogane`, `/Ai_Ebihara`, `/Yumi_Ozawa`, `/Ayane_Matsunaga`), [Social Link/Marie](https://megamitensei.fandom.com/wiki/Social_Link/Marie), [Calendar/Persona 4](https://megamitensei.fandom.com/wiki/Calendar/Persona_4), [Scooter](https://megamitensei.fandom.com/wiki/Scooter), [Part-time Jobs](https://megamitensei.fandom.com/wiki/Part-time_Jobs), [School Club](https://megamitensei.fandom.com/wiki/School_Club), [List of Persona 4 Activities](https://megamitensei.fandom.com/wiki/List_of_Persona_4_Activities), [List of Persona 4 Golden Trophies](https://megamitensei.fandom.com/wiki/List_of_Persona_4_Golden_Trophies)
- RPGSite: [social link hub](https://www.rpgsite.net/feature/9850-persona-4-golden-social-link-guide-dialogue-options-love-interests-and-full-s-link-walkthroughs), [Marie](https://rpgsite.net/feature/9849-persona-4-golden-marie-aeon-social-link-choices-unlock-guide), [Ai](https://www.rpgsite.net/feature/9834-persona-4-golden-ai-ebihara-moon-social-link-choices-unlock-guide), [Naoto](https://rpgsite.net/feature/9846-persona-4-golden-naoto-fortune-social-link-choices-unlock-guide), [scooter](https://rpgsite.net/feature/9900-persona-4-golden-scooter-riding-bike-skills-unlocking-okina-city-explained)
- Other guides: [TheGamer – Marie](https://www.thegamer.com/persona-4-golden-p4g-marie-aeon-social-link-romance-guide/), [hxchector – social links](https://hxchector.com/persona-4-golden-walkthrough/social-links), [hxchector – Hollow Forest](https://hxchector.com/persona-4-golden-walkthrough/hollow-forest)
- GameFAQs (search snippets only): [Christmas Eve thread](https://gamefaqs.gamespot.com/boards/641695-persona-4-golden/69558799), [Valentine's thread](https://gamefaqs.gamespot.com/boards/641695-persona-4-golden/64779612)
- Megaten wiki (blocked, cited from search results only): [Calendar in Persona 4 Golden](https://megatenwiki.com/wiki/Calendar_in_Persona_4_Golden)
- Modding: [Amicitia – data.cpk/bustup](https://amicitia.miraheze.org/wiki/Persona_4_Golden/data.cpk/bustup), [Amicitia – TMX](https://amicitia.miraheze.org/wiki/TMX), [ShrineFox – P4G PC](https://docs.shrinefox.com/getting-started/persona-4-golden-pc-mod-support), [Persona modding docs – extracting files](https://animatedswine37.github.io/persona-modding-docs/docs/getting-started/extracting-files)
