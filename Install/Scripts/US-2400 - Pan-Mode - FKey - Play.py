
###############################################################################
##
## US2400 - Pan-Mode - FKey - Play
##
## Custom action for FKey + Play Key on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## Save


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan FKey + Play: Save")

cmd_id = 40022 # Save as ...
RPR_Main_OnCommand(cmd_id, 0)
