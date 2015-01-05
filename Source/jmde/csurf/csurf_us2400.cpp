/*
** reaper_csurf
** Tascam US-2400 support
** Cobbled together by David Lichtenberger
** No license, no guarantees.
*/


////// PREFERENCES //////

// Meter enabled
#define METERMODE true // false // 
#define DESCSTRING "Tascam US-2400 (with VU-Meters)" // "Tascam US-2400 (M-Key Support)" // 

// Meter: -inf = x dB
#define MINUSINF -90.0

// period in which M qualifier is active (in Run cycles)
#define MDELAY 50


// Encoder resolution for volume, range: 0 > 16256
#define ENCRESVOL 100
// Encoder resolution for pan, range:  -1 > +1
#define ENCRESPAN 0.01 
// Encoder resolution for fx param, in divisions of range (e.g. -1 > +1, reso 100 > stepsize 0.02)
#define ENCRESFX 300
#define ENCRESFXFINE 3000
#define ENCRESFXTOGGLE 1
// How long will encoders be considered touch, in run circles (15 Hz -> 3 circles = 200ms)
#define ENCTCHDLY 10

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

// For finding sends see MyCSurf_Aux_Send)
#define AUXSTRING "aux---%d"

// float to int macro
#define F2I(x) (int)((x) + 0.5)


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
#define CMD_TIMESEL2ITEMS 40290
#define CMD_CLEARTIMESEL 40635
#define CMD_TGGLRECBEAT 40045
#define CMD_AUTOTOSEL 41160
#define CMD_FXBROWSER 40271
#define CMD_SEL2LASTTOUCH 40914

#include "csurf.h"

// for debug  
char debug[64];

class CSurf_US2400;
static bool g_csurf_mcpmode = true; 


inline bool dblEq(double a, double b, double prec) {
if ( ((abs(a) - abs(b)) < prec) && ((abs(a) - abs(b)) > (0 - prec)) )
  {
    return true;
  } else
  {
    return false;
  }
}


// DISPLAY
bool stp_repaint = false;
int stp_width = -1;
int stp_height = -1;
int stp_x = -1;
int stp_y = -1;
int stp_open = 0;

WDL_String stp_strings[49];
int stp_colors[24];
unsigned long stp_enc_touch = 0;
unsigned long stp_enc_touch_prev = 0;
unsigned long stp_fdr_touch = 0;
unsigned long stp_fdr_touch_prev = 0;
unsigned long stp_sel = 0;
unsigned long stp_rec = 0;
unsigned long stp_mute = 0;
bool stp_chan = false;
bool stp_flip = false;

void Stp_Paint(HWND hwnd)
{
  RECT rect;
  GetWindowRect(hwnd, &rect);
  int win_width = rect.right - rect.left;
  int win_height = rect.bottom - rect.top;

  HDC hdc;
  PAINTSTRUCT ps;
  hdc = BeginPaint(hwnd, &ps);

  HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  HFONT rfont = (HFONT)SelectObject(hdc, hfont);
  
  COLORREF bg_col = RGB(60, 60, 60);
  COLORREF separator_col = RGB(90, 90, 90);
  
  HBRUSH separator_bg = CreateSolidBrush(separator_col);
  HBRUSH touch_bg = CreateSolidBrush(RGB(240, 120, 120));

  HPEN white_ln = CreatePen(PS_SOLID, 1, RGB(250, 250, 250));
  HPEN lgrey_ln = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
  HPEN grey_ln = CreatePen(PS_SOLID, 1, RGB(90, 90, 90));  

  double box_width = (win_width - 26.0) / 26.0;
  
  double touch_height = 3.0;
  double single_height = 16.0;
  double padding = 2.0;

  double x = 0;
  
  for (char ch = 0; ch < 24; ch++)
  {
    // draw separators
    if ((ch % 8 == 0) && (ch != 0))
    {
      rect.top = 0;
      rect.left = F2I(x);
      rect.right = F2I(x + box_width + 1);
      rect.bottom = F2I(win_height);
      FillRect(hdc, &rect, separator_bg);

      x += box_width;

      // draw info
      SetBkColor(hdc, separator_col);
      
      // draw flip
      if (stp_flip) {

        rect.top = F2I(padding);
        rect.bottom = F2I(single_height - padding);

        SetTextColor(hdc, RGB(240, 60, 60));
        DrawText(hdc, "F L I P", -1, &rect, DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS);
      }

      // draw fx name
      if (stp_chan)
      {
        rect.top = F2I(single_height + touch_height + padding);
        rect.bottom = F2I(win_height - padding);

        SetTextColor(hdc, RGB(150, 150, 150));
        DrawText(hdc, stp_strings[49].Get(), -1, &rect, DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS);
      }
    }

    // draw and set background if applicable
    if (stp_colors[ch] != 0) {

      int r = GetRValue(stp_colors[ch]) / 4 + GetRValue(bg_col) / 2;
      int g = GetGValue(stp_colors[ch]) / 4 + GetGValue(bg_col) / 2;
      int b = GetBValue(stp_colors[ch]) / 4 + GetBValue(bg_col) / 2;

      HBRUSH tkbg = CreateSolidBrush(RGB(r, g, b));

      rect.top = 0;
      rect.left = F2I(x);
      rect.right = F2I(x + box_width);
      rect.bottom = F2I(win_height);
      FillRect(hdc, &rect, tkbg);

      DeleteObject(tkbg);

      SetBkColor(hdc, RGB(r, g, b));
    
    } else SetBkColor(hdc, bg_col);

    rect.left = F2I(x + padding);
    rect.right = F2I(x + box_width - padding);

    // draw text A
    SetTextColor(hdc, RGB(150, 150, 150));
    if ( (!stp_chan && (stp_sel & (1 << ch))) || ((stp_chan) && (stp_enc_touch & (1 << ch))) )
      SetTextColor(hdc, RGB(250, 250, 250));
    else if ( !stp_chan && (stp_rec & (1 << ch)) )
      SetTextColor(hdc, RGB(250, 60, 60));

    rect.top = F2I(padding);
    rect.bottom = F2I(single_height - padding);
    DrawText(hdc, stp_strings[ch].Get(), -1, &rect, DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS);

    // draw touch
    rect.top = F2I(single_height);
    rect.bottom = F2I(single_height + touch_height);

    if ( ((stp_fdr_touch & (1 << ch)) != 0) || ((stp_enc_touch & (1 << ch)) != 0) )
    {
      FillRect(hdc, &rect, touch_bg);
    
    } else 
    {
      SelectObject(hdc, lgrey_ln);
      int y = F2I(rect.top + ((rect.bottom - rect.top) / 2));
      MoveToEx(hdc, rect.left, y, NULL);
      LineTo(hdc, rect.right, y);
    }

    // draw text B
    SetTextColor(hdc, RGB(250, 250, 250));
    if ( !stp_chan && (stp_mute & (1 << ch)) ) SetTextColor(hdc, RGB(150, 150, 150));

    rect.top = F2I(single_height + touch_height + padding);
    rect.bottom = F2I(win_height - padding);
    DrawText(hdc, stp_strings[ch + 24].Get(), -1, &rect, DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS);

    // dividers
    bool draw = false;
    if ((ch + 1) % 2 != 0)
    {
      draw = true;
      SelectObject(hdc, grey_ln);
    
    } else if ((ch + 1) % 4 != 0)
    {
      draw = true;
      SelectObject(hdc, lgrey_ln);
    
    } else if ((ch + 1) % 8 != 0)
    {
      draw = true;
      SelectObject(hdc, white_ln);
    }

    x += box_width;
    if (draw)
    {
      MoveToEx(hdc, F2I(x), 0, NULL);
      LineTo(hdc, F2I(x), F2I(win_height));
    }

    x += 1.0;
  }
 
  SelectObject(hdc, rfont);
  DeleteObject(hfont);
  DeleteObject(rfont);

  DeleteObject(separator_bg);
  DeleteObject(touch_bg);

  DeleteObject(white_ln);
  DeleteObject(lgrey_ln);
  DeleteObject(grey_ln);

  EndPaint(hwnd, &ps);
}

void Stp_StoreWinCoords(HWND hwnd)
{
  RECT rect;
  GetWindowRect(hwnd, &rect);
  stp_x = rect.left;
  stp_y = rect.top;
  stp_width = rect.right - rect.left;
  stp_height = rect.bottom - rect.top;
}


LRESULT CALLBACK Stp_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_PAINT:
      Stp_Paint(hwnd);
      break;

    case WM_SIZE:
      stp_repaint = true;
      Stp_StoreWinCoords(hwnd);
      break;

    case WM_MOVE:
      stp_repaint = true;
      Stp_StoreWinCoords(hwnd);
      break;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// ON SCREEN HELP

WDL_String hlp_enc_str[3][4][2];   // pan/chan/aux | _/sh/f/m | _/fl

WDL_String hlp_tksel_str[3][4];    // pan/chan/aux | _/sh/f/m
WDL_String hlp_tksolo_str[4];      // _/sh/f/m
WDL_String hlp_tkmute_str[4];      // _/sh/f/m
WDL_String hlp_tkfdr_str[3][4][2]; // pan/chan/aux | _/sh/f/m | _/fl

WDL_String hlp_mstsel_str[4];      // _/sh/f/m
WDL_String hlp_clsolo_str[4];      // _/sh/f/m
WDL_String hlp_mstfdr_str[4];      // _/sh/f/m

WDL_String hlp_chan_str[3];        // pan/chan/aux
WDL_String hlp_fkey_str[4];        // _/sh/f/m
WDL_String hlp_shift_str[4];       // _/sh/f/m

WDL_String hlp_keys_str[12][3][4]; // aux1-6/null/req/fwd/stop/play/rec | pan/chan/aux | _/sh/f/m
WDL_String hlp_bank_str[2][3][4];  // bank -/+ | pan/chan/aux | _/sh/f/m
WDL_String hlp_inout_str[2][4]; // in/out | _/sh/f/m

// flip / chan / pan / mkey are static

int hlp_mode = 0;
int hlp_qkey = 0;
bool hlp_flip = false;

int hlp_open = 0;

int hlp_margin = 15;
int hlp_grid = 90;
int hlp_box_size = 80;
int hlp_sep = 20;


void Hlp_DrawBox(const char* caption, int top, int left, COLORREF bg_col, COLORREF mode_col, COLORREF cap_col, WDL_String command, HDC* hdc_p)
{
  HPEN lgrey_ln = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
  RECT rect;

  COLORREF text_col = RGB(255, 255, 255);
  SetTextColor(*hdc_p, text_col);

  int mode_border = 4;
  int text_padding = 4;

  if (mode_col != 0)
  {
    HBRUSH mode_bg = CreateSolidBrush(mode_col);  
    rect.top = top - mode_border;
    rect.left = left - mode_border;
    rect.bottom = top + hlp_box_size + mode_border;
    rect.right = left + hlp_box_size + mode_border;
    FillRect(*hdc_p, &rect, mode_bg);
    DeleteObject(mode_bg);
  }

  HBRUSH bg = CreateSolidBrush(bg_col);
  rect.top = top;
  rect.left = left;
  rect.bottom = top + hlp_box_size;
  rect.right = left + hlp_box_size;
  FillRect(*hdc_p, &rect, bg);
  DeleteObject(bg);
    
  HBRUSH cap_bg = CreateSolidBrush(cap_col);
  rect.top = top;
  rect.left = left;
  rect.bottom = top + 20;
  rect.right = left + hlp_box_size;
  FillRect(*hdc_p, &rect, cap_bg);
  DeleteObject(cap_bg);

  SetBkColor(*hdc_p, cap_col);
  rect.top    = top + text_padding;
  rect.left   = left + text_padding;
  rect.bottom = top + 20 - text_padding;
  rect.right  = left + hlp_box_size - text_padding;
  DrawText(*hdc_p, caption, -1, &rect, DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS);
  
  SelectObject(*hdc_p, lgrey_ln);
  MoveToEx(*hdc_p, rect.left, top + 20, NULL);
  LineTo(*hdc_p, rect.right, top + 20);

  SetBkColor(*hdc_p, bg_col);
  rect.top    = top + 20 + text_padding;
  rect.left   = left + text_padding;
  rect.bottom = top + hlp_box_size - text_padding;
  rect.right  = left + hlp_box_size - text_padding;
  DrawText(*hdc_p, command.Get(), -1, &rect, DT_CENTER | DT_WORDBREAK | DT_WORD_ELLIPSIS | DT_END_ELLIPSIS);

  DeleteObject(lgrey_ln);
}

