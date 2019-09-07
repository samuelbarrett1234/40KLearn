import py40kl
from nn_model import NNModel


def game_state_to_tensor(state):
    raise NotImplementedError()


"""
This packages up the neural network model to be
used in an actual game. This typically shouldn't
be used for training because it runs one instance
of the model per game instance, which is inefficient.
"""
class NeuralNetworkEstimatorStrategy:
    def __init__(self, team):
        self.model = NNModel()
        
    def compute_value_estimate(self, state):
        # convert game states to inputs
        state_as_tensor = game_state_to_tensor(state)
        
        values, policies = self.model.run([state_as_tensor])
        return values[0]
        
    def compute_prior_distribution(self, state, actions):
        # convert game states to inputs
        state_as_tensor = game_state_to_tensor(state)
        
        values, policies = self.model.run([state_as_tensor])
        return policies[0]
