from board import Board
from game_wrapper import GameWrapper


##### MODEL INIT

wrapper = GameWrapper(units, placements)

lastPhase = wrapper.getPhase()
lastTeam = wrapper.getCurrentTeam()
print("TEAM", lastTeam, "PHASE", lastPhase)

##### MAIN LOOP

exit = False
while not wrapper.finished() or exit:
    phase = wrapper.getPhase()
    team = wrapper.getCurrentTeam()
    
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            exit = True

    if team != lastTeam or phase != lastPhase:
        print("TEAM", team, "PHASE", phase)
        lastTeam = team
        lastPhase = phase
        #Print board
        for j in range(wrapper.currentState.size):
            print(j, end='  ')
            for i in range(wrapper.currentState.size):
                if wrapper.currentState.isOccupied(i, j):
                    print(wrapper.currentState.getTeamOnSquare(i, j), end='')
                else:
                    print(" ", end='')
            print()

    unit = wrapper.getCurrentUnit()
    options = wrapper.getCurrentOptions()
    print("CURRENT UNIT", unit)
    s = input("Decide for unit? (y/n): ").lower()
    if s == "y":
        print("OPTIONS: (option,index)")
        print([(str(options[i]), i) for i in range(len(options))])
        
        succ = False
        while not succ:
            i, succ = intTryParse(input("Enter index of decision: "))
        
        wrapper.chooseOption(options[i])
    else:
        wrapper.chooseOption(None) #No-op
        
    pygame.display.update()

pygame.quit()
quit()