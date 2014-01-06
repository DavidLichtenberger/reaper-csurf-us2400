
###############################################################################
##
## US2400 - Aux-Mode - Shift - 3
##
## Custom action for Shift + 3 on the US-2400 in Aux Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## REMOVE SEND TO AUX---3 FROM SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Aux Shift 3: Remove Aux 3")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	sendname = 'aux---3'
	aux = 0
	trackamount = RPR_CountTracks(0)

	# Go through all tracks
	for t in range(0, trackamount):

		tr = RPR_GetTrack(0, t)
		
		# get track name
		name = RPR_GetSetMediaTrackInfo_String(tr, 'P_NAME', t, False)[3]
		name = name.lower()

		# is it the track we're looking for?
		if (name.find(sendname) != -1) :
			aux = tr

	# go through tracks again
	for t in range(0, trackamount) :

		tr = RPR_GetTrack(0, t)

		# is selected?
		if (RPR_IsTrackSelected(tr)):

			SNM_RemoveReceivesFrom(aux, tr)

	RPR_Undo_EndBlock("Remove 'aux---3' send from selected tracks", 0)