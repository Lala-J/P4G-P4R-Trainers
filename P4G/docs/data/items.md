# Items and equipment

Verified live, Steam build 1.0.5.

## Item table — `*(0x1411A5940)`

The game's copy of `init/itemtbl.bin`: one 0x44-byte entry per item id (0x000–0x9FF).

| Offset | Meaning |
|---|---|
| +0x00 | kind: 0 weapon, 1 armour, 2 accessory, 3 consumable, ... |
| +0x08 | weapon attack |
| +0x0A | weapon accuracy |
| +0x10 | armour defence |
| +0x12 | armour evasion |
| +0x2C | price |
| +0x30 | who can use it: bit (character id + 4); 1 Yu, 2 Yosuke, 3 Chie, 4 Yukiko, 5 Rise, 6 Kanji, 7 Naoto, 8 Teddie |

`0x1401A0220 (i16 item, i16 character)` is the game's own "can this member use it" check
(it tests that bit).

## Equipping

- Equipped pieces are not counted in the bag: equipping takes one out, the old piece goes back in.
  Outfits (0x700–0x7FF) are a wardrobe: owning one is enough (Default Clothing is held at 99).
- The swap is the script function `SET_EQUIPMENT (member, slot 0 weapon / 1 armour / 2 accessory /
  3 outfit, item)`, which (as the script library notes) does no bag bookkeeping itself.
- Party unit +0x4C holds the four equipped item ids.

## Enemy records — `*(0x140EC0920)`

0x3C bytes per enemy id: +0x02 arcana, +0x03 level, +0x04 HP, +0x06 SP, +0x08 St Ma En Ag Lu,
+0x0E skills[8], +0x1E EXP, +0x20 yen, +0x22 drops: {item, chance per 1000} x 3, +0x3A AI index.

Encounters (`battle/ENCOUNT.TBL`, in init_free.bin): 944 x 24 bytes, enemy ids at +0x08..+0x11;
the third segment holds 68 encounter pools of 252 bytes ({encounter, weight} pairs, plus a rare
group). How the game picks a pool for a floor is not mapped yet, so the trainer doesn't list
where a shadow appears.
