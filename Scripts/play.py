import os
import py40kl
import numpy as np
from argparse import ArgumentParser
from typing import Union
from glob import iglob
from random import choice
from pyai.nn_model import NNModel
from pyai.experience_dataset import ExperienceDataset
from pyai.converter import (convert_states_to_arrays, array_to_policy,
                            phase_to_vector, policy_to_array,
                            NUM_FEATURES)
from pyapp.model import BOARD_SIZE
from pyapp.game_util import (load_units_csv, new_game_state,
                             load_unit_placements_csv)


if __name__ == "__main__":
    ap = ArgumentParser()
    ap.add_argument("--model_filename",
                    help=("The .h5 file containing the model weights. "
                          "If None, then will start from scratch."),
                    type=Union[str, None],
                    default=None)
    ap.add_argument("--data",
                    help="The wildcard pattern for all data folders.",
                    type=str)
    ap.add_argument("--search_size",
                    help="The number of MCTS searches to perform per game",
                    type=int,
                    default=100)
    ap.add_argument("--num_games",
                    help=("The number of copies of the game to run on each"
                          " iteration."),
                    type=int,
                    default=20)
    ap.add_argument("--threads",
                    help="The number of threads to use for self-play.",
                    type=int,
                    default=3)
    ap.add_argument("--iterations",
                    help=("The number of times to play a set of games, "
                          "generating a new data folder for each one, and "
                          "selecting a new initial game state at random "
                          "each time."),
                    type=int,
                    default=1)
    ap.add_argument("--turn_limit",
                    help=("The maximum number of turns per game. "
                          "Negative numbers imply no turn limit."),
                    type=int,
                    default=6)
    ap.add_argument("--initial_states",
                    help=("The wildcard pattern for all CSV files of"
                          " 'unit placement tables'; at each iteration"
                          " one of these will be selected at random."),
                    type=str)
    ap.add_argument("--unit_data",
                    help="The table of unit statistics.",
                    type=str)
    ap.add_argument("--ucb1_parameter",
                    help=("The exploration parameter (higher means the"
                          " search algorithm is more exploratory.) Must"
                          " be > 0."),
                    type=float,
                    default=(2.0 * 2.0 ** 0.5))
    ap.add_argument("--policy_temperature",
                    help=("The 'temperature' of the final policy returned"
                          " from the search tree. Higher means closer to"
                          " a uniform distribution over actions, lower"
                          " means closer to an argmax. Must be >= 0."),
                    type=float,
                    default=0.4)

    ap.parse_args()

    # Check arguments
    if not((ap.model_filename is None or
            os.path.exists(ap.model_filename)) and
            ap.search_size > 0 and
            ap.num_games > 0 and
            ap.threads > 0 and
            ap.iterations > 0 and
            ap.turn_limit != 0 and
            len(iglob(ap.initial_states)) > 0 and
            os.path.exists(ap.unit_data) and
            ap.ucb1_parameter > 0.0 and
            ap.policy_temperature >= 0.0):
        raise ValueError("Invalid command line arguments.")

    # Create the self-play manager:
    mgr = py40kl.SelfPlayManager(ap.ucb1_parameter, ap.policy_temperature,
                                 ap.search_size, ap.threads)

    # Create the neural network model:
    model = NNModel(board_size=BOARD_SIZE,
                    filename=(ap.model_filename if
                              os.path.exists(ap.model_filename)
                              else None))

    # Create the dataset:
    dataset = ExperienceDataset(filename=ap.data,
                                board_size=BOARD_SIZE,
                                num_board_features=NUM_FEATURES)

    # Load the unit statistics dataset:
    units_dataset = load_units_csv(ap.unit_data)
    # Extract a separate list of names:
    unit_names = [x[0] for x in units_dataset]

    for self_play_iteration in range(ap.iterations):
        print("*** Starting self-play iteration", self_play_iteration + 1)

        # Load initial game state:
        unit_placements_fname = choice(iglob(ap.initial_states))
        unit_placements = load_unit_placements_csv(unit_placements_fname)
        # Need to convert name column to index column:
        unit_placements = [(unit_names.index(n), t, x, y)
                           for n, t, x, y in unit_placements]
        # Check all names in the map file were valid:
        assert(all([i >= 0 and i < len(units_dataset)
                    for i, _, __, ___ in unit_placements]))
        # Now create the start game state:
        initial_game_state = new_game_state(units_dataset, unit_placements,
                                            BOARD_SIZE,
                                            turn_limit=ap.turn_limit)

        # Reserve space for next batch of experiences:
        mgr.reset(ap.num_games, initial_game_state)
        dataset.set_buffer(ap.num_games)

        print("*** Playing", ap.num_games, "games through self-play on map",
              unit_placements_fname, "...")

        # Generate the next batch of experiences:
        while not mgr.all_finished():
            while not mgr.ready_to_commit():
                # Select leaf nodes in search trees, and
                # get states at each of them:
                states = py40kl.GameStateArray()
                mgr.select(states)

                # If we selected any states which need evaluating...
                if len(states) > 0:
                    # Get game states and phases in array form
                    (game_states_arr,
                     phases_arr) = convert_states_to_arrays(states)

                    # Run the network on these states to get
                    # value/policy estimates:
                    (values,
                     policies_as_numeric) = model.predict(game_states_arr,
                                                          phases_arr)

                    # Obtain the actual policies from the numeric (array)
                    # output from the neural network:
                    policies = [array_to_policy(pol_arr, state)
                                for pol_arr, state
                                in zip(policies_as_numeric, states)]

                    # If there are options other than passing, and the current
                    # phase is the shooting phase or fight phase, then do not
                    # allow a pass (set its probability to zero and normalise).
                    # In other cases, at least discourage a pass.
                    for state, policy in zip(states, policies):
                        if len(policy) > 1:
                            if state.get_phase() == py40kl.Phase.SHOOTING\
                               or state.get_phase() == py40kl.Phase.FIGHT:
                                policy[-1] = 0.0
                            else:
                                policy[-1] *= 1.0e-3
                            s = sum(policy)
                            if s == 0.0:
                                policy[-1] = 1.0  # restore to where we were
                            else:
                                policy = [p / s for p in policy]

                    # convert to a CPP-usable form
                    val_array = py40kl.FloatArray()
                    for x in values:
                        val_array.append(float(x))

                    # Update games with this info:
                    mgr.update(val_array, policies)
                else:
                    # Empty update:
                    mgr.update([], [])

            # Get current game info:
            game_states = mgr.get_current_game_states()
            game_states_numeric, _ = convert_states_to_arrays(game_states)
            policies = mgr.get_current_action_distributions()
            game_ids = mgr.get_running_game_ids()  # some might be finished
            teams = [state.get_acting_team() for state in game_states]

            print("*** Number of unfinished games:", len(game_states),
                  "Average turn number:", np.average([state.get_turn_number()
                                                      for state in game_states
                                                      if not state.is_finished(
                                                      )]))

            # Compute phase vector for each game state:
            phases = [phase_to_vector(state.get_phase())
                      for state in game_states]

            # Convert policies into the trainable format:
            policies = [policy_to_array(pol, gs)
                        for pol, gs in zip(policies, game_states)]

            # Save to experience dataset buffer:
            dataset.add_to_buffer(game_states_numeric, teams, phases,
                                  policies, ids=game_ids)

            # Now ready to make a decision in-game:
            mgr.commit()

        # Get the game values, with respect to team 0:
        game_values = mgr.get_game_values()

        print("*** Finished playing! Average game score for team 0:",
              np.average(list(game_values)))

        print("*** Saving game results...")

        # Now we've built up a buffer of new experiences, commit them
        # to the database and train:
        dataset.commit(game_values)

        print("*** Finished! Iteration complete.")

    print("*** DONE!")
