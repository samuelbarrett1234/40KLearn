from game_wrapper import GameWrapper


"""
Represents the interface by which the views/controllers
play the game. Note that this is an extra layer over the
game wrapper object, which provides handy summary information
and handles everything in terms of board-cell-coordinates.
"""
class Model:
    def __init__(self, unitRoster, placements):
        self.game = GameWrapper(unitRoster, placements)
        self.currentActions = self.game.getCurrentOptions()
        
    def finished(self):
        return self.game.finished()
        
    def choosePosition(self, x, y):
        assert((x,y) in self.getOptionPositions())
        #Now find the option which was at this position:
        option = [action for action in self.currentActions if action is not None and (x,y)==action.getTargetPosition()][0]
        self.game.chooseOption(option)
        self.currentActions = self.game.getCurrentOptions()
        
    def skip(self):
        self.game.chooseOption(None)
        self.currentActions = self.game.getCurrentOptions()
        
    def getActivePosition(self):
        return self.game.getCurrentUnit()
        
    def getOptionPositions(self):
        return [action.getTargetPosition() for action in self.currentActions if action is not None]
        
    def getSize(self):
        return self.game.getSize()
        
    def getPositionDesc(self, x, y):
        if not self.game.currentState.isOccupied(x,y):
            return "An empty cell."
        else:
            unitDesc = str(self.game.currentState.getUnitOnSquare(x,y))
            team = str(self.game.currentState.getTeamOnSquare(x,y))
            return unitDesc + " (team " + team + ")"
    
    def getAlliedPositions(self):
        return self.game.currentState.getAllUnits(self.game.getCurrentTeam())
    
    def getEnemyPositions(self):
        return self.game.currentState.getAllUnits(1-self.game.getCurrentTeam())
    
    def getSummary(self):
        team = self.game.getCurrentTeam()
        phase = self.game.getPhase()
        phaseNames = ["Movement phase", "Shooting phase",
            "Charge phase", "Fight phase"
        ]
        return "TEAM: " + str(team) + ", PHASE: " + phaseNames[phase]