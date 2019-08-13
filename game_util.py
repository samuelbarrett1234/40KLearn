import math
import random
from copy import deepcopy
import csv


MOVEMENT_PHASE = 0
SHOOTING_PHASE = 1
CHARGE_PHASE = 2
FIGHT_PHASE = 3


def nCr(n,r):
    assert(n > 0 and r <= n and r >= 0)
    return math.factorial(n) / math.factorial(r) / math.factorial(n-r)
   
   
def binomialProbability(n, p, r):
    return nCr(n, r) * (p ** r) * ((1.0-p) ** (n-r))


#The names and order of the unit stats
columns = [
    #NOTE: no randomness in the unit stats allowed!
    
    "name",
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
    "ld",
    "sv",
    "inv",
    #Ranged weapon stats
    "rg_range",
    "rg_s",
    "rg_ap",
    "rg_dmg",
    "rg_shots",
    "rg_is_rapid",
    "rg_is_heavy", #Set this to true if you want a movement penalty for shooting accuracy
    #Melee weapon stats
    "ml_s",
    "ml_ap",
    "ml_dmg",
    #Special rules (1 represents having the rule, 0 means not)
    
    #OTHER (1 true, 0 false)
    "movedThisTurn",
    "firedThisTurn",
    "attemptedChargeThisTurn", #True whenever a unit has tried to charge, regardless of success
    "successfulChargeThisTurn", #True if the unit has attempted and made a charge
    "foughtThisTurn",
    "movedOutOfCombatThisTurn", #True iff the unit has just disengaged from combat this turn
    "modelsLostThisPhase" #The number of models lost this turn
]


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
def getPenetratingHitProbability(hitSkill, wpnStrength, wpnAp, targetToughness, targetSv, targetInv):
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
This helper function will apply a given command to a given
state distribution, to generate a new state distrbution.
"""
def combineDistributions(command, states, probabilities):
    results, resultProbs = [], []
    for s,p in zip(states, probabilities):
        ss, ps = command.apply(s)
        results += ss
        resultProbs += [p2 * p for p2 in ps] #Combine the probabilities
    return results, resultProbs
  
"""
A helper function for loading a unit database CSV file in
and returning a list of stat dictionaries.
"""  
def loadUnitsCSV(filename):
    units = []
    with open(filename) as csv_file:
        csv_reader = csv.reader(csv_file, delimiter=',')
        isHeader = True
        colHeaders = []
        for row in csv_reader:
            if isHeader:
                colHeaders = row
                isHeader = False
            else:
                u = {}
                for i,entry in enumerate(row):
                    col = colHeaders[i]
                    if col != "name":
                        entry = int(entry) #Convert datatype if not unit name
                    u[col]=entry
                units.append(u)
    return units