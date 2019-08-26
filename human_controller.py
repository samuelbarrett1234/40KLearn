
"""
Allows a human player to control the model by
issuing orders to their units.
"""
class HumanController:
    def __init__(self, model):
        self.model = model
        
    def on_update(self):
        pass #No update needed
    
    def on_click_position(self, pos, bLeft):
        if bLeft:
            self.model.set_active_position(pos)
        elif self.model.get_active_position() is not None:
            actions = self.model.get_actions()
            
            #Determine the action with the correct source and target pos
            source_pos = self.model.get_active_position()
            target_pos = pos
            
            for action in actions:
                if action.get_type() == py40kl.UNIT_ORDER:
                    if action.get_source_position() == source_pos and\
                        action.get_target_position() == target_pos:
                        self.model.choose_action(action)
                        return None #Finished
            
            #Else ignore the request because we were given an incorrect action.
            
    def onReturn(self):
        #If the user presses return, apply the end phase action if available:
        actions = self.model.get_actions()
        for action in actions:
            if action.get_type() == py40kl.END_PHASE:
                self.model.choose_action(action)
                return None #Finished
        
    def onTurnChanged(self):
        pass #No logic needed
    
