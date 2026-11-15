# Who is around, and where

How the Today page decides who can be found in the current time slot, and where. These are
the rules the [Find-a-Friend](https://github.com/AnimatedSwine37/Find-a-Friend) mod uses
(its script is also in ShrineFox's P4G Mod Menu), rewritten in `Features/WhoIsAround.cpp`.
Verified live on 4/22 (Friday) evening: Yosuke's evening flag was set but so was his block
flag, and he wasn't in the shopping district.

Flag ids below are absolute. Sections: S1 = 0x400 + n, S2 = 0xC00 + n.

## Time

`GET_TIME_OF_DAY` runs 0–5: early morning, morning, lunch, afternoon, after school, evening
(the game's own table at `0x14090C0A8`). "Holiday" means the daytime slot is Afternoon (3),
which is how Sundays and holidays look, plus 4/12.

## The main friends

| | Day set A | Day set B | Evening on | Evening blocked | Busy (Marie outing) | Scooter trip |
|---|---|---|---|---|---|---|
| Yosuke | 1456 | 1478 | 2176 | 3955 | 2240 | 1650 |
| Chie | 1458 | 1482 | 2177 | 3956 | 2241 | 1652 |
| Yukiko | 1459 | 1476 | 2178 | 3957 | 2242 | 1649 |
| Kanji | 1457 | 1481 | 2179 | 3958 | 2243 | 1651 |
| Rise | 1460 | 1475 | 2180 | 3959 | 2244 | 1648 |
| Naoto | 1485 | 1486 | 2181 | 3961 | 2245 | 1653 |

- Daytime on a school day: flag 2593 (S1+1569) set means set A is the one in use, otherwise
  set B. On a holiday either set will do.
- Evening: the evening flag must be on, the block flag off, and the link started.
- Never: the link is maxed, flag 87 is set (nobody is around), or the busy flag is set.
  Naoto is also away from 11/6 to 11/20 while 2593 is set.
- A scooter trip (daytime, 2593 off, 87 off) is offered separately from the hangout, by the
  scooters in the south shopping district. With one on, Find-a-Friend looks for Yosuke, Yukiko,
  Rise and Naoto by the scooters for their hangout too; Chie and Kanji stay at their usual place.

Places: after school Yosuke is on Learning Building 2F, Chie on the rooftop, Kanji on Practice
Building 1F, Yukiko, Rise and Naoto on Learning Building 1F. On holidays: Yosuke at Junes,
Chie, Yukiko and Rise in the south shopping district, Kanji in the north, Naoto at the Samegawa
flood plain. Evenings: Yosuke, Chie and Kanji in the north shopping district, Yukiko, Rise and
Naoto in the south. (Find-a-Friend checks for a holiday before the evening, which only matters
on 4/12.)

Evening hangouts with friends (and with Ai, Yumi or Ayane, Kou or Daisuke, and Naoki) give points
and an item but never a rank-up. Nanako, Dojima, Sayoko, Shu and Adachi are different: their
own link events happen in the evening.

## Everyone else

| Who | Daytime | Evening on / blocked | Other conditions | Where |
|---|---|---|---|---|
| Ai | 1492 | 2187 / 3966 | needs the Strength link at rank 4 to start; 2512 keeps her away | Learning Building 1F; evenings south |
| Yumi / Ayane | 1499 | 2184, 2185 / 3968 | one or the other (whichever link has started) | Learning Building 2F once started, Practice Building 1F before; evenings south |
| Kou / Daisuke | 1487 | 2182, 2183 / 3967 | one or the other; no evenings on 10/3 (set B) or from 10/5 to 1/1 | Learning Building 2F once started, 1F before; evenings north |
| Naoki | 1489 | 2186 / 3963 | | north shopping district (Learning Building 1F on school days before the link starts) |
| Hisano | 1490 | — | needs the Devil link (Sayoko) at rank 4 | Samegawa riverbank |
| Nanako | — | 1473 | evenings only | Dojima residence |
| Dojima | — | 1479 | evenings only, not while 2593 is set | Dojima residence |
| Sayoko | — | 1480 and 2664 | evenings only | south shopping district |
| Shu | — | 1494 and 2662 | evenings only | south shopping district |
| Eri | 1500 and 2665 | — | daytime only | south shopping district |
| Marie | 1736 | — | daytime only; away while 1551 is set and her link hasn't started; before 7/23 her rank 4 waits for an outing | south shopping district |
| Teddie | scooter trips only (1654), not while 2593 is set | | | south shopping district |
| Adachi | 1716 | — | up to rank 6, not while 2593 or 1726 is set; when a hangout would rank up, ranks 1 and 5 happen by day and 2–4 in the evening (evenings only then) | Junes; evenings south |
| The fox | — | — | ranks 1–9, by day: its next shrine request not taken yet, or a finished request not yet ranked up for (below) | Tatsuhime Shrine |

The fox's request for each rank is a quest (flags 255 + quest when taken, 319 + quest when done):
quests 6, 13, 20, 26, 30, 36, 41, 42, 48 for ranks 1–9. Flags 2720–2728 (S1+1696–1704) record
the rank-up after each.

Find-a-Friend's Marie check also calls a script function whose meaning isn't known; the
trainer treats it as "her link has started".
