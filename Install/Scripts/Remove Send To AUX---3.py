from reaper_python import *
from sws_python import *

##############
# lowercase!
sendname = 'aux---3'

##############

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

		SNM_RemoveReceivesFrom(aux, tr);