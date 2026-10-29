/* Sake - Atari Kernel Emulator (Pure C) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <exec/types.h>
#include <exec/memory.h>
#include <exec/tasks.h>
#include <exec/execbase.h>
#include <dos/dos.h>
#include <dos/dosextens.h>
#include <graphics/gfx.h>
#include <graphics/rastport.h>
#include <graphics/text.h>
#include <intuition/intuition.h>
#include <gadtools/gadtools.h>
#include <libraries/asl.h>

#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/asl.h>

#ifndef TRUE
#define TRUE -1
#endif
#ifndef FALSE
#define FALSE 0
#endif

typedef signed char BOOLEAN;

#define E_OK 0
#define E_ERROR -1
#define EDRVNR -3
#define EPRCFND -7
#define ENFHND -8
#define ELOCKD -10
#define ENSMEM -11
#define EIHND -12

#define GEMDOS_SEEK_START 0
#define GEMDOS_SEEK_CUR 1
#define GEMDOS_SEEK_END 2

#define AES_APPL 0
#define AES_EVNT 1
#define AES_RSRC 2
#define AES_MENU 3
#define AES_OBJC 4
#define AES_FORM 5
#define AES_SCRP 6
#define AES_FSEL 7
#define AES_WIND 8
#define AES_GRAF 9

#define WIN_NAME 1
#define WIN_CLOSER 2
#define WIN_FULLER 4
#define WIN_MOVER 8
#define WIN_SIZER 16
#define WIN_UPARROW 32
#define WIN_DNARROW 64
#define WIN_VSLIDE 128
#define WIN_LFARROW 256
#define WIN_RTARROW 512
#define WIN_HSLIDE 1024
#define WIN_SMALLER 2048
#define WIN_INFO 4096

#define WS_CLOSED 0
#define WS_OPEN 1
#define WS_ICONIFIED 2

#define MAX_WINDOWS 16
#define MAX_MENUS 8
#define MAX_OBJECTS 256
#define MAX_MESSAGES 64
#define MAX_APPS 8

#define M_ON 0
#define M_OFF 1
#define M_ARROW 2
#define M_BUSY 3
#define M_IBEAM 4
#define M_POINT 5
#define M_USERDEF 6
#define M_SPECIAL 7

#define VDI_REPLACE 1
#define VDI_TRANSPARENT 2
#define VDI_XOR 3
#define VDI_REVERSE 4

#define COOKIE_SIZE 8
#define COOKIE_JAR_MAX 64

struct GEMObject {
  short next, head, tail;
  unsigned short flags, state, type;
  long spec;
  short x, y, w, h;
};





// ---------------------------------------------------------------------------
// Global state
// ---------------------------------------------------------------------------


// GEMDOS constants for Seek mode mapping
const BOOLEAN GEMDOS_SEEK_START=0; const signed char GEMDOS_SEEK_CUR=1; const signed char GEMDOS_SEEK_END=2;

// Error code mapping
const BOOLEAN E_OK=0; const BOOLEAN E_ERROR=-1; const signed char EDRVNR=-3; const signed char EPRCFND=-7;
const signed char ENFHND=-8; const signed char ELOCKD=-10; const signed char ENSMEM=-11; const signed char EIHND=-12;


// ---------------------------------------------------------------------------
// GEM AES (Application Environment Services) dispatch
// Called via BIOS trap #2 with D0 = $C8
// Atari ST AES uses a parameter block in memory (control, global, int-in/out)
// A0 = pointer to AES parameter block (intin, ptin, intout, ptout, contrl, global)
// D0 = AES function code
// On AmigaOS, we map GEM windows/events to Intuition/gadtools
// ---------------------------------------------------------------------------

// AES function group codes
// ---------------------------------------------------------------------------
// GEM AES (Application Environment Services) dispatch and implementation
// Maps Atari GEM AES calls to AmigaOS Intuition
// ---------------------------------------------------------------------------

const BOOLEAN AES_APPL=0; const signed char AES_EVNT=1; const signed char AES_MENU=3; const signed char AES_OBJC=4;
const signed char AES_FORM=5; const signed char AES_SCRP=6; const signed char AES_FSEL=7; const signed char AES_WIND=8;
const signed char AES_GRAF=9;

// Window kinds (wind_create parameter)
const signed char WIN_NAME=1; const signed char WIN_CLOSER=2; const signed char WIN_FULLER=4; const signed char WIN_MOVER=8;
const signed char WIN_SIZER=16; const signed char WIN_UPARROW=32; const signed char WIN_DNARROW=64;
const short WIN_VSLIDE=128; const short WIN_LFARROW=256; const short WIN_RTARROW=512;
const short WIN_HSLIDE=1024; const short WIN_SMALLER=2048; const short WIN_INFO=4096;

// Window states
const BOOLEAN WS_CLOSED=0; const signed char WS_OPEN=1; const signed char WS_ICONIFIED=2;

// Window messages (for message queue)
const signed char WM_REDRAW=10; const signed char WM_TOPPED=11; const signed char WM_CLOSED=12;
const signed char WM_FULLED=13; const signed char WM_ARROWED=14; const signed char WM_HSLID=15;
const signed char WM_VSLID=16; const signed char WM_SIZED=17; const signed char WM_MOVED=18;
const signed char WM_UNTOPPED=19; const signed char WM_ONTOP=20;

// Max tracked resources
const signed char MAX_WINDOWS=16; const signed char MAX_MENUS=8; const short MAX_OBJECTS=256;
const signed char MAX_MESSAGES=64; const signed char MAX_APPS=8;


// ============================
// GRAF - Graphics services
// ============================

// Mouse shape constants
const BOOLEAN M_ON=0; const signed char M_OFF=1; const signed char M_ARROW=2; const signed char M_BUSY=3; const signed char M_IBEAM=4; const signed char M_POINT=5;
const signed char M_USERDEF=6; const signed char M_SPECIAL=7;

// VDI attribute constants
const signed char VDI_REPLACE=1; const signed char VDI_TRANSPARENT=2; const signed char VDI_XOR=3; const signed char VDI_REVERSE=4;

extern APTR oldTrapCode;
extern long ctx[32];    // Saved register context + extended params
extern long gem_handles[16]; // GEMDOS handle -> AmigaOS BPTR mapping
extern long gem_dta;                  // Disk Transfer Address (for Fsfirst/Fsnext)
extern long gem_drv;                  // Current drive (0=A:)
extern char gem_path[128]; // Current path
extern struct FileInfoBlock fib;       // FileInfoBlock for directory search
extern long gem_search_lock;          // Lock handle for current search
extern long gem_search_first;         // TRUE if we're inside a search
extern char gem_search_pattern[128]; // Pattern for search matching
extern long gem_search_attr;          // Attributes for search matching
extern char temp_string[256];
extern long bios_kb_shift;            // Keyboard shift state for BIOS Kbshift
extern long bios_vectors[256];        // Exception vectors table
extern void* gem_screen_mem;          // Virtual screen buffer pointer
long bios_vectors[256];        // Exception vectors table
void* gem_screen_mem = NULL;   // Virtual screen buffer pointer
extern long gem_aes_id;               // AES application ID counter
extern long gem_window_list[16]; // Open window handles (Intuition Window ptrs)
extern long gem_aes_global[32]; // AES global array
extern long gem_scrn_w; extern long gem_scrn_h;   // Virtual screen dimensions
static const char ver_tag[] = "\0$VER: Sake 1.0 (28.10.2026)";
extern long gadtools_base;            // gadtools.library base (opened at runtime)
extern long asl_base;                 // asl.library base (opened at runtime)

// Window info structure
extern long gem_wind_kind[MAX_WINDOWS];
extern long gem_wind_state[MAX_WINDOWS];
extern long gem_wind_x[MAX_WINDOWS];
extern long gem_wind_y[MAX_WINDOWS];
extern long gem_wind_w[MAX_WINDOWS];
extern long gem_wind_h[MAX_WINDOWS];
extern long gem_wind_work_x[MAX_WINDOWS];
extern long gem_wind_work_y[MAX_WINDOWS];
extern long gem_wind_work_w[MAX_WINDOWS];
extern long gem_wind_work_h[MAX_WINDOWS];
extern long gem_wind_full_x[MAX_WINDOWS];
extern long gem_wind_full_y[MAX_WINDOWS];
extern long gem_wind_full_w[MAX_WINDOWS];
extern long gem_wind_full_h[MAX_WINDOWS];
extern long gem_wind_parent[MAX_WINDOWS];
extern long gem_wind_handle[MAX_WINDOWS];

// Message queue for inter-application communication
extern long gem_msg_queue[MAX_MESSAGES];  // encoded messages (16-word packets)
extern long gem_msg_src[MAX_MESSAGES];    // source app ID
extern long gem_msg_len[MAX_MESSAGES];    // message length
extern long gem_msg_head; extern long gem_msg_tail;

// Application tracking
extern long gem_app_list[MAX_APPS]; // app IDs
extern long gem_app_count;

// Menu tracking
extern long gem_menu_tree[8]; // menu tree pointers
extern long gem_menu_owner[8]; // owning app ID
extern long gem_menu_count;
extern long gem_menu_bar_visible; // 0=none, 1=shown
extern long gem_menu_active_app; // app ID whose menu bar is showing
// Per-menu item state (max 64 items per menu tree, max 8 menus)
extern long gem_menu_item_count[8]; // item count per tree
extern long gem_menu_item_checked[8]; // bitmask of checked items
extern long gem_menu_item_enabled[8]; // bitmask of enabled items
extern long gem_menu_item_normal[8]; // bitmask of normal (non-inverted) items
extern long gem_menu_item_text[8]; // pointer array to text strings (simulated)
extern char gem_menu_item_text_buf[512]; // text storage buffer
extern long gem_menu_pending_action; // 0=none, 1=item_selected
extern long gem_menu_pending_item; // selected item index
extern long gem_menu_pending_tree; // selected menu tree
extern long gem_menu_pending_app; // app to notify

// Object tree tracking
extern long gem_obj_tree[8]; // object tree pointers

// Form state
extern long gem_form_active; // handle of active form dialog (-1 = none)

// Radio button gadget tracking
extern long gem_radio_gad[32]; // gadtools gadget pointers (up to 32 radio buttons)
extern long gem_radio_obj[32]; // GEM object index for each gadget
extern long gem_radio_count; // number of active radio gadgets
extern long gem_radio_tree; // tree pointer for current radio group
extern long gem_radio_win; // window gadgets are attached to

// Clipboard state
extern char gem_scrap_buffer[1024];
extern long gem_scrap_len;

// Graf (graphics) state
extern long gem_mouse_x; extern long gem_mouse_y;           // Current mouse position
extern long gem_mouse_buttons;                   // Button state (bit0=left, bit1=right)
extern long gem_mouse_kstate;                    // Keyboard shift state
extern long gem_mouse_shape;                     // Current cursor shape (M_ON..M_POINT)
extern long gem_mouse_visible;                   // 0=hidden, 1=visible
extern long gem_graf_wk_handle;                  // Workstation handle
extern long gem_graf_char_w; extern long gem_graf_char_h;    // Character cell size
extern long gem_graf_arrow_mode;                 // 0=menu nav, 1=normal arrows
extern long gem_graf_slidex[8];   // Slider positions
extern long gem_graf_slidey[8];
extern long gem_graf_accel_key;                  // Last accelerator key
// Mouse pointer data store (33 words: 16 mask + 16 data + 1 resolution/hotspot)
extern long gem_mouse_user_data[33];
extern long gem_mouse_user_hotx; extern long gem_mouse_user_hoty;
extern long gem_mouse_user_active;
// Predefined pointer shape bitmaps (16x16, 16 data words per shape)
extern long gem_mouse_arrow_data[16];
extern long gem_mouse_arrow_mask[16];
extern long gem_mouse_busy_data[16];
extern long gem_mouse_busy_mask[16];
extern long gem_mouse_ibeam_data[16];
extern long gem_mouse_ibeam_mask[16];
extern long gem_mouse_point_data[16];
extern long gem_mouse_point_mask[16];

// AES global arrays (as per GEM AES parameter block spec)
extern long gem_control[12]; // control array
extern long gem_intin[128];
extern long gem_intout[128];
extern long gem_ptrin[16];
extern long gem_pttout[16];


// ============================
// VDI - Virtual Device Interface
// ============================

extern long vdi_handle; // current workstation handle (-1 = none)
extern long vdi_work_w; extern long vdi_work_h; // workstation pixel dimensions
extern long vdi_dev_w; extern long vdi_dev_h; // device pixel dimensions
extern long vdi_n_planes; // bits per pixel
extern long vdi_line_type; extern long vdi_line_width; extern long vdi_line_color;
extern long vdi_fill_type; extern long vdi_fill_index; extern long vdi_fill_color;
extern long vdi_marker_type; extern long vdi_marker_height; extern long vdi_marker_color;
extern long vdi_text_font; extern long vdi_text_color; extern long vdi_text_rotation;
extern long vdi_wr_mode; // writing mode (1=replace, 2=transparent, 3=XOR, 4=reverseTransparent)
extern long vdi_clip_x; extern long vdi_clip_y; extern long vdi_clip_w; extern long vdi_clip_h;
extern long vdi_cur_x; extern long vdi_cur_y; // graphics cursor position

// VDI colour lookup (Atari ST standard 16-colour palette indexed by colour index)
extern char vdi_rgb[48]; // 16 colours x 3 bytes (R,G,B)

// VDI line type patterns (dash/dot definitions)
extern long vdi_line_pats[6];

// VDI pen colour to Amiga pen mapping (first 16 standard VDI colours)
extern long vdi_pen_map[16];
void vdi_init_pen_map();
void gem_SetVDIAttrs_RP(struct RastPort *rp);
long gem_GetVDIRastPort();


// ---------------------------------------------------------------------------
// Atari ST System Font Registration
// Registers the Atari ST character set as AmigaOS fonts via AddFont()
// 8x16 font for high-resolution (640x400), 8x8 font for low/medium
// Uses the Atari ST character encoding:
//   0x00-0x1F: special graphic chars (arrows, icons, music notes)
//   0x20-0x7E: standard ASCII (matching ISO-8859-1 in printable range)
//   0x7F:     solid block / alternate DEL
//   0x80-0xFF: extended chars (accented Latin, Greek, math, Hebrew)
// ---------------------------------------------------------------------------

// Font registration status
extern long gem_font_8x16_registered; extern long gem_font_8x8_registered;

// Atari ST 8x16 font bitmap data (256 chars × 16 bytes = 4096 bytes)
// Generated from Atari ST ROM glyph definitions
extern char gem_font_data_8x16[4096];

// Atari ST 8x8 font bitmap data (256 chars × 8 bytes = 2048 bytes)
extern char gem_font_data_8x8[2048];

// Character width tables (all 8 for fixed-width)
extern char gem_font_width_8x16[256];
extern char gem_font_width_8x8[256];

// Character location tables (WORD offsets into bitmap data)
extern char gem_font_loc_8x16[512];
extern char gem_font_loc_8x8[256];

// TextFont structures allocated on the heap
extern struct TextFont* gem_font_8x16;
extern struct TextFont* gem_font_8x8;
long gemdos_fopen_mode(long gem_mode);
long gemdos_to_amiga_seek(long gem_seek);
void gemdos_dispatch();
void gemdos_pterm0();
void gemdos_cconin();
void gemdos_cconout();
void gemdos_cconis();
void gemdos_cconos();
void gemdos_cconws();
void gemdos_cconrs();
void gemdos_dsetdrv();
void gemdos_dgetdrv();
void gemdos_dsetpath();
void gemdos_dgetpath();
void gemdos_fopen();
void gemdos_fclose();
void gemdos_fread();
void gemdos_fwrite();
void gemdos_fdelete();
void gemdos_fseek();
void gemdos_fattrib();
void gemdos_fdatime();
void gemdos_fsfirst();
void gemdos_fsnext();
long FileMatch(char* name, char* pattern);
void FillDTA(char* dta, struct FileInfoBlock* fib_ptr);
void gemdos_fsrename();
void gemdos_fmkdir();
void gemdos_frmdir();
void gemdos_fchdir();
void gemdos_fgetdta();
void gemdos_fsetdta();
void gemdos_malloc();
void gemdos_mfree();
void gemdos_mxalloc();
long load_prg(char* filename);
void gemdos_pexec();
void gemdos_pterm();
void gemdos_super();
void gemdos_tgetdate();
void gemdos_tsetdate();
void gemdos_tgettime();
void gemdos_tsettime();
void bios_dispatch();
void bios_getmpb();
void bios_gembp();
void bios_bconin();
void bios_bconout();
void bios_rwabs();
void bios_setexc();
void bios_tickcal();
void bios_bconstat();
void bios_mediac();
void bios_drvmap();
void bios_kbshift();
void bios_random();
void xbios_dispatch();
void xbios_initmouse();
void xbios_gettime();
void xbios_settime();
void xbios_bioskeys();
void xbios_kbrate();
void xbios_prtblk();
BOOLEAN gem_init_gadtools();
void gem_wind_alloc();
long gem_wind_find_handle(long h);
void gem_msg_push(long msg, long src, long len);
long gem_bit(long n);
void gem_msg_pop();
void gem_aes_dispatch();
void gem_appl_init();
void gem_appl_exit();
void gem_appl_read();
void gem_appl_write();
void gem_appl_find();
void gem_evnt_multi();
void gem_evnt_mesag();
void gem_evnt_button();
void gem_evnt_mouse();
void gem_evnt_keybd();
void gem_evnt_dclick();
void gem_evnt_timer();
void gem_menu_bar();
void gem_menu_icheck();
void gem_menu_ienable();
void gem_menu_tnormal();
void gem_menu_text();
void gem_menu_register();
void gem_menu_popup();
void gem_menu_attach();
void gem_menu_istart();
void gem_menu_settings();
void gem_objc_add();
void gem_objc_delete();
void gem_objc_draw();
void gem_objc_find();
void gem_objc_offset();
void gem_objc_order();
void gem_objc_edit();
void gem_objc_change();
void gem_objc_type();
void gem_form_do();
void gem_form_dial();
void gem_form_alert();
void gem_form_error();
void gem_form_center();
void gem_form_keybd();
void gem_form_button();
void gem_scrp_read();
void gem_scrp_write();
void gem_fsel_exinput();
void gem_fsel_exoutput();
void gem_wind_create();
void gem_wind_open();
void gem_wind_close();
void gem_wind_delete();
void gem_wind_get();
void gem_wind_set();
void gem_wind_find();
void gem_wind_update();
void gem_wind_calc();
void gem_wind_new();
void gem_wind_arrow();
void gem_wind_show();
void gem_wind_toolbar();
void gem_wind_sized();
void gem_mouse_init();
void gem_graf_rubberbox();
void gem_graf_dragbox();
void gem_graf_movebox();
void gem_graf_growbox();
void gem_graf_shrinkbox();
void gem_graf_watchbox();
void gem_graf_slidebox();
void gem_graf_handle();
void gem_graf_mkstate();
void gem_graf_mouse();
void gem_graf_arrow();
void gem_graf_set_screen();
void gem_graf_set_handle();
void gem_graf_accel();
void vdi_init_rgb();
void vdi_init_line_pats();
void vdi_opnvwk();
void vdi_clsvwk();
void vdi_clrwk();
void vdi_updwk();
void vdi_pline();
void vdi_pmarker();
void vdi_gtext();
void vdi_fillarea();
void vdi_bar();
void vdi_circle();
void vdi_ellipse();
void vdi_ellarc();
void vdi_ellpie();
void vdi_arc();
void vdi_pieslice();
void vdi_rbox();
void vdi_rfbox();
void vdi_justified();
void vdi_cellarray();
void vdi_bezier();
void vdi_qcolor();
void vdi_qcurpos();
void vdi_qcontxt();
void vdi_qextnd();
void vdi_qcellht();
void vdi_qcellwd();
void vdi_qchcells();
void vdi_qvgd();
void vdi_qkey_s();
void vdi_opnwk();
void vdi_set_line_type();
void vdi_set_line_width();
void vdi_set_line_color();
void vdi_set_fill_type();
void vdi_set_fill_index();
void vdi_set_fill_color();
void vdi_set_marker_type();
void vdi_set_marker_height();
void vdi_set_marker_color();
void vdi_set_text_font();
void vdi_set_text_color();
void vdi_set_text_rotation();
void vdi_set_writing_mode();
void vdi_set_clip_rect();
void vdi_set_clip_state();
void vdi_set_curpos();
void vdi_inq_fill_pats();
void gem_vdi_dispatch();
void gem_init_font_8x16();
void gem_init_font_8x8();
void gem_init_fonts();
int main(int argc, char** argv);
void xbios_cursconf();
void xbios_appl_init();
void xbios_physbase();
void xbios_logbase();
void xbios_getrez();
void xbios_setscreen();
void xbios_setpalette();
void xbios_setcolor();
void xbios_floprd();
void xbios_flopwr();
void xbios_flopfmt();
void xbios_flopstatus();
void xbios_rsconf();
void xbios_keytbl();
void xbios_random();
void xbios_cookieptr();

APTR oldTrapCode=(APTR) NULL;
APTR old_trap1 = (APTR) NULL;
APTR old_trap2 = (APTR) NULL;
APTR old_trap3 = (APTR) NULL;
APTR old_trap13 = (APTR) NULL;
APTR old_trap14 = (APTR) NULL;
unsigned char* trap_stub_mem = NULL;
long ctx[32];    // Saved register context + extended params
long gem_handles[16]; // GEMDOS handle -> AmigaOS BPTR mapping
long gem_dta;                  // Disk Transfer Address (for Fsfirst/Fsnext)
long gem_drv;                  // Current drive (0=A:)
char gem_path[128]; // Current path
struct FileInfoBlock fib;       // FileInfoBlock for directory search
long gem_search_lock;          // Lock handle for current search
long gem_search_first;         // TRUE if we're inside a search
char gem_search_pattern[128]; // Pattern for search matching
long gem_search_attr;          // Attributes for search matching
char temp_string[256];
long bios_kb_shift;            // Keyboard shift state for BIOS Kbshift
long gem_aes_id;               // AES application ID counter
long gem_window_list[16]; // Open window handles (Intuition Window ptrs)
long gem_aes_global[32]; // AES global array
long gem_scrn_w; long gem_scrn_h;   // Virtual screen dimensions
long gadtools_base;            // gadtools.library base (opened at runtime)
long asl_base;                 // asl.library base (opened at runtime)

// Window info structure
long gem_wind_kind[MAX_WINDOWS];
long gem_wind_state[MAX_WINDOWS];
long gem_wind_x[MAX_WINDOWS];
long gem_wind_y[MAX_WINDOWS];
long gem_wind_w[MAX_WINDOWS];
long gem_wind_h[MAX_WINDOWS];
long gem_wind_work_x[MAX_WINDOWS];
long gem_wind_work_y[MAX_WINDOWS];
long gem_wind_work_w[MAX_WINDOWS];
long gem_wind_work_h[MAX_WINDOWS];
long gem_wind_full_x[MAX_WINDOWS];
long gem_wind_full_y[MAX_WINDOWS];
long gem_wind_full_w[MAX_WINDOWS];
long gem_wind_full_h[MAX_WINDOWS];
long gem_wind_parent[MAX_WINDOWS];
long gem_wind_handle[MAX_WINDOWS];

// Message queue for inter-application communication
long gem_msg_queue[MAX_MESSAGES];  // encoded messages (16-word packets)
long gem_msg_src[MAX_MESSAGES];    // source app ID
long gem_msg_len[MAX_MESSAGES];    // message length
long gem_msg_head; long gem_msg_tail;

// Application tracking
long gem_app_list[MAX_APPS]; // app IDs
long gem_app_count;

// Menu tracking
long gem_menu_tree[8]; // menu tree pointers
long gem_menu_owner[8]; // owning app ID
long gem_menu_count;
long gem_menu_bar_visible; // 0=none, 1=shown
long gem_menu_active_app; // app ID whose menu bar is showing
// Per-menu item state (max 64 items per menu tree, max 8 menus)
long gem_menu_item_count[8]; // item count per tree
long gem_menu_item_checked[8]; // bitmask of checked items
long gem_menu_item_enabled[8]; // bitmask of enabled items
long gem_menu_item_normal[8]; // bitmask of normal (non-inverted) items
long gem_menu_item_text[8]; // pointer array to text strings (simulated)
char gem_menu_item_text_buf[512]; // text storage buffer
long gem_menu_pending_action; // 0=none, 1=item_selected
long gem_menu_pending_item; // selected item index
long gem_menu_pending_tree; // selected menu tree
long gem_menu_pending_app; // app to notify

// Object tree tracking
long gem_obj_tree[8]; // object tree pointers

// Form state
long gem_form_active; // handle of active form dialog (-1 = none)

// Radio button gadget tracking
long gem_radio_gad[32]; // gadtools gadget pointers (up to 32 radio buttons)
long gem_radio_obj[32]; // GEM object index for each gadget
long gem_radio_count; // number of active radio gadgets
long gem_radio_tree; // tree pointer for current radio group
long gem_radio_win; // window gadgets are attached to

// Clipboard state
char gem_scrap_buffer[1024];
long gem_scrap_len;

// Graf (graphics) state
long gem_mouse_x; long gem_mouse_y;           // Current mouse position
long gem_mouse_buttons;                   // Button state (bit0=left, bit1=right)
long gem_mouse_kstate;                    // Keyboard shift state
long gem_mouse_shape;                     // Current cursor shape (M_ON..M_POINT)
long gem_mouse_visible;                   // 0=hidden, 1=visible
long gem_graf_wk_handle;                  // Workstation handle
long gem_graf_char_w; long gem_graf_char_h;    // Character cell size
long gem_graf_arrow_mode;                 // 0=menu nav, 1=normal arrows
long gem_graf_slidex[8];   // Slider positions
long gem_graf_slidey[8];
long gem_graf_accel_key;                  // Last accelerator key
// Mouse pointer data store (33 words: 16 mask + 16 data + 1 resolution/hotspot)
long gem_mouse_user_data[33];
long gem_mouse_user_hotx; long gem_mouse_user_hoty;
long gem_mouse_user_active;
// Predefined pointer shape bitmaps (16x16, 16 data words per shape)
long gem_mouse_arrow_data[16];
long gem_mouse_arrow_mask[16];
long gem_mouse_busy_data[16];
long gem_mouse_busy_mask[16];
long gem_mouse_ibeam_data[16];
long gem_mouse_ibeam_mask[16];
long gem_mouse_point_data[16];
long gem_mouse_point_mask[16];

// AES global arrays (as per GEM AES parameter block spec)
long gem_control[12]; // control array
long gem_intin[128];
long gem_intout[128];
long gem_ptrin[16];
long gem_pttout[16];


// ============================
// VDI - Virtual Device Interface
// ============================

long vdi_handle; // current workstation handle (-1 = none)
long vdi_work_w; long vdi_work_h; // workstation pixel dimensions
long vdi_dev_w; long vdi_dev_h; // device pixel dimensions
long vdi_n_planes; // bits per pixel
long vdi_line_type; long vdi_line_width; long vdi_line_color;
long vdi_fill_type; long vdi_fill_index; long vdi_fill_color;
long vdi_marker_type; long vdi_marker_height; long vdi_marker_color;
long vdi_text_font; long vdi_text_color; long vdi_text_rotation;
long vdi_wr_mode; // writing mode (1=replace, 2=transparent, 3=XOR, 4=reverseTransparent)
long vdi_clip_x; long vdi_clip_y; long vdi_clip_w; long vdi_clip_h;
long vdi_cur_x; long vdi_cur_y; // graphics cursor position

// VDI colour lookup (Atari ST standard 16-colour palette indexed by colour index)
char vdi_rgb[48]; // 16 colours x 3 bytes (R,G,B)

// VDI line type patterns (dash/dot definitions)
long vdi_line_pats[6];

// VDI pen colour to Amiga pen mapping (first 16 standard VDI colours)
long vdi_pen_map[16];
void vdi_init_pen_map() {
  vdi_pen_map[0] = 0; vdi_pen_map[1] = 1; vdi_pen_map[2] = 2; vdi_pen_map[3] = 3;
  vdi_pen_map[4] = 4; vdi_pen_map[5] = 5; vdi_pen_map[6] = 6; vdi_pen_map[7] = 7;
  vdi_pen_map[8] = 8; vdi_pen_map[9] = 9; vdi_pen_map[10] = 10; vdi_pen_map[11] = 11;
  vdi_pen_map[12] = 12; vdi_pen_map[13] = 13; vdi_pen_map[14] = 14; vdi_pen_map[15] = 15;
}
long gem_GetVDIRastPort() {
  extern long gem_window_list[16];
  struct Window *win;
  int i;
  for (i = 0; i < 16; i++) {
    win = (struct Window *)gem_window_list[i];
    if (win) return (long)win->RPort;
  }
  return 0;
}


// ---------------------------------------------------------------------------
// Atari ST System Font Registration
// Registers the Atari ST character set as AmigaOS fonts via AddFont()
// 8x16 font for high-resolution (640x400), 8x8 font for low/medium
// Uses the Atari ST character encoding:
//   0x00-0x1F: special graphic chars (arrows, icons, music notes)
//   0x20-0x7E: standard ASCII (matching ISO-8859-1 in printable range)
//   0x7F:     solid block / alternate DEL
//   0x80-0xFF: extended chars (accented Latin, Greek, math, Hebrew)
// ---------------------------------------------------------------------------

// Font registration status
long gem_font_8x16_registered; long gem_font_8x8_registered;

// Atari ST 8x16 font bitmap data (256 chars × 16 bytes = 4096 bytes)
// Generated from Atari ST ROM glyph definitions
char gem_font_data_8x16[4096];

// Atari ST 8x8 font bitmap data (256 chars × 8 bytes = 2048 bytes)
char gem_font_data_8x8[2048];

// Character width tables (all 8 for fixed-width)
char gem_font_width_8x16[256];
char gem_font_width_8x8[256];

// Character location tables (WORD offsets into bitmap data)
char gem_font_loc_8x16[512];
char gem_font_loc_8x8[256];

// TextFont structures allocated on the heap
struct TextFont* gem_font_8x16=NULL;
struct TextFont* gem_font_8x8=NULL;

// ---------------------------------------------------------------------------
// AmigaOS mode mapping for Fopen
// GEMDOS: 0=read, 1=write, 2=read+write
// AmigaOS: MODE_OLDFILE=1005, MODE_NEWFILE=1006
// ---------------------------------------------------------------------------
long gemdos_fopen_mode(long gem_mode) {
  long result;
  if( gem_mode == 0) {
    result = 1005;
  } else {
    if( gem_mode == 1) {
      result = 1006;
    } else {
      if( gem_mode == 2) {
        result = 1006;
      } else {
        result = 1005;
      }
    }
  }
	return result;
}

long gemdos_to_amiga_seek(long gem_seek) {
  long result;
  if( gem_seek == GEMDOS_SEEK_START) {
    result = -1;
  } else {
    if( gem_seek == GEMDOS_SEEK_CUR) {
      result = 0;
    } else {
      if( gem_seek == GEMDOS_SEEK_END) {
        result = 1;
      } else {
        result = -1;
      }
    }
  }
	return result;
}

// ---------------------------------------------------------------------------
// GEMDOS dispatch - called from assembly trap handler
// ctx[0] = function number on entry, return value on exit
// ctx[1..7] = D1-D7 parameters
// ctx[8..14] = A0-A6 parameters
// ---------------------------------------------------------------------------
void gemdos_dispatch() {
  long fn;
  fn = (int)ctx[0];

  switch( fn) {

  case 0x00: gemdos_pterm0(); break;
  case 0x01: gemdos_cconin(); break;
  case 0x02: gemdos_cconout(); break;
  case 0x06: gemdos_cconws(); break;
  case 0x07: gemdos_cconis(); break;
  case 0x08: gemdos_cconos(); break;
  case 0x09: gemdos_cconws(); break;
  case 0x0A: gemdos_cconrs(); break;
  case 0x0B: gemdos_cconis(); break;
  case 0x0C: gemdos_cconin(); break;
  case 0x0E: gemdos_dsetdrv(); break;
  case 0x19: gemdos_dgetdrv(); break;
  case 0x1A: gemdos_fsetdta(); break;
  case 0x20: gemdos_super(); break;
  case 0x2A: gemdos_tgetdate(); break;
  case 0x2B: gemdos_tsetdate(); break;
  case 0x2C: gemdos_tgettime(); break;
  case 0x2D: gemdos_tsettime(); break;
  case 0x2F: gemdos_fgetdta(); break;
  case 0x31: gemdos_ptermres(); break;
  case 0x36: gemdos_dfree(); break;
  case 0x39: gemdos_fmkdir(); break;
  case 0x3A: gemdos_frmdir(); break;
  case 0x3B: gemdos_dsetpath(); break;
  case 0x3D: gemdos_fopen(); break;
  case 0x3E: gemdos_fclose(); break;
  case 0x3F: gemdos_fread(); break;
  case 0x40: gemdos_fwrite(); break;
  case 0x41: gemdos_fdelete(); break;
  case 0x42: gemdos_fseek(); break;
  case 0x43: gemdos_fattrib(); break;
  case 0x44: gemdos_mxalloc(); break;
  case 0x47: gemdos_dgetpath(); break;
  case 0x48: gemdos_malloc(); break;
  case 0x49: gemdos_mfree(); break;
  case 0x4B: gemdos_pexec(); break;
  case 0x4C: gemdos_pterm(); break;
  case 0x4E: gemdos_fsfirst(); break;
  case 0x4F: gemdos_fsnext(); break;
  case 0x56: gemdos_fsrename(); break;
  case 0x57: gemdos_fdatime(); break;
  case 0x164: gemdos_ssystem(); break;

  default:
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Pterm0 ($00) - Terminate with return code 0
// ---------------------------------------------------------------------------
void gemdos_pterm0() {
  ctx[0] = E_OK;
	return ;
  // In a real emulator, we'd clean up and exit
}


// ---------------------------------------------------------------------------
// Cconin ($01) - Read character from console (blocking, with echo)
// Cconin ($0C) - Read character with echo control
// D0 = character read
// ---------------------------------------------------------------------------
void gemdos_cconin() {
  char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  ch[0] = 0;
  Read(Input() ,(APTR) (unsigned long) ch ,1 );
  // Echo character
  Write(Output() ,(APTR) (unsigned long) ch ,1 );
  ctx[0] =  return (long)v; ;
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Cconout ($02) - Write character to console
// D1 = character to write
// ---------------------------------------------------------------------------
void gemdos_cconout() {
  char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  ch[0] =  return (unsigned char)v; ;
  Write(Output() ,(APTR) (unsigned long) ch ,1 );
  ctx[0] = E_OK;
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Cconis ($07, $0B) - Check console input status
// D0 = -1 if key pressed, 0 otherwise
// ---------------------------------------------------------------------------
void gemdos_cconis() {
  // Use WaitForChar with 0 timeout
  if( -(BOOLEAN)(0!=WaitForChar(Input() ,0 ))) { ctx[0] = -1 ;} else { ctx[0] = 0;}
	return ;
}


// ---------------------------------------------------------------------------
// Cconos ($08) - Check console output status
// Always returns TRUE on Amiga
// ---------------------------------------------------------------------------
void gemdos_cconos() {
  ctx[0] = -1;
	return ;
}


// ---------------------------------------------------------------------------
// Cconws ($09, $06) - Write null-terminated string to console
// A0 = pointer to string in emulated memory
// For now, the string is in the ctx saved as A0
// ---------------------------------------------------------------------------
void gemdos_cconws() {
  char* src=NULL; long len;
  src =  return (unsigned char*)v; ;
  // Copy string from emulated memory and write it
  len = 0;
  while( (- (src[len] != 0 )& len )< 255) {
    temp_string[len] = src[len];
    len = len + 1;
  }
  temp_string[len] = 0;
  Write(Output() ,(APTR) (unsigned long) temp_string ,len );
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Cconrs ($0A) - Read string from console with line editing
// A0 = pointer to buffer, first byte = max length
// ---------------------------------------------------------------------------
void gemdos_cconrs() {
  long maxlen; char* buf=NULL; char ch[2]; long pos, done;
  buf =  return (unsigned char*)v; ;
  maxlen = buf[0];
  pos = 0;
  done = FALSE;

  while( - (pos < maxlen )- 1 & ~ done) {
    Read(Input() ,(APTR) (unsigned long) ch ,1 );
    if( ((short) - (ch[0] == 13 )| ch[0] )== 10) {
      buf[pos + 1] = 0;
      done = TRUE;
    } else {
      if( ch[0] == 8) {
        if( pos > 0) {
          pos = pos - 1;
        }
      } else {
        buf[pos + 1] = ch[0];
        pos = pos + 1;
      }
    }
  }
  if( ~ done) {
    buf[1] = 0;
  }
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Dsetdrv ($0E) - Set default drive
// D1 = drive number (0=A:)
// ---------------------------------------------------------------------------
void gemdos_dsetdrv() {
  gem_drv = ctx[1];
  ctx[0] = 7;
	return ;
}


// ---------------------------------------------------------------------------
// Dgetdrv ($0F) - Get default drive
// D0 = current drive (0=A:)
// ---------------------------------------------------------------------------
void gemdos_dgetdrv() {
  ctx[0] = gem_drv;
	return ;
}


// ---------------------------------------------------------------------------
// Dsetpath ($10) - Set current path
// A0 = pointer to path string
// ---------------------------------------------------------------------------
void gemdos_dsetpath() {
  char* src=NULL; long i;
  src =  return (unsigned char*)v; ;
  i = 0;
  while( (- (src[i] != 0 )& i )< 127) {
    gem_path[i] = src[i];
    i = i + 1;
  }
  gem_path[i] = 0;
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Dgetpath ($11) - Get current path
// A0 = buffer for path string
// ---------------------------------------------------------------------------
void gemdos_dgetpath() {
  char* dst=NULL; long i;
  dst =  return (unsigned char*)v; ;
  i = 0;
  while( gem_path[i] != 0) {
    dst[i] = gem_path[i];
    i = i + 1;
  }
  dst[i] = 0;
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Fopen ($15) - Open file
// A0 = filename pointer, D1 = mode (0=read, 1=write, 2=read+write)
// D0 = file handle or negative error
// ---------------------------------------------------------------------------
void gemdos_fopen() {
  char* filename=NULL; long mode, handle, i, result;
  filename =  return (unsigned char*)v; ;
  mode = gemdos_fopen_mode(ctx[1]);
  handle = (long) Open(filename ,mode );
  if( handle) {
    result = ENFHND;
    for ( i = 3 ; i <= 15; i = i + 1) {
      if( gem_handles[i] == 0) {
        gem_handles[i] = handle;
        result = i;
        i = 15;
      }
    }
    if( result == ENFHND) {
      -(BOOLEAN)(0!=Close( return (BPTR)v;  ));
    }
    ctx[0] = result;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fclose ($16) - Close file
// D1 = file handle
// ---------------------------------------------------------------------------
void gemdos_fclose() {
  long handle;
  handle = ctx[1];
  if( (- (handle >= 3 )& handle )<= 15) {
    -(BOOLEAN)(0!=Close( return (BPTR)v;  ));
    gem_handles[handle] = 0;
    ctx[0] = E_OK;
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fread ($17) - Read from file
// D1 = handle, D2 = count, A0 = buffer
// D0 = bytes read or error
// ---------------------------------------------------------------------------
void gemdos_fread() {
  long handle, count; char* buf=NULL; long result;
  handle = ctx[1];
  count = ctx[2];
  buf =  return (unsigned char*)v; ;

  if( (- (handle >= 0 )& handle )<= 15) {
    if( handle == 0) {
      result = Read(Input() ,(APTR) (unsigned long) buf ,count );
    } else {
      result = Read( return (BPTR)v;  ,(APTR) (unsigned long) buf ,count );
    }
    if( (- (result == 0 )& count )> 0) {
      ctx[0] = E_ERROR;
    } else {
      ctx[0] = result;
    }
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fwrite ($18) - Write to file
// D1 = handle, D2 = count, A0 = buffer
// D0 = bytes written or error
// ---------------------------------------------------------------------------
void gemdos_fwrite() {
  long handle, count; char* buf=NULL; long result;
  handle = ctx[1];
  count = ctx[2];
  buf =  return (unsigned char*)v; ;

  if( (- (handle >= 0 )& handle )<= 15) {
    if( handle == 0) {
      result = Write(Input() ,(APTR) (unsigned long) buf ,count );
    } else {
      if( (- (handle == 1 )| handle )== 2) {
        result = Write(Output() ,(APTR) (unsigned long) buf ,count );
      } else {
        result = Write( return (BPTR)v;  ,(APTR) (unsigned long) buf ,count );
      }
    }
    if( (- (result == 0 )& count )> 0) {
      ctx[0] = E_ERROR;
    } else {
      ctx[0] = result;
    }
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fdelete ($19) - Delete file
// A0 = filename pointer
// ---------------------------------------------------------------------------
void gemdos_fdelete() {
  char* filename=NULL;
  filename =  return (unsigned char*)v; ;
  if( -(BOOLEAN)(0!=DeleteFile(filename ))) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fseek ($1A) - Seek in file
// D1 = handle, D2 = offset, D3 = mode (0=start, 1=current, 2=end)
// D0 = new position
// ---------------------------------------------------------------------------
void gemdos_fseek() {
  long handle, offset, mode, oldpos, newpos;
  handle = ctx[1];
  offset = ctx[2];
  mode = gemdos_to_amiga_seek(ctx[3]);

  if (handle >= 3 && handle <= 15) {
    if (gem_handles[handle] != 0) {
      oldpos = Seek((BPTR)gem_handles[handle], offset, mode);
      if (oldpos < 0) {
        ctx[0] = E_ERROR;
      } else {
        newpos = Seek((BPTR)gem_handles[handle], 0, 0);
        ctx[0] = newpos;
      }
    } else {
      ctx[0] = EIHND;
    }
  } else {
    ctx[0] = EIHND;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fattrib ($1B) - Get/set file attributes
// D1 = mode (0=get, 1=set), A0 = filename, D2 = new attrib (if set)
// D0 = attributes or error
// ---------------------------------------------------------------------------
void gemdos_fattrib() {
  long mode; char* filename=NULL;
  mode = ctx[1];
  filename =  return (unsigned char*)v; ;
  if( mode == 0) {
    // Get attributes - simplified, returns 0
    ctx[0] = 0;
  } else {
    // Set attributes via SetProtection
    ctx[0] = E_OK;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fdatime ($1E) - Get/set file date and time
// D1 = mode (0=get, 1=set), A0 = pointer to time struct, D2 = handle
// D0 = 0 or error
// ---------------------------------------------------------------------------
void gemdos_fdatime() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Fsfirst ($1F) - Find first matching file
// A0 = pattern string, D1 = attribute mask
// D0 = 0 if found, -1 if not
// ---------------------------------------------------------------------------
void gemdos_fsfirst() {
  char* pattern=NULL; long attr, i;
  char lock_name[256];
  pattern =  return (unsigned char*)v; ;
  attr = ctx[1];

  if( gem_search_lock) {
    UnLock( return (BPTR)v;  );
    gem_search_lock = 0;
  }

  i = 0;
  while( (- (pattern[i] != 0 )& i )< 127) {
    gem_search_pattern[i] = pattern[i];
    i = i + 1;
  }
  gem_search_pattern[i] = 0;

  -(BOOLEAN)(0!=GetCurrentDirName(lock_name ,255 ));
  gem_search_lock = (long) Lock(lock_name ,-2 );

  if( gem_search_lock) {
    if( -(BOOLEAN)(0!=Examine( return (BPTR)v;  ,& fib ))) {
      if( gem_dta != 0) {
        FillDTA( return (unsigned char*)v; , & fib);
      }
      gem_search_first = TRUE;
      ctx[0] = E_OK;
    } else {
      ctx[0] = E_ERROR;
    }
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


void gemdos_fsnext() {
  if( gem_search_lock) {
    if( -(BOOLEAN)(0!=ExNext( return (BPTR)v;  ,& fib ))) {
      if( gem_dta != 0) {
        FillDTA( return (unsigned char*)v; , & fib);
      }
      ctx[0] = E_OK;
    } else {
      ctx[0] = E_ERROR;
    }
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// FileMatch - Match filename against GEMDOS pattern (* and ?)
// Returns TRUE if name matches pattern
// ---------------------------------------------------------------------------
long FileMatch(char* name, char* pattern) {
  long i, j, result;
  i = 0;
  j = 0;
  result = TRUE;
  while( - (pattern[j] != 0 )& result) {
    if( (long) pattern[j] == (long) "*") {
      result = TRUE;
      j = 999;
    } else {
      if( (long) pattern[j] == (long) "?") {
        if( name[i] == 0 ) { result = FALSE;}
        i = i + 1;
      } else {
        if( name[i] != pattern[j] ) { result = FALSE;}
        i = i + 1;
      }
    }
    j = j + 1;
  }
  if( (long) ((short) - (name[i] != 0 )& pattern[j-1] )!= (long) "*") { result = FALSE;}
	return result;
}


// ---------------------------------------------------------------------------
// Helper: Fill GEMDOS DTA structure from AmigaOS FileInfoBlock
// ---------------------------------------------------------------------------
void FillDTA(char* dta, struct FileInfoBlock* fib_ptr) {
  long i;
  for (i = 0; i <= 43; i = i + 1) {
    dta[i] = 0;
  }
  dta[21] = 0;
  dta[26] = (unsigned char)((fib_ptr->fib_Size / 16777216) & 0xFF);
  dta[27] = (unsigned char)((fib_ptr->fib_Size / 65536) & 0xFF);
  dta[28] = (unsigned char)((fib_ptr->fib_Size / 256) & 0xFF);
  dta[29] = (unsigned char)((fib_ptr->fib_Size) & 0xFF);

  i = 0;
  while ((fib_ptr->fib_FileName[i] != 0) && (i < 12)) {
    dta[30 + i] = fib_ptr->fib_FileName[i];
    i = i + 1;
  }
  dta[30 + i] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Fsrename ($21) - Rename file
// A0 = old name, A1 = new name
// ---------------------------------------------------------------------------
void gemdos_fsrename() {
  char* oldname=NULL; char* newname=NULL;
  oldname =  return (unsigned char*)v; ;
  newname =  return (unsigned char*)v; ;

  if( -(BOOLEAN)(0!=Rename(oldname ,newname ))) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fmkdir ($22) - Create directory
// A0 = directory name
// ---------------------------------------------------------------------------
void gemdos_fmkdir() {
  char* dirname=NULL;
  dirname =  return (unsigned char*)v; ;
  if( CreateDir(dirname )) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Frmdir ($23) - Remove directory
// A0 = directory name
// ---------------------------------------------------------------------------
void gemdos_frmdir() {
  char* dirname=NULL;
  dirname =  return (unsigned char*)v; ;
  if( -(BOOLEAN)(0!=DeleteFile(dirname ))) {
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Fchdir ($24) - Change directory (same as Dsetpath behavior)
// A0 = path string
// ---------------------------------------------------------------------------
void gemdos_fchdir() {
  char* pathname=NULL;
  pathname =  return (unsigned char*)v; ;
  // Use Dsetpath logic
  gemdos_dsetpath();
	return ;
}


// ---------------------------------------------------------------------------
// Fgetdta ($25) - Get Disk Transfer Address
// D0 = DTA pointer
// ---------------------------------------------------------------------------
void gemdos_fgetdta() {
  ctx[0] =  return (long)v; ;
	return ;
}


// ---------------------------------------------------------------------------
// Fsetdta ($26) - Set Disk Transfer Address
// D1 = new DTA pointer
// ---------------------------------------------------------------------------
void gemdos_fsetdta() {
  long ptr;
  ptr = ctx[1];
  gem_dta = (long)  return (unsigned char*)v; ;
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Malloc ($29) - Allocate memory
// D1 = number of bytes
// D0 = pointer to memory, or 0 if failed
// ---------------------------------------------------------------------------
void gemdos_malloc() {
  long size, ptr;
  size = ctx[1];
  if (size == -1) {
    ctx[0] = 14 * 1024 * 1024;
    return;
  }
  // Allocate with MEMF_CLEAR for zero-initialized memory
  ptr = (long) AllocMem((ULONG) size ,(ULONG) 65538 );
  if( ptr == 0) {
    ctx[0] = 0;
  } else {
    ctx[0] =  return (long)v; ;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Mfree ($2A) - Free memory
// D1 = pointer to memory block
// ---------------------------------------------------------------------------
void gemdos_mfree() {
  long ptr;
  ptr = (long)  return (unsigned char*)v; ;
  // Note: AmigaOS FreeMem needs the size, which we don't know
  // In a real implementation, we'd track allocated block sizes
  // For now, free with a reasonable size or just stub
  FreeMem( return (APTR)v;  ,(ULONG) 0 );
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Mxalloc ($39) - Allocate memory with flags
// D1 = number of bytes, D2 = flags (bit 0 = FAST, bit 1 = CLEAR)
// D0 = pointer or 0
// ---------------------------------------------------------------------------
void gemdos_mxalloc() {
  long size, flags, memflags;
  size = ctx[1];
  if (size == -1) {
    ctx[0] = 14 * 1024 * 1024;
    return;
  }
  flags = ctx[2];
  memflags = 65538  ;// MEMF_CLEAR | MEMF_PUBLIC
  if( (flags & 1) == 0) {
    // CHIP memory requested, use MEMF_CHIP
    memflags = 65538  ;// Can't do chip on Amiga, use same
  }
  ctx[0] = (long) AllocMem((ULONG) size ,(ULONG) memflags );
	return ;
}


// ---------------------------------------------------------------------------
// Load an Atari ST PRG file into allocated memory
// Returns address of loaded program, or 0 on failure
// ---------------------------------------------------------------------------
long load_prg(char* filename) {
  BPTR fh=(BPTR) NULL; char header[32]; long result;
  long text_size, data_size, bss_size, sym_size, total_size;
  char* addr=NULL; long rel_flag;
  result = 0;

  fh = Open(filename, 1005);
  if (fh) {
    if (Read(fh, (APTR)(unsigned long)header, 32) >= 28) {
      text_size = ((unsigned char)header[2] << 24) | ((unsigned char)header[3] << 16) | ((unsigned char)header[4] << 8) | (unsigned char)header[5];
      data_size = ((unsigned char)header[6] << 24) | ((unsigned char)header[7] << 16) | ((unsigned char)header[8] << 8) | (unsigned char)header[9];
      bss_size = ((unsigned char)header[10] << 24) | ((unsigned char)header[11] << 16) | ((unsigned char)header[12] << 8) | (unsigned char)header[13];
      sym_size = ((unsigned char)header[14] << 24) | ((unsigned char)header[15] << 16) | ((unsigned char)header[16] << 8) | (unsigned char)header[17];
      rel_flag = ((unsigned char)header[26] << 8) | (unsigned char)header[27];
      total_size = text_size + data_size + bss_size;

      addr = (char*) AllocMem((ULONG)total_size, 65538);
      if (addr) {
        result = (long)addr;
        if (text_size > 0) {
          if (Read(fh, (APTR)(unsigned long)addr, text_size) < text_size) {
            result = 0;
          }
        }
        if ((data_size > 0) && (result != 0)) {
          if (Read(fh, (APTR)(unsigned long)((long)addr + text_size), data_size) < data_size) {
            result = 0;
          }
        }
        if (result != 0 && rel_flag == 0) {
          Seek(fh, 28 + text_size + data_size + sym_size, -1);
          unsigned char *base = (unsigned char *)result;
          unsigned char buf4[4];
          if (Read(fh, buf4, 4) == 4) {
            unsigned long offset = ((unsigned long)buf4[0] << 24) |
                                   ((unsigned long)buf4[1] << 16) |
                                   ((unsigned long)buf4[2] << 8)  |
                                    (unsigned long)buf4[3];
            if (offset != 0) {
              unsigned long td_size = text_size + data_size;
              unsigned long adj = (unsigned long)result;
              if (offset + 4 <= td_size) {
                unsigned char *p = base + offset;
                unsigned long val = ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | (unsigned long)p[3];
                val += adj;
                p[0] = (val >> 24) & 0xFF; p[1] = (val >> 16) & 0xFF; p[2] = (val >> 8) & 0xFF; p[3] = val & 0xFF;
              }
              while (1) {
                unsigned char b = 0;
                if (Read(fh, &b, 1) != 1 || b == 0) break;
                if (b == 1) {
                  offset += 254;
                } else {
                  offset += b;
                  if (offset + 4 <= td_size) {
                    unsigned char *p = base + offset;
                    unsigned long val = ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | (unsigned long)p[3];
                    val += adj;
                    p[0] = (val >> 24) & 0xFF; p[1] = (val >> 16) & 0xFF; p[2] = (val >> 8) & 0xFF; p[3] = val & 0xFF;
                  }
                }
              }
            }
          }
        }
        if (result == 0) {
          FreeMem((APTR)(unsigned long)addr, (ULONG)total_size);
        }
      }
    }
    Close(fh);
  }
  return result;
}


// ---------------------------------------------------------------------------
// Pexec ($2D) - Execute program
// D1 = mode (0=load, 1=load&go, 2=go, 3=load&createproc)
// A0 = filename (modes 0,1,3), A1 = command line (modes 1,3)
// D0 varies by mode
// ---------------------------------------------------------------------------
void gemdos_pexec() {
  long mode; char* filename=NULL;
  mode = ctx[1];
  filename =  return (unsigned char*)v; ;

  if( mode == 0) {
    ctx[0] = load_prg(filename);
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Pterm ($2E) - Terminate process with return code
// D1 = return code
// ---------------------------------------------------------------------------
void gemdos_pterm() {
  ctx[0] = ctx[1];
	return ;
  // In a real emulator, we'd clean up and exit the emulated process
}

void gemdos_ptermres() {
  ctx[0] = E_OK;
	return ;
}

void gemdos_dfree() {
  char* buf=NULL; long drv;
  buf = (char*)ctx[8];
  drv = ctx[1];
  if (buf != NULL) {
    unsigned char *p = (unsigned char *)buf;
    p[0] = 0; p[1] = 0; p[2] = 0x03; p[3] = 0xE8;
    p[4] = 0; p[5] = 0; p[6] = 0x07; p[7] = 0xD0;
    p[8] = 0; p[9] = 0; p[10] = 0x02; p[11] = 0;
    p[12] = 0; p[13] = 0; p[14] = 0; p[15] = 2;
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Super ($30) - Enter/exit supervisor mode
// D1 = 0: enter supervisor, D1 <> 0: exit supervisor
// D0 = old supervisor mode flag
// ---------------------------------------------------------------------------
void gemdos_super() {
  // On AmigaOS, the trap handler already runs in supervisor mode
  // So entering supervisor is a no-op
  if( ctx[1] == 0) {
    ctx[0] = -1  ;// Was already in supervisor mode
  } else {
    ctx[0] = 0   ;// Can't actually exit supervisor on Amiga
  }
	return ;
}


// ---------------------------------------------------------------------------
// Tgetdate ($31) - Get system date
// D0 = date in GEMDOS format (bit 0-4: day, bit 5-8: month, bit 9-15: year-1980)
// ---------------------------------------------------------------------------
void gemdos_tgetdate() {
  struct DateStamp now;
  DateStamp(& now );
  // GEMDOS date: bits 0-4=day, 5-8=month, 9-15=year-1980
  // Simple approximation from days since 1978
  ctx[0] = now.ds_Days / 365  * 512  + 1 * 32  + 1;
	return ;
}


// ---------------------------------------------------------------------------
// Tsetdate ($32) - Set system date
// D1 = date in GEMDOS format
// ---------------------------------------------------------------------------
void gemdos_tsetdate() {
  // On AmigaOS, we don't change the system date
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Tgettime ($33) - Get system time
// D0 = time in GEMDOS format (bit 0-4: sec/2, bit 5-10: min, bit 11-15: hour)
// ---------------------------------------------------------------------------
void gemdos_tgettime() {
  struct DateStamp now; long total_secs, hour, mins, sec;
  DateStamp(& now );
  total_secs = now.ds_Minute * 60  + now.ds_Tick / 50;
  hour = total_secs / 3600;
  mins = total_secs / 60  - hour * 60;
  sec = total_secs - hour * 3600  - mins * 60;
  ctx[0] = hour * 2048  + mins * 32  + sec / 2;
	return ;
}


// ---------------------------------------------------------------------------
// Tsettime ($34) - Set system time
// D1 = time in GEMDOS format
// ---------------------------------------------------------------------------
void gemdos_tsettime() {
  // On AmigaOS, we don't change the system time
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// BIOS (Trap #2) dispatch
// ctx[0] = function number, ctx[1]..ctx[7] = D1..D7, ctx[8]..ctx[14] = A0..A6
// ---------------------------------------------------------------------------
void bios_dispatch() {
  long fn;
  fn = (int)ctx[0];

  switch( fn) {

  case 0x00: bios_getmpb(); break;
  case 0x01: bios_bconin(); break;
  case 0x02: bios_bconout(); break;
  case 0x03: bios_rwabs(); break;
  case 0x04: bios_setexc(); break;
  case 0x05: bios_tickcal(); break;
  case 0x06: bios_gembp(); break;
  case 0x07: bios_bconstat(); break;
  case 0x08: bios_mediac(); break;
  case 0x09: bios_drvmap(); break;
  case 0x0A: bios_kbshift(); break;
  case 0x0B: bios_random(); break;

  case 0xC8: gem_aes_dispatch(); break;
  case 0xC9: gem_vdi_dispatch(); break;

  default:
    ctx[0] = E_ERROR;
    break;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Getmpb ($00) / Gembp ($06) - Get Memory Parameter Block
// A0 = pointer to MPB structure
// Returns memory layout info
// ---------------------------------------------------------------------------
void bios_getmpb() {
  long ptr = ctx[8];
  if (ptr != 0) {
    long *p = (long *)ptr;
    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
  }
  ctx[0] = E_OK;
	return ;
}

void bios_gembp() {
  bios_getmpb();
	return ;
}


// ---------------------------------------------------------------------------
// Bconin ($01) - Console input (blocking)
// D1 = device number (0=console, 1=RS232, 2=printer, etc.)
// D0 = character read
// ---------------------------------------------------------------------------
void bios_bconin() {
  long dev; char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  dev = ctx[1];
  if( dev == 0) {
    ch[0] = 0;
    Read(Input() ,(APTR) (unsigned long) ch ,1 );
    ctx[0] =  return (long)v; ;
  } else {
    ctx[0] = E_ERROR;
  }
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Bconout ($02) - Console output
// D1 = device, D2 = character
// ---------------------------------------------------------------------------
void bios_bconout() {
  long dev; char* ch=NULL;
  int temp_QUAD;
try {
	ch = NewString(1);
	temp_QUAD = exception ;
	exception = 0 ;
  dev = ctx[1];
  if( dev == 0) {
    ch[0] =  return (unsigned char)v; ;
    Write(Output() ,(APTR) (unsigned long) ch ,1 );
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
} catch(...) {}
	DisposeString(ch );
	if (exception!=0) {throw eException ;} else {EMPTY;};
	exception = temp_QUAD ;
	return ;
}


// ---------------------------------------------------------------------------
// Rwabs ($03) - Read/Write absolute sectors
// D0 = 0=read, 1=write
// D1 = device, D2 = sector number
// D3 = count, A0 = buffer
// ---------------------------------------------------------------------------
void bios_rwabs() {
  long rw, dev, sector, count; char* buf=NULL;
  rw =  return (int)v; ;
  dev = ctx[1];
  sector = ctx[2];
  count = ctx[3];
  buf =  return (unsigned char*)v; ;
  // Stub - no raw disk access on AmigaOS
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Setexc ($04) - Set exception vector
// D1 = exception number, A0 = handler address
// Returns previous handler address
// ---------------------------------------------------------------------------
void bios_setexc() {
  long vec, new_handler, old_handler;
  vec = ctx[1];
  new_handler = ctx[8];
  if (vec >= 0 && vec <= 255) {
    old_handler = bios_vectors[vec];
    if (new_handler != -1) {
      bios_vectors[vec] = new_handler;
    }
    ctx[0] = old_handler;
  } else {
    ctx[0] = 0;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Tickcal ($05) - Get tick calibration
// Returns number of microseconds per hardware tick (200Hz = 5000us on ST)
// In emulator, return 5000
// ---------------------------------------------------------------------------
void bios_tickcal() {
  ctx[0] = 5000;
	return ;
}


// ---------------------------------------------------------------------------
// Bconstat ($07) - Console status
// D1 = device
// D0 = -1 if ready, 0 otherwise
// ---------------------------------------------------------------------------
void bios_bconstat() {
  long dev;
  dev = ctx[1];
  if( dev == 0) {
    if( -(BOOLEAN)(0!=WaitForChar(Input() ,0 ))) { ctx[0] = -1 ;} else { ctx[0] = 0;}
  } else {
    ctx[0] = 0;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Mediac ($08) - Media change check
// D1 = device, D2 = media ID
// Returns media change status
// ---------------------------------------------------------------------------
void bios_mediac() {
  // Stub - assume media not changed
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Drvmap ($09) - Get drive map
// Returns bitmask of available drives (bit 0 = A:, bit 1 = B:, etc.)
// ---------------------------------------------------------------------------
void bios_drvmap() {
  // AmigaOS: assume at least drives A: and B: (but could check)
  // Return bit 0 and bit 1 set (drives A: and B:)
  ctx[0] = 3;
	return ;
}


// ---------------------------------------------------------------------------
// Kbshift ($0A) - Get/set keyboard shift state
// D1 = 0=read, 1=write, 2=read+set
// D2 = new shift state (for write)
// D0 = current shift state
// ---------------------------------------------------------------------------
void bios_kbshift() {
  long mode, new_state;
  mode = ctx[1];
  new_state = ctx[2];
  if( mode == 0) {
    ctx[0] = bios_kb_shift;
  } else {
    if( mode == 1) {
      bios_kb_shift = new_state;
      ctx[0] = bios_kb_shift;
    } else {
      if( mode == 2) {
        ctx[0] = bios_kb_shift;
        bios_kb_shift = new_state;
      } else {
        ctx[0] = 0;
      }
    }
  }
	return ;
}


// ---------------------------------------------------------------------------
// Random ($0B) - Get random number
// D0 = pseudo-random number (0-65535)
// ---------------------------------------------------------------------------
void bios_random() {
  // Use system timer low word as simple random
  struct DateStamp now;
  DateStamp(& now );
  ctx[0] = (now.ds_Days + now.ds_Minute )* 60 + now.ds_Tick;
	return ;
}


// ---------------------------------------------------------------------------
// XBIOS (Trap #3) dispatch
// ---------------------------------------------------------------------------
void xbios_dispatch() {
  long fn;
  fn = (int)ctx[0];

  switch( fn) {

  case 0x00: xbios_initmouse(); break;
  case 0x01: xbios_gettime(); break;
  case 0x02: xbios_settime(); break;
  case 0x03: xbios_bioskeys(); break;
  case 0x04: xbios_kbrate(); break;
  case 0x05: xbios_prtblk(); break;
  case 0x06: xbios_scrndump(); break;
  case 0x07: xbios_cursconf(); break;
  case 0x08: xbios_appl_init(); break;
  case 0x09: xbios_physbase(); break;
  case 0x0A: xbios_logbase(); break;
  case 0x0B: xbios_getrez(); break;
  case 0x0C: xbios_setscreen(); break;
  case 0x0D: xbios_setpalette(); break;
  case 0x0E: xbios_setcolor(); break;
  case 0x10: xbios_floprd(); break;
  case 0x11: xbios_flopwr(); break;
  case 0x12: xbios_flopfmt(); break;
  case 0x13: xbios_flopstatus(); break;
  case 0x17: xbios_rsconf(); break;
  case 0x18: xbios_keytbl(); break;
  case 0x19: xbios_random(); break;
  case 0x1E: xbios_cookieptr(); break;

  default:
    ctx[0] = E_ERROR;
    break;
  }
	return ;
}


// ---------------------------------------------------------------------------
// Initmouse ($00) - Initialize mouse
// D1 = parameters
// ---------------------------------------------------------------------------
void xbios_initmouse() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Gettime ($01) - Get system time (VBL count)
// D0 = longword VBL count (ticks since reset)
// ---------------------------------------------------------------------------
void xbios_gettime() {
  struct DateStamp now;
  DateStamp(& now );
  // Convert to VBL count (50Hz approximate)
  // Use days * 24 * 3600 * 50 + minutes * 60 * 50 + ticks * 50 / (50*60*24*365)
  // Simplified: just use a rough counter
  ctx[0] = ((now.ds_Days * 4320000 + now.ds_Minute )* 3000 + now.ds_Tick )* 50 / 300;
	return ;
}


// ---------------------------------------------------------------------------
// Settime ($02) - Set system time (VBL count)
// D1 = VBL count
// ---------------------------------------------------------------------------
void xbios_settime() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Bioskeys ($03) - Get BIOS key table
// A0 = pointer to key table buffer
// ---------------------------------------------------------------------------
void xbios_bioskeys() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Kbrate ($04) - Set keyboard repeat rate
// D1 = repeat rate
// ---------------------------------------------------------------------------
void xbios_kbrate() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Prtblk ($05) - Set printer block
// A0 = printer block
// ---------------------------------------------------------------------------
void xbios_prtblk() {
  ctx[0] = E_OK;
	return ;
}

// ---------------------------------------------------------------------------
// Open gadtools.library with fallback to gadtools13.library
// Returns 1 on success, 0 on failure
// ---------------------------------------------------------------------------
BOOLEAN gem_init_gadtools() {
  gadtools_base = 0;
   extern unsigned long gadtools_base; gadtools_base = (unsigned long)OpenLibrary("gadtools.library", 0); ;
  if( gadtools_base == 0) {
     extern unsigned long gadtools_base; gadtools_base = (unsigned long)OpenLibrary("gadtools13.library", 0); ;
  }
	return - (gadtools_base != 0);
}

// Find a free window slot, returns handle or -1 via ctx[0]
void gem_wind_alloc() {
  long i, result;
  result = -1;
  for ( i = 0 ; i <= MAX_WINDOWS - 1; i = i + 1) {
    if( (- (gem_wind_state[i] == WS_CLOSED )& gem_wind_handle[i] )== 0) {
      gem_wind_handle[i] = i + 1;
      gem_wind_state[i] = WS_CLOSED;
      result = gem_wind_handle[i];
      i = MAX_WINDOWS;
    }
  }
  ctx[0] = result;
	return ;
}

// Find window slot by handle, returns index via ENDPROC
long gem_wind_find_handle(long h) {
  long i, result;
  result = -1;
  for ( i = 0 ; i <= MAX_WINDOWS - 1; i = i + 1) {
    if( (- (result == -1 )& gem_wind_handle[i] )== h) {
      result = i;
    }
  }
	return result;
}

// Push a message onto the message queue
void gem_msg_push(long msg, long src, long len) {
  gem_msg_queue[gem_msg_tail] = msg;
  gem_msg_src[gem_msg_tail] = src;
  gem_msg_len[gem_msg_tail] = len;
  gem_msg_tail = gem_msg_tail + 1;
  if( gem_msg_tail >= MAX_MESSAGES) {
    gem_msg_tail = 0;
  }
	return ;
}

// Return bitmask with bit 'n' set (n=0..15)
long gem_bit(long n) {
  long bits[16]; long result;
  bits[0] = 1; bits[1] = 2; bits[2] = 4; bits[3] = 8;
  bits[4] = 16; bits[5] = 32; bits[6] = 64; bits[7] = 128;
  bits[8] = 256; bits[9] = 512; bits[10] = 1024; bits[11] = 2048;
  bits[12] = 4096; bits[13] = 8192; bits[14] = 16384; bits[15] = 32768;
  result = 0;
  if( (- (n >= 0 )& n )<= 15) {
    result = bits[n];
  }
	return result;
}

// Pop a message from the queue
void gem_msg_pop() {
  if( gem_msg_head != gem_msg_tail) {
    ctx[0] = gem_msg_queue[gem_msg_head];
    ctx[1] = gem_msg_src[gem_msg_head];
    ctx[2] = gem_msg_len[gem_msg_head];
    gem_msg_head = gem_msg_head + 1;
    if( gem_msg_head >= MAX_MESSAGES) {
      gem_msg_head = 0;
    }
  } else {
    ctx[0] = 0;
  }
	return ;
}


// ---------------------------------------------------------------------------
// AES dispatch
// ---------------------------------------------------------------------------
void gem_aes_dispatch() {
  long fn_group, fn_sub;

  fn_group = ctx[1];
  fn_sub = ctx[2];

  switch( fn_group) {

  case AES_APPL:
    switch( fn_sub) {
    case 0: gem_appl_init(); break;
    case 1: gem_appl_exit(); break;
    case 2: gem_appl_read(); break;
    case 3: gem_appl_write(); break;
    case 4: gem_appl_find(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_EVNT:
    switch( fn_sub) {
    case 0: gem_evnt_multi(); break;
    case 1: gem_evnt_mesag(); break;
    case 2: gem_evnt_button(); break;
    case 3: gem_evnt_mouse(); break;
    case 4: gem_evnt_keybd(); break;
    case 5: gem_evnt_dclick(); break;
    case 6: gem_evnt_timer(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_MENU:
    switch( fn_sub) {
    case 0: gem_menu_bar(); break;
    case 1: gem_menu_icheck(); break;
    case 2: gem_menu_ienable(); break;
    case 3: gem_menu_tnormal(); break;
    case 4: gem_menu_text(); break;
    case 5: gem_menu_register(); break;
    case 6: gem_menu_popup(); break;
    case 7: gem_menu_attach(); break;
    case 8: gem_menu_istart(); break;
    case 9: gem_menu_settings(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_OBJC:
    switch( fn_sub) {
    case 0: gem_objc_add(); break;
    case 1: gem_objc_delete(); break;
    case 2: gem_objc_draw(); break;
    case 3: gem_objc_find(); break;
    case 4: gem_objc_offset(); break;
    case 5: gem_objc_order(); break;
    case 6: gem_objc_edit(); break;
    case 7: gem_objc_change(); break;
    case 8: gem_objc_type(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_FORM:
    switch( fn_sub) {
    case 0: gem_form_do(); break;
    case 1: gem_form_dial(); break;
    case 2: gem_form_alert(); break;
    case 3: gem_form_error(); break;
    case 4: gem_form_center(); break;
    case 5: gem_form_keybd(); break;
    case 6: gem_form_button(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_SCRP:
    switch( fn_sub) {
    case 0: gem_scrp_read(); break;
    case 1: gem_scrp_write(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_FSEL:
    switch( fn_sub) {
    case 0: gem_fsel_exinput(); break;
    case 1: gem_fsel_exoutput(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_WIND:
    switch( fn_sub) {
    case 0: gem_wind_create(); break;
    case 1: gem_wind_open(); break;
    case 2: gem_wind_close(); break;
    case 3: gem_wind_delete(); break;
    case 4: gem_wind_get(); break;
    case 5: gem_wind_set(); break;
    case 6: gem_wind_find(); break;
    case 7: gem_wind_update(); break;
    case 8: gem_wind_calc(); break;
    case 9: gem_wind_new(); break;
    case 10: gem_wind_arrow(); break;
    case 11: gem_wind_show(); break;
    case 12: gem_wind_toolbar(); break;
    case 13: gem_wind_sized(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  case AES_GRAF:
    switch( fn_sub) {
    case 0: gem_graf_rubberbox(); break;
    case 1: gem_graf_dragbox(); break;
    case 2: gem_graf_movebox(); break;
    case 3: gem_graf_growbox(); break;
    case 4: gem_graf_shrinkbox(); break;
    case 5: gem_graf_watchbox(); break;
    case 6: gem_graf_slidebox(); break;
    case 7: gem_graf_handle(); break;
    case 8: gem_graf_mkstate(); break;
    case 9: gem_graf_mouse(); break;
    case 10: gem_graf_arrow(); break;
    case 11: gem_graf_set_screen(); break;
    case 12: gem_graf_set_handle(); break;
    case 13: gem_graf_accel(); break;
    default: ctx[0] = E_ERROR;
    }
    break;

  default:
    ctx[0] = E_ERROR;
  }
	return ;
}


// ---------------------------------------------------------------------------
// GEM AES function implementations
// Each group implements the GEM AES spec mapped to AmigaOS Intuition
// ---------------------------------------------------------------------------

// ============================
// APPL - Application services
// ============================

void gem_appl_init() {
  long ap_id;
  gem_aes_id = gem_aes_id + 1;
  ap_id = gem_aes_id;
  if( gem_app_count < MAX_APPS) {
    gem_app_list[gem_app_count] = ap_id;
    gem_app_count = gem_app_count + 1;
  }
  ctx[0] = ap_id;
	return ;
}

void gem_appl_exit() {
  long i, id;
  id = gem_aes_id;
  for ( i = 0 ; i <= gem_app_count - 1; i = i + 1) {
    if( gem_app_list[i] == id) {
      gem_app_list[i] = gem_app_list[gem_app_count - 1];
      gem_app_count = gem_app_count - 1;
      i = gem_app_count;
    }
  }
  gem_aes_id = 0;
  ctx[0] = 1;
	return ;
}

void gem_appl_read() {
  long mg_timeout;
  mg_timeout = ctx[3];
  gem_msg_pop();
	return ;
}

void gem_appl_write() {
  long dest_id, msg_len, msg_data;
  dest_id = ctx[1];
  msg_len = ctx[2];
  if( gem_msg_tail != gem_msg_head) {
    gem_msg_push(dest_id * 256  + msg_len, gem_aes_id, msg_len);
    ctx[0] = 1;
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_appl_find() {
  // Search known applications by name (parm on addr stack)
  // Without a real app registry, return -1 (not found)
  ctx[0] = -1;
	return ;
}


// ============================
// EVNT - Event services
// ============================

void gem_evnt_multi() {
  long flags, bclk, bmsk, bst;
  long m1flags, m1x, m1y, m1w, m1h;
  long m2flags, m2x, m2y, m2w, m2h;
  long timer, mg_time, messag;
  long i, result, done;

  flags = ctx[3]; bclk = ctx[4]; bmsk = ctx[5]; bst = ctx[6];
  m1flags = ctx[7]; m1x = ctx[8]; m1y = ctx[9]; m1w = ctx[10]; m1h = ctx[11];
  m2flags = ctx[12]; m2x = ctx[13]; m2y = ctx[14]; m2w = ctx[15]; m2h = ctx[16]; mg_time = ctx[18]; messag = ctx[19];

  result = 0;
  done = 0;

  // Check message queue first (highest priority)
  if( (- (done == 0 )& gem_msg_head )!= gem_msg_tail) {
    gem_msg_pop();
    result = 3;
    done = 1;
  }

  // Check button event
  if( (- (done == 0 )& (flags & 1) )!= 0) {
    result = 1;
    ctx[1] = 2;
    ctx[2] = 320;
    ctx[3] = 200;
    ctx[4] = 320;
    ctx[5] = 200;
    ctx[6] = 0;
    ctx[7] = 0;
    done = 1;
  }

  // Check timer
  if( (- (done == 0 )& (flags & 16) )!= 0) {
    result = 3;
    done = 1;
  }

  // Check mouse rectangle 1
  if( (- (done == 0 )& (flags & 32) )!= 0) {
  }

  // Default: no event or message
  ctx[0] = result;
	return ;
}

void gem_evnt_mesag() {
  gem_msg_pop();
	return ;
}

void gem_evnt_button() {
  long flag, bclk, bmsk, bst;
  flag = ctx[3];
  bclk = ctx[4];
  bmsk = ctx[5];
  bst = ctx[6];
  // Return immediate button event
  ctx[0] = 1;
  ctx[1] = 2    ;// double-click (unused)
  ctx[2] = 320  ;// x
  ctx[3] = 200  ;// y
	return ;
}

void gem_evnt_mouse() {
  long flag, mx, my, mw, mh;
  flag = ctx[3];
  mx = ctx[4]; my = ctx[5]; mw = ctx[6]; mh = ctx[7];
  // Assume mouse is in rectangle
  ctx[0] = 1;
  ctx[1] = 320;
  ctx[2] = 200;
	return ;
}

void gem_evnt_keybd() {
  // Return no key event
  ctx[0] = 0;
  ctx[1] = 0  ;// keycode
  ctx[2] = 0  ;// shift state
	return ;
}

void gem_evnt_dclick() {
  long new_dclick, set_flag;
  set_flag = ctx[3];
  new_dclick = ctx[4];
  // If set_flag is 1, set the new double-click rate; return old
  ctx[0] = 5;
	return ;
}

void gem_evnt_timer() {
  // Return immediately (0 = timer expired)
  ctx[0] = 0;
	return ;
}


// ============================
// MENU - Menu services
// ============================

void gem_menu_bar() {
  long tree, show, i;
  tree = ctx[3];
  show = ctx[4];
  if( show) {
    gem_menu_bar_visible = 1;
    gem_menu_active_app = gem_aes_id;
    // Find the menu tree owned by this app
    for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
      if( gem_menu_owner[i] == gem_aes_id) {
        gem_menu_pending_tree = gem_menu_tree[i];
      }
    }
  } else {
    gem_menu_bar_visible = 0;
    gem_menu_active_app = 0;
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_icheck() {
  long tree, item, check, i, idx;
  tree = ctx[3];
  item = ctx[4];
  check = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    if( check) {
      gem_menu_item_checked[idx] = gem_menu_item_checked[idx] | gem_bit(item);
    } else {
      gem_menu_item_checked[idx] = gem_menu_item_checked[idx] & 65535 - gem_bit(item);
    }
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_ienable() {
  long tree, item, enable, i, idx;
  tree = ctx[3];
  item = ctx[4];
  enable = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    if( enable) {
      gem_menu_item_enabled[idx] = gem_menu_item_enabled[idx] | gem_bit(item);
    } else {
      gem_menu_item_enabled[idx] = gem_menu_item_enabled[idx] & 65535 - gem_bit(item);
    }
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_tnormal() {
  long tree, item, normal, i, idx;
  tree = ctx[3];
  item = ctx[4];
  normal = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    if( normal == 0) {
      gem_menu_item_normal[idx] = gem_menu_item_normal[idx] | gem_bit(item);
    } else {
      gem_menu_item_normal[idx] = gem_menu_item_normal[idx] & 65535 - gem_bit(item);
    }
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_text() {
  long tree, item, text_ptr, i, idx;
  tree = ctx[3];
  item = ctx[4];
  text_ptr = ctx[5];
  idx = -1;
  for ( i = 0 ; i <= gem_menu_count - 1; i = i + 1) {
    if( gem_menu_tree[i] == tree) {
      idx = i;
      i = gem_menu_count;
    }
  }
  if( (- ((- (idx >= 0 )& item )>= 0 )& item )< 64) {
    gem_menu_item_text[idx] = text_ptr;
  }
  ctx[0] = 1;
	return ;
}

void gem_menu_register() {
  long pid, tree;
  pid = ctx[3];
  tree = ctx[4];
  if( gem_menu_count < 8) {
    gem_menu_tree[gem_menu_count] = tree;
    gem_menu_owner[gem_menu_count] = pid;
    gem_menu_item_count[gem_menu_count] = 0;
    gem_menu_item_checked[gem_menu_count] = 0;
    gem_menu_item_enabled[gem_menu_count] = 0;
    gem_menu_item_normal[gem_menu_count] = 0;
    gem_menu_item_text[gem_menu_count] = 0;
    gem_menu_count = gem_menu_count + 1;
  }
  ctx[0] = gem_menu_count;
	return ;
}

void gem_menu_popup() {
  long menu_id, x, y;
  menu_id = ctx[3];
  x = ctx[4];
  y = ctx[5];
  // If there's a pending menu action, return it
  if( (- (gem_menu_pending_action == 1 )& gem_menu_pending_app )== gem_aes_id) {
    ctx[1] = gem_menu_pending_item;
    gem_menu_pending_action = 0;
    ctx[0] = 1;
  } else {
    // No selection made - return 0 (cancelled)
    ctx[1] = 0;
    ctx[0] = 0;
  }
	return ;
}

void gem_menu_attach() {
  long pid, tree, item, child_tree;
  pid = ctx[3];
  tree = ctx[4];
  item = ctx[5];
  child_tree = ctx[6];
  ctx[0] = 1;
	return ;
}

void gem_menu_istart() {
  long pid, tree, item;
  pid = ctx[3];
  tree = ctx[4];
  item = ctx[5];
  // Mark that user is interacting with menu item
  ctx[0] = 1;
	return ;
}

void gem_menu_settings() {
  long pid, tree, item, settings;
  pid = ctx[3];
  tree = ctx[4];
  item = ctx[5];
  settings = ctx[6];
  // Store menu settings flags
  ctx[0] = 1;
	return ;
}


// ============================
// OBJC - Object services
// ============================

void gem_objc_add() {
  long tree, parent, child;
  tree = ctx[3];
  parent = ctx[4];
  child = ctx[5];
  ctx[0] = 1;
	return ;
}

void gem_objc_delete() {
  ctx[0] = 1;
	return ;
}

void gem_objc_draw() {
  long tree, obj, depth;
  long xc, yc, wc, hc;
  tree = ctx[3];
  obj = ctx[4];
  depth = ctx[5];
  xc = ctx[6]; yc = ctx[7]; wc = ctx[8]; hc = ctx[9];
  // Typically triggers object redraw
  ctx[0] = 1;
	return ;
}

void gem_objc_find() {
  long tree, obj, depth, mx, my;
  long found = 0;
  tree = ctx[3]; obj = ctx[4]; depth = ctx[5];
  mx = ctx[6]; my = ctx[7];
  if (tree != 0) {
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    int start = (int)obj;
    int i;
    for (i = start; objtree[i].type != 0; i++) {
      if (objtree[i].type == 21 || objtree[i].type == 22 || objtree[i].type == 6) {
        if (mx >= objtree[i].x && mx < objtree[i].x + objtree[i].w &&
            my >= objtree[i].y && my < objtree[i].y + objtree[i].h) {
          found = i;
        }
      }
    }
  }
  ctx[0] = found;
	return ;
}

void gem_objc_offset() {
  long tree, obj;
  tree = ctx[3]; obj = ctx[4];
  // Return offset of object within tree
  ctx[1] = 0;
  ctx[2] = 0;
  ctx[0] = E_OK;
	return ;
}

void gem_objc_order() {
  ctx[0] = 1;
	return ;
}

void gem_objc_edit() {
  // Returns keystroke or 0
  ctx[0] = 0;
	return ;
}

void gem_objc_change() {
  long tree, obj, depth, new_state;
  tree = ctx[3]; obj = ctx[4]; depth = ctx[5]; new_state = ctx[9];
  if (tree != 0 && obj >= 0) {
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    int idx = (int)obj;
    objtree[idx].state = (unsigned short)((int)new_state & 0xFFFF);
  }
  ctx[0] = 1;
	return ;
}

void gem_objc_type() {
  ctx[0] = E_OK;
	return ;
}


// ============================
// FORM - Form services
// ============================

void gem_form_do() {
  long tree, startobj, obj_ptr, key;
  long form_x, form_y, form_w, form_h;
  long win_idx, win_h;
  long done, event_obj, i;
  tree = ctx[3]; startobj = ctx[4];
  gem_form_active = tree;
  if (tree == 0) {
    gem_form_active = -1;
    ctx[0] = startobj;
    return;
  }
  obj_ptr = tree;
  form_x = 0; form_y = 0; form_w = 200; form_h = 100;
  {
    unsigned char *obj = (unsigned char *)obj_ptr;
    short ot = *(short *)(obj + 6);
    if (ot >= 0 && ot <= 3) {
      form_x = *(short *)(obj + 12);
      form_y = *(short *)(obj + 14);
      form_w = *(short *)(obj + 16);
      form_h = *(short *)(obj + 18);
    }
  }
  win_h = 0;
  for (i = 0; i < 16; i++) {
    if (win_h == 0 && gem_window_list[i] != 0 && gem_wind_state[i] == 1)
      win_h = gem_wind_handle[i];
  }
  gem_radio_count = 0;
  if (win_h != 0) {
    win_idx = gem_wind_find_handle(win_h);
    if (win_idx >= 0) {
      // Create radio button gadgets from the tree
      {
        extern unsigned long gadtools_base;
        struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
        struct GEMObject *objtree = (struct GEMObject *)tree;
        struct Window *winp = (struct Window *)gem_window_list[win_idx];
        struct Gadget *last_gad = NULL;
        struct NewGadget ng;
        int gi, count = 0;
        if (gadtools_base && winp) {
          for (gi = 0; objtree[gi].type != 0 && count < 32; gi++) {
            if (objtree[gi].type == 21) { /* G_RBUTTON */
              ng.ng_LeftEdge  = objtree[gi].x;
              ng.ng_TopEdge   = objtree[gi].y;
              ng.ng_Width     = objtree[gi].w;
              ng.ng_Height    = objtree[gi].h;
              ng.ng_VisualInfo = GetVisualInfo(winp->WScreen, NULL);
              ng.ng_TextAttr   = &winp->WScreen->RastPort->Font->tf_Attr;
              ng.ng_Flags      = 0;
              ng.ng_GadgetText = (STRPTR)objtree[gi].spec;
              ng.ng_GadgetID   = gi;
              ng.ng_UserData   = (APTR)0;
              gem_radio_gad[count] = (long)CreateGadgetA(RADIO_KIND, last_gad, &ng, NULL);
              if (gem_radio_gad[count]) {
                gem_radio_obj[count] = gi;
                if (objtree[gi].state & 1) {
                  GT_SetGadgetAttrs((struct Gadget *)gem_radio_gad[count], winp, NULL,
                    GTCB_Checked, 1, TAG_DONE);
                }
                last_gad = (struct Gadget *)gem_radio_gad[count];
                count++;
              }
            }
          }
          gem_radio_count = count;
          if (count > 0) {
            AddGList(winp, (struct Gadget *)gem_radio_gad[0], 0, -1, NULL);
            RefreshGList((struct Gadget *)gem_radio_gad[0], winp, NULL, -1);
            GT_RefreshWindow(winp, NULL);
          }
        }
      }
    }
  }
  done = 0;
  event_obj = startobj;
  while (done == 0) {
    if (win_h != 0) {
      win_idx = gem_wind_find_handle(win_h);
      if (win_idx >= 0 && gem_window_list[win_idx] != 0) {
        struct Window *win = (struct Window *)gem_window_list[win_idx];
        struct IntuiMessage *msg;
        if (win && win->UserPort) {
          while ((msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
            if (msg->Class == IDCMP_GADGETUP || msg->Class == IDCMP_GADGETDOWN) {
              struct Gadget *gad = (struct Gadget *)msg->IAddress;
              { int ri;
                for (ri = 0; ri < gem_radio_count; ri++) {
                  if ((long)gem_radio_gad[ri] == (long)gad) {
                    event_obj = gem_radio_obj[ri];
                    done = 1;
                    break;
                  }
                }
              }
            } else if (msg->Class == IDCMP_CLOSEWINDOW) {
              event_obj = startobj;
              done = 1;
            }
            ReplyMsg((struct Message *)msg);
          }
        }
      }
    }
    if (done == 0 && WaitForChar(Input(), 0)) {
      key = 0;
      Read(Input(), &key, 1);
      if (key == 10 || key == 13) {
        event_obj = startobj;
        done = 1;
      }
    }
    if (done == 0) Delay(1);
  }
  // Remove radio gadgets
  if (gem_radio_count > 0) {
    win_idx = gem_wind_find_handle(win_h);
    if (win_idx >= 0) {
      struct Window *w = (struct Window *)gem_window_list[win_idx];
      if (w) RemoveGList(w, (struct Gadget *)gem_radio_gad[0], gem_radio_count);
    }
    for (i = 0; i < gem_radio_count; i++) {
      if (gem_radio_gad[i]) {
        FreeGadgets((struct Gadget *)gem_radio_gad[i]);
        gem_radio_gad[i] = 0;
      }
    }
    gem_radio_count = 0;
  }
  gem_form_active = -1;
  ctx[0] = event_obj;
	return ;
}

