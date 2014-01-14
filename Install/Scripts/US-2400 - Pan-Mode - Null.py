
###############################################################################
##
## US2400 - Pan-Mode - Null
##
## Custom action for Null Key on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## ZOOM TO TIME SELECTION


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan Null: Zoom to Time Sel")

cmd_id = 40031 # zoom to time sel
RPR_Main_OnCommand(cmd_id, 0)