from game_util import *


"""
Return a list of possible move commands from the position
(x,y) on the given board. (Note: there must actually be a
unit at position (x,y)!).
"""
def getMoveCommands(x, y, board):
    assert(board.isOccupied(x,y))
    unit = board.getUnitOnSquare(x,y)
    mvmt = unit["movement"]
    #Start off by just getting all of the positions which are
    # in range.
    possible = board.getSquaresInRange(x,y,mvmt)
    team = board.getTeamOnSquare(x,y)
    #Return all of the positions in range which are not occupied
    # and not adjacent to an enemy.
    return [MoveCommand(x,y,i,j) for i,j in possible\
        if not board.isOccupied(i,j) and not board.hasAdjacentEnemy(i,j,team)]


class MoveCommand:
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
        return "Move to location " + str((self.x2,self.y2))
    
    def apply(self, board):
        assert(board.isOccupied(self.x1, self.y1))
        assert(not board.isOccupied(self.x2, self.y2))
        newboard = board.createCopy()
        team = newboard.getTeamOnSquare(self.x1, self.y1)
        unit = newboard.getUnitOnSquare(self.x1, self.y1)
        #Check that we aren't moving too far:
        assert(board.getDistance(self.x1,self.y1,self.x2,self.y2)<=unit["movement"])
        #Assert that we are not moving to a position with an adjacent enemy
        # (this can only be done in the charge phase).
        assert(not board.hasAdjacentEnemy(self.x2, self.y2, team))
        unit["movedThisTurn"] = True
        #Check if unit has just disengaged from combat
        unit["movedOutOfCombatThisTurn"] = board.hasAdjacentEnemy(self.x1, self.y1, team)
        newboard.clearSquare(self.x1, self.y1)
        newboard.setUnitOnSquare(self.x2, self.y2, unit, team)
        newboard.setMessage("Moved unit " + unit["name"] + " at " + str((self.x1,self.y1)) + " to " + str((self.x2,self.y2)))
        return [newboard], [1.0]
        
        
        