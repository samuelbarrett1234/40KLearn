import py40kl
import numpy as np


NUM_FEATURES = 19


"""
Convert a game unit into a feature vector.
The size of the returned vector is NUM_FEATURES.
"""
def unit_to_vector(unit):
    #Initially don't train on all available features. We can
    # add in extra features later.
    #Notably, we don't have wounds (this can be computed from
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


"""
Convert a board into a 2D array of feature
vectors (this returning a 3D array).
"""
def board_to_array(board, team):
    #Create empty board:
    board = [[[0.0 for k in range(NUM_FEATURES * 2)] for j in range(board.get_size())] for i in range(board.get_size())]
    
    for unit_pos in board.get_all_units():
        #Get raw unit stats:
        unit_stats = board.get_unit_on_square(unit_pos)
        #Turn unit object into feature vector:
        unit_feature_vec = unit_to_vector(unit_stats)
        zeroes = [0.0] * NUM_FEATURES
        #For allied units, we put the unit feature vector first, and
        # for enemy units we put the zeros first and the unit feature vector
        # after.
        board[unit_pos.x][unit_pos.y] = unit_feature_vec + zeroes if team == board.get_team_on_square(unit_pos) else zeroes + unit_feature_vec


"""
Convert a game phase enum into a feature
vector (a 4D one-hot vector).
"""
def phase_to_vector(phase):
    if phase == py40kl.Phase.MOVEMENT:
        return [1.0, 0.0, 0.0, 0.0]
    elif phase == py40kl.Phase.SHOOTING:
        return [0.0, 1.0, 0.0, 0.0]
    elif phase == py40kl.Phase.CHARGE:
        return [0.0, 0.0, 1.0, 0.0]
    else:  # if fight phase
        return [0.0, 0.0, 0.0, 1.0]


"""
Convert a list of game states into a list of board
arrays and a list of phase vectors. This function
is used for preparing game input in a format that
a neural network can use.
"""
def convert_states_to_arrays(game_states):
    return ([board_to_array(state.get_board_state(), state.get_acting_team()) for state in game_states],
            [phase_to_vector(state.get_phase()) for state in game_states])
