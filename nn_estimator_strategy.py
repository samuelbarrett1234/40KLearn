from nn_model import NNModel
from converter import convert_states_to_arrays, array_to_policy


class NeuralNetworkEstimatorStrategy:
    """
    This encapsulates the neural network model to be
    used in an actual game. This typically shouldn't
    be used for training because it runs one instance
    of the model per game instance, which is inefficient.
    """

    def __init__(self, team):
        self.model = NNModel()
        self.team = team

    def compute_value_estimate(self, state):
        (board_state_as_tensor,
         phase_as_one_hot_vector) = self._convert_state(state)

        values, _ = self.model.run([board_state_as_tensor,
                                    phase_as_one_hot_vector])
        val = values[0]  # values has length 1, because the NN is vectorised

        # Now convert if val is with respect to the other team:
        if state.get_acting_team() != self.team:
            val *= -1.0

        return val

    def compute_prior_distribution(self, state, actions):
        (board_state_as_tensor,
         phase_as_one_hot_vector) = self._convert_state(state)

        _, policies = self.model.run([board_state_as_tensor,
                                      phase_as_one_hot_vector])

        policy = policies[0]  # list has length 1, because the NN is vectorised

        return array_to_policy(policy, state)

    def _convert_state(self, state):
        # Need lists because the converter is vectorised
        boards, phases = convert_states_to_arrays([state])
        return boards[0], phases[0]
