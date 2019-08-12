from game_util import *


"""
Return a list of possible fight commands from the position
(x,y) on the given board. (Note: there must actually be a
unit at position (x,y)!).
"""
def getFightCommands(x, y, board):
    assert(board.isOccupied(x,y))
    unit = board.getUnitOnSquare(x,y)
    team = board.getTeamOnSquare(x,y)    
    #If not actually in melee, no fight commands!
    if not board.hasAdjacentEnemy(x,y,team):
        return []        
    #If no melee weapon, can't fight!
    if unit["ml_s"] <= 0.0 or unit["a"] <= 0.0:
        return []
    #Start off by just getting all of the positions which are
    # adjacent (note 
    possible = [(i,j) for j in range(y-1,y+2) if y >= 0 and y < board.size\
        for i in range(x-1,x+2) if x >= 0 and x < board.size]
    #Return all of the positions in range which are occupied
    # by an enemy unit.
    return [FightCommand(x,y,i,j) for i,j in possible\
        if board.isOccupied(i,j) and board.getTeamOnSquare(i,j) != team]


class FightCommand:
    def __init__(self, x1, y1, x2, y2):
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        
    def __eq__(self, other):
        if other.__class__ != self.__class__:
            return False
        return (self.x1 == other.x1 and self.y1 == other.y1\
                and self.x2 == other.x2 and self.y2 == other.y2)
                
    def __str__(self):
        return "Fight target at location " + str((self.x2,self.y2))
    
    def apply(self, board):
        #Get data and perform checks
        assert(board.isOccupied(self.x1, self.y1))
        assert(board.isOccupied(self.x2, self.y2))
        fighter = board.getUnitOnSquare(self.x1, self.y1)
        fighterTeam = board.getTeamOnSquare(self.x1, self.y1)
        target = board.getUnitOnSquare(self.x2, self.y2)
        targetTeam = board.getTeamOnSquare(self.x2, self.y2)
        assert(abs(self.x1-self.x2) + abs(self.y1-self.y2) <= 2) #Check that we are adjacent
        assert(targetTeam != fighterTeam)
        
        #Determine probability of a penetrating hit
        pPenetratingHit = getPenetratingHitProbability(fighter["ws"], fighter["ml_s"],\
            fighter["ml_ap"], target["t"], target["sv"], target["inv"])
        
        #Compute number of shots
        numHits = fighter["a"] * fighter["count"]
        
        #Get damage
        dmg = fighter["ml_dmg"]
        
        #Set this
        fighter["foughtThisTurn"] = True
        
        states, probs = [], []
        for r in range(0, numHits+1):
            probs.append(binomialProbability(numHits, pPenetratingHit, r))
            newboard = board.createCopy()
            
            #Build up message
            msg = fighter["name"] + " fought " + target["name"] + " in melee "
            msg += ", with " + str(numHits) + " attacks, " + str(r)
            msg += " of them hitting, for a total of " + str(dmg * r)
            msg += " damage. "
            
            #Resolve damage
            target2 = deepcopy(target)
            target2["total_w"] -= dmg * r
            target2["count"] = math.ceil(target2["total_w"] / target2["w"])
            
            #Track the number of models lost, for leadership tests
            target2["modelsLostThisPhase"] += target["count"]-target2["count"]
            
            if target2["total_w"] <= 0.0:
                newboard.clearSquare(self.x2, self.y2)
                msg += "The target was completely destroyed."
            else:
                newboard.setUnitOnSquare(self.x2, self.y2, target2, targetTeam)
                msg += "The target has " + str(target2["count"]) + " models remaining."                
            newboard.setUnitOnSquare(self.x1, self.y1, fighter, fighterTeam)
            
            newboard.setMessage(msg)
            states.append(newboard)
        return states, probs