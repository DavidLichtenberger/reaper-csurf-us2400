/*
** reaper_csurf
** Tascam US-2400 support
** Cobbled together by David Lichtenberger
** No license, no guarantees.
*/



////// PREFERENCES //////


// Encoder resolution for volume, range: 0 > 16256
#define ENCRESVOL 100
// Encoder resolution for pan, range:  -1 > +1
#define ENCRESPAN 0.01 
// Encoder resolution for fx param, in divisions of range (e.g. -1 > +1, reso 100 > stepsize 0.02)
#define ENCRESFX 300
#define ENCRESFXFINE 3000
#define ENCRESFXTOGGLE 1
// How long will encoders be considered touch, in run circles (15 Hz -> 3 circles = 200ms)
#define ENCTCHDLY 6

// Wheel resolution for fast scrub
#define SCRUBRESFAST 1
// Wheel resolution for slow scrub
#define SCRUBRESSLOW 5
// Wheel resolution for edit cursor move, fast
#define MOVERESFAST 10
// Wheel resolution for edit cursor move, slow
#define MOVERESSLOW 50
// Blink interval / ratio (1 = appr. 30 Hz / 0.03 s)
#define MYBLINKINTV 20
#define MYBLINKRATIO 1
// Execute only X Faders/Encoders at a time
#define EXLIMIT 10
// For finding sends (also in custom reascript actions, see MyCSurf_Aux_Send)
#define AUXSTRING "aux---%d"
// Filename of ini file (in reaper ressources dir)
#define INIFILE "csurf_us2400.ini"



////// DEBUG //////

// debug macros
#define DBGS(x) ShowConsoleMsg(x);
#define DBGF(x) sprintf(debug, "%f   ", x); ShowConsoleMsg(debug);
#define DBGD(x) sprintf(debug, "%d   ", x); ShowConsoleMsg(debug);
#define DBGX(x) sprintf(debug, "%x   ", x); ShowConsoleMsg(debug);
#define DBGB(x) if(x) ShowConsoleMsg("true   "); else ShowConsoleMsg("false   ");
#define DBGN ShowConsoleMsg("\n");



// Command Lookup
#define CMD(x) NamedCommandLookup(x)

// Unnamed Commands
#define CMD_SELALLITEMS 40182
#define CMD_UNSELALLITEMS 40289
#define CMD_RJUMP 40172     
#define CMD_FJUMP 40173
#define CMD_TIMESEL2ITEMS 40290
#define CMD_CLEARTIMESEL 40635
#define CMD_TGLPLAYSCROLL 40036
#define CMD_TGLRECSCROLL 40262
#define CMD_TGGLRECBEAT 40045
#define CMD_AUTOTOSEL 41160
#define CMD_FXBROWSER 40271
#define CMD_UNDO 40029
#define CMD_REDO 40030
#define CMD_SAVE 40026
#define CMD_SAVEAS 40022
#define CMD_SEL2LASTTOUCH 40914
#define CMD_TKAUTOMODES 40400

#include "csurf.h"

// for debug  
char debug[64];

class CSurf_US2400;
static bool g_csurf_mcpmode = true; 

// display
bool dsp_repaint = false;
int dsp_width = -1;
int dsp_height = -1;
int dsp_x = -1;
int dsp_y = -1;

char* dsp_values[24];
int dsp_colors[24];
unsigned long dsp_touch = 0;
unsigned long dsp_touch_prev = 0;


void Dsp_Paint(HWND hwnd)
{
  RECT rect;
  GetWindowRect(hwnd, &rect);
  int win_width = rect.right - rect.left;
  int win_height = rect.bottom - rect.top;

  HDC hdc;
  PAINTSTRUCT ps;
  hdc = BeginPaint(hwnd, &ps);

  HBRUSH greybg = CreateSolidBrush(RGB(180, 180, 180));
  HBRUSH redbg = CreateSolidBrush(RGB(240, 140, 120));
  HPEN light = CreatePen(PS_SOLID, 1, RGB(220, 220, 220));
  HPEN mid = CreatePen(PS_SOLID, 1, RGB(180, 180, 180));
  HPEN dark = CreatePen(PS_SOLID, 1, RGB(140, 140, 140));

  int pad = 5;
  int box_width = win_width / 26;
  int sep = box_width;
  int x = 0;
  for (char ch = 0; ch < 24; ch++)
  {
    // separators
    if ((ch % 8 == 0) && (ch != 0))
    {
      rect.left = x + 1;
      rect.top = 0;
      rect.right = x + sep;
      rect.bottom = win_height;

      FillRect(hdc, &rect, greybg);

      x += sep;
      SelectObject(hdc, dark);
      MoveToEx(hdc, x, 0, NULL);
      LineTo(hdc, x, win_height);
    }

    // touchstates
    if ( (dsp_touch & (1 << ch)) > 0 )
    {
      rect.left = x;
      rect.top = 0;
      rect.right = x + box_width;
      rect.bottom = pad;
      FillRect(hdc, &rect, redbg);
    }

    // text
    rect.left = x + pad * 2;
    rect.top = pad;
    rect.right = x + box_width - (pad * 2);
    rect.bottom = win_height - pad;
    DrawText(hdc, dsp_values[ch], -1, &rect, DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS);

    // dividers
    if ((ch + 1) % 2 != 0) SelectObject(hdc, light);
    else if ((ch + 1) % 4 != 0) SelectObject(hdc, mid);
    else SelectObject(hdc, dark);

    x += box_width;
    MoveToEx(hdc, x, 0, NULL);
    LineTo(hdc, x, win_height);
  }

  DeleteObject(greybg);
  DeleteObject(redbg);
  DeleteObject(light);
  DeleteObject(mid);
  DeleteObject(dark);
  
  EndPaint(hwnd, &ps);
}

void Dsp_StoreWinCoords(HWND hwnd)
{
  RECT rect;
  GetWindowRect(hwnd, &rect);
  dsp_x = rect.left;
  dsp_y = rect.top;
  dsp_width = rect.right - rect.left;
  dsp_height = rect.bottom - rect.top;
}


LRESULT CALLBACK Dsp_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_PAINT:
      Dsp_Paint(hwnd);
      break;

    case WM_SIZE:
      dsp_repaint = true;
      Dsp_StoreWinCoords(hwnd);
      break;

    case WM_MOVE:
      dsp_repaint = true;
      Dsp_StoreWinCoords(hwnd);
      break;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}



class CSurf_US2400 : public IReaperControlSurface
{
  int m_midi_in_dev,m_midi_out_dev;
  int m_offset, m_size;
  midi_Output *m_midiout;
  midi_Input *m_midiin;
  int m_cfg_flags;  // config_flag_fader_touch_mode etc

  WDL_String descspace;
  char configtmp[1024];


  // cmd_ids
  int cmd_aux_fkey[6];
  int cmd_aux_shift[6];
  int cmd_pan_fkey[6];
  int cmd_pan_shift[6];
  int cmd_chan_fkey[6];
  int cmd_chan_shift[6];

  // buffer for fader data
  bool waitformsb;
  unsigned char lsb;

  // for myblink
  bool s_myblink;  
  int myblink_ctr;

  // for init
  bool s_initdone;
  bool s_exitdone;

  // touchstates
  unsigned long s_touch_fdr;
  int s_touch_enc[24];

  // caches
  int cache_faders[25];
  int cache_enc[24];
  unsigned long cache_upd_faders;
  unsigned long cache_upd_enc;
  char cache_exec;
    
  // general states
  int s_ch_offset; // bank up/down
  bool s_play, s_rec, s_loop; // play states
  char s_automode; // automation modes

  // modes
  bool m_flip, m_chan, m_pan, m_scrub;
  char m_aux;

  // qualifier keys
  bool q_fkey, q_shift;

  // for channel strip
  MediaTrack* chan_rpr_tk;
  char chan_ch;
  int chan_fx;
  int chan_par_offs;
  bool chan_fx_env_arm;

  // save track sel
  MediaTrack** saved_sel;
  int saved_sel_len;

  // loop all
  bool s_loop_all;
  double s_ts_start;
  double s_ts_end;

  // display
  HWND dsp_hwnd;
  WNDCLASSEX dsp_class;

  // inifile
  WDL_String ini_path;
  HANDLE ini_file;


  //////// MIDI ////////

  void MIDIin(MIDI_event_t *evt)
  {
    unsigned char ch_id;
    
    bool btn_state = false;
    if (evt->midi_message[2] == 0x7f) btn_state = true;

    // msb of fader move?

    if ( (waitformsb) && (evt->midi_message[0] == 0xb0) && (evt->midi_message[1] < 0x19) ) {

      ch_id = evt->midi_message[1];
      int value = (evt->midi_message[2] << 7) | lsb;

      OnFaderChange(ch_id, value);

      waitformsb = false;

    } else {

      // buttons / track elements
      if (evt->midi_message[0] == 0xb1)
      {
        // master buttons
        switch (evt->midi_message[1])
        {
          case 0x61 : if (btn_state) OnMasterSel(); break;
          case 0x62 : OnClrSolo(btn_state); break;
          case 0x63 : if (btn_state) OnFlip(); break;
          case 0x64 : if (btn_state) OnChan(); break;
          case 0x6c : if (btn_state) OnPan(); break;
          //case 0x6b : if (btn_state) OnMeter(); break; // maybe later (see below)
          case 0x6d : OnFKey(btn_state); break;
          case 0x65 : if (btn_state) OnAux(1); break;
          case 0x66 : if (btn_state) OnAux(2); break;
          case 0x67 : if (btn_state) OnAux(3); break;
          case 0x68 : if (btn_state) OnAux(4); break;
          case 0x69 : if (btn_state) OnAux(5); break;
          case 0x6a : if (btn_state) OnAux(6); break;
          case 0x6e : OnNull(btn_state); break;
          case 0x6f : if (btn_state) OnScrub(); break;
          case 0x70 : OnBank(-1, btn_state); break;
          case 0x71 : OnBank(1, btn_state); break;
          case 0x72 : OnIn(btn_state); break;
          case 0x73 : OnOut(btn_state); break;
          case 0x74 : OnShift(btn_state); break;
          case 0x75 : if (btn_state) OnRew(); break;
          case 0x76 : if (btn_state) OnFwd(); break;
          case 0x77 : if (btn_state) OnStop(); break;
          case 0x78 : if (btn_state) OnPlay(); break;
          case 0x79 : if (btn_state) OnRec(); break;
          default :
          {
            // track elements
            if (evt->midi_message[1] < 0x60)
            {
              ch_id = evt->midi_message[1] / 4; // floor

              char ch_element;
              ch_element = evt->midi_message[1] % 4;    // modulo

              switch (ch_element)
              {
                case 0 : OnFaderTouch(ch_id, btn_state); break;
                case 1 : if (btn_state) OnTrackSel(ch_id); break;
                case 2 : if (btn_state) OnTrackSolo(ch_id); break;
                case 3 : if (btn_state) OnTrackMute(ch_id); break;
              } // switch (ch_element)
            } // if (evt->midi_message[1] < 0x60)
          } // default
        } // switch (evt->midi_message[1])

        // encoders, fader values, jog wheel
      } else if (evt->midi_message[0] == 0xb0) 
      {
        // calculate relative value for encoders and jog wheel
        signed char rel_value;
        if (evt->midi_message[2] < 0x40) rel_value = evt->midi_message[2];
        else rel_value = 0x40 - evt->midi_message[2];

        // fader (track and master): catch lsb - msb see above
        if ( (evt->midi_message[1] >= 0x20) && (evt->midi_message[1] <= 0x38) )
        {
          lsb = evt->midi_message[3];
          waitformsb = true;

          // jog wheel
        } else if (evt->midi_message[1] == 0x3C) 
        {

          OnJogWheel(rel_value);

          // encoders
        } else if ( (evt->midi_message[1] >= 0x40) && (evt->midi_message[1] <= 0x58) ) 
        {
          ch_id = evt->midi_message[1] - 0x40;

          OnEncoderChange(ch_id, rel_value);
        }

        // touch master fader
      } else if (evt->midi_message[0] == 0xb2) 
      {
        OnFaderTouch(24, btn_state);

        // joystick
      } else if (evt->midi_message[0] == 0xbe) 
      {
        // send on to input + 1 (Tascam[2])
        kbd_OnMidiEvent(evt, m_midi_in_dev + 1);
      } // (evt->midi_message[0] == 0xb1), else, else ...
    } // if ( (waitformsb) && (evt->midi_message[0] == 0xb0) && (evt->midi_message[1] < 0x19), else
  } // MIDIin()


