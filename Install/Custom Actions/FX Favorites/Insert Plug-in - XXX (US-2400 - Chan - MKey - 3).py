from reaper_python import *
from sws_python import *

fx_name = ""
chan_tk = 0;

if fx_name == "":
	RPR_ShowMessageBox("No FX specified yet.\n\nTo make this work, open the .py file of this\naction in a text editor and set 'fx_name'\n(line 4) to the name of the plugin (refer to\nthe FX-Browser for exact names).\n\nYou can also change the name of the .py file\naccordingly - just leave the stuff in the\nbrackets ('US-2400 ...') untouched.", "Chan Mode MKey-3 > Insert FX", 0)


else:
	
	for i in range(0, RPR_CountTracks(0)) :

		tk = RPR_GetTrack(0, i)	
		
		if RPR_TrackFX_GetChainVisible(tk) != -1 :
			chan_tk = tk


	if chan_tk != 0 :
		fx_id = RPR_TrackFX_GetByName(chan_tk, fx_name, True);
		RPR_TrackFX_SetOpen(chan_tk, fx_id, True);