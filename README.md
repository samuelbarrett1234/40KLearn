# 40KLearn
An attempt to create an agent to play the 40K tabletop game
through Reinforcement Learning. Roughly follows the AlphaZero
architecture, but with a much smaller and simpler neural network
due to lack of computing power.

The RL agent will use Monte Carlo Tree Search (MCTS), and a
convolutional neural network with a value head and a policy
head, which guides the tree search algorithm.

## Project Structure

The performance-intensive parts of the project are written in C++ and
are exposed to Python via a wrapper (using Boost Python). The core game rules
and the search tree implementation are in the Core40KLearn C++ project, which
builds into a DLL. This project has tests in the Core40KLearnTests project.
The core project is also wrapped, to expose it to Python, in the py40kl project.
The game's (very basic) GUI uses model/view/controller, where the View uses
Pygame to render, and the model is just a wrapper around a core GameState object,
and the controller is either a human player or an AI controller. The Python code
is contained in the folders: pyapp, pyai and Scripts. The first of these contains
the UI code, and some utility code. The second of these folders contains all AI
implementations in Python (including neural networks). The last of these folders,
the Scripts folder, contains the runnable Python scripts, designed to be used
from command line.

The key scripts are play.py, train.py and game.py:
- play.py : perform self-play with a given neural network setup to generate an
            experience dataset. It does this using Monte Carlo tree search, guided
            by the neural network weights provided (or from scratch), and can
            simulate many games at once. This is by far the most performance
            heavy part of the project: this is because the search algorithm can
            take quite some time and uses a lot of memory.
- train.py : this trains a given neural network on an experience dataset. It does
             so by randomly subsampling from all experiences (uniformly) and then
             training the neural network to predict the outputs of the tree search
             and the winner of the game from the game state.
- game.py : this runs the GUI to allow a human/AI to play a game against another
            human/AI. The default is set to having team 0 be a human and team 1
            being the AI.

In the core project, the main classes are:
- GameState: this is an immutable object encapsulating all info about a particular
  moment in a game. In order to "modify" it, you would need to create a copy of its
  board state, modify that board state, and then create a new game state object with
  that board.
- BoardState: this represents the layout of the board, but does not know which player's
  turn it is, etc - basically just tracks board size and unit statistics and positions.
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
built Boost binaries are in BOOST_DIR/stage/lib. It was built with Microsoft Visual
Studio (2017) but will probably work on any visual studio version.

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
once built. This allows them to be found when the game is ran.

Now it is time to run the scripts! For example, one could type:
- python Scripts/play.py --model_filename="Models/model1.h5" --data="TrainingData/data*" --initial_states="UnitData/map_*.csv" --unit_data="UnitData/unit_stats.csv" --search_size=200 --num_games=5 --iterations=1
- python Scripts/train.py --data="TrainingData/*" --model="Models/model1.h5"
- python Scripts/game.py --model_filename="Models/model1.h5" --unit_data="UnitData/unit_stats.csv" --initial_state="UnitData/map_1.csv"

Note on GUI controls: left click a blue square (one of your units) to select it. If
that unit has any available actions, the square upon which they can act will be displayed
in yellow. Enemies are in red. Right click on a yellow square to move there/shoot
that enemy/charge that location/fight that enemy. Press enter to end phase.

## Game rules and simplifications

The Warhammer 40K rules are available freely online, however the
game has been simplified for the purposes of this project in the
following way:
- Units are fixed to moving on a coarse grid,
- All units in a squad must have exactly the same unit stats (i.e. no squad leaders),
- No Psykers,
- No Advancing,
- No stratagems or other global abilities,
- No aura special rules,
- No randomness in determining statistics (e.g. a weapon with a random number of shots),
- No line of sight or cover

It should be easy to incorporate global abilities (e.g. an orbital bombardment)
and psykers with exactly one psychic power, or advancing, but this is currently
not implemented.

## TODO

- Extend the experience dataset to perform random transformations of the board such
  as rotation, reflection (there are 8 symmetries.) This is useful because it doesn't
  change the board's value, and shouldn't change the policy (after applying the
  transformation to the policy, also.)
- Create a script for evaluating different sets of model weights, do determine which
  is better.
- Maybe: refactor the MCTS tree to enforce an order on which units to order. This
  should reduce branching factor. Then, update neural network policy output.
- Stretch goal: supporting many concurrent games, each with their own large search
  trees. This would require two steps: (i) allow search trees to be stored in a database
  or file, so their sizes aren't bound by the limits of the system's RAM, and (ii) allow
  the search tree results to be streamed across networks of devices, so that different
  devices can run different search trees (for different games), which will then send their
  results across a network to a central machine. This central machine will run the neural
  network, and distribute the results of the neural network to the machines running the
  tree search.
