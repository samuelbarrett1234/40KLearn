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
        self.exploratoryParam = 0.3
        self.N = 5
        #TODO: make MCTS tree persistent and use the commit() functionality
        
    def onUpdate(self):
        #MCTS components:
        rootState = self.model.getState().createCopy()
        treePolicy = UCB1PolicyStrategy(self.exploratoryParam)
        finalPolicy = VisitCountStochasticPolicyStrategy(self.tau)
        simStrategy = UniformRandomEstimatorStrategy(self.model.getCurrentTeam())
        
        assert(len(rootState.activeUnits) > 0)
        
        #Create MCTS tree
        tree = MCTS(rootState,treePolicy,finalPolicy,simStrategy)
        
        #Simulate:
        tree.simulate(self.N)
        
        #Get results:
        actions,dist = tree.getCurrentDistribution()
        
        #Select and apply:
        action = selectRandomly(actions,dist)
        if action is None or action.getTargetPosition() is None:
            self.model.skip()
            print("AI decided to pass.")
        else:
            x,y = action.getTargetPosition()
            self.model.choosePosition(x,y)
            print("AI decided position",(x,y))
    
    def onClickPosition(self, x, y, bLeft):
        pass #AI doesn't care about clicks
            
    def onReturn(self):
        pass #AI doesn't care about clicks
    
