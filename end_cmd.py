from game_util import *


class CheckMoraleCommand:
    def __init__(self, x, y):
        self.x = x
        self.y = y
        
    def __eq__(self, other):
        if other.__class__ != self.__class__:
            return False
        return (self.x == other.x and self.y == other.y)
    
    def apply(self, board):
        assert(board.isOccupied(self.x,self.y))
        unit = board.getUnitOnSquare(self.x,self.y)
        team = board.getTeamOnSquare(self.x,self.y)
        ld = unit["ld"]
        loss = unit["modelsLostThisPhase"]
        minRollForLoss = ld - loss + 1
        if minRollForLoss >= 7: #Unit does not waver
            return [board.createCopy()], [1.0]
        else:
            states, probs = [], []            
            if minRollForLoss > 1:
                #Initialise with the probability of not losing any models:
                states.append(board.createCopy())
                probs.append((minRollForLoss-1)/6)
            
            for i in range(minRollForLoss,7):
                # i is the morale test dice roll
                numRunAway = loss + i - ld
                assert(numRunAway > 0)
                newboard = board.createCopy()
                u = newboard.getUnitOnSquare(self.x,self.y)
                u["count"] -= numRunAway
                if u["count"] <= 0:
                    newboard.clearSquare(self.x,self.y)
                else:
                    newboard.setUnitOnSquare(self.x,self.y,u,team)
                states.append(newboard)
                probs.append(1/6)
            return states,probs
                
                


class EndPhaseCommand:
    def __init__(self, curPhase, allTeams):
        self.phase = curPhase
        self.allTeams = allTeams
        
    def __eq__(self, other):
        if other.__class__ != self.__class__:
            return False
        return (self.curPhase == other.curPhase)
                
    def __str__(self):
        return "End phase."
    
    def apply(self, board):
        states, probs = [board.createCopy()], [1.0]
        for team in self.allTeams:
            for x,y in board.getAllUnits(team):
                if board.getUnitOnSquare(x,y)["modelsLostThisPhase"] > 0:
                    cmd = CheckMoraleCommand(x,y)
                    states, probs = combineDistributions(cmd, states, probs)
        #Remove all flags on the units:
        for state in states:
            for team in self.allTeams:
                for i,j in state.getAllUnits(team):
                    u = state.getUnitOnSquare(i,j)
                    if self.phase == FIGHT_PHASE: #End turn
                        u["movedThisTurn"] = False
                        u["firedThisTurn"] = False
                        u["attemptedChargeThisTurn"] = False
                        u["successfulChargeThisTurn"] = False
                        u["foughtThisTurn"] = False
                    u["modelsLostThisPhase"] = 0
                    state.setUnitOnSquare(i,j,u,team)
        return states, probs
        
        
        