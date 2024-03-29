

class TwoPlayerController:
    """
    This class combines two 'child' controllers, one for each
    team, sending each controller the commands only for their
    own units. The teams 0 and 1 are controlled by controllers
    controller0 and controller 1, respectively. This class is
    basically for automatically delegating to the correct child
    controller, depending on which team is currently acting.
    """

    def __init__(self, model, controller0, controller1):
        self.model = model
        self.controllers = [controller0, controller1]

    def on_update(self):
        self.controllers[self.model.get_acting_team()].on_update()

    def on_click_position(self, pos, bLeft):
        ctrl = self.controllers[self.model.get_acting_team()]
        ctrl.on_click_position(pos, bLeft)

    def on_return(self):
        self.controllers[self.model.get_acting_team()].on_return()

    def on_turn_changed(self):
        self.controllers[self.model.get_acting_team()].on_turn_changed()