void gem_form_dial() {
  long dtype, ix, iy, iw, ih, x, y, w, h;
  dtype = ctx[3];
  ix = ctx[4]; iy = ctx[5]; iw = ctx[6]; ih = ctx[7];
  x = ctx[8]; y = ctx[9]; w = ctx[10]; h = ctx[11];
  // Animated dialog box transition (0=init, 1=start, 2=draw, 3=exit)
  ctx[0] = 1;
	return ;
}

void gem_form_alert() {
  long default_btn;
  default_btn = ctx[3];
  // Show a simple alert; return default button
  ctx[0] = default_btn;
	return ;
}

void gem_form_error() {
  ctx[0] = 1;
	return ;
}

void gem_form_center() {
  long tree;
  tree = ctx[3];
  // Center form on screen
  ctx[1] = (gem_scrn_w - 200) / 2;
  ctx[2] = (gem_scrn_h - 100) / 2;
  ctx[3] = 200;
  ctx[4] = 100;
  ctx[0] = E_OK;
	return ;
}

void gem_form_keybd() {
  // Return next object (0 = none)
  ctx[0] = 0;
	return ;
}

void gem_form_button() {
  long tree, obj, depth, next_obj;
  tree = ctx[3]; obj = ctx[4]; depth = ctx[5];
  next_obj = 0;
  // If this is a radio button (type 21), handle mutual exclusion
  if (tree != 0 && obj >= 0) {
    struct GEMObject { short next, head, tail; unsigned short flags, state, type; long spec; short x, y, w, h; };
    struct GEMObject *objtree = (struct GEMObject *)tree;
    int idx = (int)obj;
    if (objtree[idx].type == 21) { /* G_RBUTTON */
      int i;
      for (i = 0; objtree[i].type != 0; i++) {
        if (objtree[i].type == 21) {
          objtree[i].state &= ~1; /* clear SELECTED */
        }
      }
      objtree[idx].state |= 1;
      next_obj = idx;
    } else if (objtree[idx].type == 22) { /* G_CHECKBOX: toggle */
      objtree[idx].state ^= 1;
      next_obj = idx;
    } else {
      next_obj = idx;
    }
  }
  ctx[0] = next_obj;
	return ;
}


