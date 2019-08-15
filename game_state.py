from board import Board
from move_cmd import getMoveCommands
from shoot_cmd import getShootCommands
from charge_cmd import getChargeCommands
from fight_cmd import getFightCommands
from end_cmd import EndPhaseCommand
from game_util import *


BOARD_SIZE = 24
BOARD_SCALE = 1.0


"""
Create a new game with the given unit roster (array of
units) and placements (a list of tuples (unit_idx, team, x, y)).
"""
def newGameState(unitRoster, placements):
    gs = GameState()
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
        gs.currentState.setUnitOnSquare(x, y, unit, team)
    gs.activeUnits = list(gs.currentState.getAllUnits(gs.team))
    return gs
    
    
    
"""
This command is a wrapper around a board command which
represents a unit move/shoot/charge/fight order.
It is applied to states, instead of boards, and automatically
updates the state info not related to the board (for example
it removes the active unit from the queue).
"""
class UnitOrderStateCommand:
    def __init__(self, boardCmd):
        self.cmd = boardCmd
        
    def apply(self, state):
        boards, probs = [state.getBoard().createCopy()], [1.0]
        if self.cmd is not None: #None => empty action
            boards,probs = combineDistributions(self.cmd, boards, probs)
        states = [state.createCopyFromBoard(b) for b in boards]
        for s in states:
            s.activeUnits.pop()
        return states, probs
        
    def getTargetPosition(self):
        if self.cmd is None:
            return None
        else:
            return self.cmd.getTargetPosition()
        
        
"""
This command is a wrapper around the end phase board command.
It is applied to states, instead of boards, and automatically
updates the state info not related to the board (for example
the current phase).
"""
class EndPhaseStateCommand:
    def apply(self, state):
        boardCmd = EndPhaseCommand(state.getPhase(), [0,1])
        boards,probs = boardCmd.apply(state.getBoard())
        states = [state.createCopyFromBoard(b) for b in boards]
        for s in states:            
            s.phase += 1 #Advance phase
            if s.phase == FIGHT_PHASE + 1:
                s.team = 1 - s.team #Change team
                s.phase = MOVEMENT_PHASE #Reset phase
        return states, probs
        
    def getTargetPosition(self):
        return None
        
        
"""
This command forwards a state (by advancing phase, etc) until
it is at a point where there a nonzero number of active units,
the first of which has at least one option (not including the no-op).
I.e. brings the state to a valid position where the active team
can start playing.
If the forward state command is already in a valid situation, it
will do nothing.
"""
class ForwardStateCommand:
    def apply(self, state):
        #While no more units left then advance phase
        if len(state.activeUnits) == 0 and not state.finished():
            #End turn (do morale, etc)
            cmd = EndPhaseStateCommand()
            results, probs = cmd.apply(state)
            
            for result in results:
                #Restore the list of active units:
                result.activeUnits = list(result.currentState.getAllUnits(result.team))
                #Ensure that our next unit in the list has
                # options ready (excluding no-op):
                while len(result.activeUnits) > 0\
                    and len(result.getCurrentOptions()) <= 1:
                    result.activeUnits.pop()
                    
            #Now we need to recurse to forward again if necessary:
            results,probs= combineDistributions(ForwardStateCommand(), results, probs)
            
            return results,probs
        else:
            return [state],[1.0]
        
        
    


"""
This class encapsulates all of the game's state, like
the board, the teams, whose turn it is, what phase it
is, and also which units have acted this phase and which
are left. Can also compute the game value and whether or
not the game has finished.
This is an immutable data structure, and modifications to
it instead return a distribution of possible future states.
"""
class GameState:
    team = 0
    phase = MOVEMENT_PHASE
    currentState = Board(BOARD_SIZE, BOARD_SCALE)
    activeUnits = []
        
    def __eq__(self, other):
        return (self.team == other.team and\
                self.phase == other.phase and\
                self.currentState == other.currentState and\
                self.activeUnits == other.activeUnits)
    
    """
    Create and return a full copy of the current object
    """
    def createCopy(self):
        gs = GameState()
        gs.currentState = self.currentState.createCopy()
        gs.team = self.team
        gs.phase = self.phase
        gs.activeUnits = deepcopy(self.activeUnits)
        return gs
        
    """
    Create a copy of this game state, but updating the
    board to the given 'board' state.
    """
    def createCopyFromBoard(self, board):
        gs = self.createCopy()
        gs.setBoard(board)
        return gs

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
    Get board size (in cells)
    """
    def getSize(self):
        return self.currentState.size

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
    If the game has finished, determine who won.
    Returns the value of the game for the team 'team'.
    1 = win, 0 = draw, -1 = loss
    """
    def getGameValue(self, team):
        assert(self.finished())
        # bLeft[i] is true iff team i has any units left
        bLeft = [(len(self.currentState.getAllUnits(0)) != 0),
            (len(self.currentState.getAllUnits(1)) != 0)]
        if bLeft[team]:
            return 1.0 #Win
        elif bLeft[1-team]:
            return -1.0 #Loss
        else:
            return 0.0 #Draw

    """
    Get the list of possible actions we can take. This DOES
    include "no-op", which is just the None value, and represents
    doing nothing with the current unit.
    """
    def getCurrentOptions(self):
        assert(not self.finished())
        assert(len(self.activeUnits) > 0)
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
            
        #Convert board options to state options
        options = [UnitOrderStateCommand(cmd) for cmd in options]
        return options

    """
    Once an option has been selected, pass it in here (use option=None
    for no-op). This returns a distribution over future game states,
    by returning [states], [probabilities].
    """
    def chooseOption(self, option):
        assert(len(self.activeUnits) > 0 and not self.finished())

        results, probs = [self.createCopy()], [1.0]

        #Apply results to state
        results, probs = combineDistributions(option, results, probs)

        for state in results:
            if state.finished():
                continue #Ignore finished states
            #Ensure that we ignore any other units with no options (excluding no-op):
            while len(state.activeUnits) > 0 and len(state.getCurrentOptions()) <= 1:
                state.activeUnits.pop()
        
        #The ForwardStateCommand does exactly what we want, in that
        # it will advance all results to a position where they are
        # ready to be played.
        results,probs = combineDistributions(ForwardStateCommand(), results, probs)
        
        return results,probs
            
    def getBoard(self):
        return self.currentState
        
    def setBoard(self, board):
        self.currentState = board

