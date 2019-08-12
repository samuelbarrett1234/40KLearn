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
        self.currentActions = []
        
    def finished(self):
        return self.game.finished()
        
    def getActivePosition(self):
        return self.game.getCurrentUnit()
        
    def getOptionPositions(self):
        return [action.getTargetPosition() for action in self.currentActions]
        
    def getSize(self):
        return self.game.getSize()
        
    def getPositionDescription(self, x, y):
        if not self.game.currentState.isOccupied(x,y):
            return "An empty cell."
        else:
            return str(self.game.currentState[x][y][0])