
###############################################################################
##
## US2400 - Pan-Mode - FKey - 2
##
## Custom action for FKey + 2 on the US-2400 in Pan Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## MOVE SELECTED TRACKS RIGHT

from time import *
from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("F 2: Move right")

sel_tks = RPR_CountSelectedTracks(0)

if (sel_tks > 0):

	RPR_Undo_BeginBlock()

	sel = []
	all_tks = RPR_CountTracks(0)
	
	# find rightmost in selection
	rightmost_tk = RPR_GetSelectedTrack(0, sel_tks - 1)
	
	# find target track
	target_id = int(RPR_GetMediaTrackInfo_Value(rightmost_tk, "IP_TRACKNUMBER"))
	target_tk = RPR_GetTrack(0, target_id)

	# find first visible target
	while (bool(RPR_GetMediaTrackInfo_Value(target_tk, "B_SHOWINMIXER")) != True) :

		target_id += 1
		target_tk = RPR_GetTrack(0, target_id)

		if (target_id >= all_tks - 1) :
			target_tk = rightmost_tk

	# target found?
	if (target_tk != rightmost_tk) :

		RPR_Main_OnCommand(40337, 0) # cut

		RPR_SetOnlyTrackSelected(target_tk)
		RPR_Main_OnCommand(40914, 0) #last touched to sel

		RPR_Main_OnCommand(40058, 0) # paste

	RPR_Undo_EndBlock("Move selected tracks right", 0)