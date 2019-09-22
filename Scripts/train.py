import py40kl
from pyai.nn_model import NNModel
from pyai.experience_dataset import ExperienceDataset
from pyai.converter import (convert_states_to_arrays, array_to_policy,
                            phase_to_vector, policy_to_array,
                            NUM_FEATURES)
from pyapp.model import Model, BOARD_SIZE
from pyapp.game_util import load_units_csv


# TEMP: construct game start state by using the model's
# loading logic to construct an initial game state.
unit_roster = load_units_csv("unit_stats.csv")
placements = [
    (0, 0, 10, 20),
    (7, 1, 20, 10),
]
tempmodel = Model(unit_roster, placements)
GAME_START_STATE = tempmodel.get_state()
del tempmodel


NUM_SELF_PLAY_EPOCHS = 2
NUM_TRAINING_EPOCHS = 10
NUM_GAMES = 10
EXPERIENCE_SAMPLE_EPOCH_SIZE = 1000
NUM_MCTS_SIMULATIONS = 25
UCB1_EXPLORATION = 2.0 ** 0.5
MODEL_FILENAME = 'Models/model1.h5'
DATASET_FILENAME = 'Data/data*'


# Create the self-play manager:
mgr = py40kl.SelfPlayManager(UCB1_EXPLORATION, NUM_MCTS_SIMULATIONS)

# Create the neural network model:
# TODO: load existing model
model = NNModel(num_epochs=NUM_TRAINING_EPOCHS, board_size=BOARD_SIZE,
                filename=None)

# Create the dataset:
dataset = ExperienceDataset(filename=DATASET_FILENAME,
                            board_size=BOARD_SIZE,
                            num_board_features=NUM_FEATURES)


if __name__ == "__main__":
    for epoch in range(NUM_SELF_PLAY_EPOCHS):
        print("*** Starting self-play epoch", epoch + 1)

        # Reserve space for next batch of experiences:
        mgr.reset(NUM_GAMES, GAME_START_STATE)
        dataset.set_buffer(NUM_GAMES)

        print("*** Playing", NUM_GAMES, "games through self-play...")

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

                    # Discourage passing during training
                    # TODO: need a better solution than this?
                    for policy in policies:
                        policy[-1] /= 10000.0
                        s = sum(policy)
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

            print("Number of unfinished games:", len(game_states))

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

        print("*** Finished playing!")

        # Get the game values, with respect to team 0:
        game_values = mgr.get_game_values()

        # Now we've built up a buffer of new experiences, commit them
        # to the database and train:
        dataset.commit(game_values)

        # Obtain sample:
        (game_states, phases, values,
         policies) = dataset.sample(EXPERIENCE_SAMPLE_EPOCH_SIZE)

        print("*** Dataset constructed. Training model...")

        # Now ready to perform a training epoch on the model:
        model.train(game_states, phases, values, policies)

    print("*** Finished training, saving model...")

    model.save(MODEL_FILENAME)  # save once training done

    print("*** DONE!")