import py40kl
import numpy as np
from scipy.special import logit, expit


NUM_FEATURES = 19  # 19 features, see below


def unit_to_vector(unit):
    """
    Convert a game unit into a feature vector.
    The size of the returned vector is NUM_FEATURES.
    """

    # Initially don't train on all available features. We can
    # add in extra features later.
    # Notably, we don't have wounds (this can be computed from
    # count and total_w), we don't have invulnerable save (as
    # most units don't have one), and we don't include any of
    # the flags which tell what the unit has done so far this phase.
    return [unit.count,
            unit.movement,
            unit.ws,
            unit.bs,
            unit.t,
            unit.total_w,
            unit.a,
            unit.ld,
            unit.sv,
            unit.rg_range,
            unit.rg_s,
            unit.rg_ap,
            unit.rg_dmg,
            unit.rg_shots,
            unit.ml_s,
            unit.ml_ap,
            unit.ml_dmg,
            1.0 if unit.rg_is_rapid else 0.0,
            1.0 if unit.rg_is_heavy else 0.0
            ]


def board_to_array(board, team):
    """
    Convert a board into a 2D array of feature
    vectors (this returning a 3D array).
    """
    # Create empty board:
    board = [[[0.0 for k in range(NUM_FEATURES * 2)]
              for j in range(board.get_size())]
             for i in range(board.get_size())]

    zeroes = [0.0] * NUM_FEATURES
    for unit_pos in board.get_all_units():
        # Get raw unit stats:
        unit_stats = board.get_unit_on_square(unit_pos)
        # Turn unit object into feature vector:
        unit_feature_vec = unit_to_vector(unit_stats)
        # For allied units, we put the unit feature vector first, and
        # for enemy units we put the zeros first and the unit feature vector
        # after.
        board[unit_pos.x][unit_pos.y] = unit_feature_vec + zeroes\
            if team == board.get_team_on_square(unit_pos)\
            else zeroes + unit_feature_vec


def phase_to_vector(phase):
    """
    Convert a game phase enum into a feature
    vector (a 4D one-hot vector).
    """
    if phase == py40kl.Phase.MOVEMENT:
        return [1.0, 0.0, 0.0, 0.0]
    elif phase == py40kl.Phase.SHOOTING:
        return [0.0, 1.0, 0.0, 0.0]
    elif phase == py40kl.Phase.CHARGE:
        return [0.0, 0.0, 1.0, 0.0]
    else:  # if fight phase
        return [0.0, 0.0, 0.0, 1.0]


def convert_states_to_arrays(game_states):
    """
    Convert a list of game states into a list of board
    arrays and a list of phase vectors. This function
    is used for preparing game input in a format that
    a neural network can use.
    """
    return ([board_to_array(state.get_board_state(), state.get_acting_team())
             for state in game_states],
            [phase_to_vector(state.get_phase()) for state in game_states])


def policy_to_array(policy, game_state):
    """
    Convert a policy (distribution over actions) into an array
    which the game state can use. 'Policy' should be an array
    which has the same length as game_state.get_commands().
    This function returns an array of size 2 * BOARD_SIZE * BOARD_SIZE + 1.
    This represents the probability of a particular source position, a
    particular target position, or just ending phase.
    """
    actions = game_state.get_commands()
    assert(len(policy) == len(actions))
    sz = game_state.get_board_state().get_size()
    policy_array = [0.0] * (2 * sz * sz + 1)
    for action, prob in zip(actions, policy):
        if action.get_type() == py40kl.CommandType.UNIT_ORDER:
            source = action.get_source_position()
            target = action.get_target_position()
            source_idx = source.x + source.y * sz
            target_idx = target.x + target.y * sz
            # add the probability to existing values, because
            # we may have many actions for each source and each
            # target.
            policy_array[source_idx] += prob
            policy_array[target_idx] += prob
        else:
            policy_array[-1] = prob
    # Now convert array to logit probabilities (converting to and from np)
    policy_array = np.array(policy_array)
    policy_array = logit(policy_array)
    return list(policy_array)


def array_to_policy(policy_array, game_state):
    """
    Convert the policy output of the network, in array form, into
    a true policy over the potential game actions. The input array
    is given in logit probabilities. The returned array is a distribution
    over game_state.get_commands().
    """
    sz = game_state.get_board_state().get_size()
    assert(len(policy_array) == 2 * sz * sz + 1)
    actions = game_state.get_commands()
    policy = [0.0] * len(actions)
    for i, action in enumerate(actions):
        if action.get_type() == py40kl.CommandType.UNIT_ORDER:
            source = action.get_source_position()
            target = action.get_target_position()

            source_idx = source.x + source.y * sz
            target_idx = target.x + target.y * sz

            # Convert from logit (log odds) to probabilities
            p_source = expit(policy_array[source_idx])
            p_target = expit(policy_array[target_idx])

            # Multiply the probabilities to 'and' them:
            policy[i] = p_source * p_target
        else:
            # Convert from logit (log odds) to probabilities
            policy[i] = expit(policy_array[-1])

    # Normalise:
    policy = np.array(policy)
    policy /= np.sum(policy)
    return list(policy)
