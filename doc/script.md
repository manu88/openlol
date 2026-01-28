# Lol scripting format
Lands of Lore uses a scripting language for gameplay. File extensions are `.INF` and `.INI` - the latter being reserved for level *initialization* scripts.

## File format
Scripts are binary-encoded files. They start with a `FORM` signature and then contain 3 sections:
 - `DATA`: the script instructions
 - `TEXT`: read-only strings referenced by the script.
 - `EMC2ORDR`: A list of instruction addresses/offsets

## Instructions
*TODO*


# In game usage 
Each levels comprises both one `.INF` and one `.INI` script:
 - `LEVEL1.INI` will be executed when a level is loaded, either from loading a saved game or in-game.
 - parts of `LEVEL1.INF` will be executed during the game depending on the party's position and orientation.

In addition, the game uses two additional scripts:
 -  `ONETIME.INF` script that is ran *once* at startup to initialize all in-game items.  
 -  `ITEM.INF` script contains the logic for each item.

## Offsets/functions
