# 40KLearn
An attempt to create an agent to play the 40K tabletop game
through Reinforcement Learning. Roughly follows the AlphaZero
architecture, but with a much smaller and simpler neural network
due to lack of computing power.

The RL agent will use Monte Carlo Tree Search (MCTS), and a
convolutional neural network with a value head and a policy
head, which guides the tree search algorithm.

The Warhammer 40K rules are available freely online, however the
game has been simplified for the purposes of this project in the
following way:
- Units are fixed to moving on a coarse grid,
- All units in a squad must have exactly the same unit stats,
- No Psykers,
- No stratagems or other global abilities,
- No aura special rules,
- No randomness in determining statistics (e.g. a weapon with a random number of shots),
- No line of sight or cover

It should be easy to incorporate global abilities (e.g. an orbital bombardment)
and psykers with exactly one psychic power, but this is currently not the case.

## Project Structure

The performance-intensive parts of the project are written in C++ and
are exposed to Python via a wrapper (using Boost Python). The core game rules
and the search tree implementation are in the Core40KLearn C++ project, which
builds into a DLL. This project has tests in the Core40KLearnTests project.
The core project is also wrapped, to expose it to Python, in the py40kl project.
The game's (very basic) GUI uses model/view/controller, where the View uses
Pygame to render, and the model is just a wrapper around a core GameState object,
and the controller is either a human player or an AI controller.

In the core project, the main classes are:
- GameState: this is an immutable object encapsulating all info about a particular
  moment in a game. In order to "modify" it, you would need to create a copy of its
  board state, modify that board state, and then create a new game state object with
  that board.
- BoardState: this represents the state of the board, but does not know which player's
  turn it is, etc.
- Unit: this represents all of the game statistics of a single unit (which may consist
  of many models). Units also store flags representing 'what they have done this turn',
  for example, a unit cannot move twice per turn, so it contains a flag which is true
  if and only if it has already moved this turn, so the game rules prevent it from moving twice.
- IGameCommand and IUnitOrderCommand: these abstract classes represent 'commands' in the
  game, which are basically functions which operate on the game state, and return a
  probability distribution over resulting game states. Unit order commands, a specialisation
  of game commands, have "source" and "target" positions. The source position is "who is
  doing the action", and the target position is "what is the action being done to." Their
  use is dependent on the type of action.

## Installation Instructions

This project relies on the Boost C++ libraries. Ensure they are installed and built.
The root of the Boost library directory is stored in the environment variable
BOOST_DIR. Ensure that the include files are in BOOST_DIR/boost, and that the
built Boost binaries are in BOOST_DIR/stage/lib.

Python dependencies:
- NumPy,
- TensorFlow,
- Pandas,
- PyGame,
- Dask

Note that Boost Python needs to be built with this version of Python, in order for the
Boost Python wrapper around the C++ classes to work. We need the environment
variable PYTHONPATH to point to the root directory of Python, and its DLL subdirectory,
its include subdirectory, and its static library subdirectory. This is all just
to get Boost Python to build properly.

Now you should be at a point where the core C++ project should build (Core40KLearn),
and its unit tests should build (which depend on Boost.Test), and the Python wrapper
project should build (py40kl). The DLLs for the core project and the Python wrapper
are automatically copied to the project's root directory after building, which is
where they are intended to be ran from.

Finally, put the Boost Python and Python DLLs in the root directory of the project,
once built. This allows them to be found when the game is ran. Run the file "game.py"
to play the game against an AI player.

## TODO

- Extend the experience dataset to perform random transformations of the board such
  as rotation, reflection (there are 8 symmetries.) This is useful because it doesn't
  change the board's value, and shouldn't change the policy (after applying the
  transformation to the policy, also.)
- Train neural network and incorporate into GUI. Remove the 'end phase bias' after a
  period of training, and then continue training for a bit.
- Restructure Python code into packages, fix Linter errors throughout the code.
- Redo the MCTS tree to enforce an order on which units to order. This should reduce
  branching factor. Then, update neural network policy output.
- Parallelise the self-play manager.
