from mcts import MCTS
from mcts_strategies import *
from game_util import *
import math
import py40kl


#Use a really bad (but cheap) heuristic as to the current game value:
class BasicEstimatorStrategy:
    def __init__(self,team):
        self.team=team
        
    def compute_value_estimate(self, state):
        #A VERY rough heuristic!
        board = state.get_board_state()
        
        allies = board.get_all_units(self.team)
        enemies = board.get_all_units(1 - self.team)
        
        allies = [board.get_unit_on_square(pos) for pos in allies]
        enemies = [board.get_unit_on_square(pos) for pos in enemies]
        
        allied_w = sum([unit.w for unit in allies])
        allied_s = sum([unit.rg_s + unit.ml_s for unit in allies])
        enemy_w = sum([unit.w for unit in enemies])
        enemy_s = sum([unit.rg_s + unit.ml_s for unit in enemies])
        
        return math.tanh(allied_w * 2.0 + allied_s - enemy_w * 2.0 - enemy_s)
        
    def compute_prior_distribution(self, state, actions):
        #Just return uniform distribution:
        N = len(actions)
        return [1 / N for a in actions]


"""
This AI controller uses MCTS without neural networks; in particular,
it just uses UCB1 with a uniform simulation strategy until the game
ends.
"""
class BasicAIController:
    def __init__(self, model):
        self.model = model
        self.tau = 0.2
        self.exploratoryParam = 1.4
        self.N = 200
        self.tree = None
        self.on_turn_changed() #Sets up the MCTS tree
        
    def on_update(self):        
        #Simulate:
        self.tree.simulate(self.N-self.tree.get_num_samples())
        
        #Get results:
        actions,dist = self.tree.get_distribution()
        
        #Select and apply:
        action = select_randomly(actions,dist)
        if action.get_type() == py40kl.CommandType.END_PHASE:
            self.model.skip()
            self.tree.commit(self.model.get_state())
            
            #Log what happened
            i,j = self.model.getCurrentUnit()
            unit = self.model.get_state().get_board_state().get_unit_on_square(i,j)
            print("AI decided to skip with", unit.name)
        else:
            pos = action.get_target_position()
            x,y = pos.x, pos.y
            
            #Log what happened:
            pos = self.model.get_active_position()
            i,j = pos.x, pos.y
            unit = self.model.get_state().get_board_state().get_unit_on_square(i,j)
            verb,subject = "",""
            if self.model.get_phase() == py40kl.Phase.MOVEMENT:
                verb = "move to"
                subject = str((x,y))
            elif self.model.get_phase() == py40kl.Phase.SHOOTING:
                verb = "shoot"
                target = self.model.get_state().get_board_state().get_unit_on_square(x,y)
                subject = target.name
            elif self.model.get_phase() == py40kl.Phase.CHARGE:
                verb = "charge location"
                subject = str((x,y))
            else:
                verb = "fight"
                target = self.model.get_state().get_board_state().get_unit_on_square(x,y)
                subject = target.name
            
            print("AI decided for",unit.name,"to",verb,subject)
            
            #Actually apply the changes
            self.model.choose_action(action)
            self.tree.commit(action, self.model.get_state())
    
    def on_click_position(self, x, y, bLeft):
        pass #AI doesn't care about clicks
            
    def on_return(self):
        pass #AI doesn't care about clicks
        
    def on_turn_changed(self):
        #Reconstruct MCTS tree
        #MCTS components:
        rootState = self.model.get_state()
        treePolicy = py40kl.UCB1PolicyStrategy(self.exploratoryParam, self.model.get_acting_team())
        finalPolicy = VisitCountStochasticPolicyStrategy(self.tau)
        #simStrategy = UniformRandomEstimatorStrategy(self.model.get_acting_team())
        simStrategy = BasicEstimatorStrategy(self.model.get_acting_team())
        
        #Create MCTS tree
        self.tree = MCTS(rootState,treePolicy,finalPolicy,simStrategy)
    
