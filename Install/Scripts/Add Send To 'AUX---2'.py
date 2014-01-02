from reaper_python import *
from sws_python import *

##############
# lowercase!
sendname = 'aux---2'

##############

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