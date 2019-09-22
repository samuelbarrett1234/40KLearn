from pyapp.game_util import select_randomly, new_game_state
import py40kl


# These constants are imported elsewhere in the project
BOARD_SIZE = 24
BOARD_SCALE = 1.0


class Model:
    """
    Represents the interface by which the views/controllers
    play the game. Note that this is an extra layer over the
    game wrapper object, which provides handy summary information
    and handles everything in terms of board-cell-coordinates.
    """
    def __init__(self, unit_roster, placements):
        self.game = new_game_state(unit_roster, placements, BOARD_SIZE)
        self.current_actions = self.game.get_commands()\
            if not self.game.is_finished() else []
        self.active_position = None

    def get_state(self):
        return self.game

    def get_actions(self):
        return self.current_actions

    def is_finished(self):
        return self.game.is_finished()

    def choose_action(self, action):
        assert(action in self.current_actions)
        states, probs = py40kl.GameStateArray(), py40kl.FloatArray()
        action.apply(self.game, states, probs)
        self.game = select_randomly(states, probs)
        self.current_actions = self.game.get_commands()
        self.active_position = None

    def get_active_unit_option_positions(self):
        if self.active_position is None:
            return []
        else:
            # Return all of the positions of target action locations
            # for the current active unit.
            return [action.get_target_position()
                    for action in self.current_actions
                    if action.get_type() == py40kl.CommandType.UNIT_ORDER
                    and action.get_source_position() == self.active_position]

    def get_board_size(self):
        return self.game.get_board_state().get_size()

    def get_position_desc(self, pos):
        board = self.game.get_board_state()
        if not board.is_occupied(pos):
            return "An empty cell."
        else:
            unit = board.get_unit_on_square(pos)
            unitDesc = unit.name + " at " + str(unit.count)\
                + " models, " + str(unit.total_w) + " total wounds left."
            team = str(board.get_team_on_square(pos))
            return unitDesc + " (team " + team + ")"

    def get_active_position(self):
        return self.active_position

    def set_active_position(self, pos):
        if not self.game.get_board_state().is_occupied(pos):
            self.active_position = None
        else:
            self.active_position = pos

    def get_allied_positions(self):
        cur_team = self.game.get_acting_team()
        return self.game.get_board_state().get_all_units(cur_team)

    def get_enemy_positions(self):
        enemy_team = 1 - self.game.get_acting_team()
        return self.game.get_board_state().get_all_units(enemy_team)

    def get_acting_team(self):
        return self.game.get_acting_team()

    def get_phase(self):
        return self.game.get_phase()

    def get_summary(self):
        team = self.get_acting_team()
        phase = self.get_phase()
        phase_names = {
            py40kl.Phase.MOVEMENT: "Movement phase",
            py40kl.Phase.SHOOTING: "Shooting phase",
            py40kl.Phase.CHARGE: "Charge phase",
            py40kl.Phase.FIGHT: "Fight phase"
        }
        return "TEAM: " + str(team) + ", PHASE: " + phase_names[phase]