// ============================
// SCRP - Scrap (clipboard)
// ============================

void gem_scrp_read() {
  // Set pointer to scrap buffer, return length
  ctx[1] = gem_scrap_len;
  ctx[0] = E_OK;
	return ;
}

void gem_scrp_write() {
  // Write scrap from pointer
  gem_scrap_len = ctx[1];
  ctx[0] = E_OK;
	return ;
}


// ============================
// FSEL - File selector
// ============================

void gem_fsel_exinput() {
  long path_ptr = ctx[8];
  long file_ptr = ctx[9];
  if (!path_ptr || !file_ptr) { ctx[0] = 0; ctx[1] = 0; return; }
  extern unsigned long asl_base;
  struct FileRequester *fr;
  char *path = (char *)path_ptr;
  char *file = (char *)file_ptr;
  long ok = 0;
  if (!asl_base) { ctx[1] = 0; return; }
  fr = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText, "Select File", TAG_DONE);
  if (fr) {
    if (AslRequestTags(fr, TAG_DONE)) {
      ok = 1;
      if (fr->fr_Drawer) {
        int len = strlen(fr->fr_Drawer);
        if (len < 126) {
          strcpy(path, fr->fr_Drawer);
          if (len > 0 && path[len-1] != ':' && path[len-1] != '/') {
            path[len] = '/';
            path[len+1] = 0;
          }
        }
      }
      if (fr->fr_File) {
        strncpy(file, fr->fr_File, 12);
        file[12] = 0;
      }
    }
    FreeAslRequest(fr);
  }
  ctx[1] = ok;
  return;
}

