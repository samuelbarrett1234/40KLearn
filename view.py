import pygame


"""
The window to view a game in.
NOTE: there should never be more than one
instance of this class!
"""
class View:
    def __init__(self, model, controller):
        pygame.init()
        self.model = model
        self.controller = controller
        self.display = pygame.display.set_mode((800,600))
        pygame.display.set_caption("40KLearn")
        #TODO: setup UI elements
        
    def run(self):
        exit = False
        while not exit and not self.model.finished():
            #Handle events
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    exit = True
            #Program logic
            #Render
            pygame.display.update()
        pygame.quit()