
###############################################################################
##
## US2400 - Aux-Mode - FKey - 1
##
## Custom action for FKey + 1 on the US-2400 in Aux Mode
##
## You can change the code below to whatever action you wish,
## just DON'T CHANGE THE FILENAME, the binding to the button depends on it
##
###############################################################################


## ADD SEND TO AUX---1 TO SELECTED TRACKS


from reaper_python import *
from sws_python import *

# RPR_ShowConsoleMsg("Aux F 1: Add Aux 1")

if (RPR_CountSelectedTracks(0) > 0):

	RPR_Undo_BeginBlock()

	sendname = 'aux---1'
	aux = 0
	auxname = ''

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
			auxname = name

	# go through tracks again
	for t in range(0, trackamount) :

		tr = RPR_GetTrack(0, t)

		# is selected?
		if (RPR_IsTrackSelected(tr)):

			# check if send exists
			exists = False
			sendamount = RPR_GetTrackNumSends(tr, 0)
			# count hardware outs too
			sendamount += RPR_GetTrackNumSends(tr, 1)
			
			for s in range(0, sendamount) :
				
				sendname = RPR_GetTrackSendName(tr, s, s, 256)[3].lower()
				if (sendname == auxname) :
					exists = True
				
			if (exists == False) :
				SNM_AddReceive(tr, aux, -1)

	RPR_Undo_EndBlock("Add 'aux---1' send to selected tracks", 0)