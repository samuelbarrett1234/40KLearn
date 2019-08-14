import pygame


"""
The window to view a game in.
NOTE: there should never be more than one
instance of this class!
"""
class GameView:
    def __init__(self, model, controller):
        pygame.init()
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
        self.selectedLeft = self.padding * 2 + self.boardLength
        self.activeTop = self.padding * 4 + 0.5 * (self.boardLength-self.padding)
        self.activeLeft = self.padding * 2 + self.boardLength
        self.updateTop = self.padding * 4 + self.boardLength
        self.updateLeft = self.padding
        self.display = pygame.display.set_mode((3*self.padding+2*self.boardLength,\
            4*self.padding+self.boardLength))
        #Colours:
        self.windowBackground = (149,150,156)
        self.boardBackground = (227,165,59)
        self.activeColour = (13,252,45)
        self.optionColour = (255,235,13)
        self.alliedColour = (66,135,245)
        self.enemyColour = (252,37,13)
        #Other values:
        pygame.display.set_caption("40KLearn")
        self.font = pygame.font.Font(None, 20)
        
    def run(self):
        selectedUnit = None
        exit = False
        while not exit and not self.model.finished():
            #Handle events
            for event in pygame.event.get():
                if event.type == pygame.MOUSEBUTTONDOWN:
                    mx,my = pygame.mouse.get_pos()
                    if mx >= self.boardLeft and mx < self.boardLeft+self.boardLength\
                        and my >= self.boardTop and my < self.boardTop+self.boardLength:
                        #Convert mouse position to index coordinates:
                        ix,iy = (mx-self.boardLeft)//self.pixelsPerCell,(my-self.boardTop)//self.pixelsPerCell
                        #Compute buttons
                        left,middle,right = pygame.mouse.get_pressed()
                        assert(not (left and right))
                        if right:
                            self.controller.onClickPosition(ix,iy,False)
                        else:
                            #Select a unit
                            selectedUnit = None
                            if (ix,iy) in self.model.getAlliedPositions() or (ix,iy) in self.model.getEnemyPositions():
                                selectedUnit = self.model.getPositionDesc(ix,iy)
                elif event.type == pygame.KEYDOWN:
                    if pygame.key.get_pressed()[pygame.K_RETURN]:
                        self.controller.onReturn()
                elif event.type == pygame.QUIT:
                    exit = True
                
            if self.model.changedTeam():
                self.controller.onTurnChanged()
            self.controller.onUpdate()
            #Get data from model to update:
            
            #Cache update data
            alliedPositions = self.model.getAlliedPositions()
            enemyPositions = self.model.getEnemyPositions()
            optionPositions = self.model.getOptionPositions()
            activePosition = self.model.getActivePosition()
            
            #Render
            self.display.fill(self.windowBackground)
            self.display.fill(self.boardBackground,\
                rect=pygame.Rect(self.boardLeft,self.boardTop,self.boardLength,self.boardLength))
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
                        
            if selectedUnit is not None:
                #Draw it as text:
                r = pygame.Rect(self.selectedLeft, self.selectedTop, self.boardLength, 0.5 * (self.boardLength-self.padding))
                drawText(self.display, str(selectedUnit), (0,0,0), r, self.font, aa=True)
                
            #Draw active unit text:
            ax,ay = activePosition #unpack
            activeUnit = self.model.getPositionDesc(ax,ay)
            r = pygame.Rect(self.activeLeft, self.activeTop, self.boardLength, 0.5 * (self.boardLength-self.padding))
            drawText(self.display, str(activeUnit), (0,0,0), r, self.font, aa=True)
            
            #Draw summary text at top
            r = pygame.Rect(self.padding, self.padding, 2 * self.boardLength + self.padding, self.padding)
            drawText(self.display, str(self.model.getSummary()), (0,0,0), r, self.font, aa=True)
                        
            #Update
            pygame.display.update()
        pygame.quit()


"""
OBTAINED FROM https://www.pygame.org/wiki/TextWrap
"""
# draw some text into an area of a surface
# automatically wraps words
# returns any text that didn't get blitted
def drawText(surface, text, color, rect, font, aa=False, bkg=None):
    rect = pygame.Rect(rect)
    y = rect.top
    lineSpacing = -2

    # get the height of the font
    fontHeight = font.size("Tg")[1]

    while text:
        i = 1

        # determine if the row of text will be outside our area
        if y + fontHeight > rect.bottom:
            break

        # determine maximum width of line
        while font.size(text[:i])[0] < rect.width and i < len(text):
            i += 1

        # if we've wrapped the text, then adjust the wrap to the last word      
        if i < len(text): 
            i = text.rfind(" ", 0, i) + 1

        # render the line and blit it to the surface
        if bkg:
            image = font.render(text[:i], 1, color, bkg)
            image.set_colorkey(bkg)
        else:
            image = font.render(text[:i], aa, color)

        surface.blit(image, (rect.left, y))
        y += fontHeight + lineSpacing

        # remove the text we just blitted
        text = text[i:]

    return text