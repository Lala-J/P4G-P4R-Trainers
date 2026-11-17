# Jobs, activities and the weather

How the Today page decides which jobs and activities are on offer (`Features/Activities.cpp`).
Everything here was read from the game's own scripts (`field/script/*.bf`, `scheduler/*.bf`)
and checked live on 5/7, a rainy Saturday.

## The daily schedule

Every morning the scheduler clears a block of "today" flags (`cmu_npc_flag_init`) and switches
on the ones for the date (`cmu_npc_flag_on`). That procedure is a plain date table: it only
calls `GET_MONTH`, `GET_DAY_OF_MONTH` and `BIT_ON`, and it is the same in every month's script.
P4G's weather is fixed for each date, so the table already leaves out the days a job can't
run. On 5/7 the table gives flags 1473, 1494, 1519 and 1520, exactly what the live game had
on; the day-care flag 1500 isn't among them, because it rains that day.

The same flags tell [Find-a-Friend](around.md) who is around: Eri's daytime flag is the
day-care's, Sayoko's evening flag is the hospital's, Shu's is the tutoring job's.

## Jobs

The job board in the shopping district (f008 `keijiban`) is up from 4/23 to 2/29 and taken
down in March. Signing up sets the job's flag; the board checks a social stat first for some
jobs. The second page of jobs is posted from 5/25 (before then it shows the day-care and
"Coming soon").

| Job | Signed up | Shift today | When | Board requires | Pay | Raises (board text) |
|---|---|---|---|---|---|---|
| Day-care assistant | 2665 | 1500 | daytime | — | ¥4,000+ | — |
| Hospital janitor | 2664 | 1480 | evening | Diligence 3 | ¥5,000+ | Courage |
| Tutor | 2662 | 1494 | evening | Understanding 5 | ¥10,000 | Expression |
| Shiroku Pub dishwasher | 2080 | any evening | evening | Understanding 2 | ¥1,500+ | Diligence |
| Envelope making | 2661 | any evening | evening | — | ¥1,000+ | Diligence |
| Translation | 2660 | any evening | evening | Knowledge 2 | ¥1,500+ | Expression |
| Origami cranes | 2659 | any evening | evening | Diligence 2 | volunteer | Understanding |

The board's own "Work Days": day-care Mon/Fri/Sat (not raining), hospital Wed/Thu/Fri,
tutor Tue/Thu/Sat, the pub any day at night.

- **Bus stop** (f008 `basutei`, south side): by day it offers the day-care when 2665 and 1500
  are both on. With the job but no shift, it says the day-care is open "Monday, Friday, or
  Saturday when it's not raining". It is closed by day on the 12/5 and 2/13 story days (flags
  1551 and 1544). In the evening it offers tutoring (2662 and 1494, until Shu's link is maxed)
  and the hospital (2664 and 1480).
- **The pub** is the Shiroku store's door at night (f008 `item_shop`). It calls the game's own
  pub function, which runs the job itself; flag 2081 then marks the evening as used (the
  scheduler clears it every morning). No script checks the job flag.
- **Desk jobs** (f007 `check_mini_desk`) can be done at your desk any evening once taken.
- The board also has a "Junes Stocker" posting, but its flags (2084, 2085) are only ever set
  by the debug menu.

## Weather

`GET_WEATHER` (common script function 0x004C) returns the current slot's weather. Its code is
in the protected part of the executable, so the trainer calls it the way a script does (through
FlowCaller) whenever the day or time slot changes. The town's scripts compare it against 0–7;
every rain check accepts 1 or 7.

- **Aiya** (f008 `tyuuka`): ¥800 specials by weekday: Mabo Tofu on Mon/Tue (Courage), Chow
  Mein on Wed/Thu (Diligence), Steak Bowl otherwise (Understanding). On rainy days the special
  is the ¥3,000 Mega Beef Bowl challenge ("several characteristics"), and every Friday in
  January and February too. Closed on rainy evenings and winter-Friday evenings ("run out of
  ingredients"), on 4/11–4/12 and 4/17, the story days above and the New Year days (flag 1542),
  and "too late to eat here" once you've been into the TV world that day (flag 3074, set on
  arrival and cleared overnight).
- **Bugs at the shrine** (f008 `musi_ap_shrine`): needs flag 3347; once a day (counter 215,
  reset every morning); no bugs in the rain, from 11/28 to 2/14, or while 1542 is set.
- **The garden** (f007 `check_farmfield`) is there once flag 3280 is set.

## Reading the scripts

Comparisons push their right-hand side first, so a disassembly that shows `2 >= R` after
`GET_SOCIAL_STAT_LEVEL(1)` means "Knowledge rank 2 or higher" (the board's message for failing
it asks for at least Informed).
