import py40kl
import random


NUM_TRAINING_EPOCHS = 10
NUM_GAMES = 10
GAME_START_STATE = py40kl.GameState()  # TODO: setup initial state here.
EXPERIENCE_SAMPLE_EPOCH_SIZE = 1000  # The number of experiences in a training epoch


# This central neural network model performs
# the prediction steps and is also trained.
# TODO: fill this class in, once the neural
# network code has been completed!
class CentralNeuralNetworkModel:
    def train(self, game_states, true_values, mcts_policies):  # game states provided in numeric form
        pass
        
    def predict(self, game_states):  # game states provided in numeric form
        pass


# The Experience Dataset is an important class
# for recording experiences and then creating
# training datasets from them.
class ExperienceDataset:
    def sample(self, n):  # returns game_states (as numeric), values, policies
        pass
        
    def set_buffer(self, n):
        pass
        
    def commit(self, values):  # values correspond to the games that have been set in the buffer thus far
        pass  # IMPORTANT: the values are with respect to team 0. For the game states in the buffer with acting team 1, this value needs to be flipped.
        
    def add_to_buffer(self, game_states, policies):  # game states in numerical form
        pass


# Create the self-play manager:
mgr = py40kl.SelfPlayManager(2.0 ** 0.5, 100)

# Create the neural network model:
# TODO: load from file?
model = CentralNeuralNetworkModel()

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
            # Select leaf nodes in search trees, and get states at each of them:
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
    game_states, values, policies = dataset.sample(EXPERIENCE_SAMPLE_EPOCH_SIZE)

    # Now ready to perform a training epoch on the model:
    model.train(game_states, values, policies)

# TODO: save model!