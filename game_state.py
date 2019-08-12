import pandas as pd
import numpy as np
import math
import random
from copy import deepcopy


def nCr(n,r):
    assert(n > 0 and r <= n and r >= 0)
    return math.factorial(n) / math.factorial(r) / math.factorial(n-r)


BOARD = 50 #Size of a game board
MOVEMENT_PHASE = 0
SHOOTING_PHASE = 1
CHARGE_PHASE = 2
FIGHT_PHASE = 3


#The names and order of the unit stats
columns = [
    #NOTE: no randomness in the unit stats allowed!

    #The number of models left in the squad
    # (all members of squad must be identical)
    "count",
    #The unit's statistics
    "movement",
    "ws",
    "bs",
    #NOTE: we do not have strength here because it is encoded in the ml_s value
    "t",
    "w", #This is wounds per model, however THIS IS NEVER UPDATED.
    "total_w", #Note that this is total squad wounds, not per model
    "a",
    #Note that we are ignoring leadership and morale for now
    "sv",
    "inv",
    #Ranged weapon stats
    "rg_range",
    "rg_s",
    "rg_ap",
    "rg_dmg",
    "rg_shots",
    "rg_is_rapid",
    #Melee weapon stats
    "ml_s",
    "ml_ap",
    "ml_dmg"
    #Special rules (1 represents having the rule, 0 means not)
    
]

NUM_FEATURES = len(columns)
VECTOR_DIM = 2 * NUM_FEATURES #One set of features for each of the two teams

#Converting between vector and dictionary representations of a unit
def unitToVector(unitDict):
    return np.array([unitDict[col] for col in columns])
def vectorToUnit(unitVec):
    assert(unitVec.shape == (NUM_FEATURES,) and np.any(unitVec))
    return { columns[i] : unitVec[i] for i in range(len(columns)) }