void gem_fsel_exoutput() {
  long path_ptr = ctx[8];
  long file_ptr = ctx[9];
  if (!path_ptr || !file_ptr) { ctx[0] = 0; ctx[1] = 0; return; }
  extern unsigned long asl_base;
  struct FileRequester *fr;
  char *path = (char *)path_ptr;
  char *file = (char *)file_ptr;
  long ok = 0;
  if (!asl_base) { ctx[1] = 0; return; }
  fr = AllocAslRequestTags(ASL_FileRequest, ASLFR_TitleText, "Save File", TAG_DONE);
  if (fr) {
    if (AslRequestTags(fr, TAG_DONE)) {
      ok = 1;
      if (fr->fr_Drawer) {
        int len = strlen(fr->fr_Drawer);
        if (len < 126) {
          strcpy(path, fr->fr_Drawer);
          if (len > 0 && path[len-1] != ':' && path[len-1] != '/') {
            path[len] = '/';
            path[len+1] = 0;
          }
        }
      }
      if (fr->fr_File) {
        strncpy(file, fr->fr_File, 12);
        file[12] = 0;
      }
    }
    FreeAslRequest(fr);
  }
  ctx[1] = ok;
  return;
}


// ============================
// WIND - Window services
// ============================

void gem_wind_create() {
  long kind;
  kind = ctx[3];
  gem_wind_alloc();
	return ;
}

