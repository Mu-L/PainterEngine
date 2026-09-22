#ifndef PX_OBJECT_DEBUG_H
#define PX_OBJECT_DEBUG_H
//------------------------------------------------------------------------------
//area_tab buttons ... 24
//sliderbar 8
//------------------------------------------------------------------------------
//list     1/4 |bp  lines 64|  content panel 3/4(min 75px width)				|sliderbar 20
//             |     ...    |       |        |									|
//             |     ...    |       |        |									|
//             |     ...    |bp 8   | line 32|  content (min 35px width)		|
//             |     ...    |       |        |									|
//             |     ...    |.......											|
//             |     ...    |													|
//-------------|----------------------------------------------------------------|
//tree abi 1/4 |  slider bar 20
//             |----------------------------------------------------------------
//             |  condole panel 3/4 width 1/4 height
//             |
//             |
//             |
//			   |
//-------------------------------------------------------------------------------

#include "PX_Object.h"

#define PX_OBJECT_DEBUG_REQUEST_TIMEOUT 10000

#define PX_OBJECT_DEBUG_EVENT_REQUEST 0x1000

#define PX_OBJECT_DEBUG_FSM_EVENT_CONNECTED 0x2000
#define PX_OBJECT_DEBUG_FSM_EVENT_RESPONSE 0x2001
#define PX_OBJECT_DEBUG_FSM_EVENT_DISCONNECT 0x2002

#define PX_OBJECT_DEBUG_PAGE_SIZE 1024
typedef enum
{
	PX_OBJECT_DEBUG_STATE_DISCONNECT,
	PX_OBJECT_DEBUG_STATE_RESET,
	PX_OBJECT_DEBUG_STATE_RESET_WAIT,
	PX_OBJECT_DEBUG_STATE_UPLOADING,
	PX_OBJECT_DEBUG_STATE_UPLOADING_WAIT,
	PX_OBJECT_DEBUG_STATE_GET_CONFIG,
	PX_OBJECT_DEBUG_STATE_GET_CONFIG_WAIT,
	PX_OBJECT_DEBUG_STATE_QUERY_STATE,
	PX_OBJECT_DEBUG_STATE_QUERY_STATE_WAIT,
	PX_OBJECT_DEBUG_STATE_PAUSE,
	PX_OBJECT_DEBUG_STATE_PAUSE_WAIT,
	PX_OBJECT_DEBUG_STATE_STEP_NEXT,
	PX_OBJECT_DEBUG_STATE_STEP_NEXT_WAIT,
	PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE,
	PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE_WAIT,
	PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT,
	PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT_WAIT,
	PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE,
	PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE_WAIT,
	PX_OBJECT_DEBUG_STATE_RUNNING,
	PX_OBJECT_DEBUG_STATE_READ_MEMORY,
	PX_OBJECT_DEBUG_STATE_READ_MEMORY_WAIT,
	PX_OBJECT_DEBUG_STATE_WRITE_MEMORY,
	PX_OBJECT_DEBUG_STATE_WRITE_MEMORY_WAIT,
	PX_OBJECT_DEBUG_STATE_ERROR
}PX_OBJECT_DEBUG_STATE;

typedef struct
{
	px_int row_offset;
	px_int x_offset;
}PX_Object_Debug_pagememory;

typedef struct
{
	px_int source_index;
	px_int line;
}PX_Object_Debug_breakpoint;

typedef struct
{
	union 
	{
		px_dword r[12];
		struct
		{
			px_dword r0,r1,r2,r3,ip, sp, bp;
			union
			{
				px_dword flag;
				struct
				{
					px_dword Z : 1;//(zero)
					px_dword N : 1;//(sign)
					px_dword C : 1;//(carry)
					px_dword V : 1;//(overflow)
					px_dword C0 : 1;//ST0>STi 000
					px_dword C2 : 1;//ST0<STi 100
					px_dword C3 : 1;//ST0==STi 111
					px_dword reserved : 25;
				};
				px_dword sleep,module_base;
			};
		};
		
	};
	px_float32 f[4];
}PX_Object_Debug_Register;

