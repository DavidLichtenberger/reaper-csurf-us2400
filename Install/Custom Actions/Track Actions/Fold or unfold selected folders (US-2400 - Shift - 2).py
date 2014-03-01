
###############################################################################
##
## US2400 - Pan-Mode - Shift - 2
##
## Custom action for Shift + 2 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## TOGGLE SHOW/HIDE CHILD TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Pan Shift 2: Show/hide children")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	# save folder selection
	folders = []
	for i in range(0, RPR_CountSelectedTracks(0)):

		tk = RPR_GetSelectedTrack(0, i)
		
		# is folder
		depth = int(RPR_GetMediaTrackInfo_Value(tk, 'I_FOLDERDEPTH'))
		if (depth == 1):
			folders.append(tk)
		

	# select only children
	cmd_id = RPR_NamedCommandLookup("_SWS_SELCHILDREN")
	RPR_Main_OnCommand(cmd_id, 0)

	# toggle visible
	cmd_id = RPR_NamedCommandLookup("_SWSTL_TOGMCP")
	RPR_Main_OnCommand(cmd_id, 0)

	# deselect all
	for i in range(0, RPR_CountSelectedTracks(0)):
		tk = RPR_GetSelectedTrack(0, i)
		RPR_SetTrackSelected(tk, False)

	# reselect folders
	for track in folders:
		RPR_SetTrackSelected(track, True)

	RPR_Undo_EndBlock("Show / hide children of selected folders", 0)