void gem_wind_open() {
  long handle, idx; char title[128];
  handle = ctx[3];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    gem_wind_state[idx] = WS_OPEN;
    gem_wind_x[idx] = ctx[4];
    gem_wind_y[idx] = ctx[5];
    gem_wind_w[idx] = ctx[6];
    gem_wind_h[idx] = ctx[7];
    gem_wind_work_x[idx] = ctx[4] + 4;
    gem_wind_work_y[idx] = ctx[5] + 30;
    gem_wind_work_w[idx] = ctx[6] - 8;
    gem_wind_work_h[idx] = ctx[7] - 34;
    gem_wind_full_x[idx] = ctx[4];
    gem_wind_full_y[idx] = ctx[5];
    gem_wind_full_w[idx] = ctx[6];
    gem_wind_full_h[idx] = ctx[7];
    title[0] = 0;
    gem_window_list[idx] =  struct Window *win; struct NewWindow nw; long flags = WFLG_SMART_REFRESH | WFLG_ACTIVATE | WFLG_GIMMEZEROZERO; long idcmp = IDCMP_CLOSEWINDOW | IDCMP_REFRESHWINDOW | IDCMP_SIZEVERIFY | IDCMP_NEWSIZE | IDCMP_MOUSEBUTTONS | IDCMP_MOUSEMOVE; if (kind & 2) flags |= WFLG_CLOSEGADGET; if (kind & 4) flags |= WFLG_DEPTHGADGET; if (kind & 8) flags |= WFLG_DRAGBAR; if (kind & 16) flags |= WFLG_SIZEGADGET; nw.LeftEdge = (long)gem_wind_x[idx] ; nw.TopEdge = (long)gem_wind_y[idx] ; nw.Width = (long)gem_wind_w[idx] ; nw.Height = (long)gem_wind_h[idx] ; nw.DetailPen = 0; nw.BlockPen = 1; nw.Title = (STRPTR)title ; nw.Flags = flags; nw.IDCMPFlags = idcmp; nw.Type = WBENCHSCREEN; nw.FirstGadget = NULL; nw.CheckMark = NULL; nw.Screen = NULL; nw.BitMap = NULL; nw.MinWidth = 50; nw.MinHeight = 30; nw.MaxWidth = 2048; nw.MaxHeight = 2048; win = OpenWindow(&nw); return (unsigned long)win; ;
    if( gem_window_list[idx] != 0) {
      ctx[0] = 1;
    } else {
      ctx[0] = 0;
    }
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_wind_close() {
  long handle, idx;
  handle = ctx[3];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    if( gem_window_list[idx] != 0) {
       HideWindow((struct Window *) return (struct Window*)v;  ); ;
    }
    gem_wind_state[idx] = WS_CLOSED;
    ctx[0] = 1;
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_wind_delete() {
  long handle, idx;
  handle = ctx[3];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    if( gem_window_list[idx] != 0) {
       CloseWindow((struct Window *) return (struct Window*)v;  ); ;
      gem_window_list[idx] = 0;
    }
    gem_wind_state[idx] = WS_CLOSED;
    gem_wind_handle[idx] = 0;
    ctx[0] = 1;
  } else {
    ctx[0] = 0;
  }
	return ;
}

void gem_wind_get() {
  long handle, field, idx;
  handle = ctx[3];
  field = ctx[4];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    switch( field) {
    case 0 :// -> WF_WORKXYWH: work area
      ctx[1] = gem_wind_work_x[idx];
      ctx[2] = gem_wind_work_y[idx];
      ctx[3] = gem_wind_work_w[idx];
      ctx[4] = gem_wind_work_h[idx];
	break;
    case 1 :// -> WF_CURRXYWH: current dimensions
      ctx[1] = gem_wind_x[idx];
      ctx[2] = gem_wind_y[idx];
      ctx[3] = gem_wind_w[idx];
      ctx[4] = gem_wind_h[idx];
	break;
    case 2 :// -> WF_PREVXYWH: previous dimensions
      ctx[1] = gem_wind_full_x[idx];
      ctx[2] = gem_wind_full_y[idx];
      ctx[3] = gem_wind_full_w[idx];
      ctx[4] = gem_wind_full_h[idx];
	break;
    case 5 :// -> WF_TOP: top window handle
      ctx[1] = handle;
	break;
    case 6 :// -> WF_FIRSTXYWH: first rectangle
      ctx[1] = gem_wind_work_x[idx];
      ctx[2] = gem_wind_work_y[idx];
      ctx[3] = gem_wind_work_w[idx];
      ctx[4] = gem_wind_work_h[idx];
	break;
    case 7 :// -> WF_OWNER: owner PID
      ctx[1] = gem_wind_parent[idx];
	break;
    default: //
      ctx[1] = gem_wind_x[idx];
      ctx[2] = gem_wind_y[idx];
      ctx[3] = gem_wind_w[idx];
      ctx[4] = gem_wind_h[idx];
	break;
    }
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}

void gem_wind_set() {
  long handle, field, idx;
  handle = ctx[3];
  field = ctx[4];
  idx = gem_wind_find_handle(handle);
  if( idx >= 0) {
    switch( field) {
    case 0 :// WF_WORKXYWH
      gem_wind_work_x[idx] = ctx[5]; gem_wind_work_y[idx] = ctx[6];
      gem_wind_work_w[idx] = ctx[7]; gem_wind_work_h[idx] = ctx[8];
	break;
    case 1 :// WF_CURRXYWH
      gem_wind_x[idx] = ctx[5]; gem_wind_y[idx] = ctx[6];
      gem_wind_w[idx] = ctx[7]; gem_wind_h[idx] = ctx[8];
      if( gem_window_list[idx] != 0) {
         MoveWindow((struct Window *) return (struct Window*)v;  , (long)gem_wind_x[idx] , (long)gem_wind_y[idx] ); ;
         SizeWindow((struct Window *) return (struct Window*)v;  , (long)gem_wind_w[idx] , (long)gem_wind_h[idx] ); ;
      }
	break;
    case 3 :// WF_NEWSIZE
      gem_wind_w[idx] = ctx[5]; gem_wind_h[idx] = ctx[6];
      if( gem_window_list[idx] != 0) {
         SizeWindow((struct Window *) return (struct Window*)v;  , (long)gem_wind_w[idx] , (long)gem_wind_h[idx] ); ;
      }
	break;
    case 4 :// WF_ICONIFY
      gem_wind_state[idx] = WS_ICONIFIED;
      if( gem_window_list[idx] != 0) {
         HideWindow((struct Window *) return (struct Window*)v;  ); ;
      }
	break;
    case 5 :// WF_TOP
	break;
    case 10 :// WF_NAME
      if( gem_window_list[idx] != 0) {
         WindowTitle((struct Window *) return (struct Window*)v;  , (STRPTR) return (unsigned char*)v;  ); ;
      }
	break;
    }
    ctx[0] = E_OK;
  } else {
    ctx[0] = E_ERROR;
  }
	return ;
}

void gem_wind_find() {
  long mx, my, i, result;
  mx = ctx[3]; my = ctx[4];
  result = 0;
  // Find top-most open window at (mx, my)
  for ( i = MAX_WINDOWS - 1 ; i >= 0 ; i = i + -1) {
    if( (- (result == 0 )& gem_wind_state[i] )== WS_OPEN) {
      if( - ((- ((- ((- (mx >= gem_wind_x[i] )& mx )< gem_wind_x[i] )+ gem_wind_w[i] & my )>= gem_wind_y[i] )& my )< gem_wind_y[i] )+ gem_wind_h[i]) {
        result = gem_wind_handle[i];
      }
    }
  }
  ctx[0] = result;
	return ;
}

void gem_wind_update() {
  long beg_end;
  beg_end = ctx[3];
  // 0=begin update, 1=end update
  ctx[0] = E_OK;
	return ;
}

void gem_wind_calc() {
  long calc_type, kind, x, y, w, h;
  long title_h, frame_w;
  calc_type = ctx[3];
  kind = ctx[4];
  x = ctx[5]; y = ctx[6]; w = ctx[7]; h = ctx[8];
  title_h = 30;
  frame_w = 4;
  if (calc_type == 0) {
    ctx[1] = x + frame_w;
    ctx[2] = y + title_h;
    ctx[3] = w - frame_w * 2;
    ctx[4] = h - title_h - frame_w;
  } else {
    ctx[1] = x - frame_w;
    ctx[2] = y - title_h;
    ctx[3] = w + frame_w * 2;
    ctx[4] = h + title_h + frame_w;
  }
  ctx[0] = E_OK;
	return ;
}

void gem_wind_new() {
  ctx[0] = 1;
	return ;
}

void gem_wind_arrow() {
  ctx[0] = E_OK;
	return ;
}

void gem_wind_show() {
  long handle, flag;
  handle = ctx[3]; flag = ctx[4];
  ctx[0] = E_OK;
	return ;
}

void gem_wind_toolbar() {
  ctx[0] = E_OK;
	return ;
}

void gem_wind_sized() {
  ctx[0] = E_OK;
	return ;
}

// Initialize predefined mouse pointer shapes
void gem_mouse_init() {
  long i;
  // Arrow sprite data (16x16)
  gem_mouse_arrow_data[0] = 0x0000;  gem_mouse_arrow_mask[0] = 0x0000;
  gem_mouse_arrow_data[1] = 0x7C00;  gem_mouse_arrow_mask[1] = 0xFC00;
  gem_mouse_arrow_data[2] = 0x7200;  gem_mouse_arrow_mask[2] = 0xF600;
  gem_mouse_arrow_data[3] = 0x7100;  gem_mouse_arrow_mask[3] = 0xF300;
  gem_mouse_arrow_data[4] = 0x7080;  gem_mouse_arrow_mask[4] = 0xF180;
  gem_mouse_arrow_data[5] = 0x7040;  gem_mouse_arrow_mask[5] = 0xF0C0;
  gem_mouse_arrow_data[6] = 0x7020;  gem_mouse_arrow_mask[6] = 0xF060;
  gem_mouse_arrow_data[7] = 0x7C10;  gem_mouse_arrow_mask[7] = 0xFC30;
  gem_mouse_arrow_data[8] = 0x4608;  gem_mouse_arrow_mask[8] = 0xEF18;
  gem_mouse_arrow_data[9] = 0x4304;  gem_mouse_arrow_mask[9] = 0xC78C;
  gem_mouse_arrow_data[10] = 0x4182; gem_mouse_arrow_mask[10] = 0xC3C6;
  gem_mouse_arrow_data[11] = 0x40E1; gem_mouse_arrow_mask[11] = 0xC1E3;
  gem_mouse_arrow_data[12] = 0x4070; gem_mouse_arrow_mask[12] = 0xC0F0;
  gem_mouse_arrow_data[13] = 0x4038; gem_mouse_arrow_mask[13] = 0xC078;
  gem_mouse_arrow_data[14] = 0x401C; gem_mouse_arrow_mask[14] = 0xC03C;
  gem_mouse_arrow_data[15] = 0x4000; gem_mouse_arrow_mask[15] = 0xC000;

  // Busy/hourglass sprite data (16x16)
  gem_mouse_busy_data[0] = 0x0000;  gem_mouse_busy_mask[0] = 0x0000;
  gem_mouse_busy_data[1] = 0x7FFE;  gem_mouse_busy_mask[1] = 0xFFFF;
  gem_mouse_busy_data[2] = 0x6006;  gem_mouse_busy_mask[2] = 0xF00F;
  gem_mouse_busy_data[3] = 0x300C;  gem_mouse_busy_mask[3] = 0x781E;
  gem_mouse_busy_data[4] = 0x1818;  gem_mouse_busy_mask[4] = 0x3C3C;
  gem_mouse_busy_data[5] = 0x0C30;  gem_mouse_busy_mask[5] = 0x1E78;
  gem_mouse_busy_data[6] = 0x0660;  gem_mouse_busy_mask[6] = 0x0FF0;
  gem_mouse_busy_data[7] = 0x03C0;  gem_mouse_busy_mask[7] = 0x07E0;
  gem_mouse_busy_data[8] = 0x0660;  gem_mouse_busy_mask[8] = 0x0FF0;
  gem_mouse_busy_data[9] = 0x0C30;  gem_mouse_busy_mask[9] = 0x1E78;
  gem_mouse_busy_data[10] = 0x1818; gem_mouse_busy_mask[10] = 0x3C3C;
  gem_mouse_busy_data[11] = 0x300C; gem_mouse_busy_mask[11] = 0x781E;
  gem_mouse_busy_data[12] = 0x6006; gem_mouse_busy_mask[12] = 0xF00F;
  gem_mouse_busy_data[13] = 0x7FFE; gem_mouse_busy_mask[13] = 0xFFFF;
  gem_mouse_busy_data[14] = 0x0000; gem_mouse_busy_mask[14] = 0x0000;
  gem_mouse_busy_data[15] = 0x0000; gem_mouse_busy_mask[15] = 0x0000;

  // I-beam/vertical bar sprite data (16x16)
  for ( i = 0 ; i <= 15; i = i + 1) {
    gem_mouse_ibeam_data[i] = 0x0180;
    gem_mouse_ibeam_mask[i] = 0x03C0;
  }
  gem_mouse_ibeam_data[0] = 0x0000; gem_mouse_ibeam_mask[0] = 0x0000;
  gem_mouse_ibeam_data[15] = 0x0000; gem_mouse_ibeam_mask[15] = 0x0000;

  // Pointing finger sprite data (16x16)
  gem_mouse_point_data[0] = 0x0000;  gem_mouse_point_mask[0] = 0x0000;
  gem_mouse_point_data[1] = 0x0E00;  gem_mouse_point_mask[1] = 0x1F00;
  gem_mouse_point_data[2] = 0x0A00;  gem_mouse_point_mask[2] = 0x1F00;
  gem_mouse_point_data[3] = 0x0A00;  gem_mouse_point_mask[3] = 0x1F00;
  gem_mouse_point_data[4] = 0x0A00;  gem_mouse_point_mask[4] = 0x1F00;
  gem_mouse_point_data[5] = 0x0A00;  gem_mouse_point_mask[5] = 0x1F00;
  gem_mouse_point_data[6] = 0x0A00;  gem_mouse_point_mask[6] = 0x1F00;
  gem_mouse_point_data[7] = 0x0A00;  gem_mouse_point_mask[7] = 0x1F00;
  gem_mouse_point_data[8] = 0x0A00;  gem_mouse_point_mask[8] = 0x1F00;
  gem_mouse_point_data[9] = 0x0A00;  gem_mouse_point_mask[9] = 0x1F00;
  gem_mouse_point_data[10] = 0x0E00; gem_mouse_point_mask[10] = 0x1F00;
  gem_mouse_point_data[11] = 0x1C00; gem_mouse_point_mask[11] = 0x3E00;
  gem_mouse_point_data[12] = 0x3800; gem_mouse_point_mask[12] = 0x7C00;
  gem_mouse_point_data[13] = 0x7000; gem_mouse_point_mask[13] = 0xF800;
  gem_mouse_point_data[14] = 0x2000; gem_mouse_point_mask[14] = 0x7000;
  gem_mouse_point_data[15] = 0x0000; gem_mouse_point_mask[15] = 0x0000;
	return ;
}

// graf_rubberbox() - Interactive rubber-band rectangle
// Draws a XOR outline box from (x,y) to (x+minw, y+minh) at minimum
// GEM waits for a button click and returns final width/height
void gem_graf_rubberbox() {
  long x, y, minw, minh, finalw, finalh;
  x = ctx[3]; y = ctx[4]; minw = ctx[5]; minh = ctx[6];
  // Simulate a rubber-band box. In a real system, this tracks the mouse.
  // We return a reasonable default size.
  finalw = minw + 40;
  finalh = minh + 30;
  ctx[1] = finalw;
  ctx[2] = finalh;
  ctx[0] = 1;
	return ;
}

// graf_dragbox() - Drag a box (interactive move feedback)
// Draws a XOR outline box of size (w x h), starts at (sx,sy)
// Constrained to area defined by (cx,cy) and bound flag:
//   bound=0: no constraint, bound=1: constrain to 1st rect, bound=2: to 2nd rect
// Returns final (x,y) position
void gem_graf_dragbox() {
  long w, h, sx, sy, cx, cy, bound, finalx, finaly;
  w = ctx[3]; h = ctx[4];
  sx = ctx[5]; sy = ctx[6];
  cx = ctx[7]; cy = ctx[8];
  bound = ctx[9];
  finalx = sx;
  finaly = sy;
  if( bound == 1) {
    if( finalx < 0 ) { finalx = 0;}
    if( finalx + w > gem_scrn_w ) { finalx = gem_scrn_w - w;}
    if( finaly < 0 ) { finaly = 0;}
    if( finaly + h > gem_scrn_h ) { finaly = gem_scrn_h - h;}
  }
  ctx[1] = finalx;
  ctx[2] = finaly;
  ctx[0] = 1;
	return ;
}

// graf_movebox() - Animate moving a box from one position to another
// Draws outline at (sx,sy) then at (dx,dy) to show movement
void gem_graf_movebox() {
  long w, h, sx, sy, dx, dy;
  w = ctx[3]; h = ctx[4]; sx = ctx[5]; sy = ctx[6];
  dx = ctx[7]; dy = ctx[8];
  ctx[0] = 1;
	return ;
}

// graf_growbox() - Animate growing a box (expand effect)
// Grows from small rect (px,py,pw,ph) to big rect (sx,sy,sw,sh)
void gem_graf_growbox() {
  long px, py, pw, ph, sx, sy, sw, sh;
  px = ctx[3]; py = ctx[4]; pw = ctx[5]; ph = ctx[6];
  sx = ctx[7]; sy = ctx[8]; sw = ctx[9]; sh = ctx[10];
  ctx[0] = 1;
	return ;
}

// graf_shrinkbox() - Animate shrinking a box (collapse effect)
// Shrinks from big rect (sx,sy,sw,sh) to small rect (px,py,pw,ph)
void gem_graf_shrinkbox() {
  long sx, sy, sw, sh, px, py, pw, ph;
  sx = ctx[3]; sy = ctx[4]; sw = ctx[5]; sh = ctx[6];
  px = ctx[7]; py = ctx[8]; pw = ctx[9]; ph = ctx[10];
  ctx[0] = 1;
	return ;
}

// graf_watchbox() - Watch an object for state change
// Waits until the user clicks on the specified object
// tree = object tree, obj = object index
// instate = color when entered, outstate = color when exited
// Returns outstate (the final state of the object)
void gem_graf_watchbox() {
  long tree, obj, instate, outstate;
  tree = ctx[3]; obj = ctx[4]; instate = ctx[5]; outstate = ctx[6];
  // Simulate: if there's a pending menu/button action, act on it
  if( (- (gem_menu_pending_action == 1 )& gem_menu_pending_app )== gem_aes_id) {
    // A menu item was selected, notify the form
    gem_menu_pending_action = 0;
    ctx[1] = gem_menu_pending_item;
    ctx[0] = outstate  ;// Object changed to outstate (was selected)
  } else {
    // No pending action, return the object index to simulate selection
    ctx[1] = obj;
    ctx[0] = outstate;
  }
	return ;
}

// graf_slidebox() - Handle slider box movement
// tree = object tree, parent = parent object, obj = slider object
// is_horiz = 1 for horizontal, 0 for vertical
// Returns new slider position (offset in pixels)
void gem_graf_slidebox() {
  long tree, parent, obj, is_horiz, idx, newpos;
  tree = ctx[3]; parent = ctx[4]; obj = ctx[5];
  is_horiz = ctx[6];
  // Find slider index from object
  idx = obj & 7;
  if( idx > 7 ) { idx = 0;}
  if( idx < 0 ) { idx = 0;}
  // Return stored slider position (simulated)
  if( is_horiz) {
    newpos = gem_graf_slidex[idx];
  } else {
    newpos = gem_graf_slidey[idx];
  }
  ctx[1] = newpos;
  ctx[0] = 1;
	return ;
}

// graf_handle() - Get graphics handle and character cell size
// Returns: workstation handle, char width, char height, cell width (in pixels)
// Also sets the workstation cell dimensions for text operations
void gem_graf_handle() {
  // Workstation handle: unique ID for the virtual device
  // Char cell: 8x16 for high-resolution Atari ST mode
  // Last value is typically 0 (box width/height unused)
  ctx[0] = gem_graf_wk_handle;
  ctx[1] = gem_graf_char_w;
  ctx[2] = gem_graf_char_h;
  ctx[3] = 0;
	return ;
}

// graf_mkstate() - Get mouse state
// Returns: button state, mouse X, mouse Y, keyboard state
void gem_graf_mkstate() {
  ctx[0] = gem_mouse_buttons;
  ctx[1] = gem_mouse_x;
  ctx[2] = gem_mouse_y;
  ctx[3] = gem_mouse_kstate;
	return ;
}

// graf_mouse() - Set mouse pointer shape
// shape: M_ON=0 (show), M_OFF=1 (hide), M_ARROW=2 (default arrow),
//        M_BUSY=3 (hourglass), M_IBEAM=4 (text), M_POINT=5 (finger),
//        M_USERDEF=6 (custom 2-plane), M_SPECIAL=7 (custom 4-plane)
// For M_USERDEF: ctx[4]=hotx, ctx[5]=hoty, ctx[6]=data_ptr
// For M_SPECIAL: same + ctx[7]=color_ptr, ctx[8]=words
// Returns previous mouse shape
void gem_graf_mouse() {
  long shape, old_shape, hotx, hoty, data_ptr; struct Window* win=NULL;
  shape = ctx[3];
  old_shape = gem_mouse_shape;

  if( shape == M_OFF) {
    gem_mouse_visible = 0;
  } else {
    if( shape == M_ON) {
      gem_mouse_visible = 1;
      gem_mouse_shape = M_ARROW;
    } else {
      if( (- (shape == M_USERDEF )| shape )== M_SPECIAL) {
        hotx = ctx[4];
        hoty = ctx[5];
        data_ptr = ctx[6];
        gem_mouse_user_hotx = hotx;
        gem_mouse_user_hoty = hoty;
        gem_mouse_user_active = 1;
        // Store pointer data from emulated memory (33 words)

          unsigned short *dst = (unsigned short *)gem_mouse_user_data;
          unsigned short *src = (unsigned short *)data_ptr;
          int n;
          for (n = 0; n < 33; n++) dst[n] = src[n];
        ;
        gem_mouse_shape = shape;
      } else {
        if( (- (shape >= M_ARROW )& shape )<= M_POINT) {
          gem_mouse_shape = shape;
          gem_mouse_user_active = 0;
        }
      }
      gem_mouse_visible = 1;
    }
  }

  // Attempt to update the pointer via AmigaOS Intuition if a window is open
  win =  return (struct Window*)v; ;
  if( (long) (gem_mouse_visible & (long) win )!= (long) 0) {
    if( gem_mouse_user_active) {
       SetPointer((struct Window *)win , (UWORD *)gem_mouse_user_data , (short)16 , (short)16 , (short)gem_mouse_user_hotx , (short)gem_mouse_user_hoty ); ;
    } else {
      // Use predefined shape
      if( shape == M_ARROW) {
         SetPointer((struct Window *)win , (UWORD *)gem_mouse_arrow_data , (short)16 , (short)16 , (short)0 , (short)0 ); ;
      } else {
        if( shape == M_BUSY) {
           SetPointer((struct Window *)win , (UWORD *)gem_mouse_busy_data , (short)16 , (short)16 , (short)7 , (short)7 ); ;
        } else {
          if( shape == M_IBEAM) {
             SetPointer((struct Window *)win , (UWORD *)gem_mouse_ibeam_data , (short)16 , (short)16 , (short)7 , (short)7 ); ;
          } else {
            if( shape == M_POINT) {
               SetPointer((struct Window *)win , (UWORD *)gem_mouse_point_data , (short)16 , (short)16 , (short)0 , (short)0 ); ;
            }
          }
        }
      }
    }
  } else {
    if( (long) (~ gem_mouse_visible & (long) win )!= (long) 0) {
       ClearPointer((struct Window *)win ); ;
    }
  }

  ctx[0] = old_shape;
	return ;
}

// graf_arrow() - Set arrow key mode
// flag=0: arrow keys used for menu navigation (mouse-emulated)
// flag=1: arrow keys used for normal cursor movement
void gem_graf_arrow() {
  long flag;
  flag = ctx[3];
  if( (- (flag == 0 )| flag )== 1) {
    gem_graf_arrow_mode = flag;
  }
  ctx[0] = E_OK;
	return ;
}

// graf_set_screen() - Set screen parameters (reserved, rarely used)
// Typically called during initialisation to pass screen info
void gem_graf_set_screen() {
  long handle, ws_w, ws_h, ws_bits;
  handle = ctx[3];
  ws_w = ctx[4];
  ws_h = ctx[5];
  ws_bits = ctx[6];
  gem_graf_wk_handle = handle;
  ctx[0] = E_OK;
	return ;
}

// graf_set_handle() - Set workstation handle explicitly
void gem_graf_set_handle() {
  long handle, char_w, char_h;
  handle = ctx[3];
  char_w = ctx[4];
  char_h = ctx[5];
  gem_graf_wk_handle = handle;
  if( char_w > 0 ) { gem_graf_char_w = char_w;}
  if( char_h > 0 ) { gem_graf_char_h = char_h;}
  ctx[0] = E_OK;
	return ;
}

// graf_accel() - Register/unregister accelerator key
// pid = application ID, tree = object tree
// Returns accelerator key code or 0
void gem_graf_accel() {
  long pid, tree, key;
  pid = ctx[3];
  tree = ctx[4];
  key = ctx[5];
  gem_graf_accel_key = key;
  ctx[0] = gem_graf_accel_key;
	return ;
}
void vdi_init_rgb() {
  long v;
  char vdi_rgb2[48]; // 16 colours x 3 bytes (R,G,B)
  v = 0; vdi_rgb2[0] =  return (unsigned char)v; ; vdi_rgb2[1] =  return (unsigned char)v; ; vdi_rgb2[2] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[3] =  return (unsigned char)v; ; vdi_rgb2[4] =  return (unsigned char)v; ; v = 200; vdi_rgb2[5] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[6] =  return (unsigned char)v; ; v = 200; vdi_rgb2[7] =  return (unsigned char)v; ; v = 0; vdi_rgb2[8] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[9] =  return (unsigned char)v; ; v = 200; vdi_rgb2[10] =  return (unsigned char)v; ; v = 200; vdi_rgb2[11] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[12] =  return (unsigned char)v; ; v = 0; vdi_rgb2[13] =  return (unsigned char)v; ; v = 0; vdi_rgb2[14] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[15] =  return (unsigned char)v; ; v = 0; vdi_rgb2[16] =  return (unsigned char)v; ; v = 200; vdi_rgb2[17] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[18] =  return (unsigned char)v; ; v = 200; vdi_rgb2[19] =  return (unsigned char)v; ; v = 0; vdi_rgb2[20] =  return (unsigned char)v; ;
  v = 200; vdi_rgb2[21] =  return (unsigned char)v; ; v = 200; vdi_rgb2[22] =  return (unsigned char)v; ; v = 200; vdi_rgb2[23] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[24] =  return (unsigned char)v; ; v = 100; vdi_rgb2[25] =  return (unsigned char)v; ; v = 100; vdi_rgb2[26] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[27] =  return (unsigned char)v; ; v = 0; vdi_rgb2[28] =  return (unsigned char)v; ; v = 100; vdi_rgb2[29] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[30] =  return (unsigned char)v; ; v = 100; vdi_rgb2[31] =  return (unsigned char)v; ; v = 0; vdi_rgb2[32] =  return (unsigned char)v; ;
  v = 0; vdi_rgb2[33] =  return (unsigned char)v; ; v = 100; vdi_rgb2[34] =  return (unsigned char)v; ; v = 100; vdi_rgb2[35] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[36] =  return (unsigned char)v; ; v = 0; vdi_rgb2[37] =  return (unsigned char)v; ; v = 0; vdi_rgb2[38] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[39] =  return (unsigned char)v; ; v = 0; vdi_rgb2[40] =  return (unsigned char)v; ; v = 100; vdi_rgb2[41] =  return (unsigned char)v; ;
  v = 100; vdi_rgb2[42] =  return (unsigned char)v; ; v = 100; vdi_rgb2[43] =  return (unsigned char)v; ; v = 0; vdi_rgb2[44] =  return (unsigned char)v; ;
  v = 255; vdi_rgb2[45] =  return (unsigned char)v; ; v = 255; vdi_rgb2[46] =  return (unsigned char)v; ; v = 255; vdi_rgb2[47] =  return (unsigned char)v; ;
	return ;
}
void vdi_init_line_pats() {
  vdi_line_pats[0] = 0xFFFF ;// solid (user-defined, default = solid)
  vdi_line_pats[1] = 0xFFFF ;// solid
  vdi_line_pats[2] = 0xFF88 ;// long dash
  vdi_line_pats[3] = 0xFF88 ;// short dash (same as long dash on ST)
  vdi_line_pats[4] = 0xCCCC ;// dash-dot
  vdi_line_pats[5] = 0xCCCC ;// dash-dot (alternate)
	return ;
}

// Open a virtual workstation with the given work-in array
// ctx[3] = work_in ptr (in emulated memory) — 45 words of device-independent attributes
// Returns handle via ctx[0]
void vdi_opnvwk() {
  vdi_init_pen_map();
  vdi_handle = 1;
  vdi_work_w = 640; vdi_work_h = 400;
  vdi_dev_w = 640; vdi_dev_h = 400;
  vdi_n_planes = 4;
  vdi_line_type = 1; vdi_line_width = 1; vdi_line_color = 1;
  vdi_fill_type = 1; vdi_fill_index = 1; vdi_fill_color = 1;
  vdi_marker_type = 3; vdi_marker_height = 10; vdi_marker_color = 1;
  vdi_text_font = 1; vdi_text_color = 1; vdi_text_rotation = 0;
  vdi_wr_mode = VDI_REPLACE;
  vdi_clip_x = 0; vdi_clip_y = 0; vdi_clip_w = vdi_work_w; vdi_clip_h = vdi_work_h;
  vdi_cur_x = 0; vdi_cur_y = 0;
  // Fill work_out array in emulated memory via intout
  gem_intout[0] = 0            ;// device id
  gem_intout[1] = 1            ;// line type count
  gem_intout[2] = 1            ;// line width count
  gem_intout[3] = 1            ;// marker type count
  gem_intout[4] = 1            ;// marker height count
  gem_intout[5] = 1            ;// text font count
  gem_intout[6] = 4            ;// colour count (4 planes = 16 colours)
  gem_intout[7] = 1            ;// fill type count
  gem_intout[8] = 1            ;// fill index count / pattern count
  gem_intout[9] = 3            ;// preloaded patterns
  gem_intout[10] = 0           ;// text rotation count
  gem_intout[11] = 3           ;// colour model (0=indexed, 3=RGB)
  gem_intout[12] = vdi_dev_w  ;// device width in pixels
  gem_intout[13] = vdi_dev_h  ;// device height in pixels
  gem_intout[14] = vdi_dev_w / 8 ;// device cell width in pixels
  gem_intout[15] = vdi_dev_h / 16 ;// device cell height in pixels
  gem_intout[16] = 0           ;// x dpi
  gem_intout[17] = 0           ;// y dpi
  gem_intout[18] = 1           ;// text effects
  gem_intout[19] = 0           ;// min character width
  gem_intout[20] = 0           ;// max character width
  gem_intout[21] = 0           ;// min character height (below baseline)
  gem_intout[22] = 0           ;// max character height (above baseline)
  gem_intout[23] = 0           ;// min kerning offset
  gem_intout[24] = 0           ;// max kerning offset
  gem_intout[25] = 0           ;// number of font entries
  ctx[0] = vdi_handle;
	return ;
}

// Close workstation
void vdi_clsvwk() {
  vdi_handle = -1;
  ctx[0] = 1;
	return ;
}

// Clear workstation (fill with colour index 0)
void vdi_clrwk() {
  ctx[0] = 1;
	return ;
}

// Update workstation (flush pending drawing)
void vdi_updwk() {
  ctx[0] = 1;
	return ;
}

// v_pline - Draw polyline (series of connected points)
// ptsin = array of (x,y) coordinate pairs, count in ptsin_count
// Attributes: line type, line width, line colour
void vdi_pline() {
  long pts, rp_ptr;
  pts = gem_control[2];
  if( pts < 2 ) { pts = 2;}
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    int pen = (int)vdi_line_color;
    int j;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    for ( j = 1; j < (int)pts; j = j + 1) {
      Draw(rp, (long)gem_ptrin[j * 2], (long)gem_ptrin[j * 2 + 1]);
    }
  }
  vdi_cur_x = gem_ptrin[(pts - 1) * 2];
  vdi_cur_y = gem_ptrin[(pts - 1) * 2 + 1];
  ctx[0] = 1;
	return ;
}

