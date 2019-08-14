from mcts import MCTS
from mcts_strategies import UniformRandomEstimatorStrategy,UCB1PolicyStrategy,VisitCountStochasticPolicyStrategy
from game_util import selectRandomly

"""
This AI controller uses MCTS without neural networks; in particular,
it just uses UCB1 with a uniform simulation strategy until the game
ends.
"""
class BasicAIController:
    def __init__(self, model):
        self.model = model
        self.tau = 0.3
        self.exploratoryParam = 0.3
        self.N = 5
        self.tree = None
        self.onTurnChanged() #Sets up the MCTS tree
        
    def onUpdate(self):        
        #Simulate:
        self.tree.simulate(self.N)
        
        #Get results:
        actions,dist = self.tree.getCurrentDistribution()
        
        #Select and apply:
        action = selectRandomly(actions,dist)
        if action is None or action.getTargetPosition() is None:
            self.model.skip()
            self.tree.commit(self, None, state)
        else:
            x,y = action.getTargetPosition()
            self.model.choosePosition(x,y)
            self.tree.commit((x,y), self.model.getState())
    
    def onClickPosition(self, x, y, bLeft):
        pass #AI doesn't care about clicks
            
    def onReturn(self):
        pass #AI doesn't care about clicks
        
    def onTurnChanged(self):
        #Reconstruct MCTS tree
        #MCTS components:
        rootState = self.model.getState().createCopy()
        treePolicy = UCB1PolicyStrategy(self.exploratoryParam)
        finalPolicy = VisitCountStochasticPolicyStrategy(self.tau)
        simStrategy = UniformRandomEstimatorStrategy(self.model.getCurrentTeam())
        
        assert(len(rootState.activeUnits) > 0)
        
        #Create MCTS tree
        self.tree = MCTS(rootState,treePolicy,finalPolicy,simStrategy)
    
