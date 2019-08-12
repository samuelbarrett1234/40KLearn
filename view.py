import pygame


"""
The window to view a game in.
NOTE: there should never be more than one
instance of this class!
"""
class GameView:
    def __init__(self, model, controller):
        pygame.init()
        pygame.font.init()
        self.model = model
        self.controller = controller
        #Configurable:
        self.pixelsPerCell = 20
        self.padding = 20
        self.boardSize = model.getSize() #Board size measured in number-of-cells
        self.boardLength = self.boardSize * self.pixelsPerCell
        #Computed positions:
        self.titleTop = self.padding
        self.titleLeft = self.padding
        self.boardTop = 3 * self.padding
        self.boardLeft = self.padding
        self.selectedTop = self.padding * 3
        self.selectedLeft = self.padding * 2 + self.boardWidth
        self.activeTop = self.padding * 4 + 0.5 * (self.boardWidth-self.padding)
        self.activeLeft = self.padding * 2 + self.boardWidth
        self.updateTop = self.padding * 4 + self.boardWidth
        self.updateLeft = self.padding
        self.display = pygame.display.set_mode((3*self.padding+2*self.boardWidth,\
            4*self.padding+self.boardWidth))
        #Colours:
        self.windowBackground = (149,150,156)
        self.boardBackground = (227,165,59)
        self.activeColour = (13,252,45)
        self.optionColour = (255,235,13)
        self.alliedColour = (66,135,245)
        self.enemyColour = (252,37,13)
        #Other values:
        pygame.display.set_caption("40KLearn")
        
    def run(self):
        exit = False
        while not exit and not self.model.finished():
            #Handle events
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    exit = True
                #TODO: if click, transform to index-space then pass to controller
                
            #Get data from model to update:
            
            #Cache update data
            alliedPositions = self.model.getAlliedPositions()
            enemyPositions = self.model.getEnemyPositions()
            optionPositions = self.model.getOptionPositions()
            activePosition = self.model.getActivePosition()
            
            #Render
            self.display.fill(self.windowBackground)
            self.display.fill(self.boardBackground,\
                rect=pygame.Rect(self.boardLeft,self.boardTop,self.boardLength,self.boardLength)
            #Colour each part of the board
            for i in range(self.boardSize):
                for j in range(self.boardSize):
                    x,y = self.boardLeft+i*self.pixelsPerCell,self.boardTop+j*self.pixelsPerCell
                    cell_rect = pygame.Rect(x,y,self.pixelsPerCell,self.pixelsPerCell)
                    colour = None
                    if (i,j) == activePosition:
                        colour = self.activeColour
                    elif (i,j) in optionPositions:
                        colour = self.optionColour
                    elif (i,j) in alliedPositions:
                        colour = self.alliedColour
                    elif (i,j) in enemyPositions:
                        colour = self.enemyColour
                    if colour is not None:
                        self.display.fill(colour, rect=cell_rect)
                        
            #Update
            pygame.display.update()
        pygame.font.quit()
        pygame.quit()