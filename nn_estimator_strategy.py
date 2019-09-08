import py40kl
from nn_model import NNModel


"""
This packages up the neural network model to be
used in an actual game. This typically shouldn't
be used for training because it runs one instance
of the model per game instance, which is inefficient.
"""
class NeuralNetworkEstimatorStrategy:
    def __init__(self, team):
        self.model = NNModel()
        self.team = team
        
    def compute_value_estimate(self, state):
        board_state_as_tensor, phase_as_one_hot_vector = self._convert_state(state)
        values, _ = self.model.run([board_state_as_tensor, phase_as_one_hot_vector])
        val = values[0] # values is a list of length 1, because the neural network operates on lists of states.
        
        #Now convert if val is with respect to the other team:
        if state.get_acting_team() != self.team:
            val *= -1.0
            
        return val
        
    def compute_prior_distribution(self, state, actions):
        board_state_as_tensor, phase_as_one_hot_vector = self._convert_state(state)
        _, policies = self.model.run([board_state_as_tensor, phase_as_one_hot_vector])
        policy = policies[0] # policies is a list of length 1, because the neural network operates on lists of states.
        
        #TODO: 'trim' the policy. This involves turning it from a 1+BOARD_SIZE*BOARD_SIZE
        # length vector into a probability distribution over actions.
        return trimmed_policy_distribution
        
    def _convert_state(self, state):
        #TODO: convert 'state' to a pair
        # <board-state-as-tensor, phase-as-one-hot-vector>
        return board_state_as_tensor, phase_as_one_hot_vector
