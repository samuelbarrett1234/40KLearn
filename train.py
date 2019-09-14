import py40kl
from nn_model import NNModel


NUM_TRAINING_EPOCHS = 10
NUM_GAMES = 10
GAME_START_STATE = py40kl.GameState()  # TODO: setup initial state here.
EXPERIENCE_SAMPLE_EPOCH_SIZE = 1000
MODEL_FILENAME = 'models/model1.h5'


# The Experience Dataset is an important class
# for recording experiences and then creating
# training datasets from them.
class ExperienceDataset:
    def sample(self, n):  # returns game_states (as numeric), values, policies
        pass

    def set_buffer(self, n):
        pass

    def commit(self, values):
        """
        Write all of the values in the buffer to our dataset. Furthermore,
        we are provided with the game's end state (the value) for each of
        the games we've been recording. This game value is with respect to
        team 0, so we will need to multiply it by -1 for about half of our
        recordings.
        So if we are recording 10 games, we will have many experiences for
        each of those 10 games, but len(values) == 10.
        """
        pass

    def add_to_buffer(self, game_states, policies):
        """
        Add a list of game states to our running buffer, and the corresponding
        policies to learn. 'Policies' should be in numerical form, and should
        be a 'desirable' policy - the network will train with this as a target.
        """
        pass


# Create the self-play manager:
mgr = py40kl.SelfPlayManager(2.0 ** 0.5, 100)

# Create the neural network model:
# TODO: load existing model
model = NNModel(filename=None)

# Create the dataset:
# TODO: load from file?
dataset = ExperienceDataset()


for epoch in range(NUM_TRAINING_EPOCHS):
    # Reserve space for next batch of experiences:
    mgr.reset(NUM_GAMES, GAME_START_STATE)
    dataset.set_buffer(NUM_GAMES)

    # Generate the next batch of experiences:
    while not mgr.all_finished():
        while not mgr.ready_to_commit():
            # Select leaf nodes in search trees, and
            # get states at each of them:
            states = py40kl.GameStateArray()
            mgr.select(states)

            # Run the network on these states to get value/policy estimates:
            values, policies = model.predict(states)

            # Update games with this info:
            mgr.update(values, policies)

        # Get current game info:
        game_states = mgr.get_current_game_states()
        game_states_numeric = py40kl.convert_gs_to_numeric(game_states)
        policies = mgr.get_current_action_distributions()

        # Save to experience dataset buffer:
        dataset.add_to_buffer(game_states_numeric, policies)

        # Now ready to make a decision in-game:
        mgr.commit()

    # Get the game values, with respect to team 0:
    game_states = mgr.get_current_game_states()
    game_values = [state.get_game_value(0) for state in game_states]

    # Now we've built up a buffer of new experiences, commit them
    # to the database and train:
    dataset.commit(game_values)

    # Obtain sample:
    (game_states, values,
     policies) = dataset.sample(EXPERIENCE_SAMPLE_EPOCH_SIZE)

    # Now ready to perform a training epoch on the model:
    model.train(game_states, values, policies)

model.save(MODEL_FILENAME)  # save once training done