typedef struct
{
	px_bool synchronized;
	px_dword page_address;
	px_byte buffer[PX_OBJECT_DEBUG_PAGE_SIZE];
}PX_Object_Debug_CachePage;

typedef struct
{
	px_string name;
	px_string type;
	px_string from;
	px_dword address;
	px_int size;
	px_bool ready;
}PX_Object_Debug_Monitor;

typedef px_bool (*PX_Object_Debug_MonitorTypeParser)(PX_Object* pObject, const px_char name[], const px_char type[], const px_byte data[], px_int size,px_abi *pabi);
#define PX_OBJECT_DEBUG_MONITORTYPEPARSER_FUNCTION(func_name) px_bool func_name(PX_Object* pObject,const px_char name[], const px_char type[], const px_byte data[], px_int size,px_abi *pabi)

typedef struct
{
	px_string type;
	PX_Object_Debug_MonitorTypeParser parser;
}PX_Object_Debug_MonitorTypeParse;

typedef struct
{
	px_memorypool* mp;
	PX_FontModule* fm;
	px_int  current_view_source_index; // current view source index
	px_int  current_ir_view_source_index; // current IR view source index
	px_vector pages_state; //PX_Object_Debug_pagememory
	px_vector cache_pages; //PX_Object_Debug_CachePage
	px_vector monitors; //PX_Object_Debug_Monitor
	px_vector type_parsers; //PX_Object_Debug_MonitorTypeParse
	px_vector list_contents;//px_char *
	PX_Object* area_tab;
	px_vector  tab_buttons;
	PX_Object* ui_tree,*ui_list;
	px_char list_info_content[16][64];//r0~r7,ip,sp,bp,flag,f4
	PX_Object* controller_panel;
	PX_Object* messagebox,*debugger_state_label,* vm_state_label;
	PX_Object* button_run, *button_pause, * button_step, * button_stop,* button_reset;
	px_texture texture_run, texture_pause, texture_step, texture_stop, texture_reset;
	PX_Object* source_code_viewer,*ir_code_viewer;
	PX_Object* printer;
	PX_Object* textviewer_out;
	px_dword struct_update_delay;
	px_int last_run_circles;
	px_int last_tab_index;
	px_int last_cursor_abi_index;
	px_vector sources,ir_sources; //PX_SyntaxLexer_Source (unpacked from debug info)
	px_memory bin_map_to_source, bin_map_to_ir;
	px_abi bin_packet_abi;
	px_dword ip_breakpoint[32];
	PX_Object_Debug_breakpoint breakpoints[32];
	PX_Object_Debug_Register reg;
	px_dword machine_text_size, machine_rdata_size, machine_rp, machine_gp, machine_thread_stack_size;
	px_dword machine_address_start, machine_address_end;
	
	px_dword last_step_ip;
	px_dword last_step_source_index;
	px_dword last_step_cursor_line;
	px_char  vm_state[32];
	px_int T; //current thread index reported by Machine

	px_dword alloc_request_id; //the request id of current memory alloc/free request
	px_dword last_request_id; //monotonic counter, ++ on each new outgoing request
	px_dword last_request_elapsed; //elapsed time of last request
	px_dword last_response_id;
	px_abi   last_response_abi;

	px_dword fsm_running_update_elapsed;

	PX_FSM fsm;
}PX_Object_Debug;

PX_Object* PX_Object_Debug_Create(px_memorypool* mp, PX_Object* Parent, px_int x, px_int y, px_int Width, px_int Height, PX_FontModule* fm);
px_void PX_Object_Debug_Response(PX_Object* pObject, const px_byte payload[], px_int size);

px_bool PX_Object_Debug_RegisterTypeParser(PX_Object* pObject, const px_char type[], PX_Object_Debug_MonitorTypeParser parser);
PX_Object_Debug_MonitorTypeParser PX_Object_Debug_GetTypeParser(PX_Object* pObject, const px_char type[]);

px_bool PX_Object_Debug_PackDebugInfo(PX_Syntax* psource, PX_Syntax* pir, px_abi* packabi);
px_bool PX_Object_Debug_UnPackDebugInfo(PX_Object* pObject, const px_byte packed_debug_info_abi[], px_int size);
#endif
