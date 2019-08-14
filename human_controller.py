
"""
Allows a human player to control the model by
issuing orders to their units.
"""
class HumanController:
    def __init__(self, model):
        self.model = model
        
    def onUpdate(self):
        pass #No update needed
    
    def onClickPosition(self, x, y, bLeft):
        if(x,y) in self.model.getOptionPositions() and not bLeft:
            self.model.choosePosition(x,y)
            
    def onReturn(self):
        self.model.skip()
        
    def onTurnChanged(self):
        pass
    
