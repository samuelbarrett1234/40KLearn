from mcts import MCTS
from mcts_strategies import *
from game_util import *
import math


class BasicEstimatorStrategy(EstimatorStrategy):
    def __init__(self,team):
        self.team=team
        
    def computeValueEstimate(self, rootState):
        #Compute value of game by uniform random simulation
        allies = rootState.getBoard().getAllUnits(self.team)
        allies = [rootState.getBoard().getUnitOnSquare(x,y) for x,y in allies]
        enemies = rootState.getBoard().getAllUnits(1-self.team)
        enemies = [rootState.getBoard().getUnitOnSquare(x,y) for x,y in enemies]
        aw = [a["total_w"] for a in allies]
        astr = [a["rg_s"]+a["ml_s"] for a in allies]
        aprod = [w*s for w,s in zip(aw,astr)]
        ew = [e["total_w"] for e in enemies]
        estr = [e["rg_s"]+e["ml_s"] for e in enemies]
        eprod = [w*s for w,s in zip(ew,estr)]
        return math.tanh((sum(aprod)-sum(eprod))*0.2)
        
    def computePriorDistribution(self, state, actions):
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
        self.N = 250
        self.tree = None
        self.onTurnChanged() #Sets up the MCTS tree
        
    def onUpdate(self):        
        #Simulate:
        self.tree.simulate(self.N)

        #Save MCTS tree:
        file = open("mcts_tree_output.txt", "w")
        file.write(str(self.tree))
        file.close()
        
        #Get results:
        actions,dist = self.tree.getCurrentDistribution()
        
        #Select and apply:
        action = selectRandomly(actions,dist)
        if action is None or action.getTargetPosition() is None:
            self.model.skip()
            self.tree.commit(None, self.model.getState())
            
            #Log what happened
            i,j = self.model.getActiveUnit()
            unit = self.model.getState().getBoard().getUnitOnSquare(i,j)
            print("AI decided to skip with", unit["name"])
        else:
            x,y = action.getTargetPosition()
            
            #Log what happened:
            i,j = self.model.getActivePosition()
            unit = self.model.getState().getBoard().getUnitOnSquare(i,j)
            verb,subject = "",""
            if self.model.getPhase() == MOVEMENT_PHASE:
                verb = "move to"
                subject = str((x,y))
            elif self.model.getPhase() == SHOOTING_PHASE:
                verb = "shoot"
                target = self.model.getState().getBoard().getUnitOnSquare(x,y)
                subject = target["name"]
            elif self.model.getPhase() == CHARGE_PHASE:
                verb = "charge location"
                subject = str((x,y))
            else:
                verb = "fight"
                target = self.model.getState().getBoard().getUnitOnSquare(x,y)
                subject = target["name"]
            
            print("AI decided for",unit["name"],"to",verb,subject)
            
            #Actually apply the changes
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
        simStrategy = BasicEstimatorStrategy(self.model.getCurrentTeam())
        
        assert(len(rootState.activeUnits) > 0)
        
        #Create MCTS tree
        self.tree = MCTS(rootState,treePolicy,finalPolicy,simStrategy)
    
