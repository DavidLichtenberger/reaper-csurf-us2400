
###############################################################################
##
## US2400 - Pan-Mode - FKey - 4
##
## Custom action for FKey + 4 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## RENAME SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan F 4: Rename")

if (RPR_CountSelectedTracks(0) > 0):
	
	RPR_Undo_BeginBlock()

	cmd_id = RPR_NamedCommandLookup("_XENAKIOS_RENAMETRAXDLG")
	RPR_Main_OnCommand(cmd_id, 0)

	RPR_Undo_EndBlock("Rename selected tracks", 0)