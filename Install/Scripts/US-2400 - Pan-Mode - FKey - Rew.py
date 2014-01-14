
###############################################################################
##
## US2400 - Pan-Mode - FKey - Rew
##
## Custom action for FKey + Rew Key on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## UNDO


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan FKey + Rew: Undo")

cmd_id = 40029 # Undo
RPR_Main_OnCommand(cmd_id, 0)

