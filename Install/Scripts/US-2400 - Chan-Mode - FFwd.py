
###############################################################################
##
## US2400 - Chan-Mode - FFwd
##
## Custom action for FFwd Key on the US-2400 in Chan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## FJUMP


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Chan FFwd: FJump")

cmd_id = 40173 # Jump forwards to next POI
RPR_Main_OnCommand(cmd_id, 0)

