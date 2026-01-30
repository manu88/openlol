# Lol SAV format

## Layout
Here's the rough SAV layout, see the [source code](../src/common/formats/format_sav.c) for more details.
```
0X000 Header:
    sav name
    ...
    sav type
    ...
    version

0X04C: Characters[3]:
    flags
    name
    ...

0X254: Global values:
    current block
    ...

0X487: Game objects

0X1D87: Per-level temp data[30]: # temp level data are here even if empty, meaning that the sav file has always the same size 
    Monsters[30]:
        ...
    FlyingObject[8]:
        ...

```

## Notes
There are 30 per-level temp data slots, yet the game only has 29 levels. Looks like the game doesn't mind if the SAV file only contains 29 per-level temp data slots.
