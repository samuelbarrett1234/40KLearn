from board import Board
from move_cmd import getMoveCommands
from shoot_cmd import getShootCommands
from charge_cmd import getChargeCommands
from fight_cmd import getFightCommands
from end_cmd import EndPhaseCommand
from game_util import *
import random
from bisect import bisect_left


"""
A convenient wrapper class which encapsulates the
tracking of the game state and which units have
been chosen, etc.
"""
class GameWrapper:
    """
    Initialise a game with the given unit roster (array of
    units) and placements (a list of tuples (unit_idx, team, x, y)).
    """
    def __init__(self, unitRoster, placements):
        self.team = 0
        self.phase = MOVEMENT_PHASE
        self.currentState = Board(24, 1.0)
        for unitIdx, team, x, y in placements:
            unit = unitRoster[unitIdx]
            #NOTE: there are a couple state-specific entries
            # we need to add to the unit's dictionary:
            unit["movedThisTurn"] = False
            unit["firedThisTurn"] = False
            unit["attemptedChargeThisTurn"] = False
            unit["successfulChargeThisTurn"] = False
            unit["foughtThisTurn"] = False
            unit["movedOutOfCombatThisTurn"] = False
            unit["modelsLostThisPhase"] = 0
            self.currentState.setUnitOnSquare(x, y, unit, team)
        self.activeUnits = list(self.currentState.getAllUnits(self.team))
        random.shuffle(self.activeUnits)
    
    """
    Create and return a full copy of the current object
    """
    def createCopy(self):
        return deepcopy(self)

    """
    Get the current playing team (0 or 1)
    """
    def getCurrentTeam(self):
        return self.team

    """
    Get the current phase
    """
    def getPhase(self):
        return self.phase

    """
    Get the position of the unit currently under consideration
    """
    def getCurrentUnit(self):
        return self.activeUnits[-1]
        
    """
    Get and return the current state's last message
    """
    def getMessage(self):
        return self.currentState.getMessage()

    """
    Determine if the current game has finished.
    """
    def finished(self):
        return (len(self.currentState.getAllUnits(0)) == 0 or len(self.currentState.getAllUnits(1)) == 0)

    """
    Get the list of possible actions we can take. This DOES
    include "no-op", which is just the None value, and represents
    doing nothing with the current unit.
    """
    def getCurrentOptions(self):
        #TODO: the fight phase is currently done wrong!
        #Players should alternate to make their fight
        # decisions, not like how it is done now.
        #Get options for this unit in this phase:
        options = []
        phase = self.getPhase()
        ux,uy = self.activeUnits[-1]
        if phase == MOVEMENT_PHASE:
            options = getMoveCommands(ux, uy, self.currentState)
        elif phase == SHOOTING_PHASE:
            options = getShootCommands(ux, uy, self.currentState)
        elif phase == CHARGE_PHASE:
            options = getChargeCommands(ux, uy, self.currentState)
        else: #Fight phase
            options = getFightCommands(ux, uy, self.currentState)

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
            #End turn (do morale, etc)
            cmd = EndPhaseCommand(self.phase, [0,1])
            results, probs = cmd.apply(self.currentState)
            
            self.phase += 1 #Advance phase
            if self.phase == FIGHT_PHASE + 1:
                self.team = 1.0 - self.team #Change team
                self.phase = MOVEMENT_PHASE #Reset phase
                
            self.currentState = self._selectRandomly(results, probs)
            #Restore the list of active units:
            self.activeUnits = list(self.currentState.getAllUnits(self.team))
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
            return [self.currentState], [1.0]
        else:
            return option.apply(self.currentState)

    """
    A helper function for selecting a result randomly
    based on an array of corresponding probabilities.
    """
    def _selectRandomly(self, results, probs):
        cumulative = [sum(probs[:i]) for i in range(1, len(probs) + 1)]
        r = random.random()
        i = bisect_left(cumulative, r)
        return results[i]
