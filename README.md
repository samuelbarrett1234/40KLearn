# 40KLearn
An attempt to create an agent to play the 40K tabletop game
through Reinforcement Learning.

This project aims to extend the AlphaZero architecture to a
game with nondeterminism: 40K tabletop.

The agent will be using MCTS, guided by a policy network, and
using a value network instead of simulation.

The 40K game will be simplified in the following way:
- Units are fixed to moving on a coarse grid,
- All units in a squad must have exactly the same unit stats,
- No Psykers,
- No stratagems or other global abilities,
- No aura special rules,
- No randomness in determining statistics (e.g. a weapon with a random number of shots),
- No line of sight or cover

In the future it may be easy to work in global abilities (e.g. an orbital
bombardment) and psykers with exactly one psychic power, but this is currently
not the case.

