from game_util import *


class HumanController:
    def __init__(self, model):
        self.model = model
    
    def onClickPosition(self, x, y, bLeft):
        if(x,y) in self.model.getOptionPositions() and not bLeft:
            self.model.choosePosition(x,y)
            
    def onReturn(self):
        self.model.skip()
    
