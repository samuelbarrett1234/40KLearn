from game_util import *
from shoot_cmd import ShootCommand


"""
Return a list of possible charge commands from the position
(x,y) on the given board. (Note: there must actually be a
unit at position (x,y)!).
"""
def getChargeCommands(x, y, board):
    assert(board.isOccupied(x,y))
    mvmt = 12.0 #Everybody's charge distance
    #Start off by just getting all of the positions which are
    # in range.
    possible = board.getSquaresInRange(x,y,mvmt)
    team = board.getTeamOnSquare(x,y)
    unit = board.getUnitOnSquare(x,y)
        
    if board.hasAdjacentEnemy(x,y,team):
        return [] #Can't charge out of combat into another combat
        
    if unit["movedOutOfCombatThisTurn"]:
        return [] #Can't fall out of combat then charge
        
    #Return all of the positions in range which are not occupied
    # and ARE adjacent to an enemy.
    return [ChargeCommand(x,y,i,j) for i,j in possible\
        if not board.isOccupied(i,j) and board.hasAdjacentEnemy(i,j,team)]


class ChargeCommand:
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
        return "Charge location " + str((self.x2,self.y2))
    
    def apply(self, board):
        #Get data and perform checks
        assert(board.isOccupied(self.x1, self.y1))
        assert(not board.isOccupied(self.x2, self.y2))
        chargingUnit = board.getUnitOnSquare(self.x1, self.y1)
        chargingUnitTeam = board.getTeamOnSquare(self.x1, self.y1)
        distance = board.getDistance(self.x1, self.y1, self.x2, self.y2)
        assert(board.hasAdjacentEnemy(self.x2, self.y2, chargingUnitTeam))
        #Check that we are not charging with an enemy unit next to us
        assert(not board.hasAdjacentEnemy(self.x1, self.y1, chargingUnitTeam))
        #Can't charge more than 12 inches
        assert(distance <= 12.0)
        #Check we're not trying to charge if we've just left combat:
        assert(not chargingUnit["movedOutOfCombatThisTurn"])
        
        enemies = []
        for i in range(-1,2):
            for j in range(-1,2):
                #(x2,y2)+(i,j) is an adjacent square for us to look for enemies
                x,y = self.x2+i,self.y2+j
                if x >= 0 and y >= 0 and x < board.size and y < board.size and\
                    board.isOccupied(x,y) and chargingUnitTeam != board.getTeamOnSquare(x,y):
                    enemies.append((x,y))
                    assert(i != 0 or j != 0) #Centre square should be empty
        
        assert(len(enemies)>0)
        
        msg = chargingUnit["name"] + " is charging to square " + str((self.x1,self.y1))
        msg += " with " + str(len(enemies)) + " adjacent enemies. Overwatch results: "
        
        #To begin with, initialise this with the current state distribution (which
        # is certain), and then for each overwatch unit, apply the effects of its
        # shooting to the distribution, building it up as we go.
        states1, probs1 = [board.createCopy()], [1.0]
        
        #OVERWATCH:
        for enemy in enemies:
            tx,ty = enemy #Unpack
            assert(board.isOccupied(tx,ty))
            enemyTeam = board.getTeamOnSquare(tx,ty)
            assert(chargingUnitTeam != enemyTeam)
            enemyUnit = board.getUnitOnSquare(tx,ty)
            #If the enemy is not already in melee, and
            # has a ranged weapon, and is in range, then
            # fire overwatch:
            if not board.hasAdjacentEnemy(tx,ty,enemyTeam) and enemyUnit["rg_s"] > 0.0\
                and board.getDistance(self.x1,self.y1,tx,ty) <= enemyUnit["rg_range"]:
                #Create the shoot command and then apply it to the distribution:
                cmd = ShootCommand(tx, ty, self.x1, self.y1, specialHitSkill=6.0, safeTargeting=True)
                states1, probs1 = combineDistributions(cmd, states1, probs1)
                
        #Sort out overwatch messages:
        for s in states1:
            msg2 = s.getMessage()
            s.setMessage(msg + msg2)
        
        #Register the fact that the unit has, at least, attempted the charge:
        for s in states1:
            if s.isOccupied(self.x1, self.y1): #Charging unit may have been killed in overwatch!
                unit = s.getUnitOnSquare(self.x1, self.y1)
                unit["attemptedChargeThisTurn"] = True
                s.setUnitOnSquare(self.x1, self.y1, unit, chargingUnitTeam)
        
        #Distance checking:
        
        #twoDiceProbs[i] is the probability of rolling a distance
        # of i+2.
        twoDiceProbs = [(6.0-abs(7.0-i-2.0))/36.0 for i in range(11)] #i+2 represents the dice roll
        
        #Now we need to compute the probability we make the charge.
        #Note that we "make the charge" if the distance we roll on the
        # dice is greater than or equal to the ceil of the distance between
        # the two grid cells
        pPass = sum(twoDiceProbs[math.ceil(distance):])
        
        #Now for each of these probabilities, combine with each possible overwatch outcome:
        
        states2, probs2 = [], []
        
        #Add fail scenarios
        for s,p in zip(states1, probs1):
            s = s.createCopy()
            msg2 = s.getMessage()
            msg2 += " The charge failed!"
            s.setMessage(msg2)
            states2.append(s)
            probs2.append(p * (1.0-pPass))
        #Add pass scenarios
        for s,p in zip(states1, probs1):
            if s.isOccupied(self.x1, self.y1): #Charging unit may have been killed in overwatch!
                unit = s.getUnitOnSquare(self.x1, self.y1)
                unit["successfulChargeThisTurn"] = True
                s.clearSquare(self.x1, self.y1)
                s.setUnitOnSquare(self.x2, self.y2, unit, chargingUnitTeam)
                msg2 = s.getMessage()
                msg2 += " The charge was successful!"
                s.setMessage(msg2)
            else:
                s.setMessage("The unit was killed by overwatch!")
            #Always append these to ensure that the probabilities all work out.
            states2.append(s)
            probs2.append(p*pPass)
        
        return states2, probs2
        
    def getTargetPosition(self):
        return (self.x2,self.y2)
    