class GameState:
    #The board representation
    #Note how we store double the amount of features here - this is
    # to represent team 0 and team 1 features. A full zero vector
    # represents an empty square, the first NUM_FEATURES entries are
    # nonzero iff an active team's unit is there, and the second NUM_FEATURES
    # entries are nonzero iff the other team's unit is there. Every
    # end-turn the features are swapped.
    board = [[np.zeros(VECTOR_DIM) for j in range(BOARD)] for i in range(BOARD)]

    #Whose turn is it and what phase is it
    turn = 0
    phase = MOVEMENT_PHASE

    #The message represents the last transition which brought about this
    # state. It may be something like "X fired at Y, causing Z wounds!".
    message = ""

    """
    Initialise the game model
    unitDataset : a list of all the game's units (a list of units represented as dictionaries)
    placements  : a list of integer tuples representing (team, unitIndex, x, y), where team is either 0 or 1.
    """
    def __init__(self, unitDataset, placements):
        self.units = unitDataset #Store the unit dataset
        self.armies = [[i for team,i,_,_ in placements if team==0], [i for team,i,_,_ in placements if team==1]] #Store the army lists
        self.phase = MOVEMENT_PHASE
        #This matrix, when mapped over the board, switches the features around so that
        # the active team's units are represented by the first half of the vector.
        self.switch = np.block([[np.zeros((NUM_FEATURES,NUM_FEATURES)), np.identity(NUM_FEATURES)], [np.identity(NUM_FEATURES), np.zeros((NUM_FEATURES,NUM_FEATURES))]])
        #Initialise board:
        for team, unitIdx, ux, uy in placements:
            unitVec = unitToVector(self.units[unitIdx])
            zeros = np.zeros(NUM_FEATURES)
            if team == self.turn:
                unitVec = np.concatenate((unitVec, zeros))
            else:
                unitVec = np.concatenate((zeros, unitVec))
            assert(ux < BOARD and uy < BOARD)
            self.board[ux][uy] = unitVec

    """
    Helper function
    Create and return a full copy of the current
    object.
    """
    def createCopy(self):
        newState = GameState(self.units, [])
        newState.armies = self.armies
        newState.phase = self.phase
        newState.turn = self.turn
        newState.board = deepcopy(self.board)
        return newState

    """
    Get the current state's last transition message.
    """
    def getMessage(self):
        return self.message

    """
    Returns the current phase (one of MOVEMENT_PHASE, SHOOTING PHASE,
    CHARGE_PHASE, FIGHT_PHASE)
    """
    def getPhase(self):
        return self.phase

    """
    Returns which team is currently playing (0 or 1).
    """
    def getCurrentTeam(self):
        return self.turn

    """
    This ends the current player's phase, and switches
    the active team around if necessary.
    Returns: a list (states, probabilities) of game states
    that can result from this action, and their likelihoods
    """
    def endPhase(self):
        newState = self.createCopy()
        if self.phase == FIGHT_PHASE:
            newState.turn = 1 - self.turn #switch team
            newState.phase = MOVEMENT_PHASE
            #Change the unit representations on the board by multiplying by the switch matrix
            newState.board = [[np.dot(self.switch, unit) for unit in row] for row in self.board]
        else:
            newState.phase = self.phase + 1

        #Return deterministic result
        return ([newState], [1.0])

    
    """
    Return a list of (x,y) positions of units held by the
    current (active) team.
    """
    def getAllCurrentUnits(self):
        units = []
        for i in range(BOARD):
            for j in range(BOARD):
                if self.isOccupied(i, j) and self.turn == self.getTeamOnSquare(i, j):
                    units.append((i,j))
        return units
    
    """
    Return a list of (x,y) positions of units held by the
    enemy (inactive) team.
    """
    def getAllEnemyUnits(self):
        units = []
        for i in range(BOARD):
            for j in range(BOARD):
                if self.isOccupied(i, j) and self.turn != self.getTeamOnSquare(i, j):
                    units.append((i,j))
        return units

    """
    Helper function
    Returns a list of all points (i,j) which are less than distance
    r from the given point (x,y) on the board.
    """
    def getPointsFromRadius(self, x, y, r):
        return [(i,j) for j in range(BOARD) for i in range(BOARD) if (i-x)**2.0+(j-y)**2.0 <= r**2.0 and i >= 0 and j >= 0 and i < BOARD and j < BOARD]

    """
    Helper function
    Determine if square (x,y) is occupied.
    """
    def isOccupied(self, x, y):
        assert(x < BOARD and y < BOARD)
        return np.any(self.board[x][y])

    """
    Determine which team's unit is on a given
    square. PRECONDITION: isOccupied(x,y).
    """
    def getTeamOnSquare(self, x, y):
        assert(x < BOARD and y < BOARD and self.isOccupied(x,y))
        if np.any(self.board[x][y][:NUM_FEATURES]):
            return self.turn #Must be active team
        else:
            return 1 - self.turn #Must be inactive team
        
    
    """
    Check the unit at the given position (x,y). If that
    unit is destroyed (has zero total wounds) then it will
    be removed from play and the grid cell will be zeroed out.
    """
    def checkDestroyed(self, x, y):
        #Check both allied and enemy squares
        v1 = self.board[x][y][NUM_FEATURES:]
        v2 = self.board[x][y][:NUM_FEATURES]
        v = None
        if np.any(v1):
            v = v1
        elif np.any(v2):
            v = v2
        if v is not None and vectorToUnit(v)["total_w"] <= 0.0:
            self.board[x][y] = np.zeros(VECTOR_DIM) #Destroy the grid cell by zeroing

    """
    Determine if the given square has an enemy
    adjacent to it (but NOT counting the square
    it is called on.)
    """
    def hasAdjacentEnemy(self, x, y):
        assert(x < BOARD and y < BOARD)
        return np.any([self.isOccupied(tx, ty)\
                    and self.getTeamOnSquare(tx, ty) != self.turn\
                    for tx,ty in self.getPointsFromRadius(x, y, 1.0) if tx != x or ty != y])

    """
    unit: (x,y) the position of the unit to query
    returns: a list of (x,y) potential locations to move to
    """
    def getMovementLocations(self, unit):
        ux,uy = unit #Unpack
        #Make sure we're not about to move an enemy piece
        assert(self.getTeamOnSquare(ux, uy) == self.turn)
        mvmt = int(self.board[ux][uy][columns.index("movement")])

        if not self.hasAdjacentEnemy(ux, uy):
            #Make sure we don't move to a square with an adjacent enemy!
            return [(x,y) for x,y in self.getPointsFromRadius(ux, uy, mvmt)\
                if not self.isOccupied(x,y) and not self.hasAdjacentEnemy(x, y)]
        else:
            #Once locked in combat you cannot escape!
            return []
    
    """
    unit: (x,y) the position of the unit to query
    targetPos: (x,y) the position to move the unit to (must be valid)
    Returns: a list (states, probabilities) of game states
    that can result from this action, and their likelihoods.
    """
    def makeMovement(self, unit, targetPos):
        assert(targetPos in self.getMovementLocations(unit) and self.phase == MOVEMENT_PHASE)
        tx,ty = targetPos #unwrap
        ux,uy = unit
        
        newState = self.createCopy()
        newState.board[tx][ty] = newState.board[ux][uy]
        newState.board[ux][uy] = np.zeros(VECTOR_DIM)
        newState.message = "Unit at position " + str(unit) + " moved to " + str(targetPos)

        return ([newState], [1.0]) #Deterministic

    """
    Compute the distribution of the number of successful hits.
    Returns the probability of a successful hit.
    hitSkill = BS or WS
    wpnStrength = strength of attacking weapon
    wpnAp = AP of attacking weapon
    targetToughness = toughness of target
    targetSv = SV of target
    targetInv = INV of target
    """
    def getPenetratingHitProbability(self, hitSkill, wpnStrength, wpnAp, targetToughness, targetSv, targetInv):
        assert(targetToughness > 0.0)
        #Compute the probabilities!
        pHit = (7.0 - hitSkill) / 6.0
        pWound = 0.5
        strengthRatio = wpnStrength / targetToughness
        if strengthRatio >= 2.0:
            pWound = 5 / 6
        elif strengthRatio > 1.0:
            pWound = 4 / 6
        elif strengthRatio <= 0.5:
            pWound = 1 / 6
        elif strengthRatio < 1:
            pWound = 2 / 6
        #Else it will be 3/6

        pArmourSave = (7.0 - targetSv + wpnAp) / 6.0
        pInvulnerableSave = (7.0 - targetInv) / 6.0
        pOverallSave = max(pArmourSave, pInvulnerableSave) #We only get to make one saving throw

        return pHit * pWound * pOverallSave

    """
    For a given attack, get the different game states possible
    from this attack, and their probabilities.
    Returns (states, probabilities).
    """
    def getResultingStateDistribution(self, tx, ty, pPenetratingHit, numAttacks, dmg):
        #The distribution of damage is dmg*Bin(numAttacks,pPenetratingHit)
        states, probabilities = [],[]
        for i in range(0,numAttacks+1):
            #Here i represents the number of successful shots
            p = nCr(numAttacks, i) * pPenetratingHit ** i * (1.0-pPenetratingHit) ** (numAttacks-i) #Probability of result
            probabilities.append(p)
            #Create new model
            newState = self.createCopy()
            #Create copy of target stats and resolve damage
            targetUnit = newState.board[tx][ty][NUM_FEATURES:]
            targetStats = vectorToUnit(targetUnit)
            targetStats["total_w"] -= dmg * i
            newState.message = "Unit at (" + str(tx) + "," + str(ty) + ") was attacked and lost " + str(dmg * i) + " wounds."
            assert(targetStats["w"] > 0.0)
            #Compute the new number of members in the squad:
            targetStats["count"] = math.ceil(targetStats["total_w"] / targetStats["w"])
            #Update in board
            newState.board[tx][ty] = np.concatenate((np.zeros(NUM_FEATURES), unitToVector(targetStats)))
            #Remove the unit if destroyed
            newState.checkDestroyed(tx, ty)
            states.append(newState)
        return (states, probabilities)
    
    """
    unit: (x,y) the position of the unit to query
    returns: a list of (x,y) potential locations to shoot at (there will be an enemy at each one)
    """
    def getShootingLocations(self, unit):
        ux,uy = unit #Unpack
        wpnRange = int(self.board[ux][uy][columns.index("rg_range")])
        if not self.hasAdjacentEnemy(ux, uy):
            return [(x,y) for x,y in self.getPointsFromRadius(ux, uy, wpnRange)\
                if self.isOccupied(x,y) and self.getTeamOnSquare(x,y) != self.turn]
        else:
            return [] #Can't shoot if locked in combat

    """
    unit: (x,y) the position of the unit to query
    targetPos: (x,y) the position to shoot at (must be valid)
    Returns: a list (states, probabilities) of game states
    that can result from this action, and their likelihoods
    """
    def makeShootingAttack(self, unit, targetPos):
        assert(targetPos in self.getShootingLocations(unit) and self.phase == SHOOTING_PHASE)

        tx,ty = targetPos #unwrap
        
        ux,uy = unit
        distance = ((ux-tx) ** 2.0 + (uy-ty) ** 2.0) ** 0.5

        unitStats = vectorToUnit(self.board[ux][uy][:NUM_FEATURES])
        targetStats = vectorToUnit(self.board[tx][ty][NUM_FEATURES:])

        pTotal = self.getPenetratingHitProbability(unitStats["bs"], unitStats["rg_s"], unitStats["rg_ap"],\
            targetStats["t"], targetStats["sv"], targetStats["inv"])

        dmg = unitStats["rg_dmg"]
        
        numShots = int(unitStats["count"] * unitStats["rg_shots"])
        #Check for rapid fire
        if unitStats["rg_is_rapid"] and distance <= 0.5 * unitStats["rg_range"]:
            numShots *= 2
            
        return self.getResultingStateDistribution(tx, ty, pTotal, numShots, dmg)

    """
    unit: (x,y) the position of the unit to query
    returns: a list of (x,y) potential locations to charge to (there will be an enemy adjacent to each one)
    """
    def getChargeLocations(self, unit):
        ux,uy = unit #Unwrap
        nearby = self.getPointsFromRadius(ux, uy, 12.0)
        if not self.hasAdjacentEnemy(ux, uy):
            #Must charge to a location with an adjacent enemy
            return [(tx,ty) for tx,ty in nearby if not self.isOccupied(tx, ty)\
                    and self.hasAdjacentEnemy(tx, ty)]
        else:
            return [] #Can't charge if locked in combat
        
    
    """
    unit: (x,y) the position of the unit to query
    targetPos: (x,y) the position to assault to (must be valid)
    Returns: a list (states, probabilities) of game states
    that can result from this action, and their likelihoods.
    """
    def makeCharge(self, unit, targetPos):
        assert(targetPos in self.getChargeLocations(unit) and self.phase == CHARGE_PHASE)

        #The distribution of two dice
        # where twoDice[i] is the probability of getting 2+i.
        twoDice = [(6.0 - abs(7.0-i)) / 36 for i in range(2, 13)]

        #Unwrap
        ux,uy = unit
        tx,ty = targetPos

        distance = math.ceil(((ux-tx) ** 2.0 + (uy-ty) ** 2.0) ** 0.5)

        pFail = 0.0
        pPass = 0.0

        #We have two scenarios: we make the charge, or we fail the charge.
        #TODO: add overwatch shooting in here!!

        for i in range(len(twoDice)):
            potentialDist = i + 2
            if potentialDist >= distance:
                pPass += twoDice[i]
            else:
                pFail += twoDice[i]
        
        #Now we only we only consider two models
        
        passState = self.createCopy()
        passState.board[tx][ty] = passState.board[ux][uy]
        passState.board[ux][uy] = np.zeros(VECTOR_DIM)

        #If we fail, the state remains as-is.

        self.message = "Failed charge."
        passState.message = "Successfully charged."
        
        return ([passState, self], [pPass, pFail])

    """
    unit: (x,y) the position of the unit to query
    returns: a list of (x,y) potential locations to fight
    (there will be an enemy on each one).
    NOTE: since both forces fight in the fight phase, there
    are no checks for which team is the "active one", so feel
    free to call this on enemy units, too.
    """
    def getFightLocations(self, unit):
        ux,uy = unit #Unwrap
        assert(self.isOccupied(ux, uy))
        curTeam = self.getTeamOnSquare(ux, uy)
        adjacent = self.getPointsFromRadius(ux, uy, 1.0)
        positions = []
        #Can fight any adjacent enemy square
        for tx,ty in adjacent:
            if self.isOccupied(tx, ty):
                if self.getTeamOnSquare(tx, ty) != curTeam:
                    positions.append((tx, ty))
        return positions
    
    """
    unit: (x,y) the position of the unit to query
    targetPos: (x,y) the position to fight (must be valid)
    Returns: a list (states, probabilities) of game states
    that can result from this action, and their likelihoods
    NOTE: since both forces fight in the fight phase, there
    are no checks for which team is the "active one", so feel
    free to call this on enemy units, too.
    """
    def makeFight(self, unit, targetPos):
        assert(targetPos in self.getFightLocations(unit) and self.phase == FIGHT_PHASE)

        tx,ty = targetPos #unwrap
        ux,uy = unit

        unitStats = vectorToUnit(self.board[ux][uy][:NUM_FEATURES])
        targetStats = vectorToUnit(self.board[tx][ty][NUM_FEATURES:])

        pTotal = self.getPenetratingHitProbability(unitStats["ws"], unitStats["ml_s"], unitStats["ml_ap"],\
                                targetStats["t"], targetStats["sv"], targetStats["inv"])

        dmg = unitStats["ml_dmg"]
        
        numHits = int(unitStats["count"] * unitStats["a"])
            
        return self.getResultingStateDistribution(tx, ty, pTotal, numHits, dmg)

    