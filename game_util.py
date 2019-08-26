import math
import random
from copy import deepcopy
import csv
from bisect import bisect_left


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
    # (none as of yet)
]


"""
A helper function for loading a unit database CSV file in
and returning a list of stat dictionaries.
"""  
def load_units_csv(filename):
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
    
    
"""
A helper function for selecting a result randomly
based on an array of corresponding probabilities.
"""
def select_randomly(results, probs):
    assert(len(results)==len(probs))
    r = random.random()
    total = 0.0
    for i,p in enumerate(probs):
        total += p
        if total > r:
            return results[i]
    print(probs)
    assert(False and "Should never reach here")
    
    
    