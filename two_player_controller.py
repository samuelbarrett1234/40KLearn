
"""
This class COMBINES TWO CONTROLLERS, one for each
team, sending each controller the commands only
for their own units. The teams 0 and 1 are controlled
by controllers: controller0 and controller 1, respectively.
"""
class TwoPlayerController:
    def __init__(self, model, controller0, controller1):
        self.model = model
        self.controllers = [controller0, controller1]
        
    def onUpdate(self):
        self.controllers[self.model.getCurrentTeam()].onUpdate()
    
    def onClickPosition(self, x, y, bLeft):
        self.controllers[self.model.getCurrentTeam()].onClickPosition(x,y,bLeft)
            
    def onReturn(self):
        self.controllers[self.model.getCurrentTeam()].onReturn()
    