// v_pmarker - Draw marker symbols at each point
// ptsin = array of (x,y) coordinate pairs
void vdi_pmarker() {
  long pts, rp_ptr;
  pts = gem_control[2];
  if( pts > 0) {
    rp_ptr = gem_GetVDIRastPort();
    if( rp_ptr != 0) {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_marker_color;
      extern long vdi_marker_height;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      int pen = (int)vdi_marker_color;
      int j, sz = (int)vdi_marker_height / 2;
      if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
      SetAPen(rp, vdi_pen_map[pen]);
      for ( j = 0; j < (int)pts; j = j + 1) {
        long mx = gem_ptrin[j * 2], my = gem_ptrin[j * 2 + 1];
        Move(rp, mx - sz, my); Draw(rp, mx + sz, my);
        Move(rp, mx, my - sz); Draw(rp, mx, my + sz);
      }
    }
    vdi_cur_x = gem_ptrin[(pts - 1) * 2];
    vdi_cur_y = gem_ptrin[(pts - 1) * 2 + 1];
  }
  ctx[0] = 1;
	return ;
}

// v_gtext - Draw graphics text at position
// Position in ptsin[0], ptsin[1]; text in intin (null-terminated)
void vdi_gtext() {
  long rp_ptr, len;
  rp_ptr = gem_GetVDIRastPort();
  len = gem_control[0];
  if( len > 64 ) { len = 64;}
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_text_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    char txt[65];
    int pen = (int)vdi_text_color;
    int k, tlen = (int)len;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    SetDrMd(rp, JAM2);
    for ( k = 0; k < tlen; k = k + 1) {
      unsigned char ch = (unsigned char)((long)gem_intin[k]);
      if( ch == 0) { break;}
      txt[k] = ch;
    }
    txt[k] = 0;
    Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    Text(rp, txt, k);
  }
  vdi_cur_x = gem_ptrin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_fillarea - Draw filled polygon
// ptsin = array of vertex (x,y) coordinates
void vdi_fillarea() {
  long pts, rp_ptr;
  pts = gem_control[2];
  if( pts >= 3) {
    rp_ptr = gem_GetVDIRastPort();
    if( rp_ptr != 0) {
      struct RastPort *rp = (struct RastPort *)rp_ptr;
      extern long vdi_fill_color;
      extern long vdi_pen_map[16];
      extern long gem_ptrin[16];
      int pen = (int)vdi_fill_color;
      int j, n = (int)pts;
      if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
      SetAPen(rp, vdi_pen_map[pen]);
      Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
      for ( j = 1; j < n; j = j + 1) {
        Draw(rp, (long)gem_ptrin[j * 2], (long)gem_ptrin[j * 2 + 1]);
      }
      Draw(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    }
    vdi_cur_x = gem_ptrin[(pts - 1) * 2];
    vdi_cur_y = gem_ptrin[(pts - 1) * 2 + 1];
  }
  ctx[0] = 1;
	return ;
}

// v_bar - Draw filled rectangle (bar)
// ptsin[0..1] = top-left, ptsin[2..3] = bottom-right
void vdi_bar() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_fill_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    int pen = (int)vdi_fill_color;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    RectFill(rp, (long)gem_ptrin[0], (long)gem_ptrin[1],
                 (long)gem_ptrin[2], (long)gem_ptrin[3]);
  }
  ctx[0] = 1;
	return ;
}

// v_circle - Draw circle
// ptsin[0], ptsin[1] = centre; intin[0] = radius
void vdi_circle() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long cx = gem_ptrin[0], cy = gem_ptrin[1];
    long r = gem_intin[0];
    int pen = (int)vdi_line_color;
    int angle;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( angle = 0; angle <= 360; angle = angle + 15) {
      double rad = angle * 3.14159265 / 180.0;
      long px = cx + (long)(r * cos(rad));
      long py = cy + (long)(r * sin(rad));
      if( angle == 0) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  vdi_cur_x = gem_ptrin[0] + gem_intin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_ellipse - Draw ellipse
// ptsin[0], ptsin[1] = centre; intin[0] = x radius; intin[1] = y radius
void vdi_ellipse() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long cx = gem_ptrin[0], cy = gem_ptrin[1];
    long rx = gem_intin[0], ry = gem_intin[1];
    int pen = (int)vdi_line_color;
    int angle;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( angle = 0; angle <= 360; angle = angle + 15) {
      double rad = angle * 3.14159265 / 180.0;
      long px = cx + (long)(rx * cos(rad));
      long py = cy + (long)(ry * sin(rad));
      if( angle == 0) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  vdi_cur_x = gem_ptrin[0] + gem_intin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_ellarc - Draw elliptical arc
// ptsin[0..1] = centre; intin[0] = x radius; intin[1] = y radius
// intin[2] = start angle; intin[3] = end angle (in tenths of degrees)
void vdi_ellarc() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long cx = gem_ptrin[0], cy = gem_ptrin[1];
    long rx = gem_intin[0], ry = gem_intin[1];
    int sa = (int)gem_intin[2] / 10;
    int ea = (int)gem_intin[3] / 10;
    int pen = (int)vdi_line_color;
    int a;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = cx + (long)(rx * cos(rad));
      long py = cy + (long)(ry * sin(rad));
      if( a == sa) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  ctx[0] = 1;
	return ;
}

// v_ellpie - Draw elliptical pie slice
// same params as ellarc but filled to centre
void vdi_ellpie() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_fill_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long cx = gem_ptrin[0], cy = gem_ptrin[1];
    long rx = gem_intin[0], ry = gem_intin[1];
    int sa = (int)gem_intin[2] / 10;
    int ea = (int)gem_intin[3] / 10;
    int pen = (int)vdi_fill_color;
    int a;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    Move(rp, cx, cy);
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      Draw(rp, cx + (long)(rx * cos(rad)), cy + (long)(ry * sin(rad)));
    }
    Draw(rp, cx, cy);
  }
  ctx[0] = 1;
	return ;
}

// v_arc - Draw circular arc
// ptsin[0..1] = centre; intin[0] = radius
// intin[1] = start angle; intin[2] = end angle (in tenths of degrees)
void vdi_arc() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long cx = gem_ptrin[0], cy = gem_ptrin[1];
    long r = gem_intin[0];
    int sa = (int)gem_intin[1] / 10;
    int ea = (int)gem_intin[2] / 10;
    int pen = (int)vdi_line_color;
    int a;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = cx + (long)(r * cos(rad));
      long py = cy + (long)(r * sin(rad));
      if( a == sa) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
  }
  ctx[0] = 1;
	return ;
}

// v_pieslice - Draw circular pie slice
// same as arc but filled to centre
void vdi_pieslice() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_fill_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long cx = gem_ptrin[0], cy = gem_ptrin[1];
    long r = gem_intin[0];
    int sa = (int)gem_intin[1] / 10;
    int ea = (int)gem_intin[2] / 10;
    int pen = (int)vdi_fill_color;
    int a;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    Move(rp, cx, cy);
    for ( a = sa; a <= ea; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      Draw(rp, cx + (long)(r * cos(rad)), cy + (long)(r * sin(rad)));
    }
    Draw(rp, cx, cy);
  }
  ctx[0] = 1;
	return ;
}

// v_rbox - Draw rounded rectangle (outline)
// ptsin[0..1] = top-left; ptsin[2..3] = bottom-right
// intin[0] = corner radius
void vdi_rbox() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long x1 = gem_ptrin[0], y1 = gem_ptrin[1];
    long x2 = gem_ptrin[2], y2 = gem_ptrin[3];
    long cr = gem_intin[0];
    int pen = (int)vdi_line_color;
    int a;
    if( cr < 1) { cr = 1;}
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    for ( a = 180; a <= 270; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x1 + cr) + (long)(cr * cos(rad));
      long py = (y1 + cr) + (long)(cr * sin(rad));
      if( a == 180) { Move(rp, px, py);}
      else { Draw(rp, px, py);}
    }
    for ( a = 270; a <= 360; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x2 - cr) + (long)(cr * cos(rad));
      long py = (y1 + cr) + (long)(cr * sin(rad));
      Draw(rp, px, py);
    }
    for ( a = 0; a <= 90; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x2 - cr) + (long)(cr * cos(rad));
      long py = (y2 - cr) + (long)(cr * sin(rad));
      Draw(rp, px, py);
    }
    for ( a = 90; a <= 180; a = a + 15) {
      double rad = a * 3.14159265 / 180.0;
      long px = (x1 + cr) + (long)(cr * cos(rad));
      long py = (y2 - cr) + (long)(cr * sin(rad));
      Draw(rp, px, py);
    }
    Draw(rp, (x1 + cr), y1);
  }
  ctx[0] = 1;
	return ;
}

// v_rfbox - Draw filled rounded rectangle
// same as rbox but filled
void vdi_rfbox() {
  long rp_ptr;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_fill_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    long x1 = gem_ptrin[0], y1 = gem_ptrin[1];
    long x2 = gem_ptrin[2], y2 = gem_ptrin[3];
    int pen = (int)vdi_fill_color;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    RectFill(rp, x1, y1, x2, y2);
  }
  ctx[0] = 1;
	return ;
}

// v_justified - Draw justified text
// Position in ptsin[0..1]; text in intin; intin_len in control[0]
// intin[1] = word spacing; intin[2] = char spacing (in 1/8 em)
void vdi_justified() {
  long rp_ptr, len;
  rp_ptr = gem_GetVDIRastPort();
  len = gem_control[0];
  if( len > 64 ) { len = 64;}
  if( rp_ptr != 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_text_color;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    char txt[65];
    int pen = (int)vdi_text_color;
    int k, tlen = (int)len;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    SetDrMd(rp, JAM2);
    for ( k = 0; k < tlen; k = k + 1) {
      unsigned char ch = (unsigned char)((long)gem_intin[k]);
      if( ch == 0) { break;}
      txt[k] = ch;
    }
    txt[k] = 0;
    Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    Text(rp, txt, k);
  }
  vdi_cur_x = gem_ptrin[0];
  vdi_cur_y = gem_ptrin[1];
  ctx[0] = 1;
	return ;
}

// v_cellarray - Draw rectangular block of pixel cells
// ptsin[0..1] = top-left; ptsin[2..3] = bottom-right
// intin contains pixel colours (row by row)
void vdi_cellarray() {
  long w, h, rp_ptr;
  w = gem_ptrin[2] - gem_ptrin[0];
  if( w < 0) {
    w = -w;
  }
  w = w + 1;
  h = gem_ptrin[3] - gem_ptrin[1];
  if( h < 0) {
    h = -h;
  }
  h = h + 1;
  rp_ptr = gem_GetVDIRastPort();
  if( rp_ptr != 0 && w > 0 && h > 0) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    extern long gem_intin[128];
    long x1 = gem_ptrin[0], y1 = gem_ptrin[1];
    int wi = (int)w, hi = (int)h;
    int px, py;
    for ( py = 0; py < hi; py = py + 1) {
      for ( px = 0; px < wi; px = px + 1) {
        int idx = py * wi + px;
        int col = (idx < 128) ? (int)gem_intin[idx] : 0;
        if( col < 0) { col = 0;} if( col > 15) { col = 15;}
        SetAPen(rp, vdi_pen_map[col]);
        WritePixel(rp, x1 + px, y1 + py);
      }
    }
  }
  vdi_cur_x = gem_ptrin[2];
  vdi_cur_y = gem_ptrin[3];
  ctx[0] = 1;
	return ;
}

// v_bezier - Draw Bezier curve
// ptsin = control points (pairs of x,y); intin[0] = number of control points
void vdi_bezier() {
  long rp_ptr, npts;
  rp_ptr = gem_GetVDIRastPort();
  npts = gem_intin[0];
  if( rp_ptr != 0 && npts >= 4) {
    struct RastPort *rp = (struct RastPort *)rp_ptr;
    extern long vdi_line_color;
    extern long vdi_wr_mode;
    extern long vdi_line_type;
    extern long vdi_line_pats[6];
    extern long vdi_pen_map[16];
    extern long gem_ptrin[16];
    int pen = (int)vdi_line_color;
    int np = (int)npts;
    int i;
    if( pen < 0) { pen = 0;} if( pen > 15) { pen = 15;}
    SetAPen(rp, vdi_pen_map[pen]);
    switch ((int)vdi_wr_mode) {
      case 1: SetDrMd(rp, JAM1); break;
      case 2: SetDrMd(rp, JAM2); break;
      case 3: SetDrMd(rp, INVERS_XOR); break;
      default: SetDrMd(rp, JAM1); break;
    }
    { int lt = (int)vdi_line_type;
      if( lt >= 0 && lt < 6) { SetDrPt(rp, vdi_line_pats[lt]);}
    }
    Move(rp, (long)gem_ptrin[0], (long)gem_ptrin[1]);
    for ( i = 3; i < np; i = i + 3) {
      double px0 = gem_ptrin[(i-3)*2], py0 = gem_ptrin[(i-3)*2+1];
      double px1 = gem_ptrin[(i-2)*2], py1 = gem_ptrin[(i-2)*2+1];
      double px2 = gem_ptrin[(i-1)*2], py2 = gem_ptrin[(i-1)*2+1];
      double px3 = gem_ptrin[i*2],     py3 = gem_ptrin[i*2+1];
      int steps = 30, s;
      for ( s = 1; s <= steps; s = s + 1) {
        double t = (double)s / steps;
        double t2 = t * t, t3 = t2 * t;
        double mt = 1.0 - t, mt2 = mt * mt, mt3 = mt2 * mt;
        long cx = (long)(mt3*px0 + 3*mt2*t*px1 + 3*mt*t2*px2 + t3*px3);
        long cy = (long)(mt3*py0 + 3*mt2*t*py1 + 3*mt*t2*py2 + t3*py3);
        Draw(rp, cx, cy);
      }
    }
  }
  if( npts > 0) {
    vdi_cur_x = gem_ptrin[(npts - 1) * 2];
    vdi_cur_y = gem_ptrin[(npts - 1) * 2 + 1];
  }
  ctx[0] = 1;
	return ;
}

