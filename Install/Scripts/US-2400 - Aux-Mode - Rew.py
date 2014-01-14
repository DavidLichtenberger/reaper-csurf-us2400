
###############################################################################
##
## US2400 - Aux-Mode - Rew
##
## Custom action for Rew Key on the US-2400 in Aux Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## RJUMP


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Aux Rew: RJump")

cmd_id = 40172 # Jumpt backwards to next POI
RPR_Main_OnCommand(cmd_id, 0)

