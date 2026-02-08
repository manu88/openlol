# Some special files 
- "STARTUP.PAK/ONETIME.INF": as the name says, it must be exec one, at startup
- "STARTUP.PAK/ITEM.INF": ditto, at startup.

# Extensions
https://web.archive.org/web/20180313235625/https://eob.wikispaces.com/lol.files for more details

- ‘.DIP‘ : Strings, same format as .ENG/.FRE/etc.
- ‘.SHP‘ : Sprites sheet, see https://moddingwiki.shikadi.net/wiki/Westwood_SHP_Format_(Dune_II). **Watch out!** Some SHP files are LCW encoded!
- ‘.CPS‘ : images https://moddingwiki.shikadi.net/wiki/Westwood_CPS_Format
- ‘.WSA‘ : animations. see https://web.archive.org/web/20070119221145/http://eob.wikispaces.com/lol.wsa?token=d90db02368ef4405b16fea9347e22287 and https://moddingwiki.shikadi.net/wiki/Westwood_WSA_Format
- ‘.TIM‘ : Timing and other animation WSA information?
- ‘.VMP‘ : The vmp files contains information about how to put together the 8x8 blocks defined in the corresponding vcn files, into proper walls (including the background).
- ‘.VCN‘ : .vcn files contains graphics for the walls including the background.
- ‘.LBM‘ : IFF https://moddingwiki.shikadi.net/wiki/LBM_Format, use this parser? https://github.com/svanderburg/libiff
- ‘.XXX‘ : map legend data, see https://github.com/scummvm/scummvm/blob/2816913460d4bd73795ca1356059b491f7248b05/engines/kyra/engine/lol.cpp#L4257
- ‘.WLL‘ : Wall decorations & definitions
- ‘.FRE‘ : FREnch text compressed with a Dizio-Encoding
- ‘.INF‘ : Script data similar to Eob inf, Data is arranged using Legend of Kyrandia's script format (info about Dune II's EMC: https://web.archive.org/web/20080922215518/http://minniatian.republika.pl/Dune2/EMC.HTM)
- ‘.INI‘ : INF script in disguise, used for level initialization.
- ‘.CMZ‘ : EoB maze definition, but compressed with Format 80
- '.DAT' : level decoration data 
- '.FNT' : font files, using Westwood Font Format v3 see https://moddingwiki.shikadi.net/wiki/Westwood_Font_Format_v3 and https://web.archive.org/web/20180313235144/http://eob.wikispaces.com/eob.fnt
- '.TLK' : PAK file in disguise. Contains in game voices in the VOC format
- '.VOC' : audio files
- '.TLC' : some kind of transparency table ? see [here](https://github.com/scummvm/scummvm/blob/master/engines/kyra/engine/scene_lol.cpp#L460)
- '.ADL' : Adlib compatible music. See [here](https://moddingwiki.shikadi.net/wiki/ADL_Format)
- '.XMI' : eXtended MIDI file format. See [here](https://moddingwiki.shikadi.net/wiki/XMI_Format)
- '.C55' : Probably SC-55 compatible music.

## format 80
Looks like this 'format 80' refers to 2 different compression format:
* in case of graphical data, 'format 80' is LCW,
* else 'format 80' is LZ77.

The reason? My guess is Westwood had to stop using LZ77 for graphical data, because of Unisys' enforcement of their patent on the LZW algorithm, but they still used it for non-graphical data.

More information here:
https://www.kyzer.me.uk/essays/giflzw/

## CMZ
Uses LZ77 compression!

## CPS
Uses LCW compression!

## VCN
Uses LCW compression
see https://web.archive.org/web/20180313235313/http://eob.wikispaces.com/eob.vcn#LoL

## VMP
Uses LCW compression
see https://web.archive.org/web/20180313235716/http://eob.wikispaces.com/eob.vmp

# INF
See https://github.com/scummvm/scummvm/blob/master/engines/kyra/script/script_lol.cpp#L2683

# DAT
See https://web.archive.org/web/20180313235324/http://eob.wikispaces.com/eob.dat
uses the VCN file palette

# FRE
lang files, see https://web.archive.org/web/20070119182153/http://eob.wikispaces.com/lol.dizenc?token=a26df2e3f5c81f05411e86759caf84f6
