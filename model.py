from game_util import select_randomly
import py40kl


BOARD_SIZE = 24
BOARD_SCALE = 1.0

"""
Create a new game with the given unit roster (array of
units) and placements (a list of tuples (unit_idx, team, x, y)).
"""
def new_game_state(unitRoster, placements):
    b = py40kl.BoardState(BOARD_SIZE, BOARD_SCALE)
    for unitIdx, team, x, y in placements:
        unit = unitRoster[unitIdx]
        pos = py40kl.Position(x,y)
        newunit = py40kl.Unit()
        
        newunit.name = unit["name"]
        newunit.count = unit["count"]
        newunit.movement = unit["movement"]
        newunit.ws = unit["ws"]
        newunit.bs = unit["bs"]
        newunit.t = unit["t"]
        newunit.w = unit["w"]
        newunit.total_w = unit["w"]*unit["count"]
        newunit.a = unit["a"]
        newunit.ld = unit["ld"]
        newunit.sv = unit["sv"]
        newunit.inv = unit["inv"]
        newunit.rg_range = unit["rg_range"]
        newunit.rg_s = unit["rg_s"]
        newunit.rg_ap = unit["rg_ap"]
        newunit.rg_dmg = unit["rg_dmg"]
        newunit.rg_shots = unit["rg_shots"]
        newunit.ml_s = unit["ml_s"]
        newunit.ml_ap = unit["ml_ap"]
        newunit.ml_dmg = unit["ml_dmg"]
        newunit.rg_is_rapid = unit["rg_is_rapid"]
        newunit.rg_is_heavy = unit["rg_is_heavy"]
        
        b.set_unit_on_square(pos, newunit, team)
        
    return py40kl.GameState(0, 0, py40kl.MOVEMENT_PHASE, b)

"""
Represents the interface by which the views/controllers
play the game. Note that this is an extra layer over the
game wrapper object, which provides handy summary information
and handles everything in terms of board-cell-coordinates.
"""
class Model:
    def __init__(self, unit_roster, placements):
        self.game = new_game_state(unit_roster, placements)
        self.current_actions = self.game.get_commands()\
            if not self.game.is_finished() else []
        self.active_position = None
        
    def get_state(self):
        return self.game
        
    def is_finished(self):
        return self.game.is_finished()
        
    def choose_action(self, action):
        assert(action in self.current_actions)
        states, probs = py40kl.GameStateArray(), py40kl.GameStateArray()
        action.apply(self.game, states, probs)
        self.game = select_randomly(states, probs)
        self.current_actions = self.game.getCurrentOptions()
        self.active_position = None
        
    def get_active_unit_option_positions(self):
        assert(self.active_position is not None)
        #Return all of the positions of target action locations
        # for the current active unit.
        return [action.get_target_position() for action in self.current_actions if action.get_type() == py40kl.UNIT_ORDER and action.get_source_position() == self.active_position]
        
    def get_board_size(self):
        return self.game.get_board().get_size()
        
    def get_position_desc(self, pos):
        board = self.game.get_board()
        if not board.is_occupied(pos):
            return "An empty cell."
        else:
            unit = board.get_unit_on_square(pos)
            unitDesc = unit.name + " at " + str(unit.count) + " models, " + str(unit.total_w) + " total wounds left."
            team = str(board.get_team_on_square(pos))
            return unitDesc + " (team " + team + ")"
            
            
    def get_active_position(self):
        return self.active_position
        
    def set_active_position(self, pos):
        if not self.game.get_board().is_occupied(pos):
            self.active_position = None
        else:
            self.active_position = pos
    
    def get_allied_positions(self):
        return self.game.get_board().get_all_units(self.game.get_acting_team())
    
    def get_enemy_positions(self):
        return self.game.get_board().get_all_units(1-self.game.get_acting_team())
        
    def get_acting_team(self):
        return self.game.get_acting_team()
    
    def get_phase(self):
        return self.game.get_phase()
        
    def get_summary(self):
        team = self.get_acting_team()
        phase = self.get_phase()
        phase_names = {
            py40kl.MOVEMENT_PHASE : "Movement phase",
            py40kl.SHOOTING_PHASE : "Shooting phase",
            py40kl.CHARGE_PHASE : "Charge phase",
            py40kl.FIGHT_PHASE : "Fight phase"
        }
        return "TEAM: " + str(team) + ", PHASE: " + phase_names[phase]
        
        
        