  void MIDIOut(unsigned char s, unsigned char d1, unsigned char d2) 
  {
    if (m_midiout) m_midiout->Send(s, d1, d2, 0);
  } // void MIDIOut(char s, char d1, char d2)



  //////// EVENTS (called by MIDIin) //////


  // TRACK ELEMENTS

  void OnTrackSel(char ch_id)
  {
    if (m_chan) MySetSurface_Chan_SelectTrack(ch_id, false);
    else {
      MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);

      if (q_fkey) MyCSurf_SwitchPhase(rpr_tk);
      else if (q_shift) CSurf_OnRecArmChange(rpr_tk, -1);
      else CSurf_OnSelectedChange(rpr_tk, -1);
    }
  } // OnTrackSel


  void OnTrackSolo(char ch_id)
  {
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);    

    if (q_fkey) MyCSurf_ToggleSolo(rpr_tk, true);
    else MyCSurf_ToggleSolo(rpr_tk, false);
  } // OnTrackSolo


  void OnTrackMute(char ch_id)
  {
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);    

    if (q_fkey) MyCSurf_ToggleMute(rpr_tk, true);
    else if (q_shift) MyCSurf_Chan_ToggleAllFXBypass(ch_id);
    else MyCSurf_ToggleMute(rpr_tk, false);
  } // OnTrackMute


  void OnFaderTouch(char ch_id, bool btn_state)
  {
    if (btn_state) 
    {
      s_touch_fdr = s_touch_fdr | (1 << ch_id);
      if ((q_shift) || (q_fkey)) MySetSurface_UpdateFader(ch_id);
    } else
    {
      s_touch_fdr = s_touch_fdr ^ (1 << ch_id);
    }
  } // OnFaderTouch


  void OnFaderChange(char ch_id, int value)
  {
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
    int para_amount;

    double d_value;

    bool istrack = false;
    bool ismaster = false;
    bool isactive = true;

    // is track or master?
    if ( (ch_id >= 0) && (ch_id <= 23) ) istrack = true; // no track fader
    else if (rpr_tk == CSurf_TrackFromID(0, false)) ismaster = true;

    // active fader? 
    if (!rpr_tk) isactive = false; // no corresponding rpr_tk
    
    if ( (m_chan) && (m_flip) ) 
    { // only chan and flip: inside para_amount?
   
      para_amount = TrackFX_GetNumParams(chan_rpr_tk, chan_fx);
      if (chan_par_offs + ch_id >= para_amount) isactive = false;
      else isactive = true; // is track doesn't matter when chan and flipped
    
    } else if ( (m_aux > 0) && (m_flip) ) 
    { // only aux #x and flip: has send #x?
      int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
      if (send_id == -1) isactive = false;
    }
  
    // get values
    if (ismaster || istrack)
    {
      if (ismaster)
      { // if master -> volume

        if (q_fkey) d_value = 0.0; // MINIMUM
        else if (q_shift) d_value = 1.0; // DEFAULT
        else d_value = Cnv_FaderToVol(value); 
        CSurf_OnVolumeChange(rpr_tk, d_value, false);
      
      } else if (isactive)
      { // if is active track fader

        if (m_flip)
        {
          if (m_chan) 
          { // flip & chan -> fx param

            double min, max;
            d_value = TrackFX_GetParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &min, &max);

            if (q_fkey) d_value = min; // MINIMUM
            else if (q_shift) d_value = max; // MAXIMUM
            else d_value = Cnv_FaderToFXParam(min, max, value);
            MyCSurf_Chan_SetFXParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, d_value);

          } else if (m_aux > 0) 
          { // flip + aux -> send Vol

            int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
            if (send_id != -1)
            {

              if (q_fkey) d_value = 0.0; // MINIMUM
              else if (q_shift) d_value = 1.0; // DEFAULT
              else d_value = Cnv_FaderToVol(value); 

              CSurf_OnSendVolumeChange(rpr_tk, send_id, d_value, false);
            }
          } else
          { // pan

            if (q_fkey) 
            { // flip & fkey & pan -> width
            
              if (q_shift) d_value = 1.0; // default
              else d_value = Cnv_FaderToPanWidth(value);
              CSurf_OnWidthChange(rpr_tk, d_value, false);

            } else 
            { // flip & pan mode -> pan
            
              if (q_shift) d_value = 0.0; // default
              else d_value = Cnv_FaderToPanWidth(value);
              CSurf_OnPanChange(rpr_tk, d_value, false);
            }         
          } // if (m_chan), else

        } else
        { // no flip -> volume

          // no flip / master -> track volume
          if (q_fkey) d_value = 0.0; // MINIMUM
          else if (q_shift) d_value = 1.0; // DEFAULT
          else d_value = Cnv_FaderToVol(value); 
          CSurf_OnVolumeChange(rpr_tk, d_value, false);

        } // if (m_flip), else
      } // if (ismaster), else if (isactive)
    } // if (exists)

    MySetSurface_UpdateFader(ch_id);
  } // OnFaderChange()


  void OnEncoderChange(char ch_id, signed char rel_value)
  {
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
    int para_amount;

    double d_value;
    bool dot = false; // for phase switch, rec arm

    bool exists = false;
    bool isactive = true;

    // encoder exists?
    if ( (ch_id >= 0) && (ch_id <= 23) ) exists = true;

    // active encoder? 
    if (!rpr_tk) isactive = false; // no track

    if ( (m_chan) && (!m_flip) )
    { 
      para_amount = TrackFX_GetNumParams(chan_rpr_tk, chan_fx);
      if (chan_par_offs + ch_id >= para_amount) isactive = false;
      else isactive = true; // chan + fip: is track doesn't matter

    } else if (m_aux > 0) 
    { // only aux #x: has send #x?
      int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
      if (send_id == -1) isactive = false;
    }

    if ( (exists) && (isactive) )
    { // is encoder and there is a track, fx parameter (chan, no flip), or send (aux, noflip)
      if (m_flip)
      {
        if (m_aux > 0)
        { // aux & flip -> send pan
          int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
          if (send_id != -1)
          {
            if (q_shift) d_value = 0.0; // DEFAULT
            else
            {
              GetTrackSendUIVolPan(rpr_tk, send_id, NULL, &d_value);
              d_value = Cnv_EncoderToPanWidth(d_value, rel_value);
            }
            CSurf_OnSendPanChange(rpr_tk, send_id, d_value, false);
          }
        
        } else
        { // just flip -> track volume
          
          if (q_fkey) d_value = 0.0; // MINIMUM
          else if (q_shift) d_value = 1.0; // DEFAULT
          else 
          { 
            GetTrackUIVolPan(rpr_tk, &d_value, NULL);
            d_value = Cnv_EncoderToVol(d_value, rel_value); 
          }
          CSurf_OnVolumeChange(rpr_tk, d_value, false);
        }
      } else
      {
        if (m_chan)
        { // chan -> fx_param (para_offset checked above)

          double min, max, step; //, fine, coarse;
          d_value = TrackFX_GetParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &min, &max);
          
          /* at the moment TrackFX_GetParameterStepSizes always fails!
          if (TrackFX_GetParameterStepSizes(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &step, &fine, &coarse, &toggle)
          {
            if (toggle)
            {
              if (rel_value > 0) d_value = 1.0;
              else d_value = 0.0;
         
            } else
            {
              if (q_fkey) step = fine;
              else if (q_shift) step = coarse;
              d_value = Cnv_EncoderToFXParam(d_value, min, max, step, rel_value);
            }
          }
          */
          // this is the temporary workaround
          step = (double)ENCRESFX;
          if (q_fkey) step = (double)ENCRESFXFINE;
          else if (q_shift) step = (double)ENCRESFXTOGGLE;

          d_value = Cnv_EncoderToFXParam(d_value, min, max, step, rel_value);
          MyCSurf_Chan_SetFXParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, d_value);
       
        } else if (m_aux > 0)
        {
          int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
          if (send_id != -1)
          {
            // aux & no flip -> send-level
            if (q_fkey) d_value = 0.0; // MINIMUM 
            else if (q_shift) d_value = 1.0; // DEFAULT 
            else
            {
              
              GetTrackSendUIVolPan(rpr_tk, send_id, &d_value, NULL);
              d_value = Cnv_EncoderToVol(d_value, rel_value);
            }
            CSurf_OnSendVolumeChange(rpr_tk, send_id, d_value, false);
          }
        
        } else if (m_pan)
        {
          if (q_fkey) 
          { // pan + fkey -> width
            
            
            if (q_shift) d_value = 1.0; // default
            else 
            {
              d_value = GetMediaTrackInfo_Value(rpr_tk, "D_WIDTH");
              d_value = Cnv_EncoderToPanWidth(d_value, rel_value);
            }
            CSurf_OnWidthChange(rpr_tk, d_value, false);
          
          } else
          { // pan mode -> pan

            if (q_shift) d_value = 0.0; // default
            else 
            {
              GetTrackUIVolPan(rpr_tk, NULL, &d_value);
              d_value = Cnv_EncoderToPanWidth(d_value, rel_value);
            }
            CSurf_OnPanChange(rpr_tk, d_value, false);
          }
        }
      } // if (m_flip), else

      MySetSurface_UpdateEncoder(ch_id); // because touched track doesn't get updated

      s_touch_enc[ch_id] = ENCTCHDLY;

    } // if ( (exists) && (isactive) )
  } // OnEncoderChange


  // MASTER TRACK

  void OnMasterSel()
  {
    if (m_chan) 
    {
      MySetSurface_Chan_SelectTrack(24, false);
    } else
    {
      if (q_fkey) MyCSurf_SelectMaster();
      else MyCSurf_ToggleSelectAllTracks();
    }
  } // OnMasterSel()


  void OnClrSolo(bool btn_state)
  {
    if (btn_state)
      if (q_fkey) MyCSurf_UnmuteAllTracks();
      else MyCSurf_UnsoloAllTracks();

    MySetSurface_UpdateButton(0x62, btn_state, false);
  } // OnClrSolo()


  void OnFlip()
  {
    MySetSurface_ToggleFlip();
  } // OnFlip()


  // MODE BUTTONS

  void OnChan()
  {
    if (m_chan) MySetSurface_ExitChanMode();
    else MySetSurface_EnterChanMode();
  } // OnChan()


  void OnPan()
  {
    MySetSurface_EnterPanMode();
  } // OnPan()


  void OnAux(char sel)
  { 
    if (q_fkey) 
    { 

      /* The Emulate Keystroke stuff isn't really needed, 
      better put some more custom actions here */

      if (m_aux > 0) Main_OnCommand(cmd_aux_fkey[sel-1],0);    // Reascript: Aux-Mode - FKey - X
      else if (m_chan) Main_OnCommand(cmd_chan_fkey[sel-1],0); // Reascript: Chan-Mode - FKey - X
      else Main_OnCommand(cmd_pan_fkey[sel-1],0);              // Reascript: Pan-Mode - FKey - X

      /* 

      } else
      {
        if (sel >= 5)
        {
          if (m_chan && (cmd_chan_fkey[sel-1] != -1)) 
          {
            Main_OnCommand(cmd_chan_fkey[sel-1],0);

          } else if (cmd_pan_fkey[sel-1] != -1) 
          {
            Main_OnCommand(cmd_pan_fkey[sel-1],0);

          } else
          {
            // if no actions defined emulate Esc / Enter
            switch (sel)
            {
              case 5: MyCSurf_EmulateKeyStroke(27); break;
              case 6: MyCSurf_EmulateKeyStroke(13); break;
            }
          }
        } else
        {
          if (m_chan) Main_OnCommand(cmd_chan_fkey[sel-1],0);
          else Main_OnCommand(cmd_pan_fkey[sel-1],0);
        }
      }

      */

    } else if (q_shift)
    {
      if (m_aux > 0) Main_OnCommand(cmd_aux_shift[sel-1],0);    // Reascript: Aux-Mode - Shift - X
      else if (m_chan) Main_OnCommand(cmd_chan_shift[sel-1],0); // Reascript: Chan-Mode - Shift - X
      else Main_OnCommand(cmd_pan_shift[sel-1],0);              // Reascript: Pan-Mode - Shift - X

    } else
    {
      if (m_chan)
      {
        switch(sel)                                             // Fix: channel strip actions
        {
          case 1 : MySetSurface_Chan_SetFxParamOffset(1); break;
          case 2 : MySetSurface_Chan_SetFxParamOffset(-1); break;
          case 3 : MyCSurf_Chan_ToggleFXBypass(); break;
          case 4 : MyCSurf_Chan_InsertFX(); break;
          case 5 : MyCSurf_Chan_DeleteFX(); break;
          case 6 : MyCSurf_Chan_ToggleArmFXEnv(); break;

        } 
      } else MySetSurface_EnterAuxMode(sel);                    // Fix: enter Aux Mode
    }

    MySetSurface_UpdateAuxButtons();
  } // OnAux()


  // METER MODE
  
  /* Maybe later. I don't think it's that important (and I don't know how to do it) */


  // QUALIFIERS

  void OnFKey(bool btn_state)
  {
    if (btn_state && q_shift)
    {
      if (dsp_hwnd == NULL) Dsp_OpenWindow();
      else Dsp_CloseWindow();
    } 
    MySetSurface_ToggleFKey(btn_state);
  } // OnFKey()

  void OnShift(bool btn_state)
  {
    if (btn_state && q_fkey)
    {
      if (dsp_hwnd == NULL) Dsp_OpenWindow();
      else Dsp_CloseWindow();
    } 
    MySetSurface_ToggleShift(btn_state);
  } // OnShift()


  // TRANSPORT

  void OnRew()
  {
    if (q_fkey) MyCSurf_Undo();
    else if (q_shift) MyCSurf_Auto_SetMode(0);
    else MyCSurf_OnRew();
  } // OnRew()


  void OnFwd()
  {
    if (q_fkey) MyCSurf_Redo();
    else if (q_shift) MyCSurf_Auto_SetMode(1);
    else MyCSurf_OnFwd();
  } // OnFwd()


  void OnStop()
  {
    if (q_fkey) MyCSurf_ToggleScrollOnPlay(); 
    else if (q_shift) MyCSurf_Auto_SetMode(2); 
    else CSurf_OnStop();   
  } // OnStop()


  void OnPlay()
  {
    if (q_fkey) MyCSurf_Save(false);
    else if (q_shift) MyCSurf_Auto_SetMode(3);
    else CSurf_OnPlay();
  } // OnPlay()


  void OnRec()
  {
    if (q_fkey) MyCSurf_Save(true);
    else if (q_shift) MyCSurf_Auto_WriteCurrValues();
    else MyCSurf_OnRec();
  } // OnRec()


  // OTHER KEYS

  void OnNull(bool btn_state)
  {
    if (btn_state) 
      if (q_fkey) MyCSurf_CenterHScrollToPlay();
      else if (q_shift) MyCSurf_VZoomToSelTracks();
      else MyCSurf_ToggleHZoomToTimeSel();

    MySetSurface_UpdateButton(0x6e, btn_state, false);
  } // OnNull()


  void OnScrub()
  {
    MySetSurface_ToggleScrub();
  } // OnScrub()


  void OnBank(signed char dir, bool btn_state)
  {
    char btn_id;
    if (dir > 0) btn_id = 0x71;
    else btn_id = 0x70;
    
    if (btn_state)
    {
      if (m_chan)
      {
        if (q_fkey) MyCSurf_Chan_MoveFX(dir);
        else if (q_shift)
        {
          if (dir > 0) MyCSurf_Chan_OpenFX(chan_fx);
          else MyCSurf_Chan_CloseFX(chan_fx);
        }
        else MyCSurf_Chan_SelectFX(chan_fx + dir);

      } else
      {
        if (q_fkey) MyCSurf_MoveTimeSel(dir, 0, false);
        else
        {
          char factor = 8;
          if (q_shift) factor = 24;
          MySetSurface_ShiftBanks(dir, factor);
        }
      }
      MySetSurface_UpdateButton(btn_id, true, false);
    } else
    {
      if (m_chan) MySetSurface_UpdateButton(btn_id, true, true);
      else MySetSurface_UpdateButton(btn_id, false, false);
    }
  } // OnBank


  void OnIn(bool btn_state)
  {
    if (btn_state)
    {
      if (q_fkey) MyCSurf_MoveTimeSel(0, -1, false);
      else if (q_shift) MyCSurf_Loop_ToggleAll();
      else MyCSurf_MoveTimeSel(-1, -1, true);
    }

    // keep lit if loop activated
    if (s_loop) btn_state = true;
    MySetSurface_UpdateButton(0x72, btn_state, false);
  } // OnIn()


  void OnOut(bool btn_state)
  {
    if (btn_state)
    {
      if (q_fkey) MyCSurf_MoveTimeSel(0, 1, false);
      else if (q_shift) MyCSurf_ToggleRepeat();
      else MyCSurf_MoveTimeSel(1, 1, true);
    }

    // keep lit if loop activated
    if (s_loop) btn_state = true;
    MySetSurface_UpdateButton(0x73, btn_state, false);
  } // OnOut()



  // SPECIAL INPUT  

  void OnJogWheel(signed char rel_value)
  {
    if (m_scrub)
    {
      if (q_fkey) MyCSurf_Scrub(rel_value, true);
      else MyCSurf_Scrub(rel_value, false);
    
    } else
    {
      if (q_fkey) MyCSurf_MoveEditCursor(rel_value, true);
      else MyCSurf_MoveEditCursor(rel_value, false);
    }
  } // OnJogWheel()



  ////// DISPLAY //////

  void Dsp_OpenWindow()
  {
    Dsp_Update();

    if (dsp_hwnd == NULL)
    {
      if (dsp_width == -1 || dsp_height == -1)
      {
        RECT scr;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &scr, 0);
        dsp_width = scr.right - scr.left;
        dsp_height = 60;
        dsp_x = 0;
        dsp_y = scr.bottom - dsp_height;
      }
             
      dsp_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST, "dsp", "US-2400 Display", WS_THICKFRAME | WS_POPUP, dsp_x, dsp_y, dsp_width, dsp_height, NULL, NULL, g_hInst, NULL);
    }
    ShowWindow(dsp_hwnd, SW_SHOW);
    UpdateWindow(dsp_hwnd);
  }


  void Dsp_CloseWindow()
  {
    if (dsp_hwnd != NULL)
    {
      DestroyWindow(dsp_hwnd);
      dsp_hwnd = NULL;
    }
  }


  void Dsp_Update()
  {
    MediaTrack* tk;

    int fx_amount = 0;
    if (m_chan) fx_amount = TrackFX_GetNumParams(chan_rpr_tk, chan_fx);

    for (int ch = 0; ch < 24; ch++)
    {
      if (dsp_values[ch] != NULL) delete dsp_values[ch];
      dsp_values[ch] = new char[64];

      if (m_chan)
      {
        if (ch + chan_par_offs < fx_amount) TrackFX_GetParamName(chan_rpr_tk, chan_fx, ch + chan_par_offs, dsp_values[ch], 63);
        else dsp_values[ch] = "";

      } else
      {
        tk = Cnv_ChannelIDToMediaTrack(ch);
        if (tk != NULL) 
        {
          GetSetMediaTrackInfo_String(tk, "P_NAME", dsp_values[ch], false);
          dsp_values[64] = '\0';
        } else dsp_values[ch] = "";
      }
      Hlp_Alphanumeric(dsp_values[ch]);
    }

    dsp_repaint = true;
  }


  void Dsp_GetWinCoordsIni()
  {
    ini_file = CreateFile(ini_path.Get(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (ini_file != INVALID_HANDLE_VALUE) 
    {
      DWORD read;

      ReadFile(ini_file, &dsp_x, sizeof(dsp_x), &read, NULL);
      ReadFile(ini_file, &dsp_y, sizeof(dsp_y), &read, NULL);
      ReadFile(ini_file, &dsp_width, sizeof(dsp_width), &read, NULL);
      ReadFile(ini_file, &dsp_height, sizeof(dsp_height), &read, NULL);

      CloseHandle(ini_file);
    } 
  }


  void Dsp_SaveWinCoordsIni()
  {
    ini_file = CreateFile(ini_path.Get(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (ini_file != INVALID_HANDLE_VALUE)
    {
      DWORD written;

      WriteFile(ini_file, &dsp_x, sizeof(dsp_x), &written, NULL);
      WriteFile(ini_file, &dsp_y, sizeof(dsp_y), &written, NULL);
      WriteFile(ini_file, &dsp_width, sizeof(dsp_width), &written, NULL);
      WriteFile(ini_file, &dsp_height, sizeof(dsp_height), &written, NULL);

      CloseHandle(ini_file);
    } 
  }

  ////// CONVERSION & HELPERS //////


  // TRACKS / CHANNELS / SENDS

  int Cnv_MediaTrackToChannelID(MediaTrack* rpr_tk)
  {
    int ch_id = CSurf_TrackToID(rpr_tk, g_csurf_mcpmode);

    ch_id -= s_ch_offset;
    if (ch_id == 0) ch_id = 24;
    else ch_id = ch_id - 1;

    return ch_id;
  } // Cnv_MediaTrackToChannelID


  MediaTrack* Cnv_ChannelIDToMediaTrack(unsigned char ch_id) 
  {
    if (ch_id == 24) 
    {
      ch_id = 0; // master = 0
    } else 
    { 
      ch_id += s_ch_offset + 1;
    }
    
    MediaTrack* rpr_tk = CSurf_TrackFromID(ch_id, g_csurf_mcpmode);

    return rpr_tk;
  } // Cnv_ChannelIDToMediaTrack


  int Cnv_AuxIDToSendID(int ch_id, char aux)
  {
    char search[256];
    char sendname[256];
    sprintf(search, AUXSTRING, aux);

    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
    int all_sends = GetTrackNumSends(rpr_tk, 0);
    // hardware outputs count for GetTrackSendName, too
    all_sends += GetTrackNumSends(rpr_tk, 1);

    for (int s = 0; s < all_sends; s++)
    {
      GetTrackSendName(rpr_tk, s, sendname, 256);
          
      // lowercase name - surely there has to be a better way?
      char c = 0;
      while ((c < 256) && (sendname[c] != 0))
      {
        if ((sendname[c] >= 65) && (sendname[c] <= 90)) sendname[c] += 32;
        c++;
      }

      if (strstr(sendname, search)) return s;
    }
    
    return -1;
  } // Cnv_AuxIDToSendID


  // VOLUME / SEND LEVELS

  double Cnv_FaderToVol(unsigned int value) 
  {
    // theoretically, the MIDI range is from 0 to 16383,
    // but the US2400 gets a little rough on the ends, 
    // an attempt to account for that:
    double new_val = ((double)(value + 41) * 1000.0) / 16256.0;

    if (new_val < 0.0) new_val = 0.0;
    else if (new_val > 1000.0) new_val = 1000.0;

    new_val = SLIDER2DB(new_val);
    new_val = DB2VAL(new_val);

    return(new_val);
  } // Cnv_FaderToVol


  double Cnv_EncoderToVol(double old_val, signed char rel_val) 
  {
    double lin = (DB2SLIDER( VAL2DB(old_val) ) * 16383.0 / 1000.0);
    double d_rel = (double)rel_val * (double)ENCRESVOL;
    lin = lin + d_rel;

    double new_val = ((double) lin * 1000.0) / 16383.0;

    if (new_val < 0.0) new_val = 0.0;
    else if (new_val > 1000.0) new_val = 1000.0;

    new_val = DB2VAL(SLIDER2DB(new_val));

    return new_val;
  } // Cnv_EncoderToVol


  int Cnv_VolToFader(double value) 
  {
    double new_val = VAL2DB(value);
    new_val = DB2SLIDER( new_val );

    // theoretically, the MIDI range is from 0 to 16383,
    // but the US2400 gets a little rough on the ends, 
    // an attempt to account for that:
    new_val = new_val * 16256.0 / 1000.0 + 41;

    int out;

    if (new_val < 50.0) new_val = 0.0;
    else if (new_val > 16250.0) new_val = 16383.0;

    out = (int)(new_val + 0.5);

    return out;
  } // Cnv_VolToFader


  unsigned char Cnv_VolToEncoder(double value) 
  {
    double new_val = (DB2SLIDER( VAL2DB(value) ) * 14.0 / 1000.0) + 1;

    if (new_val < 1.0) new_val = 1.0;
    else if (new_val > 15.0) new_val = 15.0;

    return (char)(new_val + 0.5);
  } // Cnv_VolToEncoder


  // PAN / WIDTH

  double Cnv_FaderToPanWidth(unsigned int value) 
  {
    double new_val = -1.0 + ((double)value / 16383.0 * 2.0);

    if (new_val < -1.0) new_val = -1.0;
    else if (new_val > 1.0) new_val = 1.0;

    return new_val;
  } // Cnv_FaderToPanWidth


  double Cnv_EncoderToPanWidth(double old_val, signed char rel_val) 
  { 
    double new_val = old_val + (double)rel_val * (double)ENCRESPAN;

    if (new_val < -1.0) new_val = -1.0;
    else if (new_val > 1.0) new_val = 1.0;
    return new_val;
  } // Cnv_EncoderToPanWidth


  int Cnv_PanWidthToFader(double value) 
  {
    double new_val = (1.0 + value) * 16383.0 * 0.5;

    if (new_val < 0.0) new_val = 0.0;
    else if ( new_val > 16383.0) new_val = 16383.0;

    return (int)(new_val + 0.5);
  } // Cnv_PanWidthToFader


  char Cnv_PanToEncoder(double value) 
  {
    double new_val = ((1.0 + value) * 14.0 * 0.5) + 1;

    if (new_val < 1.0) new_val = 1.0;
    else if ( new_val > 15.0) new_val = 15.0;

    return (char)(new_val + 0.5);
  } // Cnv_PanToEncoder


  char Cnv_WidthToEncoder(double value) 
  {
    double new_val = abs(value) * 7.0 + 1;
    if (new_val < 1.0) new_val = 1.0;
    else if ( new_val > 15.0) new_val = 15.0;

    return (char)(new_val + 0.5);
  } // Cnv_WidthToEncoder


  // FX PARAM

  double Cnv_FaderToFXParam(double min, double max, unsigned int value)
  {
    double new_val = min + ((double)value / 16256.0 * (max - min));

    if (new_val < min) new_val = min;
    else if (new_val > max) new_val = max;

    return new_val;
  } // Cnv_FaderToFXParam


  double Cnv_EncoderToFXParam(double old_val, double min, double max, double step, signed char rel_val)  
  {
    /* should TrackFX_GetParamStepSizes decide to work, we can use this: 
    double d_rel = (double)rel_val * step;
    */

    // for now:
    double d_rel = (double)rel_val * (max - min) / step;

    double new_val = old_val + d_rel;

    if (new_val < min) new_val = min;
    else if (new_val > max) new_val = max;

    return new_val;
  } // Cnv_EncoderToFXParam


  int Cnv_FXParamToFader(double min, double max, double value)
  {
    double new_val = (value - min) / (max - min) * 16383.0;

    if (new_val < 0.0) new_val = 0.0;
    else if (new_val > 16383.0) new_val = 16383.0;

    return (int)(new_val + 0.5);
  } // Cnv_FXParamToFader


  char Cnv_FXParamToEncoder(double min, double max, double value)
  { 
    double new_val = (value - min) / (max - min) * 14.0 + 1;

    if (new_val < 1.0) new_val = 1.0;
    else if (new_val > 15.0) new_val = 15.0;

    return (char)(new_val + 0.5);
  } // Cnv_FXParamToEncoder


  // HELPERS

  void Hlp_SaveSelection()
  {
    if (saved_sel == 0)
    {
    
      saved_sel_len = CountSelectedTracks(0);
      saved_sel = new MediaTrack*[saved_sel_len];

      ReaProject* rpr_pro = EnumProjects(-1, NULL, 0);

      for(int sel_tk = 0; sel_tk < saved_sel_len; sel_tk++)
        saved_sel[sel_tk] = GetSelectedTrack(rpr_pro, sel_tk);

      // unsel all tks
      int all_tks = CountTracks(0);
      MediaTrack* tk;
  
      for (int i = 0; i < all_tks; i++)
      {
        tk = GetTrack(0, i);
        SetTrackSelected(tk, false);
      }
    }
  } // Hlp_SaveSelection


  void Hlp_RestoreSelection()
  {
    if (saved_sel != 0)
    {
      // unsel all tks
      int all_tks = CountTracks(0);
      MediaTrack* tk;
  
      for (int i = 0; i < all_tks; i++)
      {
        tk = GetTrack(0, i);
        SetTrackSelected(tk, false);
      }

      for(int sel_tk = 0; sel_tk < saved_sel_len; sel_tk++)
        SetTrackSelected(saved_sel[sel_tk], true);

      delete saved_sel;
      saved_sel = 0;
    }
  } // Hlp_RestoreSelection


  void Hlp_GetCustomCmdIds()
  {
    const char* name;
    char index_string[3];

    // go through all custom commands and parse names to get cmd ids
    for (int cmd = 50000; cmd <= 65535; cmd++)
    {
      name = kbd_getTextFromCmd(cmd, NULL);
      int index;
    
      if (strstr(name, "US-2400"))
      {
        for (int s = 0; s < 6; s++)
        {
           sprintf(index_string, "- %d", s+1);
           if (strstr(name, index_string)) index = s;
        }

        if (strstr(name, "Aux-Mode"))
        {
          if (strstr(name, "FKey")) cmd_aux_fkey[index] = cmd;
          else if (strstr(name, "Shift")) cmd_aux_shift[index] = cmd;
        
        } else if (strstr(name, "Pan-Mode"))
        {
          if (strstr(name, "FKey")) cmd_pan_fkey[index] = cmd;
          else if (strstr(name, "Shift")) cmd_pan_shift[index] = cmd;

        } else if (strstr(name, "Chan-Mode"))
        {
          if (strstr(name, "FKey")) cmd_chan_fkey[index] = cmd;
          else if (strstr(name, "Shift")) cmd_chan_shift[index] = cmd;
        }
      }
    }
  } // Hlp_GetCustomCmdIds


  void Hlp_Alphanumeric(char* str)
  {
    // replace everything other than A-Z, a-z, 0-9 with a space
    for (int i = 0; i < strlen(str); i++)
      if ( (str[i] < '0') 
        || ((str[i] > '9') && (str[i] < 'A')) 
        || ((str[i] > 'Z') && (str[i] < 'a'))
        || (str[i] > 'z') ) str[i] = ' ';
  }


public:


  ////// CONSTRUCTOR / DESTRUCTOR //////

  CSurf_US2400(int indev, int outdev, int *errStats)
  {
    ////// GLOBAL VARS //////

    m_midi_in_dev = indev;
    m_midi_out_dev = outdev;

    m_offset = 0;
    m_size = 0;


    // cmd_ids
    for (char i = 0; i < 6; i++)
    {
      cmd_aux_fkey[i] = -1;
      cmd_aux_shift[i] = -1;
      cmd_pan_fkey[i] = -1;
      cmd_pan_shift[i] = -1;
      cmd_chan_fkey[i] = -1;
      cmd_chan_shift[i] = -1;
    }

    // for fader data
    waitformsb = false;

    // for myblink
    s_myblink = false;
    myblink_ctr = 0;

    // for init
    s_initdone = false;
    s_exitdone = false;
  
    // touchstates & cache
    s_touch_fdr = 0;
  
    for (char i = 0; i < 25; i++) 
    {
      cache_faders[i] = 0;
      if (i < 24) 
      {
        s_touch_enc[i] = 0;
        cache_enc[i] = 0;
      }
    }
  
    cache_upd_faders = 0;
    cache_upd_enc = 0;
    cache_exec = 0;
      
    // general states
    s_ch_offset = 0; // bank up/down
    s_play = false; // playstates
    s_rec = false;
    s_loop = false;
    s_automode = 1; // automationmodes

    // modes
    m_flip = false;
    m_chan = false;
    m_pan = true;
    m_aux = 0;
    m_scrub = false;

    // qualifier keys
    q_fkey = false;
    q_shift = false;

    // for channel strip
    chan_ch = 0;
    chan_fx = 0;
    chan_par_offs = 0;
    chan_fx_env_arm = false;

    // save selection
    saved_sel = 0;
    saved_sel_len = 0;

    // loop all
    s_loop_all = false;

    // Display
    dsp_hwnd = NULL;
    dsp_class.cbSize = sizeof(WNDCLASSEX);
    dsp_class.style = 0;
    dsp_class.lpfnWndProc = (WNDPROC)Dsp_WindowProc;
    dsp_class.cbClsExtra = 0;
    dsp_class.cbWndExtra = 0;
    dsp_class.hInstance = g_hInst;
    dsp_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    dsp_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    dsp_class.hbrBackground = CreateSolidBrush(RGB(255, 255, 255));
    dsp_class.lpszMenuName = NULL;
    dsp_class.lpszClassName = "dsp";
    dsp_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    if (!RegisterClassEx(&dsp_class)) DBGS("Register failed\n")

    // inifile
    ini_path = WDL_String(GetResourcePath());
    ini_path.Append("\\");
    ini_path.Append(INIFILE);

    // create midi hardware access
    m_midiin = m_midi_in_dev >= 0 ? CreateMIDIInput(m_midi_in_dev) : NULL;
    m_midiout = m_midi_out_dev >= 0 ? CreateThreadedMIDIOutput( CreateMIDIOutput(m_midi_out_dev, false, NULL) ) : NULL;

    if (errStats)
    {
      if (m_midi_in_dev >=0  && !m_midiin) *errStats|=1;
      if (m_midi_out_dev >=0  && !m_midiout) *errStats|=2;
    }

    if (m_midiin) m_midiin->start();
  } // CSurf_US2400()


  ~CSurf_US2400()
  {
    Dsp_CloseWindow();

    s_exitdone = MySetSurface_Exit();
    do
    { Sleep(500);  
    } while (!s_exitdone);
    
    delete saved_sel;

    for (char ch = 0; ch < 24; ch++)
    {
      if (dsp_values[ch] != NULL) delete dsp_values[ch];
    }

    delete m_midiout;
    delete m_midiin;
  } // ~CSurf_US2400()



  ////// CUSTOM SURFACE UPDATES //////

  bool MySetSurface_Init() 
  {
    Dsp_GetWinCoordsIni();

    Hlp_GetCustomCmdIds();
    CSurf_ResetAllCachedVolPanStates(); 
    TrackList_UpdateAllExternalSurfaces(); 

    // Initially Pan Mode
    MySetSurface_EnterPanMode();

    // Initially Scrub off
    m_scrub = true;
    MySetSurface_ToggleScrub();
    
    // update BankLEDs
    MySetSurface_UpdateBankLEDs();

    // update loop
    s_loop = !(bool)GetSetRepeat(-1);
    MyCSurf_ToggleRepeat();
    SetRepeatState(s_loop);

    // Set global auto mode to off / trim, CSurf cmds only change track modes
    SetAutomationMode(0, false);
    MySetSurface_UpdateAutoLEDs();
        
    return true;
  } // MySetSurface_Init


  bool MySetSurface_Exit()
  {
    Dsp_SaveWinCoordsIni();

    CSurf_ResetAllCachedVolPanStates();

    MySetSurface_ExitChanMode();

    for (char ch_id = 0; ch_id <= 24; ch_id++)
    {
      // reset faders
      MIDIOut(0xb0, ch_id + 0x1f, 0);
      MIDIOut(0xb0, ch_id, 0);

      // reset encoders
      if (ch_id < 24) MIDIOut(0xb0, ch_id + 0x40, 0);
    }

    // reset mute/solo/select
    for (char btn_id = 0; btn_id <= 0x79; btn_id ++)
      MIDIOut(0xb1, btn_id, 0);

    // reset bank leds
    MIDIOut(0xb0, 0x5d, 0);

    return true;
  } // MySetSurface_Exit


  void MySetSurface_UpdateButton(unsigned char btn_id, bool btn_state, bool blink)
  {
    unsigned char btn_cmd = 0x7f; // on
    if (blink) btn_cmd = 0x01; // blink
    if (!btn_state) btn_cmd = 0x00; // off
    MIDIOut(0xb1, btn_id, btn_cmd);
  } // MySetSurface_UpdateButton


  void MySetSurface_UpdateBankLEDs()
  {
    char led_id = s_ch_offset / 24;
    MIDIOut(0xb0, 0x5d, led_id);
  } // MySetSurface_UpdateBankLEDs


  void MySetSurface_UpdateAutoLEDs()
  {
    // update transport buttons
    for (char btn_id = 0; btn_id <= 3; btn_id ++)
    {
      bool on = false;

      if ( (btn_id == 3) && (s_play) ) on = true;
      if ( (s_automode & (1 << btn_id)) && (s_myblink) ) on = !on;

      MySetSurface_UpdateButton(0x75 + btn_id, on, false);
    }
  } // MySetSurface_UpdateAutoLEDs


  void MySetSurface_UpdateAuxButtons()
  {
    for (char aux_id = 1; aux_id <= 6; aux_id++)
    {
      bool on = false;
      
      if ( (q_fkey) || (q_shift) )
      { // qualifier keys

        on = false;

      } else if (m_aux == aux_id) 
      { // aux modes

        on = true;

      } else if (m_chan) 
      { // chan mode
      
        // bypass
        int amount_fx = TrackFX_GetCount(chan_rpr_tk);
        bool bypass_fx;

        // if fx doesn't exist yet (e.g. insert on first open)
        if (chan_fx >= amount_fx) bypass_fx = false;
        else bypass_fx = !(bool)TrackFX_GetEnabled(chan_rpr_tk, chan_fx);
        bool bypass_allfx = !(bool)GetMediaTrackInfo_Value(chan_rpr_tk, "I_FXEN");

        if ( (aux_id == 3) && (bypass_fx) && (!q_fkey) && (!q_shift) ) on = true;

        // write fx or tk auto?
        if ( (aux_id == 6) && (chan_fx_env_arm) && (!q_fkey) && (!q_shift) ) on = true;
      }

      if (m_chan) MySetSurface_UpdateButton(0x64 + aux_id, on, true);
      else MySetSurface_UpdateButton(0x64 + aux_id, on, false);
    }
  } // MySetSurface_UpdateAuxButtons


  void MySetSurface_UpdateFader(unsigned char ch_id)
  {

    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
    int para_amount;

    double d_value;
    int value;
    
    bool istrack = false;
    bool ismaster = false;
    bool isactive = true;

    // is track or master?
    if ( (ch_id >= 0) && (ch_id <= 23) ) istrack = true; // no track fader
    else if (rpr_tk == CSurf_TrackFromID(0, false)) ismaster = true;

    // active fader? 
    if (!rpr_tk) isactive = false; // no corresponding rpr_tk
    
    if ( (m_chan) && (m_flip) ) 
    { // only chan and flip: inside para_amount?
   
      para_amount = TrackFX_GetNumParams(chan_rpr_tk, chan_fx);
      if (chan_par_offs + ch_id >= para_amount) isactive = false;
      else isactive = true; // is track doesn't matter when chan and flipped
    
    } else if ( (m_aux > 0) && (m_flip) ) 
    { // only aux #x and flip: has send #x?
      
      int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
      if (send_id == -1) isactive = false;
    }

    if (istrack || ismaster)
    {
      // get values
      if (ismaster)
      { // if master -> volume

        GetTrackUIVolPan(rpr_tk, &d_value, NULL);
        value = Cnv_VolToFader(d_value);
      
      } else if (!isactive)
      { // if there is no track, parameter (chan & flip), send (aux & flip), reset

        value = 0; 
   
      } else
      { // if is active track fader

        if (m_flip)
        {
          if (m_chan) 
          { // flip & chan -> fx param

            double min, max;
            d_value = TrackFX_GetParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &min, &max);
            value = Cnv_FXParamToFader(min, max, d_value);

          } else if (m_aux > 0)
          { // flip + aux -> send Vol
            int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
            if (send_id != -1)
            {
              d_value = GetTrackSendUIVolPan(rpr_tk, send_id, &d_value, NULL);
              value = Cnv_VolToFader(d_value);
            }
          
          } else
          { // pan
            
            if (q_fkey)
            { // flip + fkey + pan -> width

              d_value = GetMediaTrackInfo_Value(rpr_tk, "D_WIDTH");
              value = Cnv_PanWidthToFader(d_value);
            
            } else {
              // flip + pan -> pan

              GetTrackUIVolPan(rpr_tk, NULL, &d_value);
              value = Cnv_PanWidthToFader(d_value);
            }
          } // if (m_chan)
        } else
        { // no flip -> volume

          GetTrackUIVolPan(rpr_tk, &d_value, NULL);
          value = Cnv_VolToFader(d_value);

        } // if (m_flip), else
      } // if (!rpr_tk || rpr_tk == CSurf_TrackFromID(0, false)), else
      
      if (value != cache_faders[ch_id])
      {
        // new value to cache (gets executed on next run cycle)
        cache_faders[ch_id] = value;

        // set upd flag 
        cache_upd_faders = cache_upd_faders | (1 << ch_id);

      }  

    } // if (active or master)
  } // MySetSurface_UpdateFader


  void ExecuteFaderUpdate(int ch_id)
  {
    if ((s_touch_fdr & (1 << ch_id)) == 0)
    {
      // send midi
      MIDIOut(0xb0, ch_id + 0x1f, (cache_faders[ch_id] & 0x7f));
      MIDIOut(0xb0, ch_id, ((cache_faders[ch_id] >> 7) & 0x7f));
      
      // remove update flag
      cache_upd_faders = cache_upd_faders ^ (1 << ch_id);  
    }
  } // ExecuteFaderUpdate


  void MySetSurface_UpdateEncoder(int ch_id)
  {
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
    int para_amount;

    double d_value;
    unsigned char value;
    bool dot = false; // for phase switch, rec arm

    bool istrack = true;
    bool exists = false;
    bool isactive = true;

    // encoder exists?
    if ( (ch_id >= 0) && (ch_id <= 23) ) exists = true;

    // active encoder? 
    if (!rpr_tk) 
    { // no track
      
      isactive = false; 
      istrack = false;
    }

    if ( (m_chan) && (!m_flip) )
    { 
      para_amount = TrackFX_GetNumParams(chan_rpr_tk, chan_fx);
      if (chan_par_offs + ch_id >= para_amount) isactive = false;
      else isactive = true; // chan + fip: is track doesn't matter

    } else if (m_aux > 0) 
    { // only aux #x: has send #x?
      int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
      if (send_id == -1) isactive = false;
    }

    // get values and update
    if (exists)
    {
      if (!isactive)
      { // is not active encoder

        if (m_pan) value = 15; // reset, show something to indicate inactive
        else value = 0; // reset
      
      } else 
      { 
        if (m_flip)
        {
          if (m_aux > 0)
          { // flip + aux -> send Pan
            int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
            if (send_id != -1)
            {
              GetTrackSendUIVolPan(rpr_tk, send_id, NULL, &d_value);
              value = Cnv_PanToEncoder(d_value);
              value += 0x10; // pan mode
            }
          } else
          { // flip -> volume

            GetTrackUIVolPan(rpr_tk, &d_value, NULL);
            value = Cnv_VolToEncoder(d_value);
            value += 0x20; // bar mode
          }
        } else
        {
          if (m_chan)
          { // chan -> fx_param (para_offset checked above)

            double min, max;
            d_value = TrackFX_GetParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &min, &max);

            value = Cnv_FXParamToEncoder(min, max, d_value);
            value += 0x20; // bar mode
         
          } else if (m_aux > 0)
          { // aux -> send level
            int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
            if (send_id != -1)
            {
              GetTrackSendUIVolPan(rpr_tk, send_id, &d_value, NULL);
              value = Cnv_VolToEncoder(d_value);
              value += 0x20; // bar mode;
            }
          
          } else if (m_pan)
          {
            if (q_fkey) 
            { // pan mode + fkey -> width
              
              d_value = GetMediaTrackInfo_Value(rpr_tk, "D_WIDTH");
              value = Cnv_WidthToEncoder(d_value);
              value += 0x30; // width mode

            } else
            { // pan mode -> pan

              GetTrackUIVolPan(rpr_tk, NULL, &d_value);
              value = Cnv_PanToEncoder(d_value);
              value += 0x10; // pan mode
            }
          }
        } // if (m_flip), else
         
      } // if !active, else

      if (istrack)
      {
        // phase states
        if ( (bool)GetMediaTrackInfo_Value(rpr_tk, "B_PHASE") ) dot = true;
          
        // rec arms: blink
        if ( ((bool)GetMediaTrackInfo_Value(rpr_tk, "I_RECARM")) && (s_myblink) ) dot = !dot;

        if (dot) value += 0x40; // set dot
      }

      if (value != cache_enc[ch_id])
      {
        // new value to cache (gets executed on next run cycle)
        cache_enc[ch_id] = value;

        // set upd flag 
        cache_upd_enc = cache_upd_enc | (1 << ch_id);
      }    

    } // if exists
  } // MySetSurface_UpdateEncoder


  void ExecuteEncoderUpdate(int ch_id)
  {
    // send midi
    MIDIOut(0xb0, ch_id + 0x40, cache_enc[ch_id]);
    
    // remove update flag
    cache_upd_enc = cache_upd_enc ^ (1 << ch_id);
  } // ExecuteEncoderUpdate


  void MySetSurface_UpdateTrackElement(char ch_id)
  {
    // get info
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
    
    // if trreck exists
    if (rpr_tk)
    {
      bool selected = IsTrackSelected(rpr_tk);
      bool solo = (bool)GetMediaTrackInfo_Value(rpr_tk, "I_SOLO");
      bool mute = (bool)GetMediaTrackInfo_Value(rpr_tk, "B_MUTE");
      bool bypass = (bool)GetMediaTrackInfo_Value(rpr_tk, "I_FXEN");

      // blink chan sel
      selected = ( (m_chan && ch_id == chan_ch && s_myblink) != selected );

      // blink track fx bypass
      mute = ( (!bypass && s_myblink) != mute );

      MySetSurface_UpdateButton(ch_id * 4 + 1, selected, false);
      MySetSurface_UpdateButton(ch_id * 4 + 2, solo, false);
      MySetSurface_UpdateButton(ch_id * 4 + 3, mute, false);
      
    } else
    {
      // reset buttons
      MySetSurface_UpdateButton(ch_id * 4 + 1, false, false);
      MySetSurface_UpdateButton(ch_id * 4 + 2, false, false);
      MySetSurface_UpdateButton(ch_id * 4 + 3, false, false);
    }
  } // MySetSurface_UpdateTrackElement


  void MySetSurface_ToggleFlip()
  {
    m_flip = !m_flip;

    CSurf_ResetAllCachedVolPanStates();
    TrackList_UpdateAllExternalSurfaces();

    MySetSurface_UpdateButton(0x63, m_flip, true);
  } // MySetSurface_ToggleFlip


  void MySetSurface_ToggleFKey(bool btn_state)
  {
    q_fkey = btn_state;
    MySetSurface_UpdateButton(0x6d, btn_state, false);
    MySetSurface_UpdateAuxButtons();

    // update encoders for width in pan mode
    if (m_pan)
      for (char ch_id = 0; ch_id < 24; ch_id++) 
        if (m_flip) MySetSurface_UpdateFader(ch_id);
        else MySetSurface_UpdateEncoder(ch_id);
  } // MySetSurface_ToggleFKey


  void MySetSurface_ToggleShift(bool btn_state)
  {
    q_shift = btn_state;
    MySetSurface_UpdateButton(0x74, btn_state, false);
    MySetSurface_UpdateAuxButtons();
  } // MySetSurface_ToggleShift


  void MySetSurface_ToggleScrub()
  {
    m_scrub = !m_scrub;
    MySetSurface_UpdateButton(0x6f, m_scrub, false);
  } // MySetSurface_ToggleScrub


  void MySetSurface_Chan_SetFxParamOffset(char dir)
  {
    chan_par_offs -= 24 * dir;
    if (chan_par_offs < 0) chan_par_offs = 0;

    // check parameter count
    int amount_paras = TrackFX_GetNumParams(chan_rpr_tk, chan_fx);
    if (chan_par_offs >= amount_paras) chan_par_offs -= 24;
    
    // update encoders or faders
    for (char ch_id = 0; ch_id < 23; ch_id++) 
      if (m_flip) MySetSurface_UpdateFader(ch_id);
      else MySetSurface_UpdateEncoder(ch_id);

    if (dsp_hwnd != NULL) Dsp_Update();

  } // MySetSurface_Chan_Set_FXParamOffset


  void MySetSurface_Chan_SelectTrack(unsigned char ch_id, bool force_upd)
  {
    if ( (ch_id != chan_ch) || (force_upd) )
    {
      // reset button of old channel
      if (IsTrackSelected(chan_rpr_tk)) MySetSurface_UpdateButton(chan_ch * 4 + 1, true, false);
      else MySetSurface_UpdateButton(chan_ch * 4 + 1, false, false);

      // close fx of old channel
      MyCSurf_Chan_CloseFX(chan_fx);

      // activate new channel
      MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
      chan_ch = ch_id;
      chan_rpr_tk = rpr_tk;
      
      MySetSurface_UpdateButton(chan_ch * 4 + 1, true, true);

      // open fx              
      MyCSurf_Chan_OpenFX(chan_fx);

      if (dsp_hwnd != NULL) Dsp_Update();

    } else MySetSurface_ExitChanMode();
  } // MySetSurface_Chan_SelectTrack


  void MySetSurface_ShiftBanks(char dir, char factor)
  { 

    double track_amount = (double)CountTracks(0);
    int max_offset = (int)( ceil( (track_amount) / (double)factor ) - (24.0 / double(factor)));
    max_offset *= factor;

    int old_ch_offset = s_ch_offset;

    // move in dir by 8 or 24 (factor)
    s_ch_offset += factor * dir;
    s_ch_offset -= (s_ch_offset % factor);

    // min / max
    if (s_ch_offset > max_offset) s_ch_offset = max_offset;
    if (s_ch_offset > 168) s_ch_offset = 168;
    if (s_ch_offset < 0) s_ch_offset = 0;

    // if correction pushes in wrong direction keep old
    if ( (dir > 0) && (old_ch_offset > s_ch_offset) ) s_ch_offset = old_ch_offset;
    if ( (dir < 0) && (old_ch_offset < s_ch_offset) ) s_ch_offset = old_ch_offset;

    // update channel strip
    if (m_chan) chan_ch = chan_ch + old_ch_offset - s_ch_offset;

    // update mixer display
    MediaTrack* leftmost = GetTrack(0, s_ch_offset);
    SetMixerScroll(leftmost);

    for(char ch_id = 0; ch_id < 24; ch_id++)
    {
      MySetSurface_UpdateEncoder(ch_id);
      MySetSurface_UpdateFader(ch_id);
      MySetSurface_UpdateTrackElement(ch_id);
    }

    MySetSurface_UpdateBankLEDs();

    if (dsp_hwnd != NULL) Dsp_Update();
  } // MySetSurface_ShiftBanks


  void MySetSurface_EnterChanMode()
  {
    if (m_pan) MySetSurface_ExitPanMode();
    if (m_aux > 0) MySetSurface_ExitAuxMode();
   
    if (!chan_fx_env_arm) MyCSurf_Chan_ToggleArmFXEnv();

    m_chan = true;
    // blink Chan Button
    MySetSurface_UpdateButton(0x64, true, true);

    chan_rpr_tk = Cnv_ChannelIDToMediaTrack(chan_ch);

    MyCSurf_Chan_OpenFX(chan_fx);
    
    // blink Track Select
    MySetSurface_UpdateButton(chan_ch * 4 + 1, true, true);

    // blink para offset, bypass
    MySetSurface_UpdateAuxButtons();
      
    // blink banks
    MySetSurface_UpdateButton(0x70, true, true);
    MySetSurface_UpdateButton(0x71, true, true);

    if (dsp_hwnd != NULL) Dsp_Update();
  } // MySetSurface_EnterChanMode


  void MySetSurface_ExitChanMode()
  {
    m_chan = false;
    MySetSurface_UpdateButton(0x64, false, false);

    // reset select button
    if (IsTrackSelected(chan_rpr_tk)) MySetSurface_UpdateButton(chan_ch * 4 + 1, true, false);
    else MySetSurface_UpdateButton(chan_ch * 4 + 1, false, false);

    // if writing fx envs, stop now
    if (chan_fx_env_arm) MyCSurf_Chan_ToggleArmFXEnv();

    // close window
    MyCSurf_Chan_CloseFX(chan_fx);

    // unblink bank buttons
    MySetSurface_UpdateButton(0x70, false, false);
    MySetSurface_UpdateButton(0x71, false, false);

    MySetSurface_EnterPanMode();

    if (dsp_hwnd != NULL) Dsp_Update();
  } // MySetSurface_ExitChanMode


  void MySetSurface_EnterPanMode()
  {
    if (m_chan) MySetSurface_ExitChanMode();
    if (m_aux > 0) MySetSurface_ExitAuxMode();

    m_pan = true;
    MySetSurface_UpdateButton(0x6c, true, false);

    // reset Aux
    MySetSurface_UpdateAuxButtons();

    // update encoders or faders
    for (char ch_id = 0; ch_id < 23; ch_id++) 
      if (m_flip) MySetSurface_UpdateFader(ch_id);
      else MySetSurface_UpdateEncoder(ch_id);
  } // MySetSurface_EnterPanMode


  void MySetSurface_ExitPanMode()
  {
    m_pan = false;
    MySetSurface_UpdateButton(0x6c, false, false);
  } // MySetSurface_ExitPanMode


  void MySetSurface_EnterAuxMode(unsigned char sel)
  {
    if (m_pan) MySetSurface_ExitPanMode();

    m_aux = sel;

    // reset Aux
    MySetSurface_UpdateAuxButtons();

    // update encoders or faders
    for (char ch_id = 0; ch_id < 23; ch_id++)
    { 
      if (m_flip) MySetSurface_UpdateFader(ch_id);
      MySetSurface_UpdateEncoder(ch_id); // update encoders on flip also (> send pan)!
    }
  } // MySetSurface_EnterAuxMode


  void MySetSurface_ExitAuxMode()
  {
    m_aux = 0;
  } // MySetSurface_ExitAuxMode

  

  // REAPER INITIATED SURFACE UPDATES

  void SetSurfaceVolume(MediaTrack* rpr_tk, double vol)
  { 
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    if ( (ch_id >= 0) && (ch_id <= 24) )
      if (!m_flip) MySetSurface_UpdateFader(ch_id);
      else if (ch_id <= 23) MySetSurface_UpdateEncoder(ch_id);
  } // SetSurfaceVolume
  

  void SetSurfacePan(MediaTrack* rpr_tk, double pan)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    if ( (ch_id >= 0) && (ch_id <= 24) )
      if (m_flip) MySetSurface_UpdateFader(ch_id);
      else if (ch_id <= 23) MySetSurface_UpdateEncoder(ch_id);
  } // SetSurfacePan
  

  void SetTrackListChange()
  {
    CSurf_ResetAllCachedVolPanStates(); // is this needed?

    // reset faders, encoders, track elements
    for (int i = 0; i < 25; i++)
    {
      MySetSurface_UpdateFader(i);
      MySetSurface_UpdateTrackElement(i);
      MySetSurface_UpdateEncoder(i);
    }

    if (dsp_hwnd != NULL) Dsp_Update();
  } // SetTrackListChange


  void SetSurfaceMute(MediaTrack* rpr_tk, bool mute)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    if ( (ch_id >= 0) && (ch_id <= 23) ) 
    {
      if (mute) MySetSurface_UpdateButton(4 * ch_id + 3, true, false);
      else MySetSurface_UpdateButton(4 * ch_id + 3, false, false);
    }
  } // SetSurfaceMute


  void SetSurfaceSelected(MediaTrack* rpr_tk, bool selected)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    if ( (ch_id >= 0) && (ch_id <= 24) ) 
    {
      MySetSurface_UpdateButton(4 * ch_id + 1, selected, false);
      if (selected) Main_OnCommand(CMD_SEL2LASTTOUCH, 0);
    }
  } // SetSurfaceSelected


  void SetSurfaceSolo(MediaTrack* rpr_tk, bool solo)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    if ( (ch_id >= 0) && (ch_id <= 23) ) 
    {
      if (solo) MySetSurface_UpdateButton(4 * ch_id + 2, true, false);
      else MySetSurface_UpdateButton(4 * ch_id + 2, false, false);
    }

    // update CLR SOLO
    if (AnyTrackSolo(0)) MySetSurface_UpdateButton(0x62, true, true);
    else MySetSurface_UpdateButton(0x62, false, false);
  } // SetSurfaceSolo


  void SetSurfaceRecArm(MediaTrack* rpr_tk, bool recarm)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);
    MySetSurface_UpdateEncoder(ch_id);
  } // SetSurfaceRecArm


  bool GetTouchState(MediaTrack* rpr_tk, int isPan)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    if (isPan == 0)
    {
      if ( (ch_id >= 0) && (ch_id <= 24) && ((s_touch_fdr & (1 << ch_id)) > 0) ) return true;
      else return false;

    } else
    {
      if ( (ch_id >= 0) && (ch_id <= 23) && (s_touch_enc[ch_id] > 0) ) return true;
      else return false;
    }

  } // GetTouchState


  void SetPlayState(bool play, bool pause, bool rec)
  {
    s_play = play && !pause;
    s_rec = rec;
    
    MySetSurface_UpdateAutoLEDs();
    MySetSurface_UpdateButton(0x79, s_rec, false);
  } // SetPlayState


  void SetRepeatState(bool rep)
  {
    s_loop = rep;

    // update in/out buttons
    MySetSurface_UpdateButton(0x72, s_loop, false); 
    MySetSurface_UpdateButton(0x73, s_loop, false); 
  } // SetRepeatState



  ////// SUBMIT CHANGES TO REAPER /////

  // API Event Triggers:
  // double CSurf_OnVolumeChange(MediaTrack *trackid, double volume, bool relative)
  // double CSurf_OnPanChange(MediaTrack *trackid, double pan, bool relative)
  // bool CSurf_OnMuteChange(MediaTrack *trackid, int mute)
  // bool CSurf_OnSelectedChange(MediaTrack *trackid, int selected)
  // bool CSurf_OnSoloChange(MediaTrack *trackid, int solo)
  // bool CSurf_OnFXChange(MediaTrack *trackid, int en)
  // bool CSurf_OnRecArmChange(MediaTrack *trackid, int recarm)
  // void CSurf_OnPlay()
  // void CSurf_OnStop()
  // void CSurf_OnFwd(int seekplay)
  // void CSurf_OnRew(int seekplay)
  // void CSurf_OnRecord()
  // void CSurf_GoStart()
  // void CSurf_GoEnd()
  // void CSurf_OnArrow(int whichdir, bool wantzoom)
  // void CSurf_OnTrackSelection(MediaTrack *trackid)
  // void CSurf_ResetAllCachedVolPanStates()
  // void CSurf_void ScrubAmt(double amt)


  // TRACK CONTROLS AND BEYOND

  void MyCSurf_ToggleSolo(MediaTrack* rpr_tk, bool this_only)
  {
    int solo = (int)GetMediaTrackInfo_Value(rpr_tk, "I_SOLO");
    if (this_only)
    {
      // unsolo all, (re-)solo only selected
      SoloAllTracks(0);
      solo = 2;
    } else {
      // toggle solo
      if (solo == 2) solo = 0;
      else solo = 2;
    }

    CSurf_OnSoloChange(rpr_tk, solo);

    bool solo_surf = true;  
    if (solo == 0) solo_surf = false;

    SetSurfaceSolo(rpr_tk, solo_surf);
  } // MyCSurf_ToggleSolo


  void MyCSurf_UnsoloAllTracks()
  {
    SoloAllTracks(0);
  } // MyCSurf_UnsoloAllTracks


  void MyCSurf_ToggleMute(MediaTrack* rpr_tk, bool this_only)
  {
    char mute;
    if (this_only)
    {
      // unmute all, (re-)mute only selected
      MuteAllTracks(0);
      mute = 1;
    } else
    {
      // toggle mute on selected
      mute = -1;
    }
    CSurf_OnMuteChange(rpr_tk, mute);
  } // MyCSurf_ToggleMute


  void MyCSurf_UnmuteAllTracks()
  {
    MuteAllTracks(0);
  } // MyCSurf_UnmuteAllTracks


  void MyCSurf_SwitchPhase(MediaTrack* rpr_tk)
  {
    char ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    bool phase = (bool)GetMediaTrackInfo_Value(rpr_tk, "B_PHASE");

    phase = !phase;
    SetMediaTrackInfo_Value(rpr_tk, "B_PHASE", phase);

    MySetSurface_UpdateEncoder(ch_id);
  } // MyCSurf_SwitchPhase


  void MyCSurf_SelectMaster() 
  {
    MediaTrack* rpr_master = Cnv_ChannelIDToMediaTrack(24);

    //bool master_sel = (bool)GetMediaTrackInfo_Value(rpr_master, "I_SELECTED");
    bool master_sel = IsTrackSelected(rpr_master);
    master_sel = !master_sel;
    CSurf_OnSelectedChange(rpr_master, (int)master_sel); // ?
    SetTrackSelected(rpr_master, master_sel); // ?
    Main_OnCommand(CMD_SEL2LASTTOUCH, 0);
  } // MyCSurf_SelectMaster


  void MyCSurf_ToggleSelectAllTracks() 
  {
    int sel_tks = CountSelectedTracks(0);
    int all_tks = CountTracks(0);

    bool sel = false;
    MediaTrack* tk;
    if (sel_tks != all_tks) sel = true;

    for (int i = 0; i < all_tks; i++)
    {
      tk = GetTrack(0, i);
      SetTrackSelected(tk, sel);
    }
  } // MyCSurf_ToggleSelectAllTracks() 


  // CUSTOM TRANSPORT 

  void MyCSurf_OnRew()
  {
    Main_OnCommand(CMD_RJUMP, 0);
  } // MyCSurf_OnRew


  void MyCSurf_OnFwd()
  {
    Main_OnCommand(CMD_FJUMP, 0);
  } // MyCSurf_OnFwd


  void MyCSurf_OnRec()
  {
    if (s_play) 
    {
      s_rec = !s_rec;
      Main_OnCommand(CMD_TGGLRECBEAT, 0);
      if (s_rec) MySetSurface_UpdateButton(0x79, true, true);
      else MySetSurface_UpdateButton(0x79, false, false);
    } else 
    {
      CSurf_OnRecord();
    }
  } // MyCSurf_OnRec


  // CHANNEL STRIP

  void MyCSurf_Chan_SelectFX(int open_fx_id)
  {
    MyCSurf_Chan_CloseFX(chan_fx);
    MyCSurf_Chan_OpenFX(open_fx_id);
  } // MyCSurf_Chan_SelectFX


  void MyCSurf_Chan_OpenFX(int fx_id)
  {
    int amount_fx = TrackFX_GetCount(chan_rpr_tk);

    if (fx_id >= amount_fx) fx_id = 0;
    else if (fx_id < 0) fx_id = amount_fx - 1;
    
    chan_fx = fx_id;
    TrackFX_Show(chan_rpr_tk, chan_fx, 2); // hide floating window
    TrackFX_Show(chan_rpr_tk, chan_fx, 1); // show chain window
    TrackFX_SetOpen(chan_rpr_tk, chan_fx, true);

    // reset param offset
    chan_par_offs = 0;

    MySetSurface_UpdateAuxButtons();

    if (dsp_hwnd != NULL) Dsp_Update();

    /* Stuff below is not needed when we update faders/encoders in a loop 

    // update encoders or faders
    for (char ch_id = 0; ch_id < 23; ch_id++) 
      if (m_flip) MySetSurface_UpdateFader(ch_id);
      else MySetSurface_UpdateEncoder(ch_id);

    */
  } // MyCSurf_Chan_OpenFX
  

  void MyCSurf_Chan_CloseFX(int fx_id)
  {
    TrackFX_Show(chan_rpr_tk, fx_id, 2); // hide floating window
    TrackFX_Show(chan_rpr_tk, fx_id, 0); // hide chain window
  } // MyCSurf_Chan_CloseFX


  void MyCSurf_Chan_DeleteFX()
  {
    int before_del = TrackFX_GetCount(chan_rpr_tk);

    //isolate track for action
    Hlp_SaveSelection();
    SetOnlyTrackSelected(chan_rpr_tk);
    Main_OnCommand(CMD_SEL2LASTTOUCH, 0);

    TrackFX_SetOpen(chan_rpr_tk, chan_fx, true);
    Main_OnCommand(CMD("_S&M_REMOVE_FX"), 0);
    
    Hlp_RestoreSelection();
    
    if (before_del > 1)
    { 
      // if there are fx left open the previous one in chain
      chan_fx--;
      MyCSurf_Chan_OpenFX(chan_fx);
    }
  } // MyCSurf_Chan_DeleteFX


  void MyCSurf_Chan_InsertFX()
  {
    // isolate track for action
    Hlp_SaveSelection();
    SetOnlyTrackSelected(chan_rpr_tk);
    Main_OnCommand(CMD_SEL2LASTTOUCH, 0);
    
    TrackFX_Show(chan_rpr_tk, chan_fx, 1); // show chain window
    TrackFX_SetOpen(chan_rpr_tk, chan_fx, true);
    Main_OnCommand(CMD_FXBROWSER, 0);

    Hlp_RestoreSelection();

    // focus coming fx, so interface will be up to date after inserting
    int amount_fx = TrackFX_GetCount(chan_rpr_tk);
    chan_fx++;
    if (chan_fx > amount_fx) chan_fx = amount_fx;
  } // MyCSurf_Chan_InsertFX


  void MyCSurf_Chan_MoveFX(char dir)
  {
    int amount_fx = TrackFX_GetCount(chan_rpr_tk);

    // isolate track for selection
    Hlp_SaveSelection();
    SetOnlyTrackSelected(chan_rpr_tk);
    Main_OnCommand(CMD_SEL2LASTTOUCH, 0);

    TrackFX_SetOpen(chan_rpr_tk, chan_fx, true);
    if ( (dir < 0) && (chan_fx > 0) )
    {
      Main_OnCommand(CMD("_S&M_MOVE_FX_UP"), 0);
      chan_fx--;
    
    } else if ( (dir > 0) && (chan_fx < amount_fx - 1) )
    {
      Main_OnCommand(CMD("_S&M_MOVE_FX_DOWN"), 0);
      chan_fx++;  
    }

    Hlp_RestoreSelection();
  } // MyCSurf_Chan_MoveFX


  void MyCSurf_Chan_ToggleAllFXBypass(int ch_id)
  {
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);

    // get info
    bool bypass = (bool)GetMediaTrackInfo_Value(rpr_tk, "I_FXEN");
    
    // toggle bypass
    bypass = !bypass;
    CSurf_OnFXChange(rpr_tk, (int)bypass);   
  } // MyCSurf_Chan_ToggleAllFXBypass


  void MyCSurf_Chan_ToggleFXBypass()
  {
    bool bypass = (bool)TrackFX_GetEnabled(chan_rpr_tk, chan_fx);
    bypass = !bypass;
    TrackFX_SetEnabled(chan_rpr_tk, chan_fx, bypass);
    MySetSurface_UpdateAuxButtons();
  } // MyCSurf_Chan_ToggleFXBypass


  void MyCSurf_Chan_SetFXParam(MediaTrack* rpr_tk, int fx_id, int para_id, double value)
  {
    char ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    TrackFX_SetParam(rpr_tk, fx_id, para_id, value);

    if (m_flip) MySetSurface_UpdateFader(ch_id);
    else MySetSurface_UpdateEncoder(ch_id);
  } // MyCSurf_Chan_SetFXParam


  void MyCSurf_Chan_ToggleArmFXEnv()
  {
    Hlp_SaveSelection();
    SetOnlyTrackSelected(Cnv_ChannelIDToMediaTrack(chan_ch));

    // arm all envelopes -> toggle fx disarms only them
    if (chan_fx_env_arm) Main_OnCommand(CMD("_S&M_ARMALLENVS"), 0);
    // disarm all envelopes -> toggle fx arms only them
    else Main_OnCommand(CMD("_S&M_DISARMALLENVS"), 0);
 
    // toggle arm fx envelopes
    Main_OnCommand(CMD("_S&M_TGLARMPLUGENV"), 0);

    chan_fx_env_arm = !chan_fx_env_arm;

    Hlp_RestoreSelection();

    MySetSurface_UpdateAuxButtons();
  }


  // AUTOMATION

  void MyCSurf_Auto_SetMode(int mode)
  {
    // mode: 0 = off / trim, 1 = read, 2 = touch, 3 = write, 4 = latch

    // exchange touch and latch
    char set_mode = mode;
    if (mode == 2) set_mode = 4;

    int sel_tks = CountSelectedTracks(0);
    int all_tks = CountTracks(0);
    MediaTrack* tk;

    // if writing fx automation only select and change mode for current track
    if (chan_fx_env_arm)
    {
      Hlp_SaveSelection();
      SetOnlyTrackSelected(chan_rpr_tk);

    // if none selected, select and change all
    } else if (sel_tks == 0)
    {
      Hlp_SaveSelection();

      for (int i = 0; i < all_tks; i++)
      {
        tk = GetTrack(0, i);
        SetTrackSelected(tk, true);
      }
    }
    
    // set mode for new selection
    int set_sel = CountSelectedTracks(0);
    for (int t = 0; t < set_sel; t++)
    {
      tk = GetSelectedTrack(0, t);
      SetTrackAutomationMode(tk, set_mode);      
    }

    // restore selection
    if ((chan_fx_env_arm) || (sel_tks == 0))
      Hlp_RestoreSelection();


    /* update CSurf here - because SetAutoMode() only works for global mode changes? */

    // check all tracks for auto modes
    int tk_mode;
    s_automode = 0;

    for (int t = 0; t < all_tks; t++)    
    {
      tk = GetTrack(0, t);
      tk_mode = GetTrackAutomationMode(tk);

      // switch touch and latch
      if (tk_mode == 4) tk_mode = 2;

      // set flags
      s_automode = s_automode | (1 << tk_mode);
    } 

    MySetSurface_UpdateAutoLEDs();

  } // MyCSurf_Auto_SetMode


  void MyCSurf_Auto_WriteCurrValues()
  {
    Main_OnCommand(CMD_AUTOTOSEL, 0);
  } // MyCSurf_Auto_WriteCurrValues


  // CUSTOM COMMANDS FOR TIME SELECTION / CURSOR

  void MyCSurf_ToggleRepeat()
  {
    s_loop = (bool)GetSetRepeat(2);
  } // MyCSurf_ToggleRepeat


  void MyCSurf_MoveTimeSel(signed char start_dir, signed char end_dir, bool markers)
  {
    // markers: true = move to next marker, false = move 1 bar
    // start_dir: move start of time sel by dir
    // end_dir move end of time sel by dir

    // get time range
    ReaProject* rpr_pro = EnumProjects(-1, NULL, 0);
    double start_time, start_beats, end_time, end_beats;
    int start_num, start_denom, end_num, end_denom;

    // get time selection
    GetSet_LoopTimeRange(false, true, &start_time, &end_time, false); // ??

    if (markers)
    {
      // go through all markers
      int x = 0;
      bool stop = false;
      double prev_marker = 0.0;
      double marker;
      while ( (x = EnumProjectMarkers(x, NULL, &marker, NULL, NULL, NULL)) && !stop )
      {
        if ( ((start_dir < 0) && (marker >= start_time))
          || ((end_dir > 0) && (marker > end_time)) )
        {
          start_time = prev_marker;
          end_time = marker;  
          stop = true;
        }
        prev_marker = marker;
      }
      
      // set time selection
      if (stop) GetSet_LoopTimeRange(true, true, &start_time, &end_time, false);

    } else
    {
      // start / end: get time sig and position in beats
      TimeMap_GetTimeSigAtTime(rpr_pro, start_time, &start_num, &start_denom, NULL);
      start_beats = TimeMap2_timeToQN(rpr_pro, start_time);

      TimeMap_GetTimeSigAtTime(rpr_pro, end_time, &end_num, &end_denom, NULL);
      end_beats = TimeMap2_timeToQN(rpr_pro, end_time);

      // shift by bars
      start_beats += start_dir * (4 * start_num / start_denom);
      end_beats += end_dir * (4 * end_num / end_denom);

      // reconvert to seconds
      start_time = TimeMap2_QNToTime(rpr_pro, start_beats);
      end_time = TimeMap2_QNToTime(rpr_pro, end_beats);

      // snap to grid
      SnapToGrid(rpr_pro, start_time);
      SnapToGrid(rpr_pro, end_time);

      // set time selection
      GetSet_LoopTimeRange(true, true, &start_time, &end_time, false);
    }

  } // MyCSurf_MoveTimeSel


  void MyCSurf_Loop_ToggleAll()
  {
    if (!s_loop_all) {
      
      // save current sel
      GetSet_LoopTimeRange(false, true, &s_ts_start, &s_ts_end, false);
      
      // time sel all
      Main_OnCommand(CMD_SELALLITEMS, 0);
      Main_OnCommand(CMD_TIMESEL2ITEMS, 0);
      Main_OnCommand(CMD_UNSELALLITEMS, 0);  

      s_loop_all = true;
    
    } else {

      // restore time sel
      GetSet_LoopTimeRange(true, true, &s_ts_start, &s_ts_end, false);

      s_loop_all = false;
    }   
  } // MyCSurf_Loop_ToggleAll


  void MyCSurf_RemoveTimeSel()
  {
    Main_OnCommand(CMD_CLEARTIMESEL, 0);
  } // MyCSurf_RemoveTimeSel


  void MyCSurf_Scrub(char rel_value, bool fast)
  {
    double d_value;
    if (fast) d_value = (double)rel_value / (double)SCRUBRESFAST;
    else d_value = (double)rel_value / (double)SCRUBRESSLOW;
    
    CSurf_ScrubAmt(d_value);
  } // MyCSurf_Scrub
    

  void MyCSurf_MoveEditCursor(char rel_value, bool fast)
  {
    ReaProject* rpr_pro = EnumProjects(-1, NULL, 0);
    double old_val = GetCursorPosition();
    double d_value;
    if (fast) d_value = (double)rel_value / (double)MOVERESFAST;
    else d_value = (double)rel_value / (double)MOVERESSLOW;

    SetEditCurPos2(rpr_pro, old_val + d_value, true, false);
  } // MyCSurf_MoveEditCursor


  // CUSTOM COMMANDS FOR SCROLL AND ZOOM

  void MyCSurf_ToggleScrollOnPlay()
  {
    Main_OnCommand(CMD_TGLPLAYSCROLL, 0);
    Main_OnCommand(CMD_TGLRECSCROLL, 0);
  } // MyCSurf_ToggleScrollOnPlay


  void MyCSurf_CenterHScrollToPlay()
  {
    Main_OnCommand(CMD("_SWS_HSCROLLPLAY50"), 0);
  } // MyCSurf_CenterHScrollToPlay


  void MyCSurf_ToggleHZoomToTimeSel()
  {
    Main_OnCommand(CMD("_SWS_TOGZOOMTT"), 0);
  } // MyCSurf_ToggleHZoomToTimeSel


  void MyCSurf_VZoomToSelTracks()
  {
    Main_OnCommand(CMD("_SWS_VZOOMFIT"), 0);
  } // MyCSurf_VZoomToSelTracks


  // CUSTOM COMMANDS FOR FILE HANDLING

  void MyCSurf_Undo()
  {
    Main_OnCommand(CMD_UNDO, 0);
  } // MyCSurf_Undo


  void MyCSurf_Redo()
  {
    Main_OnCommand(CMD_REDO, 0);
  } // MyCSurf_Redo


  void MyCSurf_Save(bool overwrite)
  {
    if (overwrite) Main_OnCommand(CMD_SAVE, 0);
    else Main_OnCommand(CMD_SAVEAS, 0);
  } // MyCSurf_Save


  // WINDOW MESSAGES (EMULATE MOUSE AND KEYSTROKES FOR SOME ACTIONS)

  void MyCSurf_EmulateKeyStroke(int key_id)
  {
    HWND top = GetForegroundWindow();
    PostMessage(top, WM_KEYDOWN, key_id, 0);
    PostMessage(top, WM_KEYUP, key_id, 0);
  } // MyCSurf_EmulateKeyStroke



  ////// CONFIG STUFF FOR REGISTERING AND PREFERENCES //////

  const char *GetTypeString() 
  {
    return "US-2400";
  }
  

  const char *GetDescString()
  {
    descspace.Set("Tascam US-2400");
    char tmp[512];
    sprintf(tmp," (dev %d,%d)",m_midi_in_dev,m_midi_out_dev);
    descspace.Append(tmp);
    return descspace.Get();     
  }
  

  const char *GetConfigString() // string of configuration data
  {
    sprintf(configtmp,"0 0 %d %d",m_midi_in_dev,m_midi_out_dev);      
    return configtmp;
  }



  ////// CENTRAL CSURF MIDI EVENT LOOP //////

  void Run()
  {
    if ( (m_midiin) ) //&& (s_initdone) )
    {
      m_midiin->SwapBufs(timeGetTime());
      int l=0;
      MIDI_eventlist *list=m_midiin->GetReadBuf();
      MIDI_event_t *evts;
      while ((evts=list->EnumItems(&l))) MIDIin(evts);
    }

    // countdown enc touch delay
    for (char i = 0; i < 24; i++)
    {
      if (s_touch_enc[i] > 0) s_touch_enc[i]--;
    }

    // Execute fader/encoder updates
    char i = 0;
    char ex = 0;
    do 
    {
    
      if ((cache_upd_faders & (1 << cache_exec)) > 0) {
        ExecuteFaderUpdate(cache_exec);
        ex += 2;
      }
      if ((cache_upd_enc & (1 << cache_exec)) > 0) {
        ExecuteEncoderUpdate(cache_exec);
        ex++;
      }
      
      // incr / wrap cache_exec
      cache_exec++;
      if (cache_exec > 24) cache_exec = 0;

      i++;
      
      // repeat loop until all channels checked or exlimit reached
    } while ((i < 25) && (ex < EXLIMIT));


    // blink
    if (myblink_ctr > MYBLINKINTV)
    {
      s_myblink = !s_myblink;

      // reset counter, on is shorter
      if (s_myblink) myblink_ctr = MYBLINKINTV - MYBLINKRATIO;
      else myblink_ctr = 0;
      
      for (char ch = 0; ch < 24; ch++)
      {
        // update encoders (rec arm)
        MySetSurface_UpdateEncoder(ch);

        // update track buttons
        MySetSurface_UpdateTrackElement(ch);

      }

      // blink Master if anything selected
      MediaTrack* master = Cnv_ChannelIDToMediaTrack(24);
      bool on = ( ((CountSelectedTracks(0) > 0) && s_myblink) != IsTrackSelected(master) );
      MySetSurface_UpdateButton(97, on, false);

      // Update automation modes
      MySetSurface_UpdateAutoLEDs();

    } else
    {
      myblink_ctr++;
    }


    // update Display
    if (dsp_hwnd != NULL) 
    {
      dsp_touch = 0;
      for (char ch = 0; ch < 24; ch++)
        if (s_touch_enc[ch] > 0) dsp_touch = dsp_touch | (1 << ch);
      dsp_touch = dsp_touch | s_touch_fdr;

      if ((dsp_touch != dsp_touch_prev) || (dsp_repaint))
      {
        InvalidateRect(dsp_hwnd, NULL, true);
        UpdateWindow(dsp_hwnd);
      }
      dsp_touch_prev = dsp_touch;
      dsp_repaint = false;
    }

    // init
    if (!s_initdone) s_initdone = MySetSurface_Init();
  } // Run



}; // class CSurf_US2400



