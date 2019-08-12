from game_util import *


"""
Return a list of possible shoot commands from the position
(x,y) on the given board. (Note: there must actually be a
unit at position (x,y)!).
"""
def getShootCommands(x, y, board):
    assert(board.isOccupied(x,y))
    unit = board.getUnitOnSquare(x,y)
    #If no ranged weapon, can't shoot!
    if unit["rg_s"] <= 0.0 or unit["rg_range"] <= 0.0 or unit["rg_shots"] <= 0:
        return []
    range = unit["rg_range"]
    team = board.getTeamOnSquare(x,y)
    if board.hasAdjacentEnemy(x,y,team):
        return [] #Can't shoot out of combat
        
    if unit["movedOutOfCombatThisTurn"]:
        return [] #Can't fall out of combat then shoot
        
    #Start off by just getting all of the positions which are
    # in range.
    possible = board.getSquaresInRange(x,y,range)
    
    #Return all of the positions in range which are occupied
    # by an enemy unit. Furthermore, we can't shoot INTO a
    # combat, so ensure we don't do that.
    return [ShootCommand(x,y,i,j) for i,j in possible\
        if board.isOccupied(i,j) and board.getTeamOnSquare(i,j) != team\
        and not board.hasAdjacentEnemy(i,j,board.getTeamOnSquare(i,j))]


class ShootCommand:
    def __init__(self, x1, y1, x2, y2, specialHitSkill=None, safeTargeting=False):
        self.x1 = x1
        self.y1 = y1
        self.x2 = x2
        self.y2 = y2
        #This is for convenience with shooting like overwatch
        # If not None, this will replace shooter BS.
        self.specialHitSkill = specialHitSkill
        #A bit of a hacky solution to the problem of overwatch
        # where many units are firing at one charging unit and
        # destroy it (invalidating the other commands).
        #I.e. if this is true, instead of assertions failing
        # we will just exit silently.
        self.safeTargeting = safeTargeting
        
    def __eq__(self, other):
        if other.__class__ != self.__class__:
            return False
        return (self.x1 == other.x1 and self.y1 == other.y1\
                and self.x2 == other.x2 and self.y2 == other.y2)
                
    def __str__(self):
        return "Shoot location " + str((self.x2,self.y2))
    
    def apply(self, board):
        #If safe targeting is enabled, fail silently if there is no target at (x2,y2):
        if not board.isOccupied(self.x2, self.y2) and self.safeTargeting:
            return [board.createCopy()], [1.0]
            
        #Get data and perform checks
        assert(board.isOccupied(self.x1, self.y1))
        assert(board.isOccupied(self.x2, self.y2))
        shooter = board.getUnitOnSquare(self.x1, self.y1)
        shooterTeam = board.getTeamOnSquare(self.x1, self.y1)
        target = board.getUnitOnSquare(self.x2, self.y2)
        targetTeam = board.getTeamOnSquare(self.x2, self.y2)
        distance = board.getDistance(self.x1, self.y1, self.x2, self.y2)
        assert(distance <= shooter["rg_range"])
        assert(targetTeam != shooterTeam)        
        #Check that we are not firing out of combat
        assert(not board.hasAdjacentEnemy(self.x1, self.y1, shooterTeam))
        assert(shooter["rg_s"] > 0.0) #Must actually have a ranged weapon
        #Check we're not trying to shoot if we've just left combat:
        assert(not shooter["movedOutOfCombatThisTurn"])
        #Can't shoot into a melee
        assert(not board.hasAdjacentEnemy(self.x2, self.y2, targetTeam))
        
        #Determine probability of a penetrating hit
        hitSkill = shooter["bs"]
        if shooter["rg_is_heavy"] and shooter["movedThisTurn"]:
            hitSkill = 6.0 #6+ to hit
        if self.specialHitSkill is not None:
            hitSkill = self.specialHitSkill
        pPenetratingHit = getPenetratingHitProbability(hitSkill, shooter["rg_s"],\
            shooter["rg_ap"], target["t"], target["sv"], target["inv"])
        
        #Compute number of shots
        numShots = shooter["rg_shots"] * shooter["count"]
        if distance <= 0.5 * shooter["rg_range"] and shooter["rg_is_rapid"]:
            numShots *= 2 #Rapid fire enabled
        
        #Get damage
        dmg = shooter["rg_dmg"]
        
        #Set this
        shooter["firedThisTurn"] = True
        
        states, probs = [], []
        for r in range(0, numShots+1):
            probs.append(binomialProbability(numShots, pPenetratingHit, r))
            newboard = board.createCopy()
            
            #Build up message
            msg = shooter["name"] + " fired at " + target["name"]
            msg += " firing " + str(numShots) + " shots, " + str(r)
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
            newboard.setUnitOnSquare(self.x1, self.y1, shooter, shooterTeam)
            
            newboard.setMessage(msg)
            states.append(newboard)
        return states, probs
        
        
        
