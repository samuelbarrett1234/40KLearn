from game_state import *
import numpy as np
import random
from bisect import bisect_left

"""
A convenient wrapper class which encapsulates the
tracking of the game state and which units have
been chosen, etc.
"""
class GameWrapper:
    def __init__(self, startState):
        self.currentState = startState
        self.activeUnits = list(self.currentState.getAllCurrentUnits())
        random.shuffle(self.activeUnits)
    
    """
    Create and return a full copy of the current object
    """
    def createCopy(self):
        copy = GameWrapper(self.currentState.createCopy())

    """
    Get the current playing team (0 or 1)
    """
    def getCurrentTeam(self):
        return self.currentState.getCurrentTeam()

    """
    Get the current phase
    """
    def getPhase(self):
        return self.currentState.getPhase()

    """
    Get the position of the unit currently under consideration
    """
    def getCurrentUnit(self):
        return self.activeUnits[-1]

    """
    Determine if the current game has finished.
    """
    def finished(self):
        return (len(self.currentState.getAllCurrentUnits()) == 0 or len(self.currentState.getAllEnemyUnits()) == 0)

    """
    Get the list of possible actions we can take. This DOES
    include "no-op", which is just the None value, and represents
    doing nothing with the current unit.
    """
    def getCurrentOptions(self):
        #Get options for this unit in this phase:
        options = []
        phase = self.getPhase()
        unit = self.activeUnits[-1]
        if phase == MOVEMENT_PHASE:
            options = self.currentState.getMovementLocations(unit)
        elif phase == SHOOTING_PHASE:
            options = self.currentState.getShootingLocations(unit)
        elif phase == CHARGE_PHASE:
            options = self.currentState.getChargeLocations(unit)
        else: #Fight phase
            options = self.currentState.getFightLocations(unit)

        options.append(None) #The no-op
        return options

    """
    Once an option has been selected, pass it in here (use option=None
    for no-op). This advances the state, possibly advancing the phase
    or turn.
    """
    def chooseOption(self, option):
        assert(len(self.activeUnits) > 0)

        if option != None:
            #Apply results to state
            results, probs = self.getResultsOf(option)
            self.currentState = self._selectRandomly(results, probs)

        #Pop current unit, and ensure that we ignore any other units with no options (excluding no-op):
        self.activeUnits.pop()
        while len(self.activeUnits) > 0 and len(self.getCurrentOptions()) <= 1:
            self.activeUnits.pop()

        #While no more units left then advance phase
        while len(self.activeUnits) == 0:
            results, probs = self.currentState.endPhase()
            self.currentState = self._selectRandomly(results, probs)
            #Restore the list of active units:
            self.activeUnits = list(self.currentState.getAllCurrentUnits())
            random.shuffle(self.activeUnits)
            #Ensure that our next unit in the list has options ready (excluding no-op):
            while len(self.activeUnits) > 0 and len(self.getCurrentOptions()) <= 1:
                self.activeUnits.pop()

    """
    Determine what the possible results of a given option
    would be, and their probabilities.
    Returns a list of game states, and a list of corresponding
    probabilities.
    """
    def getResultsOf(self, option):
        if option == None:
            return [self], [1.0]
        else:
            phase = self.getPhase()
            unit = self.activeUnits[-1]
            resultStates, resultProbs = [],[]
            if phase == MOVEMENT_PHASE:
                resultStates, resultProbs = self.currentState.makeMovement(unit, option)
            elif phase == SHOOTING_PHASE:
                resultStates, resultProbs = self.currentState.makeShootingAttack(unit, option)
            elif phase == CHARGE_PHASE:
                resultStates, resultProbs = self.currentState.makeCharge(unit, option)
            else: #Fight phase
                resultStates, resultProbs = self.currentState.makeFight(unit, option)
            return resultStates, resultProbs

    """
    A helper function for selecting a result randomly
    based on an array of corresponding probabilities.
    """
    def _selectRandomly(self, results, probs):
        cumulative = [sum(probs[:i]) for i in range(1, len(probs) + 1)]
        r = random.random()
        i = bisect_left(cumulative, r)
        return results[i]
