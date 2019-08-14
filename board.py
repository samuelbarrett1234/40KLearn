from game_util import *


"""
The game board state. This does not record what turn and
phase it is, however it can encapsulate data about which
units have moved this turn, etc, because they will be stored
in the unit dictionary.
"""
class Board:
    msg = ""
    
    def __init__(self, size, scale):
        self.board = [[None for j in range(size)] for i in range(size)]
        self.size = size
        self.scale = scale
        
    def __eq__(self, other):
        return (self.size == other.size and\
                self.scale == other.scale and\
                self.board == other.board)
        
    def createCopy(self):
        return deepcopy(self)
        
    def isOccupied(self, x, y):
        assert(x >= 0 and y >= 0 and x < self.size and y < self.size)
        return (self.board[x][y] is not None)
        
    def getTeamOnSquare(self, x, y):
        assert(self.isOccupied(x,y))
        return self.board[x][y][1]
        
    def getUnitOnSquare(self, x, y):
        assert(self.isOccupied(x,y))
        return deepcopy(self.board[x][y][0]) #Copy to prevent edits
    
    def setUnitOnSquare(self, x, y, unit, team):
        assert(x >= 0 and y >= 0 and x < self.size and y < self.size)
        self.board[x][y] = [unit, team]
        
    def clearSquare(self, x, y):
        assert(x >= 0 and y >= 0 and x < self.size and y < self.size)
        self.board[x][y] = None
        
    def setMessage(self, msg):
        self.msg = msg
        
    def getMessage(self):
        return self.msg
        
    def getDistance(self, x, y, a, b):
        return self.scale * ((x-a)**2.0 + (y-b)**2.0)**0.5
        
    def getSquaresInRange(self, x, y, r):
        #Bring r back into our scale
        r /= self.scale
        
        cr = math.ceil(r)
        #Compute rectangle
        left = max(0, x-cr)
        right = min(self.size-1, x+cr)
        top = min(self.size-1, y+cr)
        bottom = max(0, y-cr)
        
        squares = []
        for i in range(left,right+1):
            for j in range(bottom,top+1):
                assert(i >= 0 and j >= 0 and i < self.size and j < self.size)
                if ( (x-i)**2.0 + (y-j)**2.0 ) <= r ** 2.0:
                    squares.append((i,j))
                
        return squares
        
    
    def hasAdjacentEnemy(self, x, y, team):
        for i in range(max(x-1,0),min(x+2,self.size)):
            for j in range(max(y-1,0),min(y+2,self.size)):
                cell = self.board[i][j]
                if cell is not None and cell[1] != team:
                    return True
        return False
        
    def getAllUnits(self, team):
        units=[]
        for i in range(self.size):
            col = self.board[i]
            for j in range(self.size):
                cell = col[j]
                if cell is not None and cell[1] == team:
                    units.append((i,j))
        return units

        
        
      
