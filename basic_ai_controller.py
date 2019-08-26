from mcts import MCTS
from mcts_strategies import *
from game_util import *
import math


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
        self.onTurnChanged() #Sets up the MCTS tree
        
    def onUpdate(self):        
        #Simulate:
        self.tree.simulate(self.N-self.tree.get_num_samples())
        
        #Get results:
        actions,dist = self.tree.get_distribution()
        
        #Select and apply:
        action = selectRandomly(actions,dist)
        if action.get_type() == py40kl.END_PHASE:
            self.model.skip()
            self.tree.commit(self.model.getState())
            
            #Log what happened
            i,j = self.model.getCurrentUnit()
            unit = self.model.getState().get_board().get_unit_on_square(i,j)
            print("AI decided to skip with", unit.name)
        else:
            pos = action.get_target_position()
            x,y = pos.x, pos.y
            
            #Log what happened:
            pos = self.model.getActivePosition()
            i,j = pos.x, pos.y
            unit = self.model.getState().get_board().get_unit_on_square(i,j)
            verb,subject = "",""
            if self.model.getPhase() == py40kl.MOVEMENT_PHASE:
                verb = "move to"
                subject = str((x,y))
            elif self.model.getPhase() == py40kl.SHOOTING_PHASE:
                verb = "shoot"
                target = self.model.getState().get_board().get_unit_on_square(x,y)
                subject = target.name
            elif self.model.getPhase() == py40kl.CHARGE_PHASE:
                verb = "charge location"
                subject = str((x,y))
            else:
                verb = "fight"
                target = self.model.getState().get_board().get_unit_on_square(x,y)
                subject = target.name
            
            print("AI decided for",unit.name,"to",verb,subject)
            
            #Actually apply the changes
            self.model.choose_action(action)
            self.tree.commit(action, self.model.getState())
    
    def onClickPosition(self, x, y, bLeft):
        pass #AI doesn't care about clicks
            
    def onReturn(self):
        pass #AI doesn't care about clicks
        
    def onTurnChanged(self):
        print("AI ON TURN CHANGED.")
        #Reconstruct MCTS tree
        #MCTS components:
        rootState = self.model.getState()
        treePolicy = py40kl.UCB1PolicyStrategy(self.exploratoryParam, self.model.getCurrentTeam())
        finalPolicy = VisitCountStochasticPolicyStrategy(self.tau)
        simStrategy = UniformRandomEstimatorStrategy(self.model.getCurrentTeam())
        
        #Create MCTS tree
        self.tree = MCTS(rootState,treePolicy,finalPolicy,simStrategy)
    
