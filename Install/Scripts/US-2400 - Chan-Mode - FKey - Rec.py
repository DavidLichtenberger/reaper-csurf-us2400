
###############################################################################
##
## US2400 - Chan-Mode - FKey - Rec
##
## Custom action for FKey + Rec Key on the US-2400 in Chan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## Render


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Chan FKey + Rec: Render")

cmd_id = 40015 # Open Render Dialog
RPR_Main_OnCommand(cmd_id, 0)
