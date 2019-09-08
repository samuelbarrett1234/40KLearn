# 40KLearn
An attempt to create an agent to play the 40K tabletop game
through Reinforcement Learning. Roughly follows the AlphaZero
architecture, but with a much smaller and simpler neural network
due to lack of computing power.

The RL agent will use Monte Carlo Tree Search (MCTS), and a
convolutional neural network with a value head and a policy
head, which guides the tree search algorithm.

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

## TODO

- Create an ExperienceDataset class, which has the job of recording a large
  list of experiences (which are in the form <game state, value, policy>)
  and can produce a sample of these on request.
- Create the neural network class, which can predict values and policies given
  game states, and also can train on a dataset of game states, game values, and
  policies.
- Once the neural network agent is complete, it should be incorporated into
  the graphical user interface which is already complete.