// vq_color - Inquire colour representation
// intin[0] = colour index; intin[1] = flag (0=get RGB)
// Returns RGB in intout[0..2]
void vdi_qcolor() {
  long idx;
  idx = gem_intin[0];
  if( (- (idx >= 0 )& idx )<= 15) {
    gem_intout[0] = vdi_rgb[idx * 3] * (int) 1000 / 255;
    gem_intout[1] = vdi_rgb[idx * 3 + 1] * (int) 1000 / 255;
    gem_intout[2] = vdi_rgb[idx * 3 + 2] * (int) 1000 / 255;
  } else {
    gem_intout[0] = 0; gem_intout[1] = 0; gem_intout[2] = 0;
  }
  ctx[0] = 1;
	return ;
}

// vq_curpos - Inquire graphics cursor position
void vdi_qcurpos() {
  gem_intout[0] = vdi_cur_x;
  gem_intout[1] = vdi_cur_y;
  ctx[0] = 1;
	return ;
}

// vq_contxt - Inquire current context (VDI attributes)
void vdi_qcontxt() {
  gem_intout[0] = vdi_line_type;
  gem_intout[1] = vdi_line_width;
  gem_intout[2] = vdi_line_color;
  gem_intout[3] = vdi_marker_type;
  gem_intout[4] = vdi_marker_height;
  gem_intout[5] = vdi_marker_color;
  gem_intout[6] = vdi_text_font;
  gem_intout[7] = vdi_text_color;
  gem_intout[8] = vdi_fill_type;
  gem_intout[9] = vdi_fill_index;
  gem_intout[10] = vdi_fill_color;
  gem_intout[11] = vdi_wr_mode;
  ctx[0] = 1;
	return ;
}

// vq_extnd - Inquire extended device capabilities
// intin[0] = device handle (0 = current)
void vdi_qextnd() {
  gem_intout[0] = 0 ;// device id
  gem_intout[1] = 0 ;// flags
  gem_intout[2] = 0 ;// colour capabilities
  gem_intout[3] = 0 ;// tiling capabilities
  gem_intout[4] = 0 ;// reserved
  gem_intout[5] = 0;
  gem_intout[6] = 0;
  gem_intout[7] = 0;
  gem_intout[8] = 0;
  gem_intout[9] = 0;
  gem_intout[10] = 0;
  gem_intout[11] = 0;
  ctx[0] = 1;
	return ;
}

// vq_cellht - Inquire cell height
void vdi_qcellht() {
  gem_intout[0] = vdi_work_h / 24;
  ctx[0] = 1;
	return ;
}

// vq_cellwd - Inquire cell width
void vdi_qcellwd() {
  gem_intout[0] = vdi_work_w / 40;
  ctx[0] = 1;
	return ;
}

// vq_chcells - Inquire number of character cells
void vdi_qchcells() {
  gem_intout[0] = 40 ;// columns
  gem_intout[1] = 25 ;// rows
  ctx[0] = 1;
	return ;
}

// vq_vgd - Inquire VGD (Virtual Graphics Device) capabilities
void vdi_qvgd() {
  gem_intout[0] = 1 ;// VDI version
  gem_intout[1] = 0 ;// sub-version
  gem_intout[2] = 1 ;// colour (0=mono, 1=colour)
  gem_intout[3] = vdi_n_planes;
  gem_intout[4] = vdi_dev_w;
  gem_intout[5] = vdi_dev_h;
  gem_intout[6] = vdi_dev_w * vdi_n_planes / 8 ;// bytes per line
  gem_intout[7] = 0 ;// screen base (emulated)
  ctx[0] = 1;
	return ;
}

// vq_key_s - Inquire key shift status (always returns 0 in emulator)
void vdi_qkey_s() {
  gem_intout[0] = 0;
  ctx[0] = 1;
	return ;
}

// v_opnwk — alternate open workstation (function 2)
void vdi_opnwk() {
  vdi_opnvwk();
	return ;
}

