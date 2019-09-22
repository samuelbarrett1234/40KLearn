import pygame
import py40kl


class GameView:
    """
    The window to view a game in.
    NOTE: there should never be more than one
    instance of this class!
    """

    def __init__(self, model, controller):
        pygame.init()
        self.model = model
        self.controller = controller
        # Configurable:
        self.pixels_per_cell = 20
        self.padding = 20
        # Board size measured in number-of-cells
        self.board_size = model.get_board_size()
        self.board_length = self.board_size * self.pixels_per_cell
        # Computed positions:
        self.title_top = self.padding
        self.title_left = self.padding
        self.board_top = 3 * self.padding
        self.board_left = self.padding
        self.selected_top = self.padding * 3
        self.selected_left = self.padding * 2 + self.board_length
        self.active_top = self.padding * 4 + 0.5 * (self.board_length
                                                    - self.padding)
        self.active_left = self.padding * 2 + self.board_length
        self.update_top = self.padding * 4 + self.board_length
        self.update_left = self.padding
        self.display = pygame.display.set_mode((3 * self.padding
                                                + 2 * self.board_length,
                                                4 * self.padding
                                                + self.board_length))
        # Colours:
        self.window_background = (149, 150, 156)
        self.board_background = (227, 165, 59)
        self.active_colour = (13, 252, 45)
        self.option_colour = (255, 235, 13)
        self.allied_colour = (66, 135, 245)
        self.enemy_colour = (252, 37, 13)
        # Other values:
        pygame.display.set_caption("40KLearn")
        self.font = pygame.font.Font(None, 20)

    def run(self):
        selected_unit = None
        exit = False
        current_team = self.model.get_acting_team()
        while not exit and not self.model.is_finished():
            # Handle events
            for event in pygame.event.get():
                if event.type == pygame.MOUSEBUTTONDOWN:
                    mx, my = pygame.mouse.get_pos()
                    if mx >= self.board_left and my >= self.board_top\
                       and mx < self.board_left + self.board_length\
                       and my < self.board_top + self.board_length:

                        # Convert mouse position to index coordinates:
                        ix, iy = ((mx-self.board_left) // self.pixels_per_cell,
                                  (my-self.board_top) // self.pixels_per_cell)
                        ipos = py40kl.Position(ix, iy)

                        # Check if right or left click
                        if event.button == 3:
                            self.controller.on_click_position(ipos, False)
                        else:
                            self.controller.on_click_position(ipos, True)

                elif event.type == pygame.KEYDOWN:
                    if pygame.key.get_pressed()[pygame.K_RETURN]:
                        self.controller.on_return()

                elif event.type == pygame.QUIT:
                    exit = True

            if self.model.get_acting_team() != current_team:
                current_team = self.model.get_acting_team()
                self.controller.on_turn_changed()

            # Get data from model to update:

            # Cache update data
            allied_positions = self.model.get_allied_positions()
            enemy_positions = self.model.get_enemy_positions()
            option_positions = self.model.get_active_unit_option_positions()
            active_position = self.model.get_active_position()

            # Display info about a selected unit if applicable
            selected_unit = None
            if active_position is not None and\
               active_position in allied_positions or\
               active_position in enemy_positions:
                selected_unit = self.model.get_position_desc(ipos)

            self.controller.on_update()

            # Render
            self.display.fill(self.window_background)
            self.display.fill(self.board_background,
                              rect=pygame.Rect(self.board_left,
                                               self.board_top,
                                               self.board_length,
                                               self.board_length))
            # Colour each part of the board
            for i in range(self.board_size):
                for j in range(self.board_size):
                    x, y = self.board_left + i * self.pixels_per_cell,\
                        self.board_top + j * self.pixels_per_cell
                    cell_rect = pygame.Rect(x, y, self.pixels_per_cell,
                                            self.pixels_per_cell)
                    colour = None
                    cell_pos = py40kl.Position(i, j)
                    if cell_pos == active_position:
                        colour = self.active_colour
                    elif cell_pos in option_positions:
                        colour = self.option_colour
                    elif cell_pos in allied_positions:
                        colour = self.allied_colour
                    elif cell_pos in enemy_positions:
                        colour = self.enemy_colour
                    if colour is not None:
                        self.display.fill(colour, rect=cell_rect)

            if selected_unit is not None:
                # Draw it as text:
                r = pygame.Rect(self.selected_left, self.selected_top,
                                self.board_length,
                                0.5 * (self.board_length - self.padding))
                drawText(self.display, str(selected_unit), (0, 0, 0),
                         r, self.font, aa=True)

            # Draw summary text at top
            r = pygame.Rect(self.padding, self.padding,
                            2 * self.board_length + self.padding, self.padding)
            drawText(self.display, str(self.model.get_summary()), (0, 0, 0),
                     r, self.font, aa=True)

            # Update
            pygame.display.update()
        pygame.quit()


def drawText(surface, text, color, rect, font, aa=False, bkg=None):
    """
    OBTAINED FROM https://www.pygame.org/wiki/TextWrap
    """
    # draw some text into an area of a surface
    # automatically wraps words
    # returns any text that didn't get blitted

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
