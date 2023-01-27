## Overview
AlphaEngine is a free, open-source UCI chess engine written in C++ using bitboards for board representation. The project is a school project. The code is written in english exept the comments which are primarily in dutch. Everything is free to copy and can be used however needed.

## Credits:
Parts of the code is taken from following open source chess engines:  

[Name Engine] - [Name Authors]  
BBC           - Maksim Korzh or "Code Monkey King"  
Vice          - BlueFeverSoft  
Fruit         - Various Authors  
CPW-Engine    - Pawel Koziol and Edmund Moshammer  
Stokfish      - Various Authors  

## What to expect:
The latest version includes:
* Bitboard board representation.
* Evaluation function including:
  * Material scores.
  * Positional scores by piece-square tables.
  * Open file recognition.
  * Bishop/rook/knight pair recognition.
  * Piece score based on amount of pawns on the board.
  * Tapered evaluation.
  * Basic pawn evaluation.
  * Basic mobility.
  * Basic king safety.
  * Basic cohesion.
  * Basic endgame draw evaluation.
* Move ordering by:
  * Pv-move.
  * Captures by MVV-LVA.
  * Killer moves.
  * History moves.
* Transposition table.
* Draw recognition by threefold repetition and 50-move rule. 
* Negamax search using:
  * Queiscence search.
  * Null move pruning.
  * Razoring.
  * Aspiration window.
  * Mis wrs nog wat hier......
* Iterative search.
* UCI commands including time settings.
* Global hash table.

## Possible upgrades:
* Make the code readable by everyone by updating comments.
* Increase speed:
  * Faster move generation
  * Parallel search (probably lazy smp).
* Update evaluation function by NNUE or automated tuning (like "Texel's Tuning Method").
* Add an opening book.