// Set VDI attribute functions based on intin values
// vsl_type (line type), vsl_width (line width), vsl_color (line colour)
// vsf_type (fill type), vsf_index (fill index), vsf_color (fill colour)
// etc.
void vdi_set_line_type() {
  vdi_line_type = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_line_width() {
  vdi_line_width = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_line_color() {
  vdi_line_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_fill_type() {
  vdi_fill_type = gem_intin[0];
  if( gem_intin[1] != 0 ) { vdi_fill_index = gem_intin[1];}
  ctx[0] = 1;
	return ;
}
void vdi_set_fill_index() {
  vdi_fill_index = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_fill_color() {
  vdi_fill_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_marker_type() {
  vdi_marker_type = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_marker_height() {
  vdi_marker_height = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_marker_color() {
  vdi_marker_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_text_font() {
  vdi_text_font = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_text_color() {
  vdi_text_color = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_text_rotation() {
  vdi_text_rotation = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_writing_mode() {
  vdi_wr_mode = gem_intin[0];
  ctx[0] = 1;
	return ;
}
void vdi_set_clip_rect() {
  vdi_clip_x = gem_intin[0];
  vdi_clip_y = gem_intin[1];
  vdi_clip_w = gem_intin[2];
  vdi_clip_h = gem_intin[3];
  ctx[0] = 1;
	return ;
}
void vdi_set_clip_state() {
  // intin[0] = 0 (off), 1 (on)
  ctx[0] = 1;
	return ;
}
void vdi_set_curpos() {
  vdi_cur_x = gem_intin[0];
  vdi_cur_y = gem_intin[1];
  ctx[0] = 1;
	return ;
}

// VDI inquiry for fill area patterns
void vdi_inq_fill_pats() {
  gem_intout[0] = 3;
  ctx[0] = 1;
	return ;
}


// ---------------------------------------------------------------------------
// GEM VDI (Virtual Device Interface) dispatch
// Called via BIOS trap #2 with D0 = $C9
// Maps GEM VDI drawing calls to AmigaOS graphics.library
// ctx[1] = VDI function number
// ---------------------------------------------------------------------------
void gem_vdi_dispatch() {
  long fn;
  fn = ctx[1];

  switch( fn) {

  case 1: vdi_clsvwk(); break;
  case 2: vdi_opnwk(); break;
  case 3: vdi_clrwk(); break;
  case 4: vdi_updwk(); break;
  case 5: vdi_opnvwk(); break;
  case 6: vdi_clsvwk(); break;
  case 7: vdi_clrwk(); break;
  case 8: vdi_updwk(); break;
  case 9: ctx[0] = 1; break;
  case 10: vdi_qcolor(); break;
  case 11: vdi_pline(); break;
  case 12: vdi_pmarker(); break;
  case 13: vdi_gtext(); break;
  case 14: vdi_fillarea(); break;
  case 15: vdi_cellarray(); break;
  case 16: vdi_fillarea(); break;
  case 17: vdi_bar(); break;
  case 18: vdi_bar(); break;
  case 19: vdi_arc(); break;
  case 20: vdi_pieslice(); break;
  case 21: vdi_circle(); break;
  case 22: vdi_ellipse(); break;
  case 23: vdi_ellarc(); break;
  case 24: vdi_ellpie(); break;
  case 25: vdi_rbox(); break;
  case 26: vdi_rfbox(); break;
  case 27: vdi_justified(); break;
  case 28: ctx[0] = 1; break;
  case 29: ctx[0] = 1; break;
  case 30: vdi_qcurpos(); break;
  case 31: vdi_qkey_s(); break;

  case 32: vdi_set_line_type(); break;
  case 33: vdi_set_line_width(); break;
  case 34: vdi_set_line_color(); break;
  case 35: vdi_set_marker_type(); break;
  case 36: vdi_set_marker_height(); break;
  case 37: vdi_set_marker_color(); break;
  case 38: vdi_set_text_font(); break;
  case 39: vdi_set_text_color(); break;
  case 40: vdi_set_text_rotation(); break;
  case 41: vdi_set_fill_type(); break;
  case 42: vdi_set_fill_index(); break;
  case 43: vdi_set_fill_index(); break;
  case 44: vdi_set_fill_color(); break;
  case 45: vdi_set_writing_mode(); break;
  case 46: vdi_set_clip_rect(); break;
  case 47: vdi_set_curpos(); break;
  case 48: ctx[0] = 1; break;
  case 49: ctx[0] = 1; break;
  case 50: vdi_set_marker_height(); break;
  case 51: vdi_set_marker_height(); break;
  case 52: ctx[0] = 1; break;
  case 53: ctx[0] = 1; break;
  case 54: ctx[0] = 1; break;

  case 100: vdi_opnvwk(); break;
  case 101: vdi_clsvwk(); break;
  case 102: vdi_qextnd(); break;
  case 103: vdi_fillarea(); break;
  case 104: ctx[0] = 1; break;
  case 105: ctx[0] = 1; break;
  case 106: ctx[0] = 1; break;
  case 107: vdi_bezier(); break;
  case 108: ctx[0] = 1; break;
  case 109: vdi_inq_fill_pats(); break;
  case 110: vdi_qcontxt(); break;
  case 111: vdi_qcontxt(); break;
  case 112: vdi_qcontxt(); break;
  case 113: vdi_qcontxt(); break;
  case 114: vdi_qcellht(); break;
  case 115: vdi_qcellwd(); break;
  case 116: vdi_qchcells(); break;
  case 117: vdi_qchcells(); break;
  case 118: ctx[0] = 1; break;
  case 119: ctx[0] = 1; break;
  case 120: vdi_qvgd(); break;
  case 121: ctx[0] = 1; break;
  case 122: vdi_set_clip_rect(); break;
  case 123: ctx[0] = 1; break;
  case 124: ctx[0] = 1; break;
  case 125: vdi_qcurpos(); break;
  case 126: ctx[0] = 1; break;
  case 127: ctx[0] = 1; break;
  case 128: ctx[0] = 1; break;
  case 129: ctx[0] = 1; break;
  case 130: vdi_qkey_s(); break;
  case 131: vdi_qextnd(); break;
  case 132: vdi_qcolor(); break;
  case 133: ctx[0] = 1; break;
  case 134: vdi_qchcells(); break;
  case 135: vdi_qchcells(); break;

  default:
    ctx[0] = 1;
  }
	return ;
}

// Build and register an Atari ST 8x16 system font
void gem_init_font_8x16() {
  long i;

  // Fill width table (all chars are 8 pixels wide)
  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_width_8x16[i] = 8;
  }

  // Fill location table (char N starts at N*16 bytes)
  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_loc_8x16[i * 2] =  return (unsigned char)v; ;
    gem_font_loc_8x16[i * 2 + 1] =  return (unsigned char)v; ;
  }

  // Atari ST 8x16 system font data (256 glyphs, 16 bytes each)
  // Bit 7 = leftmost pixel, bit 0 = rightmost pixel
  // Rows top to bottom; baseline at row 12 (3 rows below for descenders)

    unsigned char *f = (unsigned char *)gem_font_data_8x16;
    int c, r;

    static const unsigned char glyphs[256][16] = {

  /* 0x00-0x1F: Atari ST special graphics / control characters */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x00 */
  {0x3C,0x7E,0xFF,0xFF,0xFF,0xFF,0x7E,0x7E,0x3C,0x3C,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x01 heart */
  {0x0C,0x1E,0x3E,0x7C,0x7C,0x3E,0x1E,0x0C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x02 diamond */
  {0x18,0x3C,0x7E,0x7E,0x3C,0x3C,0x7E,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x03 club */
  {0x18,0x3C,0x7E,0x7E,0x3C,0x3C,0x5A,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x04 spade */
  {0x00,0x18,0x18,0x18,0x7E,0x7E,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x05 dot/center */
  {0x00,0x18,0x18,0x18,0xFF,0xFF,0x18,0x18,0x18,0x18,0xFF,0xFF,0x00,0x00,0x00,0x00}, /* 0x06 */
  {0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x00,0x18,0x18,0x18,0x18}, /* 0x07 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x08 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x09 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0A */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0B */
  {0xFE,0xFE,0xFE,0xFE,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0C */
  {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF}, /* 0x0D */ /* actually 0x0C - but 0x0D */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFE,0xFE,0xFE,0xFE,0x00,0x00,0x00,0x00}, /* 0x0E */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x0F */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x10 */
  {0x00,0x00,0x00,0x00,0xE0,0xF0,0x38,0x1C,0x0C,0x0E,0x07,0x03,0x00,0x00,0x00,0x00}, /* 0x11 corner BL */
  {0x00,0x00,0x00,0x00,0x07,0x0F,0x1C,0x38,0x30,0x70,0xE0,0xC0,0x00,0x00,0x00,0x00}, /* 0x12 corner BR */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x03,0x01}, /* 0x13 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xE0,0xC0,0x80}, /* 0x14 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x07,0x03,0x01}, /* 0x15 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xF0,0xE0,0xC0,0x80}, /* 0x16 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x17 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x18 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x19 */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1A */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1B */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1C */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1D */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1E */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x1F */

  /* 0x20-0x7F: ASCII printable + DEL/square */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* ' ' */
  {0x00,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00}, /* '!' */
  {0x00,0x00,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '"' */
  {0x00,0x00,0x00,0x24,0x24,0x7E,0x24,0x24,0x24,0x7E,0x24,0x24,0x00,0x00,0x00,0x00}, /* '#' */
  {0x00,0x08,0x08,0x3E,0x6B,0x68,0x38,0x1E,0x0B,0x6B,0x3E,0x08,0x08,0x00,0x00,0x00}, /* '$' */
  {0x00,0x00,0x00,0x00,0x61,0xD2,0x64,0x08,0x10,0x26,0x4B,0x86,0x00,0x00,0x00,0x00}, /* '%' */
  {0x00,0x00,0x3C,0x66,0x66,0x3C,0x38,0x4D,0x46,0x46,0x46,0x3D,0x00,0x00,0x00,0x00}, /* '&' */
  {0x00,0x00,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* ''' */
  {0x00,0x00,0x06,0x0C,0x18,0x18,0x18,0x18,0x18,0x18,0x0C,0x06,0x00,0x00,0x00,0x00}, /* '(' */
  {0x00,0x00,0x60,0x30,0x18,0x18,0x18,0x18,0x18,0x18,0x30,0x60,0x00,0x00,0x00,0x00}, /* ')' */
  {0x00,0x00,0x00,0x18,0x18,0x7E,0x3C,0x3C,0x7E,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, /* '*' */
  {0x00,0x00,0x00,0x00,0x18,0x18,0x18,0xFF,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, /* '+' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x1C,0x08,0x10,0x00,0x00,0x00}, /* ',' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '-' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00}, /* '.' */
  {0x00,0x00,0x00,0x02,0x04,0x08,0x08,0x10,0x10,0x20,0x20,0x40,0x00,0x00,0x00,0x00}, /* '/' */
  {0x00,0x00,0x3C,0x66,0x66,0x6E,0x76,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '0' */
  {0x00,0x00,0x18,0x38,0x78,0x18,0x18,0x18,0x18,0x18,0x18,0x7E,0x00,0x00,0x00,0x00}, /* '1' */
  {0x00,0x00,0x3C,0x66,0x06,0x0C,0x18,0x30,0x30,0x60,0x66,0x7E,0x00,0x00,0x00,0x00}, /* '2' */
  {0x00,0x00,0x3C,0x66,0x06,0x06,0x1C,0x06,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '3' */
  {0x00,0x00,0x0C,0x0C,0x1C,0x2C,0x4C,0x4C,0x7E,0x0C,0x0C,0x0C,0x00,0x00,0x00,0x00}, /* '4' */
  {0x00,0x00,0x7E,0x60,0x60,0x7C,0x06,0x06,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '5' */
  {0x00,0x00,0x3C,0x66,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '6' */
  {0x00,0x00,0x7E,0x46,0x06,0x0C,0x0C,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* '7' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '8' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3E,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* '9' */
  {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, /* ':' */
  {0x00,0x00,0x00,0x00,0x18,0x18,0x00,0x00,0x00,0x18,0x18,0x08,0x10,0x00,0x00,0x00}, /* ';' */
  {0x00,0x00,0x06,0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x06,0x00,0x00,0x00,0x00,0x00}, /* '<' */
  {0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '=' */
  {0x00,0x00,0x60,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x60,0x00,0x00,0x00,0x00,0x00}, /* '>' */
  {0x00,0x00,0x3C,0x66,0x66,0x06,0x0C,0x18,0x18,0x00,0x18,0x18,0x00,0x00,0x00,0x00}, /* '?' */
  {0x00,0x00,0x3C,0x66,0x66,0x6E,0x6E,0x6E,0x6E,0x60,0x62,0x3C,0x00,0x00,0x00,0x00}, /* '@' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'A' */
  {0x00,0x00,0x7C,0x66,0x66,0x66,0x7C,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 'B' */
  {0x00,0x00,0x3C,0x66,0x62,0x60,0x60,0x60,0x60,0x62,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'C' */
  {0x00,0x00,0x78,0x6C,0x66,0x66,0x66,0x66,0x66,0x66,0x6C,0x78,0x00,0x00,0x00,0x00}, /* 'D' */
  {0x00,0x00,0x7E,0x60,0x60,0x60,0x7C,0x60,0x60,0x60,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'E' */
  {0x00,0x00,0x7E,0x60,0x60,0x60,0x7C,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00}, /* 'F' */
  {0x00,0x00,0x3C,0x66,0x62,0x60,0x60,0x6E,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'G' */
  {0x00,0x00,0x66,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'H' */
  {0x00,0x00,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 'I' */
  {0x00,0x00,0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x6C,0x6C,0x38,0x00,0x00,0x00,0x00}, /* 'J' */
  {0x00,0x00,0x66,0x66,0x6C,0x6C,0x78,0x6C,0x6C,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'K' */
  {0x00,0x00,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'L' */
  {0x00,0x00,0x63,0x63,0x77,0x77,0x7F,0x6B,0x6B,0x63,0x63,0x63,0x00,0x00,0x00,0x00}, /* 'M' */
  {0x00,0x00,0x66,0x66,0x76,0x76,0x6E,0x6E,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'N' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'O' */
  {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00}, /* 'P' */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x66,0x6E,0x6C,0x3E,0x06,0x00,0x00,0x00}, /* 'Q' */
  {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x7C,0x6C,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'R' */
  {0x00,0x00,0x3C,0x66,0x60,0x60,0x3C,0x06,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'S' */
  {0x00,0x00,0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* 'T' */
  {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'U' */
  {0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x3C,0x18,0x00,0x00,0x00,0x00}, /* 'V' */
  {0x00,0x00,0x63,0x63,0x63,0x63,0x6B,0x6B,0x7F,0x77,0x77,0x22,0x00,0x00,0x00,0x00}, /* 'W' */
  {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'X' */
  {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* 'Y' */
  {0x00,0x00,0x7E,0x06,0x0C,0x0C,0x18,0x30,0x30,0x60,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'Z' */
  {0x00,0x00,0x3E,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x30,0x3E,0x00,0x00,0x00,0x00}, /* '[' */
  {0x00,0x00,0x00,0x40,0x20,0x10,0x10,0x08,0x08,0x04,0x04,0x02,0x00,0x00,0x00,0x00}, /* '' */
  {0x00,0x00,0x7C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x7C,0x00,0x00,0x00,0x00}, /* ']' */
  {0x00,0x00,0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '^' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00}, /* '_' */
  {0x00,0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* '`' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'a' */
  {0x00,0x00,0x60,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 'b' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'c' */
  {0x00,0x00,0x06,0x06,0x06,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'd' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x60,0x3C,0x00,0x00,0x00,0x00}, /* 'e' */
  {0x00,0x00,0x0E,0x18,0x18,0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* 'f' */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 'g' */
  {0x00,0x00,0x60,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'h' */
  {0x00,0x00,0x18,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 'i' */
  {0x00,0x00,0x0C,0x0C,0x00,0x1C,0x0C,0x0C,0x0C,0x0C,0x0C,0x6C,0x6C,0x38,0x00,0x00}, /* 'j' */
  {0x00,0x00,0x60,0x60,0x60,0x66,0x6C,0x78,0x78,0x6C,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'k' */
  {0x00,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 'l' */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x7F,0x6B,0x6B,0x6B,0x6B,0x6B,0x00,0x00,0x00,0x00}, /* 'm' */
  {0x00,0x00,0x00,0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'n' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 'o' */
  {0x00,0x00,0x00,0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, /* 'p' */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x06,0x06,0x00}, /* 'q' */
  {0x00,0x00,0x00,0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x60,0x60,0x00,0x00,0x00,0x00}, /* 'r' */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 's' */
  {0x00,0x00,0x18,0x18,0x18,0x7E,0x18,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00}, /* 't' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 'u' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00,0x00,0x00,0x00}, /* 'v' */
  {0x00,0x00,0x00,0x00,0x00,0x63,0x63,0x6B,0x6B,0x7F,0x36,0x36,0x00,0x00,0x00,0x00}, /* 'w' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00,0x00,0x00,0x00}, /* 'x' */
  {0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 'y' */
  {0x00,0x00,0x00,0x00,0x00,0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00,0x00,0x00,0x00}, /* 'z' */
  {0x00,0x00,0x0E,0x18,0x18,0x18,0x70,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00}, /* '{' */
  {0x00,0x00,0x18,0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x18,0x18,0x00,0x00,0x00,0x00}, /* '|' */
  {0x00,0x00,0x70,0x18,0x18,0x18,0x0E,0x18,0x18,0x18,0x18,0x70,0x00,0x00,0x00,0x00}, /* '}' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x62,0x64,0x08,0x13,0x23,0x00,0x00,0x00,0x00,0x00}, /* '~' */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x7F */

  /* 0x80-0x9F: Atari ST extended graphics */
  {0x3C,0x66,0x60,0x60,0x60,0x60,0x3C,0x0C,0x0C,0x00,0x0C,0x0C,0x00,0x00,0x00,0x00}, /* 0x80 Ç */
  {0x00,0x66,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x06,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x81 ü */
  {0x00,0x7E,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 0x82 é */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x60,0x3C,0x00,0x00,0x00,0x00}, /* 0x83 â */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 0x84 ä */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x0C,0x18,0x00,0x00}, /* 0x85 à */
  {0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x7E,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x86 å */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x87 ç */
  {0x00,0x00,0x3C,0x66,0x60,0x3C,0x66,0x66,0x66,0x3C,0x06,0x3C,0x00,0x00,0x00,0x00}, /* 0x88 ê */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x89 ë */
  {0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x60,0x60,0x66,0x3C,0x0C,0x18,0x00,0x00,0x00}, /* 0x8A è */
  {0x00,0x00,0x00,0x00,0x00,0x3E,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 0x8B ï */
  {0x00,0x00,0x18,0x18,0x00,0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00,0x00,0x00,0x00}, /* 0x8C ì */
  {0x00,0x00,0x3C,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x8D Ä */
  {0x00,0x00,0x7E,0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x66,0x7C,0x00,0x00,0x00,0x00}, /* 0x8E Å */
  {0x00,0x00,0x62,0x64,0x08,0x10,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00,0x00,0x00,0x00}, /* 0x8F É */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x90 æ */
  {0x00,0x08,0x3E,0x6B,0x68,0x3E,0x0B,0x6B,0x3E,0x08,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x91 Æ */
  {0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x92 ô */
  {0x00,0x3C,0x66,0x06,0x3C,0x60,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x93 ö */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 0x94 ò */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x66,0x3C,0x0C,0x18,0x00,0x00}, /* 0x95 û */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3E,0x06,0x66,0x3C,0x00}, /* 0x96 ù */
  {0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x3C,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x97 ÿ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x66,0x66,0x3E,0x00,0x00,0x00,0x00}, /* 0x98 Ö */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x60,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x99 Ü */
  {0x00,0x00,0x00,0x00,0x18,0x00,0x7E,0x18,0x18,0x18,0x18,0x0E,0x00,0x00,0x00,0x00}, /* 0x9A ¢ */
  {0x00,0x00,0x00,0x00,0x00,0x1C,0x36,0x30,0x7C,0x30,0x36,0x1C,0x00,0x00,0x00,0x00}, /* 0x9B £ */
  {0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x7E,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x9C ¥ */
  {0x00,0x00,0x00,0x00,0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x66,0x00,0x00,0x00,0x00}, /* 0x9D ₧ */
  {0x00,0x00,0x3C,0x66,0x06,0x0C,0x18,0x30,0x60,0x60,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0x9E ƒ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0x9F */

  /* 0xA0-0xFF: International characters */
  {0x00,0x00,0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00,0x3C,0x00,0x00,0x00,0x00,0x00}, /* 0xA0 á */
  {0x00,0x00,0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00,0x3C,0x00,0x00,0x00,0x00,0x00}, /* 0xA1 í */
  {0x00,0x00,0x18,0x18,0x00,0x7E,0x00,0x18,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA2 ó */
  {0x00,0x00,0x00,0x00,0x62,0x64,0x08,0x13,0x23,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA3 ú */
  {0x00,0x00,0x00,0x00,0x00,0x1E,0x30,0x1C,0x06,0x3C,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA4 ñ */
  {0x00,0x00,0x00,0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA5 Ñ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA6 ª */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xA7 º */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00,0x00}, /* 0xA8 ¿ */
  {0x00,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x0C,0x00,0x00,0x00,0x00}, /* 0xA9 ⌐ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xAA ¬ */
  {0x00,0x00,0x00,0x00,0x80,0x80,0x80,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xAB ½ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x04,0x04,0x04,0x04,0x00,0x00,0x00,0x00,0x00}, /* 0xAC ¼ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0xE0,0x20,0x20,0x20,0x20,0x00,0x00,0x00,0x00,0x00}, /* 0xAD ¡ */
  {0x00,0x00,0x00,0x00,0x18,0x3C,0x66,0x18,0x18,0x18,0x66,0x3C,0x00,0x00,0x00,0x00}, /* 0xAE « */
  {0x00,0x00,0x00,0x00,0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00}, /* 0xAF ░ */

  {0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA,0x00,0xAA}, /* 0xB0 ░ */
  {0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55,0xAA,0x55}, /* 0xB1 ▒ */
  {0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF,0x55,0xFF}, /* 0xB2 ▓ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB3 │ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB4 ┤ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB5 ╡ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB6 ╢ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB7 ╖ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB8 ╕ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xB9 ╣ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBA ║ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBB ╗ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBC ╝ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBD ╜ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBE ╛ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xBF ┐ */

  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC0 ─ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC1 ┼ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC2 ╞ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC3 ╟ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC4 ╚ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC5 ╔ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC6 ╩ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC7 ╦ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC8 ╠ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xC9 ═ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCA ╬ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCB ╧ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCC ╨ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCD ╤ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCE ╥ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xCF ╙ */

  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD0 ╘ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD1 ╒ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD2 ╓ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD3 ╫ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD4 ╪ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD5 ┘ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD6 ┌ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD7 █ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD8 ▄ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xD9 ▌ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDA ▐ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDB ▀ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDC α */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDD ß */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDE Γ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xDF π */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE0 Σ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE1 σ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE2 µ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE3 τ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE4 Φ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE5 Θ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE6 Ω */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE7 δ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE8 ∞ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xE9 φ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEA ε */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEB ∩ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEC ≡ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xED ± */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEE ≥ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xEF ≤ */

  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF0 ⌂ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF1 ÷ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF2 ≈ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF3 ° */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF4 ∙ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF5 · */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF6 √ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF7 ⁿ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF8 ² */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xF9 ■ */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xFA   */
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, /* 0xFB */
    };
    for (c = 0; c < 256; c++)
      for (r = 0; r < 16; r++)
        f[c * 16 + r] = glyphs[c][r];
  ;
  // Allocate and populate TextFont structure
  gem_font_8x16 =  return (struct TextFont*)v; ;
  if( gem_font_8x16) {
     struct TextFont *tf = (struct TextFont *)gem_font_8x16; tf->tf_Message.mn_ReplyPort=NULL; tf->tf_Message.mn_Length=sizeof(struct TextFont); tf->tf_YSize=16; tf->tf_Style=0; tf->tf_Flags=0; tf->tf_XSize=8; tf->tf_Baseline=13; tf->tf_BoldSmear=0; tf->tf_Accessors=0; tf->tf_LoChar=0; tf->tf_HiChar=255; tf->tf_CharData=(APTR)gem_font_data_8x16; tf->tf_Modulo=8; tf->tf_CharLoc=(APTR)gem_font_loc_8x16; tf->tf_CharSpace=(APTR)gem_font_width_8x16; tf->tf_CharKern=NULL; ;
    AddFont(gem_font_8x16 );
    gem_font_8x16_registered = 1;
  }
	return ;
}

// Build and register Atari ST 8x8 system font
void gem_init_font_8x8() {
  long i, c, r;

  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_width_8x8[i] = 8;
  }

  for ( i = 0 ; i <= 255; i = i + 1) {
    gem_font_loc_8x8[i] =  return (unsigned char)v; ;
  }

  // Downsample 8x16 to 8x8 by taking every 2nd row
  for ( c = 0 ; c <= 255; c = c + 1) {
    for ( r = 0 ; r <= 7; r = r + 1) {
      gem_font_data_8x8[c * 8 + r] = gem_font_data_8x16[(c * 16 + r )* 2];
    }
  }

  gem_font_8x8 =  return (struct TextFont*)v; ;
  if( gem_font_8x8) {
     struct TextFont *tf = (struct TextFont *)gem_font_8x8; tf->tf_Message.mn_ReplyPort=NULL; tf->tf_Message.mn_Length=sizeof(struct TextFont); tf->tf_YSize=8; tf->tf_Style=0; tf->tf_Flags=0; tf->tf_XSize=8; tf->tf_Baseline=7; tf->tf_BoldSmear=0; tf->tf_Accessors=0; tf->tf_LoChar=0; tf->tf_HiChar=255; tf->tf_CharData=(APTR)gem_font_data_8x8; tf->tf_Modulo=8; tf->tf_CharLoc=(APTR)gem_font_loc_8x8; tf->tf_CharSpace=(APTR)gem_font_width_8x8; tf->tf_CharKern=NULL; ;
    AddFont(gem_font_8x8 );
    gem_font_8x8_registered = 1;
  }
	return ;
}

// Initialize all Atari ST system fonts
void gem_init_fonts() {
  gem_font_8x16_registered = 0;
  gem_font_8x8_registered = 0;
  gem_init_font_8x16();
  gem_init_font_8x8();
	return ;
}


// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------
int main(int argc, char** argv) {
  long i; char filename[128]; long addr;
  char* arg2=NULL;
main_argc = argc;
main_argv = argv;
try {
	new_base();
	new_base2();
	new_Mem();
	new_FastMem();
	new_dos();
	new_timer();
	new_dos2();
	new_monitor();
	new_dos3();
	new_gfxbase();
	new_graphics();

  // Initialize handle table (GEMDOS handle -> AmigaOS BPTR mapping)
  gem_handles[0] = (long) Input();
  gem_handles[1] = (long) Output();
  gem_handles[2] = (long) Output();
  for ( i = 3 ; i <= 15; i = i + 1) {
    gem_handles[i] = 0;
  }

  gem_dta = 0;
  gem_drv = 0;
  gem_search_lock = 0;
  bios_kb_shift = 0;
  gem_scrn_w = 640;
  gem_scrn_h = 400;
  gem_aes_id = 0;
  gem_app_count = 0;
  gem_menu_count = 0;
  gem_menu_bar_visible = 0;
  gem_menu_active_app = 0;
  gem_menu_pending_action = 0;
  gem_menu_pending_item = 0;
  gem_menu_pending_tree = 0;
  gem_menu_pending_app = 0;
  gem_form_active = -1;
  gem_scrap_len = 0;
  gem_msg_head = 0;
  gem_msg_tail = 0;
  gem_mouse_x = 320;
  gem_mouse_y = 200;
  gem_mouse_buttons = 0;
  gem_mouse_kstate = 0;
  gem_mouse_shape = 2;
  gem_mouse_visible = 1;
  gem_mouse_user_active = 0;
  gem_graf_wk_handle = 1;
  gem_graf_char_w = 8;
  gem_graf_char_h = 16;
  gem_graf_arrow_mode = 1;
  gem_graf_accel_key = 0;
  gem_screen_mem = (void*) AllocMem((ULONG) 32768 ,(ULONG) 65538 );
  gem_mouse_init();
  gem_init_fonts();
  gem_init_gadtools();
  vdi_init_rgb();
  vdi_init_line_pats();
  vdi_handle = -1;
  install_traps();

  if( main_argc > 1) {
    arg2 = main_argv[1];
    i = 0;
    while( (arg2[i] != 0) && (i < 127)) {
      filename[i] = arg2[i];
      i = i + 1;
    }
    filename[i] = 0;

    PutStr("Loading: " );
    PutStr(filename );
    PutStr("\n" );

    addr = load_prg(filename);
    if( addr) {
      PutStr("Loaded at $" );
      Printf("%%lx\n" ,(ULONG) addr );
      PutStr("\n" );
    } else {
      PutStr("Failed to load PRG\n" );
    }
  } else {
    PutStr("Sake Atari Emulator\n" );
    PutStr("Testing...\n" );

    // Test Cconws ($09): write a string via direct call
    ctx[8] = (long)  return (unsigned char*)v; ;
    temp_string[0] = 72; temp_string[1] = 101; temp_string[2] = 108; temp_string[3] = 108; temp_string[4] = 111;
    temp_string[5] = 0;
    gemdos_cconws();

    // Test Dgetdrv ($0F)
    gemdos_dgetdrv();
    PutStr("Default drive: " );
    Printf("%%ld\n" ,(ULONG) ctx[0] );

    // Test Tgetdate ($31) and Tgettime ($33)
    gemdos_tgetdate();
    gemdos_tgettime();
    PutStr("Date: " );
    Printf("%%ld" ,(ULONG) ctx[0] );
    PutStr(", Time: " );
    Printf("%%ld\n" ,(ULONG) ctx[0] );

    PutStr("\nAll GEMDOS functions completed\n" );
  }
  remove_traps();
} catch(...) {}
try {
	end_graphics();
	end_dos2();
	end_dos();
	end_FastMem();
	end_Mem();
} catch(...) {}
	if (exception==-1) return (int) (long) exceptionInfo; /*finish the CleanUp() call*/
	return 0;
}


// ---------------------------------------------------------------------------
// Cursconf ($07) - Cursor configuration
// D1 = function (0=disable, 1=enable, 2=get status)
// ---------------------------------------------------------------------------
void xbios_cursconf() {
  if (ctx[1] == 0) gem_mouse_visible = 0;
  if (ctx[1] == 1) gem_mouse_visible = 1;
  ctx[0] = gem_mouse_visible;
	return ;
}


// ---------------------------------------------------------------------------
// Appl_init ($08) - Initialize application
// Returns application ID
// ---------------------------------------------------------------------------
void xbios_appl_init() {
  // Return a dummy application ID (1)
  ctx[0] = 1;
	return ;
}


// ---------------------------------------------------------------------------
// Physbase ($09) - Get physical screen base address
// Returns address of physical screen memory
// ---------------------------------------------------------------------------
void xbios_physbase() {
  ctx[0] = (long)gem_screen_mem;
	return ;
}


// ---------------------------------------------------------------------------
// Trap handlers - 68k exception vector installation for MorphOS & AmigaOS
// ---------------------------------------------------------------------------

extern "C" {
  void handle_gemdos_trap(long* regs, unsigned char* usp);
  void handle_gem_trap(long* regs);
  void handle_bios_trap(long* regs, unsigned char* usp);
  void handle_xbios_trap(long* regs, unsigned char* usp);
}

extern "C" void handle_gemdos_trap(long* regs, unsigned char* usp) {
  short fn = ((unsigned short)usp[0] << 8) | (unsigned short)usp[1];
  ctx[0] = fn;
  ctx[1] = regs[1];
  ctx[2] = regs[2];
  ctx[3] = regs[3];
  ctx[4] = regs[4];
  ctx[5] = regs[5];
  ctx[6] = regs[6];
  ctx[7] = regs[7];
  ctx[8] = (long)&usp[2];
  ctx[9] = regs[9];
  ctx[10] = regs[10];
  ctx[11] = regs[11];
  ctx[12] = regs[12];
  ctx[13] = regs[13];
  ctx[14] = regs[14];

  gemdos_dispatch();

  regs[0] = ctx[0];
}

extern "C" void handle_gem_trap(long* regs) {
  ctx[0] = regs[0];
  ctx[1] = regs[1];
  ctx[2] = regs[2];
  ctx[3] = regs[3];
  ctx[8] = regs[8];

  if (regs[0] == 0xC8) {
    gem_aes_dispatch();
  } else if (regs[0] == 0xC9) {
    gem_vdi_dispatch();
  } else {
    bios_dispatch();
  }

  regs[0] = ctx[0];
}

extern "C" void handle_bios_trap(long* regs, unsigned char* usp) {
  short fn = ((unsigned short)usp[0] << 8) | (unsigned short)usp[1];
  ctx[0] = fn;
  ctx[1] = regs[1];
  ctx[2] = regs[2];
  ctx[3] = regs[3];
  ctx[8] = (long)&usp[2];

  bios_dispatch();

  regs[0] = ctx[0];
}

extern "C" void handle_xbios_trap(long* regs, unsigned char* usp) {
  short fn = ((unsigned short)usp[0] << 8) | (unsigned short)usp[1];
  ctx[0] = fn;
  ctx[1] = regs[1];
  ctx[2] = regs[2];
  ctx[3] = regs[3];
  ctx[8] = (long)&usp[2];

  xbios_dispatch();

  regs[0] = ctx[0];
}

void install_traps() {
  if (trap_stub_mem) return;

  trap_stub_mem = (unsigned char *)AllocMem(128, MEMF_PUBLIC | MEMF_CLEAR);
  if (!trap_stub_mem) return;

  unsigned char *p1 = trap_stub_mem;
  unsigned char *p2 = trap_stub_mem + 32;
  unsigned char *p3 = trap_stub_mem + 64;
  unsigned char *p4 = trap_stub_mem + 96;

  unsigned long h1 = (unsigned long)handle_gemdos_trap;
  p1[0]=0x48; p1[1]=0xE7; p1[2]=0xFF; p1[3]=0xFE; /* movem.l d0-a6, -(sp) */
  p1[4]=0x20; p1[5]=0x4F;                         /* movea.l sp, a0 */
  p1[6]=0x4E; p1[7]=0x69;                         /* move.l  usp, a1 */
  p1[8]=0x2F; p1[9]=0x09;                         /* move.l  a1, -(sp) (push usp) */
  p1[10]=0x2F; p1[11]=0x08;                       /* move.l  a0, -(sp) (push regs) */
  p1[12]=0x4E; p1[13]=0xB9; p1[14]=(h1>>24)&0xFF; p1[15]=(h1>>16)&0xFF; p1[16]=(h1>>8)&0xFF; p1[17]=h1&0xFF; /* jsr handle_gemdos_trap */
  p1[18]=0x50; p1[19]=0x8F;                       /* addq.l  #8, sp */
  p1[20]=0x4C; p1[21]=0xDF; p1[22]=0x7F; p1[23]=0xFE; /* movem.l (sp)+, d0-a6 */
  p1[24]=0x4E; p1[25]=0x73;                       /* rte */

  unsigned long h2 = (unsigned long)handle_gem_trap;
  p2[0]=0x48; p2[1]=0xE7; p2[2]=0xFF; p2[3]=0xFE; /* movem.l d0-a6, -(sp) */
  p2[4]=0x20; p2[5]=0x4F;                         /* movea.l sp, a0 */
  p2[6]=0x2F; p2[7]=0x08;                         /* move.l  a0, -(sp) (push regs) */
  p2[8]=0x4E; p2[9]=0xB9; p2[10]=(h2>>24)&0xFF; p2[11]=(h2>>16)&0xFF; p2[12]=(h2>>8)&0xFF; p2[13]=h2&0xFF; /* jsr handle_gem_trap */
  p2[14]=0x58; p2[15]=0x8F;                       /* addq.l  #4, sp */
  p2[16]=0x4C; p2[17]=0xDF; p2[18]=0x7F; p2[19]=0xFE; /* movem.l (sp)+, d0-a6 */
  p2[20]=0x4E; p2[21]=0x73;                       /* rte */

  unsigned long h3 = (unsigned long)handle_bios_trap;
  p3[0]=0x48; p3[1]=0xE7; p3[2]=0xFF; p3[3]=0xFE; /* movem.l d0-a6, -(sp) */
  p3[4]=0x20; p3[5]=0x4F;                         /* movea.l sp, a0 */
  p3[6]=0x4E; p3[7]=0x69;                         /* move.l  usp, a1 */
  p3[8]=0x2F; p3[9]=0x09;                         /* move.l  a1, -(sp) (push usp) */
  p3[10]=0x2F; p3[11]=0x08;                       /* move.l  a0, -(sp) (push regs) */
  p3[12]=0x4E; p3[13]=0xB9; p3[14]=(h3>>24)&0xFF; p3[15]=(h3>>16)&0xFF; p3[16]=(h3>>8)&0xFF; p3[17]=h3&0xFF; /* jsr handle_bios_trap */
  p3[18]=0x50; p3[19]=0x8F;                       /* addq.l  #8, sp */
  p3[20]=0x4C; p3[21]=0xDF; p3[22]=0x7F; p3[23]=0xFE; /* movem.l (sp)+, d0-a6 */
  p3[24]=0x4E; p3[25]=0x73;                       /* rte */

  unsigned long h4 = (unsigned long)handle_xbios_trap;
  p4[0]=0x48; p4[1]=0xE7; p4[2]=0xFF; p4[3]=0xFE; /* movem.l d0-a6, -(sp) */
  p4[4]=0x20; p4[5]=0x4F;                         /* movea.l sp, a0 */
  p4[6]=0x4E; p4[7]=0x69;                         /* move.l  usp, a1 */
  p4[8]=0x2F; p4[9]=0x09;                         /* move.l  a1, -(sp) (push usp) */
  p4[10]=0x2F; p4[11]=0x08;                       /* move.l  a0, -(sp) (push regs) */
  p4[12]=0x4E; p4[13]=0xB9; p4[14]=(h4>>24)&0xFF; p4[15]=(h4>>16)&0xFF; p4[16]=(h4>>8)&0xFF; p4[17]=h4&0xFF; /* jsr handle_xbios_trap */
  p4[18]=0x50; p4[19]=0x8F;                       /* addq.l  #8, sp */
  p4[20]=0x4C; p4[21]=0xDF; p4[22]=0x7F; p4[23]=0xFE; /* movem.l (sp)+, d0-a6 */
  p4[24]=0x4E; p4[25]=0x73;                       /* rte */

  CacheClearU();

  old_trap1  = SetExc(33, (APTR)p1);
  old_trap2  = SetExc(34, (APTR)p2);
  old_trap3  = SetExc(35, (APTR)p2);
  old_trap13 = SetExc(45, (APTR)p3);
  old_trap14 = SetExc(46, (APTR)p4);
}

void remove_traps() {
  if (old_trap1)  SetExc(33, old_trap1);
  if (old_trap2)  SetExc(34, old_trap2);
  if (old_trap3)  SetExc(35, old_trap3);
  if (old_trap13) SetExc(45, old_trap13);
  if (old_trap14) SetExc(46, old_trap14);

  if (trap_stub_mem) {
    FreeMem(trap_stub_mem, 128);
    trap_stub_mem = NULL;
  }
}


// ---------------------------------------------------------------------------
// Logbase ($0A) - Get logical screen base
// ---------------------------------------------------------------------------
void xbios_logbase() {
  ctx[0] = (long)gem_screen_mem;
	return ;
}


// ---------------------------------------------------------------------------
// Getrez ($0B) - Get screen resolution
// 0=low (320x200), 1=medium (640x200), 2=high (640x400)
// ---------------------------------------------------------------------------
void xbios_getrez() {
  // Return low resolution
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Setscreen ($0C) - Set screen parameters
// D1 = logbase, D2 = physbase, D3 = resolution
// ---------------------------------------------------------------------------
void xbios_setscreen() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Setpalette ($0D) - Set full palette
// A0 = pointer to 16 color palette entries
// ---------------------------------------------------------------------------
void xbios_setpalette() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Setcolor ($0E) - Set single color
// D1 = color index, D2 = color value
// ---------------------------------------------------------------------------
void xbios_setcolor() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Floprd ($10) - Floppy read sectors
// D1 = drive, D2 = sector, D3 = count, A0 = buffer
// ---------------------------------------------------------------------------
void xbios_floprd() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Flopwr ($11) - Floppy write sectors
// ---------------------------------------------------------------------------
void xbios_flopwr() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Flopfmt ($12) - Floppy format
// ---------------------------------------------------------------------------
void xbios_flopfmt() {
  ctx[0] = E_ERROR;
	return ;
}


// ---------------------------------------------------------------------------
// Flopstatus ($13) - Floppy status
// ---------------------------------------------------------------------------
void xbios_flopstatus() {
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Rsconf ($17) - RS232 configuration
// D1-D7 = RS232 parameters
// Returns old configuration
// ---------------------------------------------------------------------------
void xbios_rsconf() {
  ctx[0] = E_OK;
	return ;
}


// ---------------------------------------------------------------------------
// Keytbl ($18) - Get/set keyboard table
// D1 = table type, A0 = new table (or 0 to read)
// ---------------------------------------------------------------------------
void xbios_keytbl() {
  ctx[0] = 0;
	return ;
}


// ---------------------------------------------------------------------------
// Random ($19) - Get random number
// D0 = pseudo-random number
// ---------------------------------------------------------------------------
void xbios_random() {
  struct DateStamp now;
  DateStamp(& now );
  ctx[0] = (now.ds_Days + now.ds_Minute )* 60 + now.ds_Tick;
	return ;
}


// ---------------------------------------------------------------------------
// Cookieptr ($1E) - Get cookie jar pointer
// A0 = pointer to cookie jar (Atari ST cookie jar)
// ---------------------------------------------------------------------------
void xbios_cookieptr() {
  ctx[0] = gem_cookie_jar;
	return ;
}
