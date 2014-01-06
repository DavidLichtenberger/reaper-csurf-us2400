
###############################################################################
##
## US2400 - Pan-Mode - Shift - 5
##
## Custom action for Shift + 5 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## DELETE SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan Shift 5: Delete")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	# This one prompts first:
	# cmd_id = RPR_NamedCommandLookup("_SWS_DELTRACKCHLD")
	# RPR_Main_OnCommand(cmd_id, 0)


	# This one doesn't:
	selection = []

	for i in range(0, RPR_CountSelectedTracks(0)):
		selection.append(RPR_GetSelectedTrack(0, i))

	for track in selection:
		RPR_DeleteTrack(track)


	RPR_Undo_EndBlock("Delete selected tracks", 0)