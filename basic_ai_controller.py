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
        self.tau = 0.4
        self.exploratoryParam = 1.0
        self.N = 25
        #TODO: make MCTS tree persistent and use the commit() functionality
        
    def onUpdate(self):
        #MCTS components:
        rootState = self.model.getState()
        treePolicy = UCB1PolicyStrategy(self.exploratoryParam)
        finalPolicy = VisitCountStochasticPolicyStrategy(self.tau)
        simStrategy = UniformRandomEstimatorStrategy()
        
        #Create MCTS tree
        tree = MCTS(rootState,treePolicy,finalPolicy,simStrategy)
        
        #Simulate:
        tree.simulate(self.N)
        
        #Get results:
        actions,dist = tree.getCurrentDistribution()
        
        #Select and apply:
        action = selectRandomly(actions,dist)
        x,y = action.getTargetPosition()
        self.model.choosePosition(x,y)
    
    def onClickPosition(self, x, y, bLeft):
        pass #AI doesn't care about clicks
            
    def onReturn(self):
        pass #AI doesn't care about clicks
    
