from mcts import MCTS
from mcts_strategies import UniformRandomEstimatorStrategy, VisitCountStochasticPolicyStrategy
from game_util import *
import math
import py40kl


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
        self.N = 25
        self.tree = None
        self.on_turn_changed() #Sets up the MCTS tree
        
    def on_update(self):        
        #Simulate:
        self.tree.simulate(self.N - self.tree.get_num_samples())
        
        #Get results:
        actions, dist = self.tree.get_distribution()
        
        #Select and apply:
        action = select_randomly(actions, dist)
        if action.get_type() != py40kl.CommandType.UNIT_ORDER:            
            #Log what happened
            if len(actions) > 1:
                print("AI decided to end turn/phase")
            else:
                print("AI ended turn (no other possible actions.)")
        else:
            #Determine target position:
            pos = action.get_target_position()
            x, y = pos.x, pos.y
            
            #Determine source position:
            pos = action.get_source_position()
            i, j = pos.x, pos.y
            
            #Log what happened:
            unit = self.model.get_state().get_board_state().get_unit_on_square(py40kl.Position(i, j))
            verb, subject = "", ""
            if self.model.get_phase() == py40kl.Phase.MOVEMENT:
                verb = "move to"
                subject = str((x, y))
            elif self.model.get_phase() == py40kl.Phase.SHOOTING:
                verb = "shoot"
                target = self.model.get_state().get_board_state().get_unit_on_square(py40kl.Position(x, y))
                subject = target.name
            elif self.model.get_phase() == py40kl.Phase.CHARGE:
                verb = "charge location"
                subject = str((x, y))
            else:
                verb = "fight"
                target = self.model.get_state().get_board_state().get_unit_on_square(py40kl.Position(x, y))
                subject = target.name
            
            print("AI decided for", unit.name, "to", verb, subject)
            
        #Actually apply the changes
        self.model.choose_action(action)
        self.tree.commit(self.model.get_state())
    
    def on_click_position(self, x, y, bLeft):
        pass #AI doesn't care about clicks
            
    def on_return(self):
        pass #AI doesn't care about clicks
        
    def on_turn_changed(self):
        #Reconstruct MCTS tree every turn
        
        #MCTS components:
        rootState = self.model.get_state()
        treePolicy = py40kl.UCB1PolicyStrategy(self.exploratoryParam, self.model.get_acting_team())
        finalPolicy = VisitCountStochasticPolicyStrategy(self.tau)
        simStrategy = UniformRandomEstimatorStrategy(self.model.get_acting_team())
        
        #Create MCTS tree
        self.tree = MCTS(rootState, treePolicy, finalPolicy, simStrategy)