////// REGISTRATION, SETUP AND CONFIGURATION DIALOGS //////

static void parseParms(const char *str, int parms[4])
{
  parms[0]=0;
  parms[1]=9;
  parms[2]=parms[3]=-1;

  const char *p=str;
  if (p)
  {
    int x=0;
    while (x<4)
    {
      while (*p == ' ') p++;
      if ((*p < '0' || *p > '9') && *p != '-') break;
      parms[x++]=atoi(p);
      while (*p && *p != ' ') p++;
    }
  }  
}


static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
  int parms[4];
  parseParms(configString,parms);

  return new CSurf_US2400(parms[2],parms[3],errStats);
}


static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      {
        int parms[4];
        parseParms((const char *)lParam,parms);

        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT1),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT1_LBL),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT2),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT2_LBL),SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg,IDC_EDIT2_LBL2),SW_HIDE);

        int n=GetNumMIDIInputs();
        int x=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)"None");
        SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,x,-1);
        x=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)"None");
        SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,x,-1);
        for (x = 0; x < n; x ++)
        {
          char buf[512];
          if (GetMIDIInputName(x,buf,sizeof(buf)))
          {
            int a=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_ADDSTRING,0,(LPARAM)buf);
            SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETITEMDATA,a,x);
            if (x == parms[2]) SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_SETCURSEL,a,0);
          }
        }
        n=GetNumMIDIOutputs();
        for (x = 0; x < n; x ++)
        {
          char buf[512];
          if (GetMIDIOutputName(x,buf,sizeof(buf)))
          {
            int a=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_ADDSTRING,0,(LPARAM)buf);
            SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETITEMDATA,a,x);
            if (x == parms[3]) SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_SETCURSEL,a,0);
          }
        }
      }
    break;
    case WM_USER+1024:
      if (wParam > 1 && lParam)
      {
        char tmp[512];

        int indev=-1, outdev=-1, offs=0, size=9;
        int r=SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETCURSEL,0,0);
        if (r != CB_ERR) indev = SendDlgItemMessage(hwndDlg,IDC_COMBO2,CB_GETITEMDATA,r,0);
        r=SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETCURSEL,0,0);
        if (r != CB_ERR)  outdev = SendDlgItemMessage(hwndDlg,IDC_COMBO3,CB_GETITEMDATA,r,0);

        sprintf(tmp,"0 0 %d %d",indev,outdev);
        lstrcpyn((char *)lParam, tmp,wParam);       
      }
    break;
  }
  return 0;
}


static HWND configFunc(const char *type_string, HWND parent, const char *initConfigString)
{
  return CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SURFACEEDIT_MCU), parent, dlgProc, (LPARAM) initConfigString);
}


reaper_csurf_reg_t csurf_us2400_reg = 
{
  "US-2400",
  "Tascam US-2400",
  createFunc,
  configFunc,
};