void Hlp_Paint(HWND hwnd)
{
  RECT rect;

  HDC hdc;
  PAINTSTRUCT ps;
  hdc = BeginPaint(hwnd, &ps);

  HFONT hfont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
  HFONT rfont = (HFONT)SelectObject(hdc, hfont);

  COLORREF bg_col = RGB(50, 50, 50);
  COLORREF shift_col = RGB(50, 50, 100);
  COLORREF fkey_col = RGB(100, 50, 50);
  COLORREF mkey_col = RGB(50, 100, 50);
  COLORREF pan_col = RGB(50, 100, 100);
  COLORREF chan_col = RGB(100, 100, 50);
  COLORREF aux_col = RGB(100, 50, 100);
  COLORREF flip_col = RGB(100, 100, 100);

  COLORREF flipbtn_col = flip_col;
  int flip_i = 1;
  if (!hlp_flip)
  {
    flip_col = bg_col;
    flip_i = 0;
  }

  COLORREF mode_col = pan_col;
  if (hlp_mode == 1) mode_col = chan_col;
  else if (hlp_mode == 2) mode_col = aux_col;

  COLORREF auxbtn_col = aux_col;
  COLORREF qkey_col = bg_col;
  if ((hlp_qkey > 0) || (hlp_mode == 1))
  {
    auxbtn_col = bg_col;
    if (hlp_qkey == 1) qkey_col = shift_col;
    else if (hlp_qkey == 2) qkey_col = fkey_col;
    else if (hlp_qkey == 3) qkey_col = mkey_col;
  }

  COLORREF text_col = RGB(255, 255, 255);
  SetTextColor(hdc, text_col);

  HBRUSH standard_bg = CreateSolidBrush(bg_col);
  HBRUSH mode_bg = CreateSolidBrush(mode_col);
  HBRUSH qkey_bg = CreateSolidBrush(qkey_col);
  
  // global output
  rect.top    = hlp_margin + 0*hlp_sep + 0*hlp_grid;  
  rect.left   = hlp_margin + 2*hlp_sep + 4*hlp_grid;
  rect.bottom = rect.top + 50;
  rect.right  = hlp_margin + 2*hlp_sep + 7*hlp_grid + hlp_box_size;  
  FillRect(hdc, &rect, standard_bg);

  
  SetBkColor(hdc, bg_col);
  rect.top    += 7;
  rect.left   += 7;
  DrawText(hdc, "US-2400 On-Screen Help - F-Key + Shift to close", -1, &rect, 0);

  rect.top    = hlp_margin + 0*hlp_sep + 0*hlp_grid + 25;  
  rect.left   = hlp_margin + 2*hlp_sep + 4*hlp_grid;
  rect.right  = hlp_margin + 2*hlp_sep + 6*hlp_grid;  
  FillRect(hdc, &rect, mode_bg);
  
  SetBkColor(hdc, mode_col);
  rect.top    += 7;
  rect.left   += 7;
  if (hlp_mode == 0) DrawText(hdc, "Mode: Normal (Pan)", -1, &rect, 0);
  else if (hlp_mode == 1) DrawText(hdc, "Mode: Channel Strip", -1, &rect, 0);
  else if (hlp_mode == 2) DrawText(hdc, "Mode: Aux Sends", -1, &rect, 0);

  rect.top    = hlp_margin + 0*hlp_sep + 0*hlp_grid + 25;  
  rect.left   = hlp_margin + 2*hlp_sep + 6*hlp_grid;
  rect.right  = hlp_margin + 2*hlp_sep + 7*hlp_grid + hlp_box_size;  
  FillRect(hdc, &rect, qkey_bg);

  rect.top  += 7;
  rect.left += 7;
  SetBkColor(hdc, qkey_col);
  if (hlp_qkey == 0) DrawText(hdc, "Qualifier Key: None", -1, &rect, 0);
  else if (hlp_qkey == 1) DrawText(hdc, "Qualifier Key: Shift", -1, &rect, 0);
  else if (hlp_qkey == 2) DrawText(hdc, "Qualifier Key: F-Key", -1, &rect, 0);
  else if (hlp_qkey == 3) DrawText(hdc, "Qualifier Key: M-Key", -1, &rect, 0);

  // draw fader tracks
  rect.top    = hlp_margin + 1*hlp_sep + 4*hlp_grid;
  rect.left   = hlp_margin + 0*hlp_sep + 0*hlp_grid + hlp_box_size / 2 - 2;
  rect.bottom = hlp_margin + 2*hlp_sep + 6*hlp_grid + hlp_box_size;
  rect.right  = hlp_margin + 0*hlp_sep + 0*hlp_grid + hlp_box_size / 2 + 2;
  FillRect(hdc, &rect, standard_bg);

  rect.left   = hlp_margin + 1*hlp_sep + 1*hlp_grid + hlp_box_size / 2 - 2;
  rect.right  = hlp_margin + 1*hlp_sep + 1*hlp_grid + hlp_box_size / 2 + 2;
  FillRect(hdc, &rect, standard_bg);


  // draw boxes
  Hlp_DrawBox("Encoders",       hlp_margin + 0*hlp_sep + 0*hlp_grid, hlp_margin + 0*hlp_sep + 0*hlp_grid, flip_col, mode_col, qkey_col, hlp_enc_str[hlp_mode][hlp_qkey][flip_i], &hdc);

  Hlp_DrawBox("Tracks: Select", hlp_margin + 1*hlp_sep + 1*hlp_grid, hlp_margin + 0*hlp_sep + 0*hlp_grid, bg_col, mode_col, qkey_col, hlp_tksel_str[hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("Tracks: Solo",   hlp_margin + 1*hlp_sep + 2*hlp_grid, hlp_margin + 0*hlp_sep + 0*hlp_grid, bg_col, bg_col, qkey_col, hlp_tksolo_str[hlp_qkey], &hdc);
  Hlp_DrawBox("Tracks: Mute",   hlp_margin + 1*hlp_sep + 3*hlp_grid, hlp_margin + 0*hlp_sep + 0*hlp_grid, bg_col, bg_col, qkey_col, hlp_tkmute_str[hlp_qkey], &hdc);
  Hlp_DrawBox("Track Faders",   hlp_margin + 2*hlp_sep + 4*hlp_grid, hlp_margin + 0*hlp_sep + 0*hlp_grid, flip_col, mode_col, qkey_col, hlp_tkfdr_str[hlp_mode][hlp_qkey][flip_i], &hdc);

  Hlp_DrawBox("Master Select",  hlp_margin + 1*hlp_sep + 1*hlp_grid, hlp_margin + 1*hlp_sep + 1*hlp_grid, bg_col, bg_col, qkey_col, hlp_mstsel_str[hlp_qkey], &hdc);
  Hlp_DrawBox("Clear Solo",     hlp_margin + 1*hlp_sep + 2*hlp_grid, hlp_margin + 1*hlp_sep + 1*hlp_grid, bg_col, bg_col, qkey_col, hlp_clsolo_str[hlp_qkey], &hdc);
  Hlp_DrawBox("Flip",           hlp_margin + 1*hlp_sep + 3*hlp_grid, hlp_margin + 1*hlp_sep + 1*hlp_grid, flipbtn_col, bg_col, flipbtn_col, WDL_String("Enter Flip Mode"), &hdc);
  Hlp_DrawBox("Master Fader",   hlp_margin + 2*hlp_sep + 4*hlp_grid, hlp_margin + 1*hlp_sep + 1*hlp_grid, bg_col, bg_col, qkey_col, hlp_mstfdr_str[hlp_qkey], &hdc);

  Hlp_DrawBox("Chan",           hlp_margin + 1*hlp_sep + 1*hlp_grid, hlp_margin + 2*hlp_sep + 2*hlp_grid, chan_col, mode_col, chan_col, hlp_chan_str[hlp_mode], &hdc);
  Hlp_DrawBox("Pan",            hlp_margin + 1*hlp_sep + 2*hlp_grid, hlp_margin + 2*hlp_sep + 2*hlp_grid, pan_col, mode_col, pan_col, WDL_String("Enter Normal (Pan) Mode"), &hdc);

  Hlp_DrawBox("1",              hlp_margin + 1*hlp_sep + 1*hlp_grid, hlp_margin + 2*hlp_sep + 3*hlp_grid, auxbtn_col, mode_col, qkey_col, hlp_keys_str[0][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("2",              hlp_margin + 1*hlp_sep + 1*hlp_grid, hlp_margin + 2*hlp_sep + 4*hlp_grid, auxbtn_col, mode_col, qkey_col, hlp_keys_str[1][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("3",              hlp_margin + 1*hlp_sep + 1*hlp_grid, hlp_margin + 2*hlp_sep + 5*hlp_grid, auxbtn_col, mode_col, qkey_col, hlp_keys_str[2][hlp_mode][hlp_qkey], &hdc);

  Hlp_DrawBox("4",              hlp_margin + 1*hlp_sep + 2*hlp_grid, hlp_margin + 2*hlp_sep + 3*hlp_grid, auxbtn_col, mode_col, qkey_col, hlp_keys_str[3][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("5",              hlp_margin + 1*hlp_sep + 2*hlp_grid, hlp_margin + 2*hlp_sep + 4*hlp_grid, auxbtn_col, mode_col, qkey_col, hlp_keys_str[4][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("6",              hlp_margin + 1*hlp_sep + 2*hlp_grid, hlp_margin + 2*hlp_sep + 5*hlp_grid, auxbtn_col, mode_col, qkey_col, hlp_keys_str[5][hlp_mode][hlp_qkey], &hdc);

  Hlp_DrawBox("Null",           hlp_margin + 1*hlp_sep + 3*hlp_grid, hlp_margin + 2*hlp_sep + 6*hlp_grid, bg_col, mode_col, qkey_col, hlp_keys_str[6][hlp_mode][hlp_qkey], &hdc);

  if (!METERMODE) 
    Hlp_DrawBox("(Meter)",      hlp_margin + 1*hlp_sep + 1*hlp_grid, hlp_margin + 2*hlp_sep + 7*hlp_grid, mkey_col, bg_col, qkey_col, WDL_String("M-Key"), &hdc);
  
  Hlp_DrawBox("F-Key",          hlp_margin + 1*hlp_sep + 2*hlp_grid, hlp_margin + 2*hlp_sep + 7*hlp_grid, fkey_col, bg_col, qkey_col, hlp_fkey_str[hlp_qkey], &hdc);

  Hlp_DrawBox("Bank -",         hlp_margin + 2*hlp_sep + 5*hlp_grid, hlp_margin + 2*hlp_sep + 2*hlp_grid, bg_col, mode_col, qkey_col, hlp_bank_str[0][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("Bank +",         hlp_margin + 2*hlp_sep + 5*hlp_grid, hlp_margin + 2*hlp_sep + 3*hlp_grid, bg_col, mode_col, qkey_col, hlp_bank_str[1][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("In",             hlp_margin + 2*hlp_sep + 5*hlp_grid, hlp_margin + 2*hlp_sep + 4*hlp_grid, bg_col, bg_col, qkey_col, hlp_inout_str[0][hlp_qkey], &hdc);
  Hlp_DrawBox("Out",            hlp_margin + 2*hlp_sep + 5*hlp_grid, hlp_margin + 2*hlp_sep + 5*hlp_grid, bg_col, bg_col, qkey_col, hlp_inout_str[1][hlp_qkey], &hdc);
  Hlp_DrawBox("Shift",          hlp_margin + 2*hlp_sep + 5*hlp_grid, hlp_margin + 2*hlp_sep + 7*hlp_grid, shift_col, bg_col, qkey_col, hlp_shift_str[hlp_qkey], &hdc);

  Hlp_DrawBox("Rewind",         hlp_margin + 2*hlp_sep + 6*hlp_grid, hlp_margin + 2*hlp_sep + 2*hlp_grid, bg_col, mode_col, qkey_col, hlp_keys_str[7][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("Fast Forward",   hlp_margin + 2*hlp_sep + 6*hlp_grid, hlp_margin + 2*hlp_sep + 3*hlp_grid, bg_col, mode_col, qkey_col, hlp_keys_str[8][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("Stop",           hlp_margin + 2*hlp_sep + 6*hlp_grid, hlp_margin + 2*hlp_sep + 4*hlp_grid, bg_col, mode_col, qkey_col, hlp_keys_str[9][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("Play",           hlp_margin + 2*hlp_sep + 6*hlp_grid, hlp_margin + 2*hlp_sep + 5*hlp_grid, bg_col, mode_col, qkey_col, hlp_keys_str[10][hlp_mode][hlp_qkey], &hdc);
  Hlp_DrawBox("Record",         hlp_margin + 2*hlp_sep + 6*hlp_grid, hlp_margin + 2*hlp_sep + 7*hlp_grid, bg_col, mode_col, qkey_col, hlp_keys_str[11][hlp_mode][hlp_qkey], &hdc);

  SelectObject(hdc, rfont);
  DeleteObject(hfont);
  DeleteObject(rfont);

  DeleteObject(standard_bg);
  DeleteObject(mode_bg);
  DeleteObject(qkey_bg);
}


LRESULT CALLBACK Hlp_WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_PAINT:
      Hlp_Paint(hwnd);
      break;
  }

  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// CSURF CLASS

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
  int cmd_ids[4][3][12]; //[none/shift/fkey/mkey][pan/chan/aux][1-6/null/rew/ffwd/stop/play/rec]

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
  int cache_meters[24];
  unsigned long cache_upd_faders;
  unsigned long cache_upd_enc;
  unsigned long cache_upd_meters;
  char cache_exec;
  bool master_sel;

  // general states
  int s_ch_offset; // bank up/down
  bool s_play, s_rec, s_loop; // play states
  char s_automode; // automation modes

  // modes
  bool m_flip, m_chan, m_pan, m_scrub;
  char m_aux;

  // qualifier keys
  bool q_fkey, q_shift, q_mkey;
  int s_mkey_on;

  // for channel strip
  MediaTrack* chan_rpr_tk;
  char chan_ch;
  int chan_fx;
  int chan_fx_count;
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
  HWND stp_hwnd;
  WNDCLASSEX stp_class;

  // on screen help
  HWND hlp_hwnd;
  WNDCLASSEX hlp_class;



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
          case 0x6b : OnMeter(); break;
          case 0x6c : if (btn_state) OnPan(); break;
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
    MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
    if (ch_id < 24)
    {
      if (q_fkey)
      {
        if (m_aux > 0) MyCSurf_AddSwitchAuxSend(rpr_tk, m_aux);
        else MyCSurf_SwitchPhase(rpr_tk);

      } else if (q_shift)
      {
        if (m_aux > 0) MyCSurf_RemoveAuxSend(rpr_tk, m_aux);
        else CSurf_OnRecArmChange(rpr_tk, -1);

      } else if (m_chan)
      {
        MySetSurface_Chan_SelectTrack(ch_id, false);

      } else CSurf_OnSelectedChange(rpr_tk, -1);
    
    } else if (ch_id == 24) OnMasterSel();
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
      s_touch_fdr = s_touch_fdr & (~(1 << ch_id));
    }
    
    Stp_Update(ch_id);
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

    Stp_Update(ch_id);
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

          double min, max, step, fine, coarse; 
          bool toggle;
          bool has_steps = false;
          d_value = TrackFX_GetParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &min, &max);
          
          // most of the time this fails because not implemented by plugins!
          if ( ( TrackFX_GetParameterStepSizes(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &step, &fine, &coarse, &toggle) ) )
          {
            if (toggle)
            {
              has_steps = true;
              
              if (rel_value > 0) d_value = 1.0;
              else d_value = 0.0;
            
            } else if (step != 0)
            {
              has_steps = true;
            
              if (q_fkey)
              {
                if (fine != 0) step = fine;
                else step = step / 10.0;

              } else if (q_shift)
              {
                if (coarse != 0) step = coarse;
                else step = step * 10.0;
              }
            }
          } 
          
          // this is the workaround
          if (!has_steps)
          {
            step = 1/(double)ENCRESFX;
            if (q_fkey) step = 1/(double)ENCRESFXFINE;
            else if (q_shift) step = 1/(double)ENCRESFXTOGGLE;
          }

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
      Stp_Update(ch_id);

      s_touch_enc[ch_id] = ENCTCHDLY;

    } // if ( (exists) && (isactive) )
  } // OnEncoderChange


  // MASTER TRACK

  void OnMasterSel()
  {
    if (q_fkey)
    {
      MyCSurf_SelectMaster();

    } else
    {
      if (m_chan) MySetSurface_Chan_SelectTrack(24, false);
      else MyCSurf_ToggleSelectAllTracks();
    }
  } // OnMasterSel()


  void OnClrSolo(bool btn_state)
  {
    if (btn_state)
      if (q_fkey) MyCSurf_UnmuteAllTracks();
      else if (q_shift) MyCSurf_ToggleMute(Cnv_ChannelIDToMediaTrack(24), false);
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
    char mode = 0;
    if (m_chan) mode = 1;
    else if (m_aux > 0) mode = 2;
    
    char qkey = 0;
    if (q_shift) qkey = 1;
    else if (q_fkey) qkey = 2;
    else if (q_mkey) qkey = 3;

    if (qkey == 0)
    {
      if (m_chan)
      {
        switch(sel)
        {
          case 1 : MySetSurface_Chan_SetFxParamOffset(1); break;
          case 2 : MySetSurface_Chan_SetFxParamOffset(-1); break;
          case 3 : MyCSurf_Chan_ToggleFXBypass(); break;
          case 4 : MyCSurf_Chan_InsertFX(); break;
          case 5 : MyCSurf_Chan_DeleteFX(); break;
          case 6 : MyCSurf_Chan_ToggleArmFXEnv(); break;
        } 

      } else MySetSurface_EnterAuxMode(sel);  
    
    } else Main_OnCommand(cmd_ids[qkey][mode][sel-1], 0);

    MySetSurface_UpdateAuxButtons();
  } // OnAux()


  void OnMeter()
  {
    // reset holds
    if (METERMODE) MySetSurface_OutputMeters(true);
    // reset button
    else {
      q_mkey = !q_mkey;
      if (q_mkey) 
      {
        s_mkey_on = MDELAY;
        hlp_qkey = 3;
      } else hlp_qkey = 0;

      Hlp_Update();

      MySetSurface_UpdateButton(0x6b, q_mkey, false);
    }
  } // OnMeter


  // QUALIFIERS

  void OnFKey(bool btn_state)
  {
    if ((btn_state && q_shift) && (s_initdone))
    {
      if (stp_hwnd == NULL) Stp_OpenWindow();
      else Stp_CloseWindow();
    } 
    MySetSurface_ToggleFKey(btn_state);
  } // OnFKey()


  void OnShift(bool btn_state)
  {
    if ((btn_state && q_fkey) && (s_initdone)) Hlp_ToggleWindow();
    MySetSurface_ToggleShift(btn_state);
  } // OnShift()


  // TRANSPORT

  void OnRew()
  {
    char mode = 0;
    if (m_chan) mode = 1;
    else if (m_aux > 0) mode = 2;
    
    char qkey = 0;
    if (q_shift) qkey = 1;
    else if (q_fkey) qkey = 2;
    else if (q_mkey) qkey = 3;

    if ((qkey == 0) && (cmd_ids[0][mode][7] == -1)) CSurf_OnRew(1);
    else if ((qkey == 1) && (cmd_ids[1][mode][10] == -1)) MyCSurf_Auto_SetMode(0);
    else Main_OnCommand(cmd_ids[qkey][mode][7], 0);
  } // OnRew()


  void OnFwd()
  {
    char mode = 0;
    if (m_chan) mode = 1;
    else if (m_aux > 0) mode = 2;
    
    char qkey = 0;
    if (q_shift) qkey = 1;
    else if (q_fkey) qkey = 2;
    else if (q_mkey) qkey = 3;

    if ((qkey == 0) && (cmd_ids[0][mode][8] == -1)) CSurf_OnFwd(1);
    else if ((qkey == 1) && (cmd_ids[1][mode][10] == -1)) MyCSurf_Auto_SetMode(1);
    else Main_OnCommand(cmd_ids[qkey][mode][8], 0);
  } // OnFwd()


  void OnStop()
  {
    char mode = 0;
    if (m_chan) mode = 1;
    else if (m_aux > 0) mode = 2;
    
    char qkey = 0;
    if (q_shift) qkey = 1;
    else if (q_fkey) qkey = 2;
    else if (q_mkey) qkey = 3;

    if ((qkey == 0) && (cmd_ids[0][mode][9] == -1)) CSurf_OnStop();
    else if ((qkey == 1) && (cmd_ids[1][mode][10] == -1)) MyCSurf_Auto_SetMode(2);
    else Main_OnCommand(cmd_ids[qkey][mode][9], 0);
  } // OnStop()


  void OnPlay()
  {
    char mode = 0;
    if (m_chan) mode = 1;
    else if (m_aux > 0) mode = 2;
    
    char qkey = 0;
    if (q_shift) qkey = 1;
    else if (q_fkey) qkey = 2;
    else if (q_mkey) qkey = 3;

    if ((qkey == 0) && (cmd_ids[0][mode][10] == -1)) CSurf_OnPlay(); 
    else if ((qkey == 1) && (cmd_ids[1][mode][10] == -1)) MyCSurf_Auto_SetMode(3);
    else Main_OnCommand(cmd_ids[qkey][mode][10], 0);
  } // OnPlay()


  void OnRec()
  {
    char mode = 0;
    if (m_chan) mode = 1;
    else if (m_aux > 0) mode = 2;
    
    char qkey = 0;
    if (q_shift) qkey = 1;
    else if (q_fkey) qkey = 2;
    else if (q_mkey) qkey = 3;

    if ((qkey == 0) && (cmd_ids[0][mode][11] == -1)) MyCSurf_OnRec();
    else Main_OnCommand(cmd_ids[qkey][mode][11], 0);
  } // OnRec()


  // OTHER KEYS

  void OnNull(bool btn_state)
  {
    if (btn_state) 
    {
      char mode = 0;
      if (m_chan) mode = 1;
      else if (m_aux > 0) mode = 2;
      
      char qkey = 0;
      if (q_shift) qkey = 1;
      else if (q_fkey) qkey = 2;
      else if (q_mkey) qkey = 3;

      Main_OnCommand(cmd_ids[qkey][mode][6], 0);
    }

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
        else if (q_shift) MySetSurface_ShiftBanks(dir, 24);
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
    if (s_loop_all) btn_state = true;
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

  void Stp_OpenWindow()
  { 
    if (stp_hwnd == NULL)
    {
      if (stp_width == -1 || stp_height == -1)
      {
        RECT scr;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &scr, 0);
        stp_width = scr.right - scr.left;
        stp_height = 80;
        stp_x = 0;
        stp_y = scr.bottom - stp_height;
      }
             
      stp_hwnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE, "stp", "US-2400 Display", WS_THICKFRAME | WS_POPUP, stp_x, stp_y, stp_width, stp_height, NULL, NULL, g_hInst, NULL);
    }

    for (int ch = 0; ch < 24; ch++) Stp_Update(ch);

    ShowWindow(stp_hwnd, SW_SHOW);
    UpdateWindow(stp_hwnd);

    stp_open = 1;
  } // Stp_OpenWindow


  void Stp_CloseWindow()
  {
    if (stp_hwnd != NULL)
    {
      DestroyWindow(stp_hwnd);
      stp_hwnd = NULL;
    }

    stp_open = 0;
  } // Stp_CloseWindow


  void Stp_Update(int ch)
  {
    if ((ch >= 0) && (ch < 24))
    {
      MediaTrack* tk;
      int tk_num, fx_amount;
      char buffer[64];
      WDL_String tk_name;
      WDL_String tk_num_c;
      WDL_String par_name;
      WDL_String par_val;    
      bool sel = false;
      bool rec = false;
      bool muted = false;

      // get info

      tk = Cnv_ChannelIDToMediaTrack(ch);
      if (tk != NULL) 
      {
        // track number
        tk_num = (int)GetMediaTrackInfo_Value(tk, "IP_TRACKNUMBER");
        sprintf(buffer, "%d", tk_num);
        tk_num_c = WDL_String(buffer);

        // track name
        GetSetMediaTrackInfo_String(tk, "P_NAME", buffer, false);
        buffer[64] = '\0';
        tk_name = WDL_String(buffer);
      
        // track mute, selected, rec arm
        int flags;
        GetTrackState(tk, &flags);

        // muted
        if ( (bool)(flags & 8) ) stp_mute = stp_mute | (1 << ch);
        else stp_mute = stp_mute & (~(1 << ch));

        // selected
        if ( (bool)(flags & 2) ) stp_sel = stp_sel | (1 << ch);
        else stp_sel = stp_sel & (~(1 << ch));

        // armed
        if ( (bool)(flags & 64) ) stp_rec = stp_rec | (1 << ch);
        else stp_rec = stp_rec & (~(1 << ch));

      } else
      {
        tk_num_c = "";
        tk_name = "";
        stp_colors[ch] = 0;
        stp_mute = stp_mute & (~(1 << ch));
        stp_sel = stp_sel & (~(1 << ch));
        stp_rec = stp_rec & (~(1 << ch));
      }


      fx_amount = TrackFX_GetNumParams(chan_rpr_tk, chan_fx);
      if (ch + chan_par_offs < fx_amount)
      {
        // fx param value
        TrackFX_GetFormattedParamValue(chan_rpr_tk, chan_fx, ch + chan_par_offs, buffer, 64);
        if (strlen(buffer) == 0)
        {
          double min, max;
          double par = TrackFX_GetParam(chan_rpr_tk, chan_fx, ch + chan_par_offs, &min, &max);
          sprintf(buffer, "%.4f", par);
        }
        par_val = WDL_String(buffer);

        // fx param name
        TrackFX_GetParamName(chan_rpr_tk, chan_fx, ch + chan_par_offs, buffer, 64);
        par_name = WDL_String(buffer);

      } else
      {
        par_val = "";
        par_name = "";
      }   

      // transmit

      stp_chan = false;
      stp_colors[ch] = GetTrackColor(tk);
      stp_strings[ch + 24] = tk_name;
      stp_strings[ch] = tk_num_c;
      stp_flip = m_flip;

      if (m_chan)
      {
        stp_chan = true;
        stp_strings[ch] = WDL_String(par_val);

        if ( ( !m_flip && ((s_touch_fdr & (1 << ch)) == 0) ) || ( m_flip && (s_touch_enc[ch] == 0) ) )
        {
          stp_colors[ch] = 0;
          stp_strings[ch + 24] = par_name;
        }

        TrackFX_GetFXName(chan_rpr_tk, chan_fx, buffer, 64);
        stp_strings[49] = WDL_String(buffer);
        stp_strings[49] = Utl_Alphanumeric(stp_strings[49]);
      }

      // keep only alphanumeric, replace everything else with space
      stp_strings[ch + 24] = Utl_Alphanumeric(stp_strings[ch + 24]);

      stp_repaint = true;
    }
  } // Stp_Update


  void Stp_RetrieveCoords()
  {
    if (HasExtState("US2400", "stp_x")) stp_x = atoi(GetExtState("US2400", "stp_x"));
    else stp_x = -1;

    if (HasExtState("US2400", "stp_y")) stp_y = atoi(GetExtState("US2400", "stp_y"));
    else stp_y = -1;

    if (HasExtState("US2400", "stp_height")) stp_height = atoi(GetExtState("US2400", "stp_height"));
    else stp_height = -1;

    if (HasExtState("US2400", "stp_width")) stp_width = atoi(GetExtState("US2400", "stp_width"));
    else stp_width = -1;    

    if (HasExtState("US2400", "stp_open")) stp_open = atoi(GetExtState("US2400", "stp_open"));
    else stp_open = 0;
  } // Stp_RetrieveCoords


  void Stp_SaveCoords()
  {
    char buffer[32];
    
    sprintf(buffer, "%d", stp_x);
    SetExtState("US2400", "stp_x", buffer, true);

    sprintf(buffer, "%d", stp_y);
    SetExtState("US2400", "stp_y", buffer, true);

    sprintf(buffer, "%d", stp_width);
    SetExtState("US2400", "stp_width", buffer, true);

    sprintf(buffer, "%d", stp_height);
    SetExtState("US2400", "stp_height", buffer, true);

    sprintf(buffer, "%d", stp_open);
    SetExtState("US2400", "stp_open", buffer, true);
  } // Stp_SaveCoords



  ////// ONSCREEN HELP //////  

  void Hlp_ToggleWindow()
  { 
    if (hlp_open == 0)
    {
      if (hlp_hwnd == NULL)
      {
        RECT scr;
        SystemParametersInfo(SPI_GETWORKAREA, 0, &scr, 0);
        int width = 2*hlp_margin + 2*hlp_sep + 7*hlp_grid + hlp_box_size;
        int height = 2*hlp_margin + 2*hlp_sep + 6*hlp_grid + hlp_box_size;
        int left = scr.right / 2 - width / 2;
        int top = scr.bottom / 2 - height / 2;

              
        hlp_hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE, "hlp", "US-2400 On-Screen Help", WS_POPUP | WS_BORDER, left, top, width, height, NULL, NULL, g_hInst, NULL);
       
        SetLayeredWindowAttributes(hlp_hwnd, NULL, 220, LWA_ALPHA);
      }

      ShowWindow(hlp_hwnd, SW_SHOW);
      UpdateLayeredWindow(hlp_hwnd, NULL, NULL, NULL, NULL, NULL, RGB(0, 0, 0), NULL, ULW_COLORKEY);

      hlp_open = 1;
    } else {
      if (hlp_hwnd != NULL)
      {
        DestroyWindow(hlp_hwnd);
        hlp_hwnd = NULL;
      }

      hlp_open = 0; 
    }
  } // Hlp_ToggleWindow


  void Hlp_Update()
  {
    if (hlp_hwnd != NULL)
    {
      InvalidateRect(hlp_hwnd, NULL, true);
      UpdateWindow(hlp_hwnd);
    }
  }


  void Hlp_FillStrs()
  {
    hlp_enc_str[0][0][0] = WDL_String("Track Pan");   // pan | _ | _
    hlp_enc_str[0][0][1] = WDL_String("Track Volume");   // pan | _ | fl
    hlp_enc_str[0][1][0] = WDL_String("Track Pan -> C");   // pan | sh | _
    hlp_enc_str[0][1][1] = WDL_String("Track Volume -> 0 dB");   // pan | sh | fl
    hlp_enc_str[0][2][0] = WDL_String("Track Stereo Width");   // pan | f | _
    hlp_enc_str[0][2][1] = WDL_String("Track Volume -> -inf dB");   // pan | f | fl
    hlp_enc_str[0][3][0] = WDL_String("Track Pan");   // pan | m | _
    hlp_enc_str[0][3][1] = WDL_String("Track Volume");   // pan | m | fl
    hlp_enc_str[1][0][0] = WDL_String("FX Parameter");   // chan | _ | _
    hlp_enc_str[1][0][1] = WDL_String("Track Volume");   // chan | _ | fl
    hlp_enc_str[1][1][0] = WDL_String("FX Parameter (Toggle)");   // chan | sh | _
    hlp_enc_str[1][1][1] = WDL_String("Track Volume -> 0 dB");   // chan | sh | fl
    hlp_enc_str[1][2][0] = WDL_String("FX Parameter (Fine)");   // chan | f | _
    hlp_enc_str[1][2][1] = WDL_String("Track Volume -> -inf dB");   // chan | f | fl
    hlp_enc_str[1][3][0] = WDL_String("FX Parameter");   // chan | m | _
    hlp_enc_str[1][3][1] = WDL_String("Track Volume");   // chan | m | fl
    hlp_enc_str[2][0][0] = WDL_String("Aux Send Level");   // aux | _ | _
    hlp_enc_str[2][0][1] = WDL_String("Aux Send Pan");   // aux | _ | fl
    hlp_enc_str[2][1][0] = WDL_String("Aux Send Level -> 0 dB");   // aux | sh | _
    hlp_enc_str[2][1][1] = WDL_String("Aux Send Pan -> C");   // aux | sh | fl
    hlp_enc_str[2][2][0] = WDL_String("Aux Send Level -> -inf dB");   // aux | f | _
    hlp_enc_str[2][2][1] = WDL_String("Aux Send Pan");   // aux | f | fl
    hlp_enc_str[2][3][0] = WDL_String("Aux Send Level");   // aux | m | _
    hlp_enc_str[2][3][1] = WDL_String("Aux Send Pan");   // aux | m | fl

    hlp_tkfdr_str[0][0][0] = WDL_String("Track Volume");   // pan | _ | _
    hlp_tkfdr_str[0][0][1] = WDL_String("Track Pan");   // pan | _ | fl
    hlp_tkfdr_str[0][1][0] = WDL_String("Track Volume -> 0 dB");   // pan | sh | _
    hlp_tkfdr_str[0][1][1] = WDL_String("Track Pan -> C");   // pan | sh | fl
    hlp_tkfdr_str[0][2][0] = WDL_String("Track Volume -> - inf dB");   // pan | f | _
    hlp_tkfdr_str[0][2][1] = WDL_String("Track Stereo Width");   // pan | f | fl
    hlp_tkfdr_str[0][3][0] = WDL_String("Track Volume");   // pan | m | _
    hlp_tkfdr_str[0][3][1] = WDL_String("Track Pan");   // pan | m | fl
    hlp_tkfdr_str[1][0][0] = WDL_String("Track Volume");   // chan | _ | _
    hlp_tkfdr_str[1][0][1] = WDL_String("FX Parameter");   // chan | _ | fl
    hlp_tkfdr_str[1][1][0] = WDL_String("Track Volume -> 0 dB");   // chan | sh | _
    hlp_tkfdr_str[1][1][1] = WDL_String("FX Parameter -> max");   // chan | sh | fl
    hlp_tkfdr_str[1][2][0] = WDL_String("Track Volume -> -inf dB");   // chan | f | _
    hlp_tkfdr_str[1][2][1] = WDL_String("FX Parameter -> min");   // chan | f | fl
    hlp_tkfdr_str[1][3][0] = WDL_String("Track Volume");   // chan | m | _
    hlp_tkfdr_str[1][3][1] = WDL_String("FX Parameter");   // chan | m | fl
    hlp_tkfdr_str[2][0][0] = WDL_String("Track Volume");   // aux | _ | _
    hlp_tkfdr_str[2][0][1] = WDL_String("Aux Send Level");   // aux | _ | fl
    hlp_tkfdr_str[2][1][0] = WDL_String("Track Volume -> 0 dB");   // aux | sh | _
    hlp_tkfdr_str[2][1][1] = WDL_String("Aux Send Level -> 0 dB");   // aux | sh | fl
    hlp_tkfdr_str[2][2][0] = WDL_String("Track Volume -> - inf dB");   // aux | f | _
    hlp_tkfdr_str[2][2][1] = WDL_String("Aux Send Level -> - inf dB");   // aux | f | fl
    hlp_tkfdr_str[2][3][0] = WDL_String("Track Volume");   // aux | m | _
    hlp_tkfdr_str[2][3][1] = WDL_String("Aux Send Level");   // aux | m | fl

    hlp_tksel_str[0][0] = WDL_String("Select Track");    // pan | _
    hlp_tksel_str[0][1] = WDL_String("Arm Track for Record");    // pan | sh
    hlp_tksel_str[0][2] = WDL_String("Switch Phase");    // pan | f
    hlp_tksel_str[0][3] = WDL_String("Select Track");    // pan | m
    hlp_tksel_str[1][0] = WDL_String("Select Track for Channel Strip");    // chan | _
    hlp_tksel_str[1][1] = WDL_String("Arm Track for Record");    // chan | sh
    hlp_tksel_str[1][2] = WDL_String("Switch Phase");    // chan | f
    hlp_tksel_str[1][3] = WDL_String("Select Track for Channel Strip");    // chan | m
    hlp_tksel_str[2][0] = WDL_String("Select Track");    // aux | _
    hlp_tksel_str[2][1] = WDL_String("Remove Aux Send");    // aux | sh
    hlp_tksel_str[2][2] = WDL_String("Add Aux Send");    // aux | f
    hlp_tksel_str[2][3] = WDL_String("Select Track");    // aux | m

    hlp_tksolo_str[0] = WDL_String("Solo Track");      // _
    hlp_tksolo_str[1] = WDL_String("Solo This Track Only");      // sh
    hlp_tksolo_str[2] = WDL_String("Solo Track");      // f
    hlp_tksolo_str[3] = WDL_String("Solo Track");      // m

    hlp_tkmute_str[0] = WDL_String("Mute Track");      // _
    hlp_tkmute_str[1] = WDL_String("Mute This Track Only");      // sh
    hlp_tkmute_str[2] = WDL_String("Bypass All Track FX");      // f
    hlp_tkmute_str[3] = WDL_String("Mute Track");      // m

    hlp_mstsel_str[0] = WDL_String("Select Tracks: None / All");      // _
    hlp_mstsel_str[1] = WDL_String("Select Master");      // sh
    hlp_mstsel_str[2] = WDL_String("Select Tracks: None / All");      // f
    hlp_mstsel_str[3] = WDL_String("Select Tracks: None / All");      // m

    hlp_clsolo_str[0] = WDL_String("Clear all Solos");      // _
    hlp_clsolo_str[1] = WDL_String("Unmute Master");      // sh
    hlp_clsolo_str[2] = WDL_String("Clear all Mutes");      // f
    hlp_clsolo_str[3] = WDL_String("Clear all Solos");      // m
    
    hlp_mstfdr_str[0] = WDL_String("Master Volume");      // _
    hlp_mstfdr_str[1] = WDL_String("Master Volume -> 0 dB");      // sh
    hlp_mstfdr_str[2] = WDL_String("Master Volume -> - inf dB");      // f
    hlp_mstfdr_str[3] = WDL_String("Master Volume");      // m

    hlp_chan_str[0] = WDL_String("Enter Channnel Strip Mode");      // pan
    hlp_chan_str[1] = WDL_String("Exit Channnel Strip Mode (Enter Pan Mode)");      // chan
    hlp_chan_str[2] = WDL_String("Enter Channnel Strip Mode");      // aux

    hlp_fkey_str[0] = WDL_String("");      // _
    hlp_fkey_str[1] = WDL_String("Open / Close Scribble Strip");      // sh
    hlp_fkey_str[2] = WDL_String("");      // f
    hlp_fkey_str[3] = WDL_String("");      // m

    hlp_shift_str[0] = WDL_String("");      // _
    hlp_shift_str[1] = WDL_String("");      // sh
    hlp_shift_str[2] = WDL_String("Open / Close On-Screen Help");      // f
    hlp_shift_str[3] = WDL_String("");      // m

    hlp_keys_str[0][0][0] = WDL_String("Enter Aux Mode: Aux---1"); // aux1 | pan | _
    hlp_keys_str[0][0][1] = WDL_String(""); // aux1 | pan | sh
    hlp_keys_str[0][0][2] = WDL_String(""); // aux1 | pan | f
    hlp_keys_str[0][0][3] = WDL_String(""); // aux1 | pan | m
    hlp_keys_str[0][1][0] = WDL_String("FX Parameters: Shift Bank (< 24)"); // aux1 | chan | _
    hlp_keys_str[0][1][1] = WDL_String(""); // aux1 | chan | sh
    hlp_keys_str[0][1][2] = WDL_String(""); // aux1 | chan | f
    hlp_keys_str[0][1][3] = WDL_String(""); // aux1 | chan | m
    hlp_keys_str[0][2][0] = WDL_String("Enter Aux Mode: Aux---1"); // aux1 | aux | _
    hlp_keys_str[0][2][1] = WDL_String(""); // aux1 | aux | sh
    hlp_keys_str[0][2][2] = WDL_String(""); // aux1 | aux | f
    hlp_keys_str[0][2][3] = WDL_String(""); // aux1 | aux | m

    hlp_keys_str[1][0][0] = WDL_String("Enter Aux Mode: Aux---2"); // aux2 | pan | _
    hlp_keys_str[1][0][1] = WDL_String(""); // aux2 | pan | sh
    hlp_keys_str[1][0][2] = WDL_String(""); // aux2 | pan | f
    hlp_keys_str[1][0][3] = WDL_String(""); // aux2 | pan | m
    hlp_keys_str[1][1][0] = WDL_String("FX Parameters: Shift Bank (24 >)"); // aux2 | chan | _
    hlp_keys_str[1][1][1] = WDL_String(""); // aux2 | chan | sh
    hlp_keys_str[1][1][2] = WDL_String(""); // aux2 | chan | f
    hlp_keys_str[1][1][3] = WDL_String(""); // aux2 | chan | m
    hlp_keys_str[1][2][0] = WDL_String("Enter Aux Mode: Aux---2"); // aux2 | aux | _
    hlp_keys_str[1][2][1] = WDL_String(""); // aux2 | aux | sh
    hlp_keys_str[1][2][2] = WDL_String(""); // aux2 | aux | f
    hlp_keys_str[1][2][3] = WDL_String(""); // aux2 | aux | m

    hlp_keys_str[2][0][0] = WDL_String("Enter Aux Mode: Aux---3"); // aux3 | pan | _
    hlp_keys_str[2][0][1] = WDL_String(""); // aux3 | pan | sh
    hlp_keys_str[2][0][2] = WDL_String(""); // aux3 | pan | f
    hlp_keys_str[2][0][3] = WDL_String(""); // aux3 | pan | m
    hlp_keys_str[2][1][0] = WDL_String("Current FX: Toggle Bypass "); // aux3 | chan | _
    hlp_keys_str[2][1][1] = WDL_String(""); // aux3 | chan | sh
    hlp_keys_str[2][1][2] = WDL_String(""); // aux3 | chan | f
    hlp_keys_str[2][1][3] = WDL_String(""); // aux3 | chan | m
    hlp_keys_str[2][2][0] = WDL_String("Enter Aux Mode: Aux---3"); // aux3 | aux | _
    hlp_keys_str[2][2][1] = WDL_String(""); // aux3 | aux | sh
    hlp_keys_str[2][2][2] = WDL_String(""); // aux3 | aux | f
    hlp_keys_str[2][2][3] = WDL_String(""); // aux3 | aux | m

    hlp_keys_str[3][0][0] = WDL_String("Enter Aux Mode: Aux---4"); // aux4 | pan | _
    hlp_keys_str[3][0][1] = WDL_String(""); // aux4 | pan | sh
    hlp_keys_str[3][0][2] = WDL_String(""); // aux4 | pan | f
    hlp_keys_str[3][0][3] = WDL_String(""); // aux4 | pan | m
    hlp_keys_str[3][1][0] = WDL_String("Insert FX"); // aux4 | chan | _
    hlp_keys_str[3][1][1] = WDL_String(""); // aux4 | chan | sh
    hlp_keys_str[3][1][2] = WDL_String(""); // aux4 | chan | f
    hlp_keys_str[3][1][3] = WDL_String(""); // aux4 | chan | m
    hlp_keys_str[3][2][0] = WDL_String("Enter Aux Mode: Aux---4"); // aux4 | aux | _
    hlp_keys_str[3][2][1] = WDL_String(""); // aux4 | aux | sh
    hlp_keys_str[3][2][2] = WDL_String(""); // aux4 | aux | f
    hlp_keys_str[3][2][3] = WDL_String(""); // aux4 | aux | m

    hlp_keys_str[4][0][0] = WDL_String("Enter Aux Mode: Aux---5"); // aux5 | pan | _
    hlp_keys_str[4][0][1] = WDL_String(""); // aux5 | pan | sh
    hlp_keys_str[4][0][2] = WDL_String(""); // aux5 | pan | f
    hlp_keys_str[4][0][3] = WDL_String(""); // aux5 | pan | m
    hlp_keys_str[4][1][0] = WDL_String("Delete FX"); // aux5 | chan | _
    hlp_keys_str[4][1][1] = WDL_String(""); // aux5 | chan | sh
    hlp_keys_str[4][1][2] = WDL_String(""); // aux5 | chan | f
    hlp_keys_str[4][1][3] = WDL_String(""); // aux5 | chan | m
    hlp_keys_str[4][2][0] = WDL_String("Enter Aux Mode: Aux---5"); // aux5 | aux | _
    hlp_keys_str[4][2][1] = WDL_String(""); // aux5 | aux | sh
    hlp_keys_str[4][2][2] = WDL_String(""); // aux5 | aux | f
    hlp_keys_str[4][2][3] = WDL_String(""); // aux5 | aux | m

    hlp_keys_str[5][0][0] = WDL_String("Enter Aux Mode: Aux---6"); // aux6 | pan | _
    hlp_keys_str[5][0][1] = WDL_String(""); // aux6 | pan | sh
    hlp_keys_str[5][0][2] = WDL_String(""); // aux6 | pan | f
    hlp_keys_str[5][0][3] = WDL_String(""); // aux6 | pan | m
    hlp_keys_str[5][1][0] = WDL_String("Toggle Track / FX Automation"); // aux6 | chan | _
    hlp_keys_str[5][1][1] = WDL_String(""); // aux6 | chan | sh
    hlp_keys_str[5][1][2] = WDL_String(""); // aux6 | chan | f
    hlp_keys_str[5][1][3] = WDL_String(""); // aux6 | chan | m
    hlp_keys_str[5][2][0] = WDL_String("Enter Aux Mode: Aux---6"); // aux6 | aux | _
    hlp_keys_str[5][2][1] = WDL_String(""); // aux6 | aux | sh
    hlp_keys_str[5][2][2] = WDL_String(""); // aux6 | aux | f
    hlp_keys_str[5][2][3] = WDL_String(""); // aux6 | aux | m

    hlp_keys_str[6][0][0] = WDL_String(""); // null | pan | _
    hlp_keys_str[6][0][1] = WDL_String(""); // null | pan | sh
    hlp_keys_str[6][0][2] = WDL_String(""); // null | pan | f
    hlp_keys_str[6][0][3] = WDL_String(""); // null | pan | m
    hlp_keys_str[6][1][0] = WDL_String(""); // null | chan | _
    hlp_keys_str[6][1][1] = WDL_String(""); // null | chan | sh
    hlp_keys_str[6][1][2] = WDL_String(""); // null | chan | f
    hlp_keys_str[6][1][3] = WDL_String(""); // null | chan | m
    hlp_keys_str[6][2][0] = WDL_String(""); // null | aux | _
    hlp_keys_str[6][2][1] = WDL_String(""); // null | aux | sh
    hlp_keys_str[6][2][2] = WDL_String(""); // null | aux | f
    hlp_keys_str[6][2][3] = WDL_String(""); // null | aux | m

    hlp_keys_str[7][0][0] = WDL_String("Rewind"); // rew | pan | _
    hlp_keys_str[7][0][1] = WDL_String("Automation Mode: Off / Trim"); // rew | pan | sh
    hlp_keys_str[7][0][2] = WDL_String(""); // rew | pan | f
    hlp_keys_str[7][0][3] = WDL_String(""); // rew | pan | m
    hlp_keys_str[7][1][0] = WDL_String("Rewind"); // rew | chan | _
    hlp_keys_str[7][1][1] = WDL_String("Automation Mode: Off / Trim"); // rew | chan | sh
    hlp_keys_str[7][1][2] = WDL_String(""); // rew | chan | f
    hlp_keys_str[7][1][3] = WDL_String(""); // rew | chan | m
    hlp_keys_str[7][2][0] = WDL_String("Rewind"); // rew | aux | _
    hlp_keys_str[7][2][1] = WDL_String("Automation Mode: Off / Trim"); // rew | aux | sh
    hlp_keys_str[7][2][2] = WDL_String(""); // rew | aux | f
    hlp_keys_str[7][2][3] = WDL_String(""); // rew | aux | m

    hlp_keys_str[8][0][0] = WDL_String("Fast Forward"); // fwd | pan | _
    hlp_keys_str[8][0][1] = WDL_String("Automation Mode: Read"); // fwd | pan | sh
    hlp_keys_str[8][0][2] = WDL_String(""); // fwd | pan | f
    hlp_keys_str[8][0][3] = WDL_String(""); // fwd | pan | m
    hlp_keys_str[8][1][0] = WDL_String("Fast Forward"); // fwd | chan | _
    hlp_keys_str[8][1][1] = WDL_String("Automation Mode: Read"); // fwd | chan | sh
    hlp_keys_str[8][1][2] = WDL_String(""); // fwd | chan | f
    hlp_keys_str[8][1][3] = WDL_String(""); // fwd | chan | m
    hlp_keys_str[8][2][0] = WDL_String("Fast Forward"); // fwd | aux | _
    hlp_keys_str[8][2][1] = WDL_String("Automation Mode: Read"); // fwd | aux | sh
    hlp_keys_str[8][2][2] = WDL_String(""); // fwd | aux | f
    hlp_keys_str[8][2][3] = WDL_String(""); // fwd | aux | m

    hlp_keys_str[9][0][0] = WDL_String("Stop"); // stop | pan | _
    hlp_keys_str[9][0][1] = WDL_String("Automation Mode: Latch"); // stop | pan | sh
    hlp_keys_str[9][0][2] = WDL_String(""); // stop | pan | f
    hlp_keys_str[9][0][3] = WDL_String(""); // stop | pan | m
    hlp_keys_str[9][1][0] = WDL_String("Stop"); // stop | chan | _
    hlp_keys_str[9][1][1] = WDL_String("Automation Mode: Latch"); // stop | chan | sh
    hlp_keys_str[9][1][2] = WDL_String(""); // stop | chan | f
    hlp_keys_str[9][1][3] = WDL_String(""); // stop | chan | m
    hlp_keys_str[9][2][0] = WDL_String("Stop"); // stop | aux | _
    hlp_keys_str[9][2][1] = WDL_String("Automation Mode: Latch"); // stop | aux | sh
    hlp_keys_str[9][2][2] = WDL_String(""); // stop | aux | f
    hlp_keys_str[9][2][3] = WDL_String(""); // stop | aux | m

    hlp_keys_str[10][0][0] = WDL_String("Play"); // play | pan | _
    hlp_keys_str[10][0][1] = WDL_String("Automation Mode: Write"); // play | pan | sh
    hlp_keys_str[10][0][2] = WDL_String(""); // play | pan | f
    hlp_keys_str[10][0][3] = WDL_String(""); // play | pan | m
    hlp_keys_str[10][1][0] = WDL_String("Play"); // play | chan | _
    hlp_keys_str[10][1][1] = WDL_String("Automation Mode: Write"); // play | chan | sh
    hlp_keys_str[10][1][2] = WDL_String(""); // play | chan | f
    hlp_keys_str[10][1][3] = WDL_String(""); // play | chan | m
    hlp_keys_str[10][2][0] = WDL_String("Play"); // play | aux | _
    hlp_keys_str[10][2][1] = WDL_String("Automation Mode: Write"); // play | aux | sh
    hlp_keys_str[10][2][2] = WDL_String(""); // play | aux | f
    hlp_keys_str[10][2][3] = WDL_String(""); // play | aux | m

    hlp_keys_str[11][0][0] = WDL_String("Record (Punch in when playing)"); // rec | pan | _
    hlp_keys_str[11][0][1] = WDL_String("Automation: Write Current Value to Time Selection"); // rec | pan | sh
    hlp_keys_str[11][0][2] = WDL_String(""); // rec | pan | f
    hlp_keys_str[11][0][3] = WDL_String(""); // rec | pan | m
    hlp_keys_str[11][1][0] = WDL_String("Record (Punch in when playing)"); // rec | chan | _
    hlp_keys_str[11][1][1] = WDL_String("Automation: Write Current Value to Time Selection"); // rec | chan | sh
    hlp_keys_str[11][1][2] = WDL_String(""); // rec | chan | f
    hlp_keys_str[11][1][3] = WDL_String(""); // rec | chan | m
    hlp_keys_str[11][2][0] = WDL_String("Record (Punch in when playing)"); // rec | aux | _
    hlp_keys_str[11][2][1] = WDL_String("Automation: Write Current Value to Time Selection"); // rec | aux | sh
    hlp_keys_str[11][2][2] = WDL_String(""); // rec | aux | f
    hlp_keys_str[10][2][3] = WDL_String(""); // rec | aux | m

    hlp_bank_str[0][0][0] = WDL_String("Tracks: Shift Bank (< 8)");  // bank - | pan | _
    hlp_bank_str[0][0][1] = WDL_String("Tracks: Shift Bank (< 24)");  // bank - | pan | sh
    hlp_bank_str[0][0][2] = WDL_String("Time Selection: Move Left Locator (< 1 Bar)");  // bank - | pan | f
    hlp_bank_str[0][0][3] = WDL_String("Tracks: Shift Bank (< 8)");  // bank - | pan | m
    hlp_bank_str[0][1][0] = WDL_String("Select Previous FX in Chain");  // bank - | chan | _
    hlp_bank_str[0][1][1] = WDL_String("Tracks: Shift Bank (< 24)");  // bank - | chan | sh
    hlp_bank_str[0][1][2] = WDL_String("Move FX Up in Chain");  // bank - | chan | f
    hlp_bank_str[0][1][3] = WDL_String("Select Previous FX in Chain");  // bank - | chan | m
    hlp_bank_str[0][2][0] = WDL_String("Tracks: Shift Bank (< 8)");  // bank - | aux | _
    hlp_bank_str[0][2][1] = WDL_String("Tracks: Shift Bank (< 24)");  // bank - | aux | sh
    hlp_bank_str[0][2][2] = WDL_String("Time Selection: Move Left Locator (< 1 Bar)");  // bank - | aux | f
    hlp_bank_str[0][2][3] = WDL_String("Tracks: Shift Bank (< 8)");  // bank - | aux | m

    hlp_bank_str[1][0][0] = WDL_String("Tracks: Shift Bank (8 >)");  // bank + | pan | _
    hlp_bank_str[1][0][1] = WDL_String("Tracks: Shift Bank (24 >)");  // bank + | pan | sh
    hlp_bank_str[1][0][2] = WDL_String("Time Selection: Move Left Locator (1 Bar >)");  // bank + | pan | f
    hlp_bank_str[1][0][3] = WDL_String("Tracks: Shift Bank (8 >)");  // bank + | pan | m
    hlp_bank_str[1][1][0] = WDL_String("Select Next FX in Chain");  // bank + | chan | _
    hlp_bank_str[1][1][1] = WDL_String("Tracks: Shift Bank (24 >)");  // bank + | chan | sh
    hlp_bank_str[1][1][2] = WDL_String("Move FX Down in Chain");  // bank + | chan | f
    hlp_bank_str[1][1][3] = WDL_String("Select Previous FX in Chain");  // bank + | chan | m
    hlp_bank_str[1][2][0] = WDL_String("Tracks: Shift Bank (8 >)");  // bank + | aux | _
    hlp_bank_str[1][2][1] = WDL_String("Tracks: Shift Bank (24 >)");  // bank + | aux | sh
    hlp_bank_str[1][2][2] = WDL_String("Time Selection: Move Left Locator (1 Bar >)");  // bank + | aux | f
    hlp_bank_str[1][2][3] = WDL_String("Tracks: Shift Bank (8 >)");  // bank + | aux | m

    hlp_inout_str[0][0] = WDL_String("Time Selection: Select Previous Region"); // in | _
    hlp_inout_str[0][1] = WDL_String("Time Selection: Select All / Last Selected"); // in | sh
    hlp_inout_str[0][2] = WDL_String("Time Selection: Move Right Locator (1 Bar >)"); // in | f
    hlp_inout_str[0][3] = WDL_String("Time Selection: Select Previous Region"); // in | m

    hlp_inout_str[1][0] = WDL_String("Time Selection: Select Next Region"); // out | _
    hlp_inout_str[1][1] = WDL_String("Loop Time Selection On / Off"); // out | pan| sh
    hlp_inout_str[1][2] = WDL_String("Time Selection: Move Right Locator (< 1 Bar)"); // out | f
    hlp_inout_str[1][3] = WDL_String("Time Selection: Select Next Region"); // out | m
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


  unsigned char Cnv_PeakToEncoder(double value) 
  {
    double new_val = VAL2DB(value);

    return Cnv_DBToEncoder(new_val);
  } // Cnv_PeakToEncoder


  unsigned char Cnv_DBToEncoder(double value)
  {
    if (value > MINUSINF) value = ((MINUSINF - value) / MINUSINF * 15.0) + 0.5;
    else value = 0.0;

    if (value > 15.0) value = 15.0;

    return char(value);
  } //Cnv_DBToEncoder


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
    double d_rel = (double)rel_val * (max - min) * step;

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

  void Utl_SaveSelection()
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
  } // Utl_SaveSelection


  void Utl_RestoreSelection()
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
  } // Utl_RestoreSelection


  void Utl_GetCustomCmdIds()
  {
    const char* name;

    // go through all custom commands and parse names to get cmd ids
    for (int cmd = 50000; cmd <= 65535; cmd++)
    {
      name = kbd_getTextFromCmd(cmd, NULL);
      const char* end = strstr(name, "US-2400 -");
      if (end != NULL)
      {
        int len = end - name - 9;

        int qkey = -1;
        int mode = -1;
        int key = -1;

        // find qualifier key
        if (strstr(name, "- NoKey -")) qkey = 0;
        else if (strstr(name, "- Shift -")) qkey = 1;
        else if (strstr(name, "- FKey -")) qkey = 2;
        else if (strstr(name, "- MKey -")) qkey = 3;

        // find mode
        if (strstr(name, "- Pan -")) mode = 0;
        else if (strstr(name, "- Chan -")) mode = 1;
        else if (strstr(name, "- Aux -")) mode = 2;

        // find key
        if (strstr(name, "- Null")) key = 6;
        else if (strstr(name, "- Rew")) key = 7;
        else if (strstr(name, "- FFwd")) key = 8;
        else if (strstr(name, "- Stop")) key = 9;
        else if (strstr(name, "- Play")) key = 10;
        else if (strstr(name, "- Rec")) key = 11;
        else
        {
          char index_string[3];
          for (int s = 0; s < 6; s++)
          {
             sprintf(index_string, "- %d", s+1);
             if (strstr(name, index_string)) key = s;
          }
        }

        // save cmd id
        if (key != -1)
        {
          // if no mode or qkey specified enter found action for all undefined modes / qkeys
          for (int q = 0; q <= 3; q++)
          {
            for (int m = 0; m <= 2; m++)
            {
              if ( 
                ( (q == qkey) || ((qkey == -1) && (cmd_ids[q][m][key] == -1)) ) && 
                ( (m == mode) || ((mode == -1) && (cmd_ids[q][m][key] == -1)) ) 
              )
              {
                cmd_ids[q][m][key] = cmd;
                hlp_keys_str[key][m][q] = WDL_String(name);
                hlp_keys_str[key][m][q].DeleteSub(0, 8);
                hlp_keys_str[key][m][q].SetLen(len);
              }
            }
          }
        }
      }
    }
  } // Utl_GetCustomCmdIds


  void Utl_CheckFXInsert()
  {
    int real_fx_count = TrackFX_GetCount(chan_rpr_tk);
    if (chan_fx_count != real_fx_count)
    {
     
      chan_fx_count = real_fx_count;
      chan_fx = chan_fx_count - 1;

      // update display and encoders / faders
      for (int ch = 0; ch < 24; ch++)
      {
        Stp_Update(ch);
        if (!m_flip) MySetSurface_UpdateEncoder(ch);
        else MySetSurface_UpdateFader(ch);
      }

      MyCSurf_Chan_OpenFX(chan_fx);
    }
  } // Utl_CheckFXInsert


  WDL_String Utl_Alphanumeric(WDL_String in_str)
  {
    char* str_buf = in_str.Get();
    bool replace = false;
    // replace everything other than A-Z, a-z, 0-9 with a space
    for (int i = 0; i < (int)strlen(str_buf); i++)
    {
      replace = true;
      if (str_buf[i] == '\n') replace = false;
      if ((str_buf[i] >= '0') && (str_buf[i] <= '9')) replace = false;
      if ((str_buf[i] >= 'A') && (str_buf[i] <= 'Z')) replace = false;
      if ((str_buf[i] >= 'a') && (str_buf[i] <= 'z')) replace = false;

      if (replace) str_buf[i] = ' ';
    }

    WDL_String out_str = WDL_String(str_buf);
    return out_str;
  } // Utl_Alphanumeric


  MediaTrack* Utl_FindAux(int aux_id)
  {
    MediaTrack* tk;
    
    for (int tk_id = 0; tk_id < CountTracks(0); tk_id++)
    {
      tk = GetTrack(0, tk_id);
      const char* name = GetTrackState(tk, 0);
      
      // lowercase aux
      int c = 0;
      int chr_found = 0;
      while ((name[c] != '\0') && (c < (int)strlen(name)))
      {
        if (chr_found == 0 && ((name[c] == 'A') || (name[c] == 'a'))) chr_found = 1;
        else if (chr_found == 1 && ((name[c] == 'U') || (name[c] == 'u'))) chr_found = 2;
        else if (chr_found == 2 && ((name[c] == 'X') || (name[c] == 'x'))) chr_found = 3;
        else if ((chr_found >= 3) && (chr_found <= 5) && (name[c] == '-')) chr_found++;
        else if ((chr_found == 6) && (name[c] == (aux_id + 48))) chr_found = 7;

        if (chr_found == 7) return tk;

        c++;
      }
    }
    
    return NULL;
  }

  WDL_String Utl_Chunk_InsertLine(WDL_String chunk, WDL_String line, WDL_String before)
  {
    char* pstr = chunk.Get();
    char* ppos = strstr(pstr, before.Get());
    
    // found 'before'? insert
    if (ppos != NULL)
    {
      chunk.Insert( line.Get(), ppos - pstr, strlen(line.Get()) );
    }

    return chunk;
  }

  WDL_String Utl_Chunk_RemoveLine(WDL_String chunk, WDL_String search)
  {
    char* pstr = chunk.Get();
    char* ppos = strstr(pstr, search.Get());
    
    if (ppos != NULL)
    {
      // search beggining of line
      char* pstart = ppos;
      while ( (*pstart != '\n') && (pstart > pstr) )
        pstart--;
      
      // search end of line
      char* pend = ppos;
      while ( (*pend != '\n') && (pend < pstr + strlen(pstr)) )
        pend++;

      chunk.DeleteSub(pstart - pstr, pend - pstart);
    }

    return chunk;
  }

  WDL_String Utl_Chunk_Replace(WDL_String chunk, WDL_String search, WDL_String replace)
  {
    char* pstr = chunk.Get();
    char* ppos = strstr(pstr, search.Get());

    if (ppos != NULL)
    {
      chunk.DeleteSub(ppos - pstr, strlen(search.Get()));
      chunk.Insert( replace.Get(), ppos - pstr, strlen(replace.Get()) );
    }

    return chunk;
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


    // reset cmd_ids
    for (char qkey = 0; qkey < 4; qkey++)
      for (char mode = 0; mode < 3; mode++)
        for (char key = 0; key < 12; key++)
          cmd_ids[qkey][mode][key] = -1;

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
    master_sel = false;


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
    q_mkey = false;

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
    stp_hwnd = NULL;
    stp_class.cbSize = sizeof(WNDCLASSEX);
    stp_class.style = 0;
    stp_class.lpfnWndProc = (WNDPROC)Stp_WindowProc;
    stp_class.cbClsExtra = 0;
    stp_class.cbWndExtra = 0;
    stp_class.hInstance = g_hInst;
    stp_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    stp_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    stp_class.hbrBackground = CreateSolidBrush(RGB(60, 60, 60));
    stp_class.lpszMenuName = NULL;
    stp_class.lpszClassName = "stp";
    stp_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassEx(&stp_class);

    // On-Screen Help

    hlp_hwnd = NULL;
    hlp_class.cbSize = sizeof(WNDCLASSEX);
    hlp_class.style = 0;
    hlp_class.lpfnWndProc = (WNDPROC)Hlp_WindowProc;
    hlp_class.cbClsExtra = 0;
    hlp_class.cbWndExtra = 0;
    hlp_class.hInstance = g_hInst;
    hlp_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    hlp_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    hlp_class.hbrBackground = CreateSolidBrush(RGB(60, 60, 60));
    hlp_class.lpszMenuName = NULL;
    hlp_class.lpszClassName = "hlp";
    hlp_class.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassEx(&hlp_class);

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
    s_exitdone = MySetSurface_Exit();
    do
    { Sleep(500);  
    } while (!s_exitdone);
    
    delete saved_sel;

    delete m_midiout;
    delete m_midiin;
  } // ~CSurf_US2400()



  ////// CUSTOM SURFACE UPDATES //////

  bool MySetSurface_Init() 
  {
    Stp_RetrieveCoords();

    Hlp_FillStrs(); // insert hardcoded strings into hlp_xxx_str

    Utl_GetCustomCmdIds(); // inserts custom cmd strs into hlp_keys_str
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
    Stp_SaveCoords();

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

    Stp_CloseWindow();

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


  void MySetSurface_ExecuteFaderUpdate(int ch_id)
  {
    if ((s_touch_fdr & (1 << ch_id)) == 0)
    {
      // send midi
      MIDIOut(0xb0, ch_id + 0x1f, (cache_faders[ch_id] & 0x7f));
      MIDIOut(0xb0, ch_id, ((cache_faders[ch_id] >> 7) & 0x7f));
      
      // remove update flag
      cache_upd_faders = cache_upd_faders & (~(1 << ch_id));  
    }
  } // MySetSurface_ExecuteFaderUpdate


  void MySetSurface_UpdateEncoder(int ch_id)
  {
    // encoder exists?
    if ( (ch_id >= 0) && (ch_id <= 23) )
    {

      MediaTrack* rpr_tk = Cnv_ChannelIDToMediaTrack(ch_id);
      int para_amount;

      double d_value;
      unsigned char value;
      bool dot = false; // for phase switch, rec arm

      bool istrack = true;
      bool exists = false;
      bool isactive = true;
      bool ispre = false;

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
        else isactive = true; // chan + !flip: is track doesn't matter

      } else if ((m_aux > 0) && (rpr_tk != NULL))
      { // only aux #x: has send #x?
        int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
        isactive = false;
        if (send_id != -1) 
        {
          isactive = true;
          int* send_mode = (int*)GetSetTrackSendInfo(rpr_tk, 0, send_id, "I_SENDMODE", NULL);
          if (*send_mode > 0) ispre = true;
        }
      }

      // get values and update
     
      if (!isactive)
      { // is not active encoder

        if (m_pan) value = 15; // reset, show something to inhlpate inactive
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
              if (METERMODE) value += 0x10; // pan mode
            }
          } else
          { // flip -> volume

            GetTrackUIVolPan(rpr_tk, &d_value, NULL);
            value = Cnv_VolToEncoder(d_value);
            if (METERMODE) value += 0x20; // bar mode
          }
        } else
        {
          if (m_chan)
          { // chan -> fx_param (para_offset checked above)

            double min, max;
            d_value = TrackFX_GetParam(chan_rpr_tk, chan_fx, chan_par_offs + ch_id, &min, &max);

            value = Cnv_FXParamToEncoder(min, max, d_value);
            if (METERMODE) value += 0x20; // bar mode
         
          } else if (m_aux > 0)
          { // aux -> send level
            int send_id = Cnv_AuxIDToSendID(ch_id, m_aux);
            if (send_id != -1)
            {
              GetTrackSendUIVolPan(rpr_tk, send_id, &d_value, NULL);
              value = Cnv_VolToEncoder(d_value);
              if (METERMODE) value += 0x20; // bar mode;
            }
          
          } else if (m_pan)
          {
            if (q_fkey) 
            { // pan mode + fkey -> width
              
              d_value = GetMediaTrackInfo_Value(rpr_tk, "D_WIDTH");
              if (!METERMODE) 
              {
                value = Cnv_PanToEncoder(d_value);
              
              } else {
                value = Cnv_WidthToEncoder(d_value);
                value += 0x30; // width mode
              }

            } else
            { // pan mode -> pan

              GetTrackUIVolPan(rpr_tk, NULL, &d_value);
              value = Cnv_PanToEncoder(d_value);
              if (METERMODE) value += 0x10; // pan mode
            }
          }
        } // if (m_flip), else
         
      } // if !active, else

      if (istrack)
      {

        // aux mode: active & pre/post
        if (m_aux > 0)
        {
          if (isactive) dot = true;
          if ((ispre) && (s_myblink)) dot = !dot;

        // other modes: phase / rec arm
        } else
        {
          // phase states
          if ( (bool)GetMediaTrackInfo_Value(rpr_tk, "B_PHASE") ) dot = true;
          
          // rec arms: blink
          if ( ((bool)GetMediaTrackInfo_Value(rpr_tk, "I_RECARM")) && (s_myblink) ) dot = !dot;
        }

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


  void MySetSurface_ExecuteEncoderUpdate(int ch_id)
  {
    unsigned char out = cache_enc[ch_id];
    
    // send midi -> pan
    MIDIOut(0xb0, ch_id + 0x40, cache_enc[ch_id]);

    // send midi -> meter mode
    if (!METERMODE) 
    {
      if (out & 0x40) MIDIOut(0xb0, ch_id + 0x60, 0x60);
      MIDIOut(0xb0, ch_id + 0x60, cache_enc[ch_id] + 0x50);
    }
    
    // remove update flag
    cache_upd_enc = cache_upd_enc & (~(1 << ch_id));
  } // MySetSurface_ExecuteEncoderUpdate


  void MySetSurface_OutputMeters(bool reset)
  {
    MediaTrack* tk;
    double tk_peak_l, tk_peak_r, tk_hold;
    int peak_out, hold_out;

    // iterate through channels
    for(int ch = 0; ch < 24; ch++)
    {
      // get data
      tk = Cnv_ChannelIDToMediaTrack(ch);
      
      tk_peak_l = Track_GetPeakInfo(tk, 0);
      tk_peak_r = Track_GetPeakInfo(tk, 1);

      peak_out = Cnv_PeakToEncoder((tk_peak_l + tk_peak_r) / 2);
      
      tk_hold = (Track_GetPeakHoldDB(tk, 0, reset) + Track_GetPeakHoldDB(tk, 1, reset)) * 50;
      hold_out = Cnv_DBToEncoder(tk_hold) + 0x10;

      // over
      if ((tk_peak_l > 1.0) || (tk_peak_r > 1.0)) peak_out += 0x60;
      else peak_out += 0x40;
      
      if (hold_out != cache_meters[ch])
      {
        // new value to cache (gets executed on next run cycle)
        cache_meters[ch] = hold_out;

        // set upd flag 
        cache_upd_meters = cache_upd_meters | (1 << ch);
      }  
      // midi out
      MIDIOut(0xb0, ch + 0x60, peak_out);
    }
  } // MySetSurface_OutputMeters


  void MySetSurface_ExecuteMeterUpdate(char ch_id)
  {
    MIDIOut(0xb0, ch_id + 0x60, cache_meters[ch_id]);

    // remove update flag
    cache_upd_meters = cache_upd_meters & (~(1 << ch_id));
  } // MySetSurface_ExecuteMeterUpdate


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

    hlp_flip = m_flip;
    Hlp_Update();
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

    if (btn_state) hlp_qkey = 2;
    else hlp_qkey = 0;
    Hlp_Update();
  } // MySetSurface_ToggleFKey


  void MySetSurface_ToggleShift(bool btn_state)
  {
    q_shift = btn_state;

    MySetSurface_UpdateButton(0x74, btn_state, false);
    MySetSurface_UpdateAuxButtons();

    if (btn_state) hlp_qkey = 1;
    else hlp_qkey = 0;
    Hlp_Update();
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
    
    // update encoders or faders and scribble strip
    for (char ch_id = 0; ch_id <= 23; ch_id++) 
    {
      if (m_flip) MySetSurface_UpdateFader(ch_id);
      else MySetSurface_UpdateEncoder(ch_id);

      if (stp_hwnd != NULL) Stp_Update(ch_id);
    }

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

      for (int enc = 0; enc < 24; enc++)
        if (stp_hwnd != NULL) Stp_Update(enc);

    }

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false); 
  } // MySetSurface_Chan_SelectTrack


  void MySetSurface_ShiftBanks(char dir, char factor)
  { 
    double track_amount = (double)CountTracks(0);

    int old_ch_offset = s_ch_offset;

    // move in dir by 8 or 24 (factor)
    int oct_steps = (s_ch_offset / 8);
    if (s_ch_offset % 8 != 0) oct_steps++;
    oct_steps += dir * factor / 8;
    s_ch_offset = oct_steps * 8;

    // min / max
    if (s_ch_offset > (CSurf_NumTracks(true) - 24)) s_ch_offset = CSurf_NumTracks(true) - 24;
    if (s_ch_offset > 168) s_ch_offset = 168;
    if (s_ch_offset < 0) s_ch_offset = 0;

    // update channel strip
    if (m_chan) chan_ch = chan_ch + old_ch_offset - s_ch_offset;

    // update mixer display
    SetMixerScroll(Cnv_ChannelIDToMediaTrack(0));

    // update encoders, faders, track buttons, scribble strip
    for(char ch_id = 0; ch_id < 24; ch_id++)
    {
      MySetSurface_UpdateEncoder(ch_id);
      MySetSurface_UpdateFader(ch_id);
      MySetSurface_UpdateTrackElement(ch_id);

      if (stp_hwnd != NULL) Stp_Update(ch_id);
    }

    MySetSurface_UpdateBankLEDs();
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

    Utl_CheckFXInsert();
    
    // blink Track Select
    MySetSurface_UpdateButton(chan_ch * 4 + 1, true, true);

    // blink para offset, bypass
    MySetSurface_UpdateAuxButtons();
      
    // blink banks
    MySetSurface_UpdateButton(0x70, true, true);
    MySetSurface_UpdateButton(0x71, true, true);

    // update scribble strip
    for (int enc = 0; enc < 24; enc++)
      if (stp_hwnd != NULL) Stp_Update(enc);

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false); 

    hlp_mode = 1;
    Hlp_Update();
  } // MySetSurface_EnterChanMode


  void MySetSurface_ExitChanMode()
  {
    m_chan = false;

    // reset chan button
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

    // update scribble strip
    for (int enc = 0; enc < 24; enc++)
      if (stp_hwnd != NULL) Stp_Update(enc);

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false); 
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

    hlp_mode = 0;
    Hlp_Update();
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

    hlp_mode = 2;
    Hlp_Update();
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

    // reset faders, encoders, track elements, update scribble strip
    for (int i = 0; i < 25; i++)
    {
      MySetSurface_UpdateFader(i);
      MySetSurface_UpdateTrackElement(i);
      MySetSurface_UpdateEncoder(i);

      if ((stp_hwnd != NULL) && (i < 24)) Stp_Update(i);
    }
  } // SetTrackListChange


  void SetSurfaceMute(MediaTrack* rpr_tk, bool mute)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    if ( (ch_id >= 0) && (ch_id <= 23) ) 
    {
      if (mute) MySetSurface_UpdateButton(4 * ch_id + 3, true, false);
      else MySetSurface_UpdateButton(4 * ch_id + 3, false, false);

      Stp_Update(ch_id);
    }
  } // SetSurfaceMute


  void SetSurfaceSelected(MediaTrack* rpr_tk, bool selected)
  {
    int ch_id = Cnv_MediaTrackToChannelID(rpr_tk);

    // update buttons
    if ( (ch_id >= 0) && (ch_id <= 24) ) 
    {
      MySetSurface_UpdateButton(4 * ch_id + 1, selected, false);
      Stp_Update(ch_id);
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

    if ((ch_id > 0) && (ch_id < 24))
    {
      MySetSurface_UpdateEncoder(ch_id);
      Stp_Update(ch_id);
    }
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
    
    /*
    bool solo_surf = true;  
    if (solo == 0) solo_surf = false;

    SetSurfaceSolo(rpr_tk, solo_surf);*/
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


  void MyCSurf_AddSwitchAuxSend(MediaTrack* rpr_tk, int aux)
  {
    int tk_id = (int)GetMediaTrackInfo_Value(rpr_tk, "IP_TRACKNUMBER") - 1;
    MediaTrack* aux_tk = Utl_FindAux(aux);
   
    char* chunk = GetSetObjectState(aux_tk, "");
    WDL_String chunk_wdl = WDL_String(chunk);

    // search for existing sends
    char search[90];
    char insert[90];
    int found_mode = -1;
    for (int m = 0; m <= 3; m++)
    {
      sprintf(search, "AUXRECV %d %d", tk_id, m);
      if (strstr(chunk, search)) found_mode = m;
    }

    FreeHeapPtr(chunk);

    if (found_mode == -1)
    {
      // new line for post send
      sprintf(insert, "AUXRECV %d 0 1.00000000000000 0.00000000000000 0 0 0 0 0 -1.00000000000000 0 -1 ''\n", tk_id);
      chunk_wdl = Utl_Chunk_InsertLine(chunk_wdl, WDL_String(insert), WDL_String("MIDIOUT "));
  
    } else if (found_mode == 0)
    {
      // new line for post send
      sprintf(search, "AUXRECV %d 0", tk_id);
      sprintf(insert, "AUXRECV %d 3", tk_id);

      chunk_wdl = Utl_Chunk_Replace(chunk_wdl, WDL_String(search), WDL_String(insert));
    
    } else
    {
      // new line for post send
      sprintf(search, "AUXRECV %d %d", tk_id, found_mode);
      sprintf(insert, "AUXRECV %d 0", tk_id);

      chunk_wdl = Utl_Chunk_Replace(chunk_wdl, WDL_String(search), WDL_String(insert));     
    } 

    GetSetObjectState(aux_tk, chunk_wdl.Get());

  } // MyCSurf_AddSwitchAuxSend


  void MyCSurf_RemoveAuxSend(MediaTrack* rpr_tk, int aux)
  {
    int tk_id = GetMediaTrackInfo_Value(rpr_tk, "IP_TRACKNUMBER") - 1;
    MediaTrack* aux_tk = Utl_FindAux(aux);
   
    char* chunk = GetSetObjectState(aux_tk, "");
    WDL_String chunk_wdl = WDL_String(chunk);
    FreeHeapPtr(chunk); 

    char search[90];
    const char* pos;
    
    // search for post send
    sprintf(search, "AUXRECV %d", tk_id);

    chunk_wdl = Utl_Chunk_RemoveLine(chunk_wdl, WDL_String(search));

    GetSetObjectState(aux_tk, chunk_wdl.Get()); 
  } // MyCSurf_RemoveAuxSend


  void MyCSurf_SelectMaster() 
  {
    MediaTrack* rpr_master = Cnv_ChannelIDToMediaTrack(24);

    master_sel = IsTrackSelected(rpr_master);
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

    // no track selected, master also not selected?
    if ( (sel_tks == 0) && ( !IsTrackSelected( Cnv_ChannelIDToMediaTrack(24) ) ) ) sel = true;

    // set tracks sel or unsel
    for (int i = 0; i < all_tks; i++)
    {
      tk = GetTrack(0, i);
      SetTrackSelected(tk, sel);
    }

    // apply to master also
    MediaTrack* rpr_master = Cnv_ChannelIDToMediaTrack(24);
    SetTrackSelected(rpr_master, sel); 
    master_sel = sel;

  } // MyCSurf_ToggleSelectAllTracks() 


  // CUSTOM TRANSPORT 

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

    for (int enc = 0; enc < 24; enc++)
      if (stp_hwnd != NULL) Stp_Update(enc);

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false); 
  } // MyCSurf_Chan_OpenFX
  

  void MyCSurf_Chan_CloseFX(int fx_id)
  {
    TrackFX_Show(chan_rpr_tk, fx_id, 2); // hide floating window
    TrackFX_Show(chan_rpr_tk, fx_id, 0); // hide chain window

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false); 
  } // MyCSurf_Chan_CloseFX


  void MyCSurf_Chan_DeleteFX()
  {
    Undo_BeginBlock();

    // count fx
    int before_del = TrackFX_GetCount(chan_rpr_tk);

    //isolate track for action
    Utl_SaveSelection();
    SetOnlyTrackSelected(chan_rpr_tk);
    Main_OnCommand(CMD_SEL2LASTTOUCH, 0);

    TrackFX_SetOpen(chan_rpr_tk, chan_fx, true);
    Main_OnCommand(CMD("_S&M_REMOVE_FX"), 0);
    
    Utl_RestoreSelection();
    
    if (before_del > 1)
    { 
      // if there are fx left open the previous one in chain
      chan_fx--;
      MyCSurf_Chan_OpenFX(chan_fx);
    }

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false);

    Undo_EndBlock("Delete FX", 0);
  } // MyCSurf_Chan_DeleteFX


  void MyCSurf_Chan_InsertFX()
  {
    // isolate track for action
    Utl_SaveSelection();
    SetOnlyTrackSelected(chan_rpr_tk);
    Main_OnCommand(CMD_SEL2LASTTOUCH, 0);
    
    TrackFX_Show(chan_rpr_tk, chan_fx, 1); // show chain window
    TrackFX_SetOpen(chan_rpr_tk, chan_fx, true);
    Main_OnCommand(CMD_FXBROWSER, 0);

    Utl_RestoreSelection();

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false); 

  } // MyCSurf_Chan_InsertFX


  void MyCSurf_Chan_MoveFX(char dir)
  {
    Undo_BeginBlock();

    int amount_fx = TrackFX_GetCount(chan_rpr_tk);

    // isolate track for selection
    Utl_SaveSelection();
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

    Utl_RestoreSelection();

    // bugfix: deselect master
    if (!master_sel) SetTrackSelected(Cnv_ChannelIDToMediaTrack(24), false); 

    Undo_EndBlock("Move FX", 0);
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
    Utl_SaveSelection();
    SetOnlyTrackSelected(Cnv_ChannelIDToMediaTrack(chan_ch));

    // arm all envelopes -> toggle fx disarms only them
    if (chan_fx_env_arm) Main_OnCommand(CMD("_S&M_ARMALLENVS"), 0);
    // disarm all envelopes -> toggle fx arms only them
    else Main_OnCommand(CMD("_S&M_DISARMALLENVS"), 0);
 
    // toggle arm fx envelopes
    Main_OnCommand(CMD("_S&M_TGLARMPLUGENV"), 0);

    chan_fx_env_arm = !chan_fx_env_arm;

    Utl_RestoreSelection();

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
      Utl_SaveSelection();
      SetOnlyTrackSelected(chan_rpr_tk);

    // if none selected, select and change all
    } else if (sel_tks == 0)
    {
      Utl_SaveSelection();

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
      Utl_RestoreSelection();


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
    // markers: true = move to next marker/region, false = move 1 bar
    // start_dir: move start of time sel by dir
    // end_dir: move end of time sel by dir


    // do nothing if sel all is on
    if (!s_loop_all)
    {

      // get time range
      ReaProject* rpr_pro = EnumProjects(-1, NULL, 0);
      double start_time, start_beats, end_time, end_beats;
      int start_num, start_denom, end_num, end_denom;

      // get time selection
      GetSet_LoopTimeRange(false, true, &start_time, &end_time, false);

      if (markers)
      {

        // max 100 markers / regions (couldn't get vectors to work)
        double* starts = new double[100];
        double* ends = new double[100];

        double curr_region_end = 9999999999.0;
        double last_pos = 0.0;
        bool inside_region;
        int idx = 0;
        int sel = 0;

        double sel_approx = 9999999999.0;

        double pos, region_end, start_diff, end_diff;
        bool is_region;
        int x = 0;
        while ( (x = EnumProjectMarkers(x, &is_region, &pos, &region_end, NULL, NULL)) && (idx <= 100) )
        {

          // did we leave a previously established region?
          if (pos > curr_region_end) 
          {
            // count region end as marker
            pos = curr_region_end;
            x--;

            // reset region flags
            inside_region = false;
            curr_region_end = 9999999999.0;
          }
          
          // add this range [last -> current marker (or region start/end)] to list
          if (
            (dblEq(pos, last_pos, 0.001) == false) &&
            (idx <= 100) && 
            ( 
              (idx == 0) || 
              (dblEq(last_pos, starts[idx-1], 0.001) == false) ||
              (dblEq(pos, ends[idx-1], 0.001) == false)
            ) )
          {
            starts[idx] = last_pos;
            ends[idx] = pos;

            // range fits current time selection?
            start_diff = abs(abs(starts[idx]) - abs(start_time));
            end_diff = abs(abs(ends[idx]) - abs(end_time));
            if (start_diff + end_diff < sel_approx)
            {
              sel_approx = start_diff + end_diff;
              sel = idx;

              // exact match? then select next or previous range depending on start_dir
              if (sel_approx < 0.002)
              {
                sel += start_dir;
              }
            }

            idx++;
          }

          // add region to list
          if (is_region) 
          {
            inside_region = true;
            curr_region_end = region_end;
            
            starts[idx] = pos;
            ends[idx] = region_end;

            // range fits current time selection? 
            start_diff = abs(abs(starts[idx]) - abs(start_time));
            end_diff = abs(abs(ends[idx]) - abs(end_time));
            if (start_diff + end_diff < sel_approx)
            {
              sel_approx = start_diff + end_diff;
              sel = idx;

              // exact match? then select next or previous range depending on start_dir
              if (sel_approx < 0.002)
              {
                sel += start_dir;
              }
            }

            idx++;
                      
          } 

          last_pos = pos;

        }

        // end reached, but still inside a region? do one last goround
        if ( (inside_region) && (idx <= 100) )
        {
          pos = curr_region_end;
          if ( 
            (dblEq(pos, last_pos, 0.001) == false) &&
            ( 
              (dblEq(last_pos, starts[idx-1], 0.001) == false) ||
              (dblEq(pos, ends[idx-1], 0.001) == false)
            ) )
          {
            starts[idx] = last_pos;
            ends[idx] = pos;

            // range fits current time selection?
            start_diff = abs(abs(starts[idx]) - abs(start_time));
            end_diff = abs(abs(ends[idx]) - abs(end_time));
            if (start_diff + end_diff < sel_approx)
            {
              sel_approx = start_diff + end_diff;
              sel = idx;

              // exact match? then select next or previous range depending on start_dir
              if (sel_approx < 0.002)
              {
                sel += start_dir;
              }
            }

            idx++;
          }
        }

        // clamp selection to boundaries of list, wrap around
        if (sel >= idx)
        {
          sel = 0;
        }

        if (sel < 0)
        {
          sel = idx - 1;
        }
 
        // set new time selection
        start_time = starts[sel];
        end_time = ends[sel];
        GetSet_LoopTimeRange(true, true, &start_time, &end_time, false);

        // clean up arrays
        delete [] starts;
        delete [] ends;

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

    MySetSurface_UpdateButton(0x72, s_loop_all, false); 

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



  ////// CONFIG STUFF FOR REGISTERING AND PREFERENCES //////

  const char *GetTypeString() 
  {
    return "US-2400";
  }
  

  const char *GetDescString()
  {
    descspace.Set(DESCSTRING);
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
    // midi processing
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
      if (s_touch_enc[i] > 0) s_touch_enc[i]--;


    // Execute fader/encoder updates
    char i = 0;
    char ex = 0;
    do 
    {
    
      if ((cache_upd_faders & (1 << cache_exec)) > 0) {
        MySetSurface_ExecuteFaderUpdate(cache_exec);
        ex += 2;
      }
      if ((cache_upd_enc & (1 << cache_exec)) > 0) {
        MySetSurface_ExecuteEncoderUpdate(cache_exec);
        ex++;
      }

      if (METERMODE && ((cache_upd_meters & (1 << cache_exec)) > 0)) {
        MySetSurface_ExecuteMeterUpdate(cache_exec);
        ex++;
      }
           
      // incr / wrap cache_exec
      cache_exec++;
      if (cache_exec > 24) cache_exec = 0;

      i++;
      
      // repeat loop until all channels checked or exlimit reached
    } while ((i < 25) && (ex < EXLIMIT));


    // meters
    if (METERMODE) MySetSurface_OutputMeters(false); // false = no reset


    // countdown m button delay, update if applicable
    if (q_mkey)
    {
      if (s_mkey_on > 0) {
        s_mkey_on--;
        
        // blink all participating buttons to indicate mkey mode
        bool mblnk = false;
        if (s_mkey_on % 2 == 0) mblnk = true;
        
        char mde = 0;
        if (m_chan) mde = 1;
        if (m_aux > 0) mde = 2;

        bool mact = false;
        
        // aux buttons / transport
        for (char b = 0; b < 6; b++)
        {
          if (cmd_ids[3][mde][b] != -1)
          {
            MySetSurface_UpdateButton(0x65 + b, mblnk, false);
            mact = true;
          }
          if ((b < 5) && (cmd_ids[3][mde][b+7] != -1))
          {
            MySetSurface_UpdateButton(0x75 + b, mblnk, false);
            mact = true;
          }
        }

        // null button
        if (cmd_ids[3][mde][6] != -1)
        {
          MySetSurface_UpdateButton(0x6e, mblnk, false);
          mact = true;
        }

        // meter button
        if (mact) MySetSurface_UpdateButton(0x6b, mblnk, false); // meter
      } 
      else
      {
        q_mkey = false;

        hlp_qkey = 0;
        Hlp_Update();

        //reset buttons
        MySetSurface_UpdateAuxButtons(); // aux
        MySetSurface_UpdateAutoLEDs(); // transport
        MySetSurface_UpdateButton(0x6e, false, false); // null
        MySetSurface_UpdateButton(0x6b, false, false); // meter
      }
    }


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
      bool on = ( ( ( (CountSelectedTracks(0) > 0) || IsTrackSelected(master) ) && s_myblink ) != IsTrackSelected(master) );
      MySetSurface_UpdateButton(97, on, false);

      // Update automation modes
      MySetSurface_UpdateAutoLEDs();

    } else
    {
      myblink_ctr++;
    }


    // update Strip Display
    if (stp_hwnd != NULL) 
    {

      stp_fdr_touch = s_touch_fdr;
      stp_enc_touch = 0;      
      for (char ch = 0; ch < 24; ch++)
      {
        if (m_chan && (s_touch_enc[ch] > 0)) stp_enc_touch = stp_enc_touch | (1 << ch);
       
        if ( (stp_enc_touch & (1 << ch)) != (stp_enc_touch_prev & (1 << ch)) 
          || (stp_fdr_touch & (1 << ch)) != (stp_fdr_touch_prev & (1 << ch)) )
            Stp_Update(ch);
      }
      
      stp_enc_touch_prev = stp_enc_touch;
      stp_fdr_touch_prev = stp_fdr_touch;

      if (stp_repaint)
      {
        InvalidateRect(stp_hwnd, NULL, true);
        UpdateWindow(stp_hwnd);
        stp_repaint = false;
      }
    }

    // check fx count if chan mode
    if (m_chan)
    {
      Utl_CheckFXInsert();
    }

    // init
    if (!s_initdone) 
    {
      s_initdone = MySetSurface_Init();
      if (stp_open == 1) Stp_OpenWindow();
    }
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
  DESCSTRING,
  createFunc,
  configFunc,
};