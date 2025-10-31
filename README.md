# Lands of Lore CLI

## build
```bash
make
```

note: SDL2 is required.

## game assets
You need to create a "data" folder in the root of the repo and copy all ‘pak' files into it.



## Usage
```bash
./lol game 
./lol game  ~/dosbox/WESTWOOD/LOLCD/ # specify a dir containing saved games
./lol game  ~/dosbox/WESTWOOD/LOLCD/_SAVE000.DAT #directly start from a saved game
```

## Exploring game assets
The program also exposes a list of commands to extract or inspect game assets like CPS, WSA, or LANG files. to see the complete list of available commands: 
```bash
./lol -h
```

## What's working, what's not
Most of the game logic and rendering code is setup, but there's still a lot to cover. Next big thing to tackle is to correctly render WSA/TIM animations.

## Notes

Looks like some palette are embedded in the bin and not shipped in the pak files. ([see forum](https://www.dungeon-master.com/forum/viewtopic.php?t=23792)). Currently the default palette is taken from `GERIM.CPS` in chapter1.pak.
