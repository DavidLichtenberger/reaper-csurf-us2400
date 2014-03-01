
###############################################################################
##
## US2400 - Pan-Mode - FKey - Stop
##
## Custom action for FKey + Stop Key on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## TOGGLE SCROLL ON PLAY


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan FKey + Stop: Scroll on play")

cmd_id = 40036 # Toggle Scroll on play
RPR_Main_OnCommand(cmd_id, 0)

cmd_id = 40262 # Toggle Scroll on rec
RPR_Main_OnCommand(cmd_id, 0)

