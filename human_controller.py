from game_util import *


class HumanController:
    def __init__(self, model):
        self.model = model
    
    def onClick(self, x, y):
        assert((x,y) in self.model.getOptionPositions)
        self.model.choosePosition(x,y)