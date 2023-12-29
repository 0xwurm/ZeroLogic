# ZeroLogic

A simple chess engine for graphing calculators. 

**Latest tests: 2000 elo | 1,800,000,000 nps**

## Use

We use the uci-protocol for communication with the engine. At the moment the only possible search mode is 'depth', 
'movetime' and such are in the works.


These are some extra commands supported at the moment:

**'go single (depth)'** does a depth first search for the given depth (instead of the usual iterative deepening - 
breadth first approach).

**'go see (move)'** does a static exchange evaluation on a given move. 

**'go eval'** returns the static evaluation of the position.

**'d'** displays the fen and Zobrist key of the internal gamestate.

## Compilation

To compile for systems supporting the *PEXT*, *tzcnt*, *popcount* and *bslr* instructions use the included CMake file.

To compile for a TI calculator use the [ndless-sdk](https://ndless.me/) and add the following flags in the Makefile it generates:
```
GCCFLAGS = -Wall -W -marm -std=c++23
LDFLAGS = -Wl,--nspireio
```

## Speed (Movegeneration)

We reach a speed of up to 1,800,000,000 nodes (1.8 Gn) per second per thread in Movegeneration (Tested on a Core i5-11400H), 
making the Open Source Abstract Movegenerating Algorithm (**OSAMA**) used in this program the fastest chess movegenerator 
in the world! In the time it takes to calculate one legal position, a photon would travel about 16cm.

```
Perft 8:
Overall nodes: 84,998,978,956
Time taken: 46.982s
Mn/s: 1809
s/Mn: 0.000552746
light distance equivalent: 0.165709 meters
tt hits: 129,787,406
```

This speed is due to heavy use of function inlining, templates and a transposition table.
The use of a transposition table triples movegeneration speed, but also results in the movegenerator *technically* not 
being perfect, due to the possibility of hash collisions, however I have never observed that.

The calculator version is capable of generating ~2 Meganodes per second. (Tested on a TI-nspire CX II-T)

Movegeneration is heavily inspired by Daniel Infuehr's [Gigantua](https://github.com/Gigantua/Gigantua/tree/2e82933789af6d83e7bfa2500b3de92e1698ddff).

## Calculator

Playing ZeroLogic on a calculator is quite a cumbersome undertaking, as you have to speak fluent uci so to say 
(Meaning no GUI is included). On a well-supported OS like Windows/Linux/Mac, where Chess GUIs are very easy to find,
this is not a problem. On TI calculators, however it makes the program virtually useless. That is why an integrated 
GUI for the calculator version is the top priority right now.
