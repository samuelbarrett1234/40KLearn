from game_state import *
from game_wrapper import *

def intTryParse(value):
    try:
        return int(value), True
    except ValueError:
        return value, False

units = [
    #Tactical space marines
    { "count" : 5, "ws" : 3, "bs" : 3,
        "t" : 4, "w" : 1, "total_w" : 5,
        "a" : 1, "sv" : 3, "inv" : 7,
        "rg_range" : 24, "rg_s" : 4,
        "rg_ap" : 0, "rg_dmg" : 1,
        "rg_shots" : 1, "rg_is_rapid" : 1,
        "ml_s" : 4, "ml_ap" : 0,
        "ml_dmg" : 1, "movement" : 3
    },
    #Devestator marines
    { "count" : 5, "ws" : 3, "bs" : 3,
        "t" : 4, "w" : 1, "total_w" : 5,
        "a" : 1, "sv" : 3, "inv" : 7,
        "rg_range" : 36, "rg_s" : 5,
        "rg_ap" : -1, "rg_dmg" : 1,
        "rg_shots" : 3, "rg_is_rapid" : 0,
        "ml_s" : 4, "ml_ap" : 0,
        "ml_dmg" : 1, "movement" : 3
    },
    #Guardsmen
    { "count" : 10, "ws" : 4, "bs" : 4,
        "t" : 3, "w" : 1, "total_w" : 10,
        "a" : 1, "sv" : 5, "inv" : 7,
        "rg_range" : 24, "rg_s" : 3,
        "rg_ap" : 0, "rg_dmg" : 1,
        "rg_shots" : 1, "rg_is_rapid" : 1,
        "ml_s" : 3, "ml_ap" : 0,
        "ml_dmg" : 1, "movement" : 3
    },
    #Leman Russ Battle Tank
    { "count" : 1, "ws" : 6, "bs" : 4,
        "t" : 8, "w" : 12, "total_w" : 12,
        "a" : 3, "sv" : 3, "inv" : 7,
        "rg_range" : 72, "rg_s" : 8,
        "rg_ap" : -2, "rg_dmg" : 2,
        "rg_shots" : 4, "rg_is_rapid" : 0,
        "ml_s" : 7, "ml_ap" : 0,
        "ml_dmg" : 1, "movement" : 3
    },
    #Space Marine Redemptor Dreadnought
    { "count" : 1, "ws" : 3, "bs" : 3,
        "t" : 7, "w" : 13, "total_w" : 13,
        "a" : 4, "sv" : 3, "inv" : 7,
        "rg_range" : 24, "rg_s" : 5,
        "rg_ap" : -1, "rg_dmg" : 1,
        "rg_shots" : 6, "rg_is_rapid" : 0,
        "ml_s" : 14, "ml_ap" : -3,
        "ml_dmg" : 4, "movement" : 3
    },
    #Slugga Boyz
    { "count" : 20, "ws" : 3, "bs" : 5,
        "t" : 4, "w" : 1, "total_w" : 20,
        "a" : 3, "sv" : 6, "inv" : 7,
        "rg_range" : 6, "rg_s" : 4,
        "rg_ap" : 0, "rg_dmg" : 1,
        "rg_shots" : 1, "rg_is_rapid" : 0,
        "ml_s" : 4, "ml_ap" : 0,
        "ml_dmg" : 1, "movement" : 3
    },
    #Shoota Boyz
    { "count" : 20, "ws" : 3, "bs" : 5,
        "t" : 4, "w" : 1, "total_w" : 20,
        "a" : 2, "sv" : 6, "inv" : 7,
        "rg_range" : 18, "rg_s" : 4,
        "rg_ap" : 0, "rg_dmg" : 1,
        "rg_shots" : 2, "rg_is_rapid" : 0,
        "ml_s" : 4, "ml_ap" : 0,
        "ml_dmg" : 1, "movement" : 3
    },
    #Meganobz
    { "count" : 3, "ws" : 4, "bs" : 5,
        "t" : 4, "w" : 3, "total_w" : 9,
        "a" : 3, "sv" : 2, "inv" : 7,
        "rg_range" : 18, "rg_s" : 4,
        "rg_ap" : 0, "rg_dmg" : 1,
        "rg_shots" : 4, "rg_is_rapid" : 0,
        "ml_s" : 10, "ml_ap" : -3,
        "ml_dmg" : 2, "movement" : 3
    },
    #Tankbustas
    { "count" : 5, "ws" : 3, "bs" : 5,
        "t" : 4, "w" : 1, "total_w" : 5,
        "a" : 2, "sv" : 6, "inv" : 7,
        "rg_range" : 24, "rg_s" : 8,
        "rg_ap" : -2, "rg_dmg" : 3,
        "rg_shots" : 1, "rg_is_rapid" : 0,
        "ml_s" : 4, "ml_ap" : 0,
        "ml_dmg" : 1, "movement" : 3
    },
    #Burna Boyz
    { "count" : 5, "ws" : 3, "bs" : 1,
        "t" : 4, "w" : 1, "total_w" : 5,
        "a" : 2, "sv" : 6, "inv" : 7,
        "rg_range" : 8, "rg_s" : 4,
        "rg_ap" : 0, "rg_dmg" : 1,
        "rg_shots" : 2, "rg_is_rapid" : 0,
        "ml_s" : 4, "ml_ap" : -2,
        "ml_dmg" : 1, "movement" : 3
    }
]

#UNIT ORDERS
# 0 - Tactical space marines
# 1 - Devestator marines
# 2 - Guardsmen
# 3 - Leman Russ Battle Tank
# 4 - Space Marine Redemptor Dreadnought
# 5 - Slugga Boyz
# 6 - Shoota Boyz
# 7 - Meganobz
# 8 - Tankbustas
# 9 - Burna Boyz


placements = [
    (0, 1, 5, 20),
    (0, 1, 15, 20),
    (0, 1, 25, 20),
    (1, 5, 5, 30),
    (1, 5, 10, 30),
    (1, 5, 15, 30),
    (1, 5, 20, 30),
    (1, 5, 25, 30)
]

wrapper = GameWrapper(GameState(units, placements))

lastPhase = wrapper.getPhase()
lastTeam = wrapper.getCurrentTeam()
print("TEAM", lastTeam, "PHASE", lastPhase)

while not wrapper.finished():
    phase = wrapper.getPhase()
    team = wrapper.getCurrentTeam()

    if team != lastTeam or phase != lastPhase:
        print("TEAM", team, "PHASE", phase)
        lastTeam = team
        lastPhase = phase
        #Print board
        for j in range(BOARD):
            print(j, end='  ')
            for i in range(BOARD):
                if wrapper.currentState.isOccupied(i, j):
                    print(wrapper.currentState.getCurrentTeamOnSquare(i, j), end='')
                else:
                    print(" ", end='')
            print()

    unit = wrapper.getCurrentUnit()
    options = wrapper.getCurrentOptions()
    print("CURRENT UNIT", unit)
    s = input("Decide for unit? (y/n): ").lower()
    if s == "y":
        print("OPTIONS: (option,index)")
        print([(options[i], i) for i in range(len(options))])
        
        succ = False
        while not succ:
            i, succ = intTryParse(input("Enter index of decision: "))
        
        wrapper.chooseOption(options[i])
    else:
        wrapper.chooseOption(None) #No-op

