#include "PX_Object_Debug.h"
#include "PX_Syntax_ir.h"

PX_Object_Debug_MonitorTypeParser PX_Object_Debug_GetTypeParser(PX_Object* pObject, const px_char type[])
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int i;
	if (!type)
		return PX_NULL;
	for (i = 0; i < PX_VectorSize(&pDesc->type_parsers); i++)
	{
		PX_Object_Debug_MonitorTypeParse* pParser = PX_VECTORAT(PX_Object_Debug_MonitorTypeParse, &pDesc->type_parsers, i);
		if (PX_Syntax_TypeMatch(type, PX_StringGetText(&pParser->type)))
			return pParser->parser;
	}
	return PX_NULL;
}

static px_void PX_Object_Debug_ClearCache(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_VectorClear(&pDesc->cache_pages);
}

px_bool PX_Object_Debug_UpdateCacheHit(PX_Object* pObject, px_dword address)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int i;
	PX_Object_Debug_CachePage newpage = {0};
	px_dword compare_address=address / PX_OBJECT_DEBUG_PAGE_SIZE * PX_OBJECT_DEBUG_PAGE_SIZE;
	for (i = 0; i < pDesc->cache_pages.size; i++)
	{
		PX_Object_Debug_CachePage* ppage = PX_VECTORAT(PX_Object_Debug_CachePage, &pDesc->cache_pages, i);
		PX_ASSERTIFX(ppage == PX_NULL, "cache page address is PX_NULL");
		if (ppage&&ppage->page_address == compare_address)
		{
			return PX_TRUE;
		}
	}
	newpage.page_address = compare_address;
	if (!PX_VectorPushback(&pDesc->cache_pages, &newpage))
	{
		PX_FSM_SetState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_ERROR);
		return PX_FALSE;
	}
	return PX_FALSE;
}

static px_void PX_Object_Debug_RefreshMonitor(PX_Object* pObject, px_int index)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_dword page_start_address;
	px_int page_count, j;
	PX_Object_Debug_Monitor* pmonitor = PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, index);
	if (pmonitor == PX_NULL)
	{
		PX_ASSERT();
		return;
	}
	//check range
	if (pmonitor->address<pDesc->machine_address_start || pmonitor->address+pmonitor->size>pDesc->machine_address_end)
	{
		pmonitor->ready = PX_FALSE;
		return;
	}
	if (PX_strequ(PX_StringGetText(&pmonitor->from), "global"))
	{}
	else if (PX_strequ(PX_StringGetText(&pmonitor->from), "local"))
	{
		if (pmonitor->address < pDesc->reg.sp)
		{
			pmonitor->ready = PX_FALSE;
			return;
		}
	}
	else if (PX_strequ(PX_StringGetText(&pmonitor->from), "param"))
	{
		if (pmonitor->address < pDesc->reg.bp)
		{
			pmonitor->ready = PX_FALSE;
			return;
		}

	}
	else
	{
		pmonitor->ready = PX_FALSE;
		return;
	}

	page_start_address = pmonitor->address / PX_OBJECT_DEBUG_PAGE_SIZE * PX_OBJECT_DEBUG_PAGE_SIZE;
	page_count = (pmonitor->size + pmonitor->address%PX_OBJECT_DEBUG_PAGE_SIZE + PX_OBJECT_DEBUG_PAGE_SIZE - 1) / PX_OBJECT_DEBUG_PAGE_SIZE;
	for (j = 0; j < page_count; j++)
	{
		if (PX_Object_Debug_UpdateCacheHit(pObject, page_start_address + j * PX_OBJECT_DEBUG_PAGE_SIZE))
		{
			pmonitor->ready = PX_TRUE;
		}
		else
		{
			pmonitor->ready = PX_FALSE;
			return;
		}
	}
}

static px_void PX_Object_Debug_RefreshAllMonitors(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int i;
	PX_Object_Debug_ClearCache(pObject);
	for ( i = 0; i < pDesc->monitors.size; i++)
	{
		PX_Object_Debug_RefreshMonitor(pObject, i);
	}
}

static px_bool PX_Object_Debug_MonitorParseAbi(PX_Object* pObject, px_int index, px_abi* poutabi)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_Object_Debug_Monitor* pmonitor = PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, index);
	PX_ASSERTIFX(!pmonitor, "monitor should not be PX_NULL");
	if (pmonitor->ready)
	{
		px_dword address = pmonitor->address;
		px_dword size = pmonitor->size;
		px_memory data;
		if (size==0)
		{
			return PX_TRUE;
		}
		PX_MemoryInitialize(pDesc->mp, &data);
		while (size > 0)
		{
			px_int j;
			for (j = 0; j < pDesc->cache_pages.size; j++)
			{
				PX_Object_Debug_CachePage* pcachepage = PX_VECTORAT(PX_Object_Debug_CachePage, &pDesc->cache_pages, j);
				if (pcachepage && address >= pcachepage->page_address && address < pcachepage->page_address + sizeof(pcachepage->buffer))
				{
					px_dword page_offset = address - pcachepage->page_address;
					px_dword page_remaining = sizeof(pcachepage->buffer) - page_offset;
					if (size >= page_remaining)
					{
						if (!PX_MemoryCat(&data, pcachepage->buffer + page_offset, page_remaining))
						{
							return PX_FALSE;
						}
						size -= page_remaining;
						address = pcachepage->page_address + sizeof(pcachepage->buffer);
						break;
					}
					else
					{
						if (!PX_MemoryCat(&data, pcachepage->buffer + page_offset, size))
						{
							return PX_FALSE;
						}
						size = 0;
						break;
					}
				}
			}
			if (j == pDesc->cache_pages.size)
			{
				PX_MemoryClear(&data);
				break;
			}
		}
		if (data.usedsize)
		{
			//match monitor type parser
			PX_Object_Debug_MonitorTypeParser parser = PX_Object_Debug_GetTypeParser(pObject, PX_StringGetText(&pmonitor->type));
			if (parser)
				parser(pObject, PX_StringGetText(&pmonitor->name), PX_StringGetText(&pmonitor->type), PX_MemoryGetData(&data), PX_MemoryGetSize(&data), poutabi);
		}
		PX_MemoryFree(&data);
	}
	else
	{
		PX_AbiSet_string(poutabi, PX_StringGetText(&pmonitor->name),"N/A");
	}
	return PX_TRUE;
}

static px_void PX_Object_Debug_RefreshMonitorTree(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int i;
	px_abi* ptree_abi;
	PX_Object_TreeClear(pDesc->ui_tree);
	ptree_abi = PX_Object_TreeGetRootAbi(pDesc->ui_tree);
	for (i = 0; i < pDesc->monitors.size; i++)
	{
		PX_Object_Debug_Monitor* pmonitor = PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, i);
		PX_ASSERTIFX(pmonitor == PX_NULL, "monitor is PX_NULL");
		PX_Object_Debug_MonitorParseAbi(pObject, i, ptree_abi);
	}
	PX_Object_TreeReRender(pDesc->ui_tree);

}

static px_int PX_Object_Debug_Request(PX_Object *pObject,px_abi *prequest_abi)
{
	const px_char* popcode;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	pDesc->alloc_request_id +=1+ PX_rand() % 0xff;
	pDesc->last_request_id = pDesc->alloc_request_id;
	pDesc->last_request_elapsed = 0;
	if (!PX_AbiSet_dword(prequest_abi,"id", pDesc->last_request_id))
	{
		return -1;
	}

	//set if
	popcode = PX_AbiGet_string(prequest_abi, "opcode");
	if (popcode)
	{
		PX_printf("request---->%s:%d\n", popcode, pDesc->last_request_id);
	}
	PX_ObjectExecuteEvent(pObject, PX_OBJECT_BUILD_EVENT_DATA(PX_OBJECT_DEBUG_EVENT_REQUEST, PX_AbiGet_Pointer(prequest_abi), PX_AbiGet_Size(prequest_abi)));
	return pDesc->last_request_id;
}


static px_void PX_Object_Debug_ToggleBreakpoint(PX_Object_Debug* pDesc, px_int source_index, px_int line)
{

}

PX_OBJECT_EVENT_FUNCTION(PX_Object_Debug_OnRun)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, (PX_Object*)ptr);
	//TODO: implement run logic
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_Debug_OnPause)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, (PX_Object *)ptr);
	//TODO: implement pause logic
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_Debug_OnStep)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, (PX_Object *)ptr);
	px_int state= PX_FSM_GetCurrentState(&pDesc->fsm);
	if (state ==PX_OBJECT_DEBUG_STATE_PAUSE)
	{
		PX_Object_Code* pcode = PX_ObjectGetDesc0(PX_Object_Code, pDesc->source_code_viewer);
		if (pcode->bordercolor._argb.a)
		{
			pDesc->last_step_source_index = pDesc->current_view_source_index;
			pDesc->last_step_cursor_line = PX_Object_Code_GetCurrentCursorLine(pDesc->source_code_viewer);

			PX_FSM_SetState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_NEXT);
		}
		//pcode = PX_ObjectGetDesc0(PX_Object_Code, pDesc->ir_code_viewer);
		else //if (pcode->bordercolor._argb.a)
		{
			pDesc->last_step_ip = pDesc->reg.ip;
			PX_FSM_SetState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT);
		}
	}
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_Debug_OnStop)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug,  (PX_Object*)ptr);
	//TODO: implement stop logic
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_Debug_OnReset)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, (PX_Object*)ptr);
	PX_FSM_SetState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_RESET);
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_Debug_OnTabButtonExecute)
{
	px_int i;
	PX_Object* pDebugObject = (PX_Object*)ptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pDebugObject);
	for (i = 0; i < pDesc->tab_buttons.size; i++)
	{
		PX_Object* pButton = *PX_VECTORAT(PX_Object*, &pDesc->tab_buttons, i);
		if (pButton == pObject)
		{
			PX_SyntaxLexer_Source* psrc;
			pDesc->current_view_source_index = i;
			psrc = PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->sources, i);
			PX_Object_Code_SetSource(pDesc->source_code_viewer, psrc);
			return;
		}
	}
}

PX_OBJECT_EVENT_FUNCTION(PX_Object_Debug_OnTreeExecute)
{
	PX_Object* pDebugObject = (PX_Object*)ptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pDebugObject);
	PX_Object_TreeNode* pNode =	PX_Object_TreeGetCurrentSelectNode(pDesc->ui_tree);
	if (pNode)
	{
		PX_Object_TextViewerSetText(pDesc->textviewer_out, PX_StringGetText(&pNode->content));
	}
	else
	{
		PX_Object_TextViewerSetText(pDesc->textviewer_out, "");
	}
}

static px_void PX_Object_Debug_ReorderTab(PX_Object* pObject)
{
	px_int i;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_float x = 0;
	for (i = 0; i < pDesc->tab_buttons.size; i++)
	{
		PX_Object* pButton = *PX_VECTORAT(PX_Object*, &pDesc->tab_buttons, i);
		pButton->x = x;
		x += pButton->Width + 2;
	}
	PX_Object_ScrollAreaUpdateRange(pDesc->area_tab);
}

px_void  PX_Object_Debug_UpdateTabButtons(PX_Object* pObject)
{
	px_int i;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	for (i = 0; i < pDesc->tab_buttons.size; i++)
	{
		PX_Object* pObject = *PX_VECTORAT(PX_Object*, &pDesc->tab_buttons, i);
		PX_ObjectDelete(pObject);
	}
	PX_VectorClear(&pDesc->tab_buttons);
	for (i = 0; i < pDesc->sources.size; i++)
	{
		px_int  w, h; PX_Object* pButtonObject;
		PX_SyntaxLexer_Source* psource = PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->sources, i);
		PX_FontModuleTextGetRenderWidthHeight(pDesc->fm, psource->name, &w, &h);
		pButtonObject = PX_Object_PushButtonCreate(pObject->mp, pDesc->area_tab, 0, 0, w + 10, 24, psource->name, pDesc->fm);
		PX_VectorPushback(&pDesc->tab_buttons, &pButtonObject);
		PX_ObjectRegisterEvent(pButtonObject, PX_OBJECT_EVENT_EXECUTE, PX_Object_Debug_OnTabButtonExecute, pObject);
	}
	PX_Object_Debug_ReorderTab(pObject);
}



PX_OBJECT_RENDER_FUNCTION(PX_Object_Debug_List_Item_Render)
{
	px_char* pcontent = (px_char*)PX_Object_ListItemGetData(pObject);
	PX_FontModuleDrawText(psurface, PX_NULL, (px_int)pObject->x+2, (px_int)pObject->y+2, PX_ALIGN_LEFTTOP, pcontent, PX_COLOR_BLACK);
}

PX_OBJECT_LIST_ITEM_CREATE_FUNCTION(PX_Object_Debug_ListCreate)
{
	PX_ObjectSetRenderFunction(ItemObject, PX_Object_Debug_List_Item_Render, 0);
	return PX_TRUE;
}

px_bool PX_Object_Debug_is_responsed(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	if (pDesc->last_request_id && pDesc->last_request_id == pDesc->last_response_id)
	{
		return PX_TRUE;
	}
	return PX_FALSE;
}

const px_char* PX_Object_Debug_response_return(PX_Object* pObject, const px_char* opcode)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	if (PX_Object_Debug_is_responsed(pObject))
	{
		const px_char* pop = PX_AbiGet_string(&pDesc->last_response_abi, "opcode");
		if (pop && PX_strequ(pop, opcode))
			return PX_AbiGet_string(&pDesc->last_response_abi, "return");
	}
	return PX_NULL;
}


static px_void PX_Object_Debug_UpdateSourceCodeViewer(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int found = -1;
	px_int source_index = -1;
	px_int source_line = -1;
	px_int map_count, i;
	PX_SyntaxLexer_Source* psrc;
	PX_Syntax_bin_map_to_source* pmap;
	px_dword ip;

	if (!pDesc) return;
	if (!pDesc->bin_map_to_source.buffer || pDesc->bin_map_to_source.usedsize < (px_int)sizeof(PX_Syntax_bin_map_to_source))
		return;
	pmap = (PX_Syntax_bin_map_to_source*)pDesc->bin_map_to_source.buffer;
	map_count = pDesc->bin_map_to_source.usedsize / (px_int)sizeof(PX_Syntax_bin_map_to_source);
	ip = pDesc->reg.ip;
	//find largest map entry whose ip <= reg.ip
	for (i = 0; i < map_count; i++)
	{
		if (pmap[i].ip <= ip)
			found = i;
		else
			break;
	}
	if (found == -1) return;
	source_index = (px_int)pmap[found].source_index;
	if (!PX_VectorCheckIndex(&pDesc->sources, source_index))
	{
		source_index = -1;
		return;
	}

	psrc = PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->sources, source_index);

	if ((px_int)pmap[found].source_line < psrc->line_count)
		source_line = pmap[found].source_line;
	else
		source_line = -1;

	if (source_index != pDesc->current_view_source_index || source_line != PX_Object_Code_GetCurrentCursorLine(pDesc->source_code_viewer))
	{
		if (source_index != pDesc->current_view_source_index)
		{
			pDesc->current_view_source_index = source_index;
			PX_Object_Code_SetSource(pDesc->source_code_viewer, psrc);
		}
		PX_Object_Code_SetCursorLineAndView(pDesc->source_code_viewer, source_line);
	}

}


static px_void PX_Object_Debug_UpdateIRCodeViewer(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int found = -1;
	px_int source_index = -1;
	px_int source_line = -1;
	px_int map_count, i;
	PX_SyntaxLexer_Source* psrc;
	PX_Syntax_bin_map_to_ir* pmap;
	px_dword ip;

	if (!pDesc) return;
	if (!pDesc->bin_map_to_ir.buffer || pDesc->bin_map_to_ir.usedsize < (px_int)sizeof(PX_Syntax_bin_map_to_ir))
		return;
	pmap = (PX_Syntax_bin_map_to_ir*)pDesc->bin_map_to_ir.buffer;
	map_count = pDesc->bin_map_to_ir.usedsize / (px_int)sizeof(PX_Syntax_bin_map_to_ir);
	ip = pDesc->reg.ip;
	//find largest map entry whose ip <= reg.ip

	for (i = 0; i < map_count; i++)
	{
		if (pmap[i].ip <= ip)
		{
			found = i;
		}
		else
		{
			break;
		}
	}
	if (found == -1) return;
	source_index = (px_int)pmap[found].source_index;
	if (!PX_VectorCheckIndex(&pDesc->ir_sources, source_index))
	{
		source_index = -1;
		return;
	}

	psrc = PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->ir_sources, source_index);

	if ((px_int)pmap[found].source_line < psrc->line_count)
		source_line = pmap[found].source_line;
	else
		source_line = -1;

	if (source_index != pDesc->current_ir_view_source_index || source_line != PX_Object_Code_GetCurrentCursorLine(pDesc->ir_code_viewer))
	{
		if (source_index != pDesc->current_ir_view_source_index)
		{
			pDesc->current_ir_view_source_index = source_index;
			PX_Object_Code_SetSource(pDesc->ir_code_viewer, psrc);
		}
		PX_Object_Code_SetCursorLineAndView(pDesc->ir_code_viewer, source_line);
	}
}


static px_void PX_Object_Debug_UpdateRegisters(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int ri;
	px_char hexbuf[16];
	px_dword val;
	const px_char* names[] = { "R0","R1","R2","R3","R4(IP)","R5(SP)","R6(BP)","R7(FLAG)" };
	PX_Object_Debug_UpdateSourceCodeViewer(pObject);
	PX_Object_Debug_UpdateIRCodeViewer(pObject);
	for (ri = 0; ri < 8; ri++)
	{
		px_int hi;
		val = pDesc->reg.r[ri];
		PX_strcpy(hexbuf, "00000000", 9);
		for (hi = 7; hi >= 0; hi--)
		{
			px_dword nibble = val & 0xF;
			hexbuf[hi] = (px_char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
			val >>= 4;
		}
		if (ri == 7)
		{
			PX_sprintf3(pDesc->list_info_content[ri], 64, "%1 0x%2 %3",
				PX_STRINGFORMAT_STRING(names[ri]),
				PX_STRINGFORMAT_STRING(hexbuf),
				PX_STRINGFORMAT_STRING(
					(pDesc->reg.Z ? "Z1:" : "Z0:")
				));
			PX_strcat(pDesc->list_info_content[ri], pDesc->reg.N ? "N1:" : "N0:");
			PX_strcat(pDesc->list_info_content[ri], pDesc->reg.C ? "C1:" : "C0:");
			PX_strcat(pDesc->list_info_content[ri], pDesc->reg.V ? "V1" : "V0");
		}
		else
		{
			PX_sprintf3(pDesc->list_info_content[ri], 64, "%1 0x%2 %3",
				PX_STRINGFORMAT_STRING(names[ri]),
				PX_STRINGFORMAT_STRING(hexbuf),
				PX_STRINGFORMAT_INT((px_int)pDesc->reg.r[ri]));
		}
	}
	for (ri = 0; ri < 4; ri++)
	{
		px_int hi;
		px_dword fraw;
		PX_memcpy(&fraw, &pDesc->reg.f[ri], sizeof(px_dword));
		val = fraw;
		PX_strcpy(hexbuf, "00000000", 9);
		for (hi = 7; hi >= 0; hi--)
		{
			px_dword nibble = val & 0xF;
			hexbuf[hi] = (px_char)(nibble < 10 ? '0' + nibble : 'a' + nibble - 10);
			val >>= 4;
		}
		PX_sprintf3(pDesc->list_info_content[8 + ri], 64, "F%1 0x%2 %3",
			PX_STRINGFORMAT_INT(ri),
			PX_STRINGFORMAT_STRING(hexbuf),
			PX_STRINGFORMAT_FLOAT(pDesc->reg.f[ri]));
	}
}
static px_void PX_Object_Debug_HandleGetStateResponse(PX_Object* pObject)
{
	px_dword* pr, * pf, * pbp;
	px_dword size;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_FSM* pfsm = &pDesc->fsm;
	const px_char* pstate = PX_AbiGet_string(&pDesc->last_response_abi, "state");
	PX_strset(pDesc->vm_state, "Machine:");
	if (pstate)
		PX_strcat_s(pDesc->vm_state,sizeof(pDesc->vm_state), pstate);
	//update registers
	pr = PX_AbiGet_data(&pDesc->last_response_abi, "r", &size);
	if (pr && size == sizeof(px_dword) * 12)
	{
		PX_memcpy(pDesc->reg.r, pr, sizeof(px_dword) * 12);
	}
	else
	{
		PX_printf("Failed to get registers from response ABI.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
	}

	pf = PX_AbiGet_data(&pDesc->last_response_abi, "f", &size);
	if (pf && size == sizeof(px_float32) * 4)
	{
		PX_memcpy(pDesc->reg.f, pf, sizeof(px_float32) * 4);
	}
	else
	{
		PX_printf("Failed to get floating-point registers from response ABI.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
	}

	pbp = PX_AbiGet_data(&pDesc->last_response_abi, "bp", &size);
	if (pbp && size == sizeof(px_dword) * 32)
	{
		PX_memcpy(pDesc->ip_breakpoint, pbp, sizeof(px_dword) * 32);
	}
	else
	{
		PX_printf("Failed to get breakpoints from response ABI.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
	}

	PX_Object_Debug_UpdateRegisters(pObject);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_DISCONNECT)
{

}




PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_RESET)
{
	//query statepx_abi load_abi;
	px_abi request_abi;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "reset"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_RESET: request failed.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_RESET_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_RESET_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "reset");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_UPLOADING);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_RESET_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_RESET_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_UPLOADING)
{
	px_abi request_abi;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "load"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (!PX_AbiSet_Abi(&request_abi, "bin", &pDesc->bin_packet_abi))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	PX_Object_Debug_Request(pObject, &request_abi);
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_UPLOADING_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_UPLOADING_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "load");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_GET_CONFIG);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_UPLOADING_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_UPLOADING_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			return;
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_GET_CONFIG)
{
	px_abi request_abi;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "get_config"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_GET_CONFIG: request failed.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_GET_CONFIG_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_GET_CONFIG_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "get_config");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			px_dword* p;
			p = PX_AbiGet_dword(&pDesc->last_response_abi, "text_size");
			if (p) pDesc->machine_text_size = *p;
			p = PX_AbiGet_dword(&pDesc->last_response_abi, "rdata_size");
			if (p) pDesc->machine_rdata_size = *p;
			p = PX_AbiGet_dword(&pDesc->last_response_abi, "gp");
			if (p) pDesc->machine_gp = *p;
			else PX_printf("PX_Object_Debug_STATE_GET_CONFIG_WAIT: response missing 'gp' field.\n");
			p = PX_AbiGet_dword(&pDesc->last_response_abi, "rp");
			if (p) pDesc->machine_rp = *p;
			p = PX_AbiGet_dword(&pDesc->last_response_abi, "address_start");
			if (p) pDesc->machine_address_start = *p;
			p = PX_AbiGet_dword(&pDesc->last_response_abi, "address_end");
			if (p) pDesc->machine_address_end = *p;
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_QUERY_STATE);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_GET_CONFIG_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_GET_CONFIG_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_READ_MEMORY)
{
	//query state px_abi;
	px_dword start_address ;
	px_dword size;
	px_abi request_abi;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);

	if(PX_FSM_CheckParameterExist(pfsm, "address") && PX_FSM_CheckParameterExist(pfsm, "size"))
	{
		start_address = PX_FSM_GetParameter_dword(pfsm, "address");
		size = PX_FSM_GetParameter_dword(pfsm, "size");
	}
	else
	{
		PX_printf("PX_Object_Debug_STATE_READ_MEMORY: missing parameter 'address'(%s) or 'size'(%s).\n",
			PX_FSM_CheckParameterExist(pfsm, "address") ? "exist" : "missing",
			PX_FSM_CheckParameterExist(pfsm, "size") ? "exist" : "missing");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}

	if (!PX_AbiSet_string(&request_abi, "opcode", "read_memory"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (!PX_AbiSet_dword(&request_abi, "address", start_address))
	{
		PX_AbiFree(&request_abi);
		return;
	}

	//1024bytes page
	if (!PX_AbiSet_dword(&request_abi, "size",PX_OBJECT_DEBUG_PAGE_SIZE))
	{
		PX_AbiFree(&request_abi);
		return;
	}

	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_READ_MEMORY: request failed (address=0x%08X, size=%u).\n", start_address, size);
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_READ_MEMORY_WAIT);
}

static px_bool PX_Object_Debug_SetMonitor(PX_Object* pObject, px_int index, const px_char pname[], const px_char ptype[], const px_char pfrom[], const px_dword offset, const px_dword size)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_Object_Debug_Monitor* pmonitor = PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, index);
	if (!pmonitor) return PX_FALSE;
	if (!PX_StringSet(&pmonitor->name, pname) ||
		!PX_StringSet(&pmonitor->type, ptype) ||
		!PX_StringSet(&pmonitor->from, pfrom))
		return PX_FALSE;
	if (PX_strequ(pfrom, "global"))
		pmonitor->address = pDesc->machine_gp + offset;
	else if (PX_strequ(pfrom, "local"))
	{
		if (pDesc->reg.bp - offset - size >= 0)
			pmonitor->address = pDesc->reg.bp - offset - size;
	}
	else if (PX_strequ(pfrom, "param"))
		pmonitor->address = pDesc->reg.bp + offset;
	else
		return PX_FALSE;
	pmonitor->size = size;
	PX_Object_Debug_RefreshMonitor(pObject, index);
	return PX_TRUE;
}

static px_bool PX_Object_Debug_NewMonitor(PX_Object* pObject, const px_char pname[], const px_char ptype[], const px_char pfrom[], const px_dword offset, const px_dword size)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_Object_Debug_Monitor newMonitor = { 0 };
	px_int new_index;
	PX_StringInitialize(pDesc->mp, &newMonitor.name);
	PX_StringInitialize(pDesc->mp, &newMonitor.type);
	PX_StringInitialize(pDesc->mp, &newMonitor.from);
	if (!PX_VectorPushback(&pDesc->monitors, &newMonitor))
		goto _ERROR;
	new_index = pDesc->monitors.size - 1;
	if (!PX_Object_Debug_SetMonitor(pObject, new_index, pname, ptype, pfrom, offset, size))
		return PX_FALSE; // vector entry holds initialized-empty strings; SetMonitor only fails on OOM
	PX_Object_Debug_RefreshMonitor(pObject, new_index);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&newMonitor.name);
	PX_StringFree(&newMonitor.type);
	PX_StringFree(&newMonitor.from);
	return PX_FALSE;
}

// Called every Update tick: detects cursor movement and updates monitor[0] with the new variable info.
static px_void PX_Object_Debug_UpdateCursorMonitor(PX_Object* pObject)
{
	px_int current_abi_index;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	if (!pObject->Visible)
		return;
	current_abi_index = PX_Object_Code_GetCursorAbiIndex(pDesc->source_code_viewer);
	if (current_abi_index!=-1&&current_abi_index != pDesc->last_cursor_abi_index)
	{
		// clear runtime hint on the previously highlighted abi entry
		px_abi* pabi;
		// parse variable info from cursor abi and load into monitor[0]
		pabi = PX_Object_Code_GetCursorAbi(pDesc->source_code_viewer);
		const px_char* ptype = PX_AbiGet_string(pabi, "type");
		if (ptype && PX_strequ(ptype, "variable"))
		{
			const px_char* pname = PX_AbiGet_string(pabi, "variable_name");
			const px_char* pvtype = PX_AbiGet_string(pabi, "variable_type");
			const px_char* pfrom = PX_AbiGet_string(pabi, "variable_from");
			px_dword* poffset = PX_AbiGet_dword(pabi, "variable_offset");
			px_dword* psize = PX_AbiGet_dword(pabi, "variable_size");
			if (!poffset || !psize || !pname || !pvtype || !pfrom)
			{
				PX_ASSERTX("cursor abi variable fields missing");
				return;
			}
			PX_ASSERTIFX(pDesc->monitors.size == 0, "monitors.size == 0, should not happen");
			PX_Object_Debug_SetMonitor(pObject, 0, pname, pvtype, pfrom, *poffset, *psize);
			do//build runtime hint string from monitor[0] data
			{
				px_abi newabi;
				px_string build_abi_string;
				PX_AbiCreate_DynamicWriter(&newabi, pDesc->mp);
				if (!PX_Object_Debug_MonitorParseAbi(pObject, 0, &newabi))
				{
					PX_AbiFree(&newabi);
					break;
				}
				if (!PX_StringInitialize(pDesc->mp, &build_abi_string))
				{
					PX_AbiFree(&newabi);
					break;
				}
				if (!PX_Abi2String(&newabi, &build_abi_string))
				{
					PX_ASSERTX("PX_Abi2String failed");
					PX_StringFree(&build_abi_string);
					PX_AbiFree(&newabi);
					break;
				}
				if (!PX_AbiSet_string(pabi, "runtime", PX_StringGetText(&build_abi_string)))
				{
					PX_ASSERTX("PX_AbiSet_string failed");
					PX_StringFree(&build_abi_string);
					PX_AbiFree(&newabi);
					break;
				}
				PX_StringFree(&build_abi_string);
				PX_AbiFree(&newabi);
			} while (0);
			PX_Object_Debug_MonitorParseAbi(pObject, 0, pabi);
			PX_Object_Debug_RefreshMonitorTree(pObject);
			pDesc->last_cursor_abi_index = current_abi_index;
		}
		
	}
}

// Called after cache pages are fully synchronised: reads monitor[0] data from cache
// and writes the formatted value into the code-viewer abi as a "runtime" hint.
px_void PX_Object_Debug_RefreshCursorMonitor(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_abi* pabi;
	if (!pObject->Visible)
		return;
	if(pDesc->last_cursor_abi_index==-1)
		return;
	if(pDesc->last_cursor_abi_index!=PX_Object_Code_GetCursorAbiIndex(pDesc->source_code_viewer))
		return;
	pabi = PX_Object_Code_GetDescAbi(pDesc->source_code_viewer, pDesc->last_cursor_abi_index);
	if (pabi)
	{
		px_abi newabi;
		PX_AbiCreate_DynamicWriter(&newabi, pDesc->mp);
		if (PX_Object_Debug_MonitorParseAbi(pObject, 0, &newabi))
		{
			px_string build_abi_string;
			PX_StringInitialize(pDesc->mp, &build_abi_string);
			if (!PX_Abi2String(&newabi, &build_abi_string))
			{
				PX_ASSERTX("PX_Abi2String failed");
				PX_StringFree(&build_abi_string);
				PX_AbiFree(&newabi);
				return;
			}
			if (!PX_AbiSet_string(pabi, "runtime", PX_StringGetText(&build_abi_string)))
			{
				PX_ASSERTX("PX_AbiSet_string failed");
				PX_StringFree(&build_abi_string);
				PX_AbiFree(&newabi);
				return;
			}
			PX_StringFree(&build_abi_string);
		}
		PX_AbiFree(&newabi);
		PX_Object_Code_Refresh(pDesc->source_code_viewer);
	}
}


px_void PX_Object_Debug_HandleReadMemoryResponse(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_dword *paddress;
	px_dword size=0;
	px_void* data;
	paddress = PX_AbiGet_dword(&pDesc->last_response_abi, "address");
	data = PX_AbiGet_data(&pDesc->last_response_abi, "data", &size);
	if (paddress&&data && size > 0)
	{
		px_int i;
		for (i = 0; i < pDesc->cache_pages.size; i++)
		{
			PX_Object_Debug_CachePage* pPage = PX_VECTORAT(PX_Object_Debug_CachePage, &pDesc->cache_pages, i);
			if (pPage && !pPage->synchronized && pPage->page_address == *paddress&&size==sizeof(pPage->buffer))
			{
				PX_memcpy(pPage->buffer, data, size);
				pPage->synchronized = PX_TRUE;
				PX_printf("Memory page 0x%08X synchronized.\n", pPage->page_address);
				break;
			}
		}
		
		for (i = 0; i < pDesc->cache_pages.size; i++)
		{
			PX_Object_Debug_CachePage* pPage = PX_VECTORAT(PX_Object_Debug_CachePage, &pDesc->cache_pages, i);
			if (pPage&&!pPage->synchronized)
			{
				break;
			}
		}

		if (i== pDesc->cache_pages.size)
		{
			PX_Object_Debug_RefreshMonitorTree(pObject);
			PX_Object_Debug_RefreshCursorMonitor(pObject);
		}
	}
	else
	{
		PX_printf("Failed to get memory data from response ABI.\n");
	}
}


PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_READ_MEMORY_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "read_memory");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_Object_Debug_HandleReadMemoryResponse(pObject);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_PAUSE);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_READ_MEMORY_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_READ_MEMORY_WAIT time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			return;
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_WRITE_MEMORY)
{
	px_abi request_abi;
	px_dword address;
	px_int data_size;
	px_void* data;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);

	if (!PX_FSM_CheckParameterExist(pfsm, "address") ||
		!PX_FSM_CheckParameterExist(pfsm, "data"))
	{
		PX_printf("PX_Object_Debug_STATE_WRITE_MEMORY: missing parameter 'address'(%s) or 'data'(%s).\n",
			PX_FSM_CheckParameterExist(pfsm, "address") ? "exist" : "missing",
			PX_FSM_CheckParameterExist(pfsm, "data") ? "exist" : "missing");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		return;
	}

	address = PX_FSM_GetParameter_dword(pfsm, "address");
	data_size = PX_FSM_GetParameter_datasize(pfsm, "data");
	data = PX_FSM_GetParameter_dataptr(pfsm, "data");
	if (!data || data_size <= 0)
	{
		PX_printf("PX_Object_Debug_STATE_WRITE_MEMORY: invalid data (address=0x%08X, data=%p, data_size=%d).\n", address, data, data_size);
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		return;
	}

	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "write_memory") ||
		!PX_AbiSet_dword(&request_abi, "address", address) ||
		!PX_AbiSet_data(&request_abi, "data", data, data_size))
	{
		PX_printf("PX_Object_Debug_STATE_WRITE_MEMORY: build request abi failed (address=0x%08X, data_size=%d).\n", address, data_size);
		PX_AbiFree(&request_abi);
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		return;
	}

	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_WRITE_MEMORY: request failed (address=0x%08X, data_size=%d).\n", address, data_size);
		PX_AbiFree(&request_abi);
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		return;
	}

	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_WRITE_MEMORY_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_WRITE_MEMORY_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "write_memory");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_PAUSE);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_WRITE_MEMORY_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}

	if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
	{
		PX_printf("PX_Object_Debug_STATE_WRITE_MEMORY_WAIT: time out error!\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_QUERY_STATE)
{
	//query state px_abi;
	px_abi request_abi;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "get_state"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_QUERY_STATE: request failed.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_QUERY_STATE_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_QUERY_STATE_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "get_state");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			const px_char *pstate = PX_AbiGet_string(&pDesc->last_response_abi, "state");
			if (!pstate)
			{
				PX_printf("PX_Object_Debug_STATE_QUERY_STATE_WAIT: response missing 'state' field.\n");
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
				return;
			}
			if (PX_strequ(pstate, "idle"))
			{
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_UPLOADING);
			}
			else if (PX_strequ(pstate, "pause"))
			{
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_PAUSE);
			}
			else if (PX_strequ(pstate, "running"))
			{
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_RUNNING);
				PX_Object_Debug_RefreshAllMonitors((PX_Object*)userptr);
			}
			else if (PX_strequ(pstate, "error"))
			{
				PX_printf("PX_Object_Debug_STATE_QUERY_STATE_WAIT: VM reported error state.\n");
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			}
			else
			{
				PX_printf("PX_Object_Debug_STATE_QUERY_STATE_WAIT: unknown state '%s'.\n", pstate);
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			}
			PX_Object_Debug_HandleGetStateResponse(pObject);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_QUERY_STATE_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_QUERY_STATE_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			return;
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_PAUSE)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int i;
	for (i = 0; i < pDesc->cache_pages.size; i++)
	{
		PX_Object_Debug_CachePage* pPage = PX_VECTORAT(PX_Object_Debug_CachePage, &pDesc->cache_pages, i);
		PX_ASSERTIFX(pPage == PX_NULL, "Cache page is null");
		if (pPage && !pPage->synchronized)
		{
			PX_FSM_SetParameter_dword(pfsm, "address", pPage->page_address);
			PX_FSM_SetParameter_dword(pfsm, "size", sizeof(pPage->buffer));
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_READ_MEMORY);
			return;
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_NEXT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	//query state px_abi;
	px_abi request_abi;
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "step"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (!PX_AbiSet_dword(&request_abi, "tick", 1))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_STEP_NEXT: request failed.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_Object_Debug_RefreshAllMonitors((PX_Object*)userptr);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_STEP_NEXT_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_NEXT_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "step");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_STEP_NEXT_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_STEP_NEXT_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			return;
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_NEXT_QUERY_STATE)
{
	//query state px_abi;
	px_abi request_abi;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "get_state"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_STEP_NEXT_QUERY_STATE: request failed.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_NEXT_QUERY_STATE_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "get_state");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_Object_Debug_HandleGetStateResponse(pObject);
			if (pDesc->last_step_cursor_line!=PX_Object_Code_GetCurrentCursorLine(pDesc->source_code_viewer) || pDesc->last_step_source_index!=(px_dword)pDesc->current_view_source_index)
			{
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_PAUSE);
			}
			else
			{
				PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_STEP_NEXT);
			}
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_STEP_NEXT_QUERY_STATE_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_STEP_NEXT_QUERY_STATE_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			return;
		}
	}
}

//
PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_IR_NEXT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	//query state px_abi;
	px_abi request_abi;
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "step"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (!PX_AbiSet_dword(&request_abi, "tick", 1))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_STEP_IR_NEXT: request failed.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT_WAIT);
	PX_Object_Debug_RefreshAllMonitors((PX_Object*)userptr);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_IR_NEXT_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "step");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_STEP_IR_NEXT_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_STEP_IR_NEXT_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			return;
		}
	}
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_IR_QUERY_STATE)
{
	//query state px_abi;
	px_abi request_abi;
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	PX_AbiCreate_DynamicWriter(&request_abi, pDesc->mp);
	if (!PX_AbiSet_string(&request_abi, "opcode", "get_state"))
	{
		PX_AbiFree(&request_abi);
		return;
	}
	if (PX_Object_Debug_Request(pObject, &request_abi) == -1)
	{
		PX_printf("PX_Object_Debug_STATE_STEP_IR_QUERY_STATE: request failed.\n");
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		PX_AbiFree(&request_abi);
		return;
	}
	PX_AbiFree(&request_abi);
	PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE_WAIT);
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_STEP_IR_QUERY_STATE_WAIT)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	const px_char* preturn = PX_Object_Debug_response_return(pObject, "get_state");
	if (preturn)
	{
		if (PX_strequ(preturn, "ok"))
		{
			PX_Object_Debug_HandleGetStateResponse(pObject);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_PAUSE);
		}
		else
		{
			PX_printf("PX_Object_Debug_STATE_STEP_IR_QUERY_STATE_WAIT return:%s\n", preturn);
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
		}
		return;
	}
	else
	{
		if (pDesc->last_request_elapsed >= PX_OBJECT_DEBUG_REQUEST_TIMEOUT)
		{
			PX_printf("PX_Object_Debug_STATE_STEP_IR_QUERY_STATE_WAIT: time out error!\n");
			PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_ERROR);
			return;
		}
	}
}
//

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_RUNNING)
{
	PX_Object* pObject = (PX_Object*)userptr;
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	pDesc->fsm_running_update_elapsed += elapsed;
	if (pDesc->fsm_running_update_elapsed >= 500)
	{
		pDesc->fsm_running_update_elapsed = 0;
		PX_FSM_SetState(pfsm, PX_OBJECT_DEBUG_STATE_QUERY_STATE);
		PX_Object_Debug_RefreshAllMonitors((PX_Object*)userptr);
	}
	return;
}

PX_FSM_UPDATE_FUNCTION(PX_Object_Debug_STATE_ERROR)
{
	//do nothing, wait for user command or state change
	return;
}

PX_OBJECT_UPDATE_FUNCTION(PX_Object_Debug_Update)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	if (!pObject->Visible)
	{
		return;
	}
	pDesc->last_request_elapsed += elapsed;
	PX_FSM_Update(&pDesc->fsm, elapsed);
	PX_Object_Debug_UpdateCursorMonitor(pObject);
}




px_void PX_Object_Debug_Response(PX_Object* pObject, const px_byte payload[], px_int size)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	if (size>0)
	{
		px_abi resp_abi;
		const px_char* popcode;
		px_dword* pid;
		if (!pDesc) return;
		PX_AbiCreate_StaticReader(&resp_abi, payload, size);
		if (!PX_AbiCheck(&resp_abi)) return;
		popcode = PX_AbiGet_string(&resp_abi, "opcode");
		if (!popcode) return;
		pid = PX_AbiGet_dword(&resp_abi, "id");
		if (!pid) return;

		PX_printf("response<----%s:%d\n", popcode,*pid);
		if (pDesc->last_request_id != *pid) return;
		PX_AbiClear(&pDesc->last_response_abi);
		if (PX_AbiCopy_FromBuffer(&pDesc->last_response_abi, payload, size))
		{
			pDesc->last_response_id = pDesc->last_request_id;
		}
		pDesc->last_request_elapsed = 0;
		PX_FSM_ExecuteEvent(&pDesc->fsm, PX_FSM_BUILD_EVENT(PX_OBJECT_DEBUG_FSM_EVENT_RESPONSE));
	}
	else
	{
		PX_FSM_ExecuteEvent(&pDesc->fsm, PX_FSM_BUILD_EVENT(PX_OBJECT_DEBUG_FSM_EVENT_DISCONNECT));
	}

}

PX_OBJECT_RENDER_FUNCTION(PX_Object_Debug_Render)
{
	px_rect region = PX_ObjectGetRect(pObject);
	PX_Object_Debug* pDesc = PX_ObjectGetDesc(PX_Object_Debug, pObject);
	px_int content_ir_panel_width= (px_int)region.width * 3 / 4 - 20;
	px_int content_panel_width, content_panel_height;

	content_panel_width = content_ir_panel_width * 5 / 8;
	content_panel_height = ((px_int)region.height - 32) * 3 / 4 - 20;
	if (content_panel_height<100)
	{
		content_panel_height = 100;
	}

	if (pObject->Width<100)
	{
		pObject->Width = 100;
	}
	if (pObject->Height < 100)
	{
		pObject->Height = 100;
	}

	pDesc->area_tab->x = 0;
	pDesc->area_tab->y = 0;
	pDesc->area_tab->Width = region.width/2;
	pDesc->area_tab->Height = 32;
	pDesc->controller_panel->x = pDesc->area_tab->Width+32;
	pDesc->controller_panel->y = 0;

	pDesc->ui_list->x = 0;
	pDesc->ui_list->y = pDesc->area_tab->Height;
	pDesc->ui_list->Width = region.width / 4;
	pDesc->ui_list->Height = (region.height - (px_int)pDesc->area_tab->Height)/2;

	pDesc->ui_tree->x = 0;
	pDesc->ui_tree->y = pDesc->area_tab->Height + pDesc->ui_list->Height;
	pDesc->ui_tree->Width = region.width / 4;
	pDesc->ui_tree->Height = (region.height - (px_int)pDesc->area_tab->Height) / 2;

	pDesc->source_code_viewer->x = pDesc->ui_tree->Width;
	pDesc->source_code_viewer->y = pDesc->area_tab->Height;
	pDesc->source_code_viewer->Width = (px_float)content_panel_width + 20;
	pDesc->source_code_viewer->Height = (px_float)content_panel_height + 20;

	pDesc->textviewer_out->x = pDesc->ui_tree->Width + content_panel_width + 20.f;
	pDesc->textviewer_out->y = pDesc->ui_list->y;
	pDesc->textviewer_out->Width = region.width - content_panel_width - pDesc->ui_tree->Width - 20;
	pDesc->textviewer_out->Height = content_panel_height+20.f;


	pDesc->ir_code_viewer->x = pDesc->textviewer_out->x;
	pDesc->ir_code_viewer->y = pDesc->textviewer_out->y;
	pDesc->ir_code_viewer->Width = pDesc->textviewer_out->Width;
	pDesc->ir_code_viewer->Height = pDesc->textviewer_out->Height;

	pDesc->printer->Width = region.width - pDesc->ui_tree->Width;
	pDesc->printer->Height = (region.height - 32) / 4;
	pDesc->printer->x = pDesc->ui_tree->Width;
	pDesc->printer->y = pDesc->area_tab->Height + content_panel_height + 40;

	switch (PX_FSM_GetCurrentState(&pDesc->fsm))
	{
	case PX_OBJECT_DEBUG_STATE_DISCONNECT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: disconnect");
		break;
	case PX_OBJECT_DEBUG_STATE_RESET:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: reset");
		break;
	case PX_OBJECT_DEBUG_STATE_RESET_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: reset wait");
		break;
	case PX_OBJECT_DEBUG_STATE_UPLOADING:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: uploading");
		break;
	case PX_OBJECT_DEBUG_STATE_UPLOADING_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: uploading wait");
		break;
	case PX_OBJECT_DEBUG_STATE_GET_CONFIG:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: get config");
		break;
	case PX_OBJECT_DEBUG_STATE_GET_CONFIG_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: get config wait");
		break;
	case PX_OBJECT_DEBUG_STATE_QUERY_STATE:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: query state");
		break;
	case PX_OBJECT_DEBUG_STATE_QUERY_STATE_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: query state wait");
		break;
	case PX_OBJECT_DEBUG_STATE_PAUSE:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: pause");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_NEXT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step next");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_NEXT_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step next wait");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step query state");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step query state wait");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step IR next");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step IR next wait");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step IR query state");
		break;
	case PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE_WAIT:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: step IR query state wait");
		break;
	case PX_OBJECT_DEBUG_STATE_RUNNING:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: running");
		break;
	case PX_OBJECT_DEBUG_STATE_ERROR:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: error");
		break;
	default:
		PX_Object_LabelSetText(pDesc->debugger_state_label, "State: unknown");
		break;
	}

	PX_Object_LabelSetText(pDesc->vm_state_label, pDesc->vm_state);
}



static px_void PX_Object_Debug_FreeSource(px_memorypool* mp, PX_SyntaxLexer_Source* psrc)
{
	px_int j;
	if (psrc->name) MP_Free(mp, psrc->name);
	if (psrc->source) MP_Free(mp, psrc->source);
	if (psrc->source_index_map_to_line_index) MP_Free(mp, psrc->source_index_map_to_line_index);
	if (psrc->cells) MP_Free(mp, psrc->cells);
	if (psrc->source_index_map_to_cell_index) MP_Free(mp, psrc->source_index_map_to_cell_index);
	if (psrc->line_begin_cell_index_map) MP_Free(mp, psrc->line_begin_cell_index_map);
	for (j = 0; j < PX_VectorSize(&psrc->descriptor); j++)
	{
		PX_AbiFree(PX_VECTORAT(px_abi, &psrc->descriptor, j));
	}
	PX_VectorFree(&psrc->descriptor);
}

static px_void PX_Object_Debug_ClearMonitors(PX_Object_Debug* pDesc)
{
	px_int i;
	for (i = 0; i < PX_VectorSize(&pDesc->monitors); i++)
	{
		PX_Object_Debug_Monitor* pMonitor = PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, i);
		PX_StringFree(&pMonitor->name);
		PX_StringFree(&pMonitor->type);
		PX_StringFree(&pMonitor->from);
	}
	PX_VectorClear(&pDesc->monitors);
}

static px_void PX_Object_Debug_ClearTypeParsers(PX_Object_Debug* pDesc)
{
	px_int i;
	for (i = 0; i < PX_VectorSize(&pDesc->type_parsers); i++)
	{
		PX_Object_Debug_MonitorTypeParse* pParser = PX_VECTORAT(PX_Object_Debug_MonitorTypeParse, &pDesc->type_parsers, i);
		PX_StringFree(&pParser->type);
	}
	PX_VectorClear(&pDesc->type_parsers);
}

px_bool PX_Object_Debug_RegisterTypeParser(PX_Object* pObject, const px_char type[], PX_Object_Debug_MonitorTypeParser parser)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int i;
	PX_Object_Debug_MonitorTypeParse newParser = { 0 };
	if (!type || !parser)
		return PX_FALSE;
	//if a parser for this type already exists, overwrite it
	for (i = 0; i < PX_VectorSize(&pDesc->type_parsers); i++)
	{
		PX_Object_Debug_MonitorTypeParse* pParser = PX_VECTORAT(PX_Object_Debug_MonitorTypeParse, &pDesc->type_parsers, i);
		if (PX_strequ(PX_StringGetText(&pParser->type), type))
		{
			pParser->parser = parser;
			return PX_TRUE;
		}
	}
	PX_StringInitialize(pDesc->mp, &newParser.type);
	if (!PX_StringSet(&newParser.type, type))
	{
		PX_StringFree(&newParser.type);
		return PX_FALSE;
	}
	newParser.parser = parser;
	if (!PX_VectorPushback(&pDesc->type_parsers, &newParser))
	{
		PX_StringFree(&newParser.type);
		return PX_FALSE;
	}
	return PX_TRUE;
}



px_void PX_Object_Debug_Clear(PX_Object* pObject)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int i;
	if (!pDesc)
	{
		return;
	}
	pDesc->current_view_source_index = -1;
	pDesc->current_ir_view_source_index = -1;
	PX_Object_Code_SetSource(pDesc->source_code_viewer, PX_NULL);

	PX_Object_TreeClear(pDesc->ui_tree);
	PX_Object_ListClear(pDesc->ui_list);
	for (i = 0; i < pDesc->list_contents.size; i++)
	{
		px_char* p = *PX_VECTORAT(px_char*, &pDesc->list_contents, i);
		MP_Free(pDesc->mp, p);
	}
	PX_VectorClear(&pDesc->list_contents);

	for (i = 0; i < PX_VectorSize(&pDesc->sources); i++)
	{
		PX_Object_Debug_FreeSource(pDesc->mp, PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->sources, i));
	}
	PX_VectorClear(&pDesc->sources);
	for (i = 0; i < PX_VectorSize(&pDesc->ir_sources); i++)
	{
		PX_Object_Debug_FreeSource(pDesc->mp, PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->ir_sources, i));
	}
	PX_VectorClear(&pDesc->ir_sources);
	PX_MemoryClear(&pDesc->bin_map_to_source);
	PX_MemoryClear(&pDesc->bin_map_to_ir);
	PX_AbiClear(&pDesc->bin_packet_abi);
	PX_memset(pDesc->ip_breakpoint, 0xFF, sizeof(pDesc->ip_breakpoint));
	PX_memset(pDesc->breakpoints, 0, sizeof(pDesc->breakpoints));
	PX_VectorClear(&pDesc->pages_state);
	PX_Object_Debug_ClearMonitors(pDesc);
	PX_VectorResize(&pDesc->monitors, 1); //restore cursor monitor slot
	PX_StringInitialize(pDesc->mp, &PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, 0)->name);
	PX_StringInitialize(pDesc->mp, &PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, 0)->type);
	PX_StringInitialize(pDesc->mp, &PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, 0)->from);
}

PX_OBJECT_FREE_FUNCTION(PX_Object_Debug_Free)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	if (pDesc)
	{
		PX_Object_Debug_Clear(pObject);
		PX_VectorFree(&pDesc->tab_buttons);
		PX_VectorFree(&pDesc->list_contents);
		PX_VectorFree(&pDesc->pages_state);
		PX_VectorFree(&pDesc->sources);
		PX_VectorFree(&pDesc->cache_pages);
		PX_VectorFree(&pDesc->ir_sources);
		PX_VectorFree(&pDesc->monitors);
		PX_Object_Debug_ClearTypeParsers(pDesc);
		PX_VectorFree(&pDesc->type_parsers);
		PX_TextureFree(&pDesc->texture_pause);
		PX_TextureFree(&pDesc->texture_run);
		PX_TextureFree(&pDesc->texture_step);
		PX_TextureFree(&pDesc->texture_stop);
		PX_TextureFree(&pDesc->texture_reset);
		PX_MemoryFree(&pDesc->bin_map_to_source);
		PX_MemoryFree(&pDesc->bin_map_to_ir);
		PX_AbiFree(&pDesc->bin_packet_abi);
		PX_AbiFree(&pDesc->last_response_abi);
		PX_FSM_Free(&pDesc->fsm);
	}
}



PX_OBJECT_EVENT_FUNCTION(PX_OBJECT_OnSourceCodeViewerCursorDown)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, (PX_Object*)ptr);
	if (PX_ObjectIsCursorInRegion(pObject, e))
	{
		PX_Object_Code_SetBorderColor(pObject, PX_COLOR_RED);
		PX_Object_Code_SetBorderColor(pDesc->ir_code_viewer, PX_COLOR_NONE);
	}
}

PX_OBJECT_EVENT_FUNCTION(PX_OBJECT_OnIRCodeViewerCursorDown)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, (PX_Object*)ptr);
	if (PX_ObjectIsCursorInRegion(pObject, e))
	{
		PX_Object_Code_SetBorderColor(pObject, PX_COLOR_RED);
		PX_Object_Code_SetBorderColor(pDesc->source_code_viewer, PX_COLOR_NONE);
	}
}

PX_OBJECT_DEBUG_MONITORTYPEPARSER_FUNCTION(PX_Object_Debug_MonitorType_ix_i)
{
	return PX_AbiSet_string(pabi, name, PX_itos(*(px_int*)data,10).data);
}

PX_OBJECT_DEBUG_MONITORTYPEPARSER_FUNCTION(PX_Object_Debug_MonitorType_ix_u)
{
	return PX_AbiSet_string(pabi, name, PX_utos(*(px_uint*)data, 10).data);
}

PX_OBJECT_DEBUG_MONITORTYPEPARSER_FUNCTION(PX_Object_Debug_MonitorType_fx)
{
	return PX_AbiSet_string(pabi, name, PX_ftos(*(px_float*)data, 10).data);
}

PX_OBJECT_DEBUG_MONITORTYPEPARSER_FUNCTION(PX_Object_Debug_MonitorType_array_ix_iu_8)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_int count = size;
	px_int i;
	px_string str;
	px_bool istext = PX_TRUE;
	if (!PX_StringInitialize(pDesc->mp, &str))
	{
		PX_ASSERTX("PX_StringInitialize failed");
		return PX_FALSE;
	}
	count > 128 ? count = 128 : 0;
	for (i = 0; i < count; i++)
	{
		if (!PX_charIsCommonlyCharacter((px_char)data[i]))
		{
			istext = PX_FALSE;
			break;
		}
	}
	if (istext&& count!=1)
	{
		for (i = 0; i < count; i++)
		{
			PX_StringCatChar(&str,(px_char)data[i]);
		}
	}
	else
	{
		for (i = 0; i < count; i++)
		{
			PX_StringCat(&str, PX_itos(data[i], 10).data);
			if (i != count - 1)
				PX_StringCat(&str, ",");
		}
	}
	PX_AbiSet_string(pabi, name, PX_StringGetText(&str));
	PX_StringFree(&str);
	return PX_TRUE;
}


PX_Object* PX_Object_Debug_Create(px_memorypool* mp, PX_Object* Parent, px_int x, px_int y, px_int Width, px_int Height, PX_FontModule* fm)
{
	PX_Object* pObject;
	PX_Object_Debug* pDesc;
	px_int i;
	pObject = PX_ObjectCreateEx(mp, Parent, (px_float)x, (px_float)y,0, (px_float)Width, (px_float)Height,0, PX_OBJECT_TYPE_DEBUG, PX_Object_Debug_Update, PX_Object_Debug_Render, PX_Object_Debug_Free,0, sizeof(PX_Object_Debug));
	pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	if (!pDesc)
	{
		return PX_NULL;
	}
	pDesc->fm = fm;
	pDesc->mp = mp; 
	pDesc->last_cursor_abi_index = -1;
	pDesc->area_tab = PX_Object_ScrollAreaCreate(mp, pObject, 0, 0, Width/2, 32);
	if (!pDesc->area_tab)
		goto _ERROR;
	PX_Object_ScrollAreaSetHSliderBarHeight(pDesc->area_tab, 8);
	if(!PX_VectorInitialize(mp, &pDesc->tab_buttons, sizeof(PX_Object*), 0))
		goto _ERROR;
	if(!PX_VectorInitialize(mp, &pDesc->pages_state, sizeof(PX_Object_Debug_pagememory), 32))
		goto _ERROR;
	if(!PX_VectorInitialize(mp, &pDesc->list_contents, sizeof(px_char *), 32))
		goto _ERROR;
	if(!PX_VectorInitialize(mp, &pDesc->sources, sizeof(PX_SyntaxLexer_Source), 0))
		goto _ERROR;
	if(!PX_VectorInitialize(mp, &pDesc->ir_sources, sizeof(PX_SyntaxLexer_Source), 0))
		goto _ERROR;
	if(!PX_VectorInitialize(mp, &pDesc->cache_pages, sizeof(PX_Object_Debug_CachePage), 32))
		goto _ERROR;
	if(!PX_VectorInitialize(mp, &pDesc->monitors, sizeof(PX_Object_Debug_Monitor), 0))
		goto _ERROR;
	if (!PX_VectorResize(&pDesc->monitors, 1))//cursor monitor
		goto _ERROR;
	PX_StringInitialize(pDesc->mp, &PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, 0)->name);
	PX_StringInitialize(pDesc->mp, &PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, 0)->type);
	PX_StringInitialize(pDesc->mp, &PX_VECTORAT(PX_Object_Debug_Monitor, &pDesc->monitors, 0)->from);
	if(!PX_VectorInitialize(mp, &pDesc->type_parsers, sizeof(PX_Object_Debug_MonitorTypeParse), 0))
		goto _ERROR;
	PX_AbiCreate_DynamicWriter(&pDesc->last_response_abi, mp);
	PX_memset(pDesc->ip_breakpoint, 0xFF, sizeof(pDesc->ip_breakpoint));
	PX_memset(pDesc->breakpoints, 0, sizeof(pDesc->breakpoints));
	PX_MemoryInitialize(mp, &pDesc->bin_map_to_source);
	PX_MemoryInitialize(mp, &pDesc->bin_map_to_ir);
	PX_AbiCreate_DynamicWriter(&pDesc->bin_packet_abi, mp);
	PX_memset(&pDesc->reg, 0, sizeof(pDesc->reg));
	pDesc->current_view_source_index = -1;
	pDesc->current_ir_view_source_index = -1;
	pDesc->last_request_id = 1;
	pDesc->messagebox = PX_Object_MessageBoxCreate(mp, pObject, fm);
	if (!pDesc->messagebox)
		goto _ERROR;
	PX_ObjectSetVisible(pDesc->messagebox, PX_FALSE);

	pDesc->ui_list = PX_Object_ListCreate(mp, pObject, 0, (px_int)pDesc->area_tab->Height, Width / 4, (Height - (px_int)pDesc->area_tab->Height)/2, 20, PX_Object_Debug_ListCreate, pObject);
	if (!pDesc->ui_list)
		goto _ERROR;
	//add content
	for(i=0;i<16; i++)
	{
		//PX_strset(pDesc->list_info_content[i], "N/A");
		PX_Object_ListAdd(pDesc->ui_list,pDesc->list_info_content[i]);
	}

	pDesc->ui_tree = PX_Object_TreeCreate(mp, pObject, 0, (px_int)(pDesc->area_tab->Height+ pDesc->ui_list->Height), Width / 4, (Height - (px_int)pDesc->area_tab->Height) / 2, 20 ,fm);
	if (!pDesc->ui_tree)
		goto _ERROR;
	PX_ObjectRegisterEvent(pDesc->ui_tree, PX_OBJECT_EVENT_EXECUTE, PX_Object_Debug_OnTreeExecute, pObject);

	pDesc->printer = PX_Object_PrinterCreate(mp, pObject, 0, 0, Width / 4 * 3, Height, fm);
	if (!pDesc->printer)
		goto _ERROR;
	pDesc->source_code_viewer = PX_Object_Code_Create(mp, pObject, 0, 0, Width * 5 / 8, Height * 3 / 4, fm);
	if (!pDesc->source_code_viewer)
		goto _ERROR;


	pDesc->textviewer_out=PX_Object_TextViewerCreate(mp, pObject, 0, 0, 200, Height, fm);
	if (!pDesc->textviewer_out)
		goto _ERROR;
	PX_ObjectSetVisible(pDesc->textviewer_out, PX_FALSE);
	pDesc->ir_code_viewer= PX_Object_Code_Create(mp, pObject, 0, 0, 200, Height, fm);
	if (!pDesc->ir_code_viewer)
		goto _ERROR;
	PX_ObjectSetVisible(pDesc->ir_code_viewer, PX_TRUE);

	PX_ObjectRegisterEvent(pDesc->source_code_viewer, PX_OBJECT_EVENT_CURSORDOWN, PX_OBJECT_OnSourceCodeViewerCursorDown, pObject);
	PX_ObjectRegisterEvent(pDesc->ir_code_viewer, PX_OBJECT_EVENT_CURSORDOWN, PX_OBJECT_OnIRCodeViewerCursorDown, pObject);

	if(!PX_TextureCreate(mp,&pDesc->texture_run,20,20))
		goto _ERROR;

	if (!PX_TextureCreate(mp, &pDesc->texture_pause, 20, 20))
		goto _ERROR;

	if (!PX_TextureCreate(mp, &pDesc->texture_stop, 20, 20))
		goto _ERROR;
	if (!PX_TextureCreate(mp, &pDesc->texture_step, 20, 20))
		goto _ERROR;

	if (!PX_TextureCreate(mp, &pDesc->texture_reset, 20, 20))
		goto _ERROR;

	PX_GeoDrawRightTriangle(&pDesc->texture_run, 0, 0, 19, 19, PX_COLOR(255, 128, 128, 128));
	PX_GeoDrawRect(&pDesc->texture_stop, 0, 0, 19, 19, PX_COLOR(255, 128, 128, 128));
	PX_GeoDrawArrow(&pDesc->texture_step, PX_POINT2D(0, 10), PX_POINT2D(19, 10), 1, PX_COLOR(255, 128, 128, 128));
	PX_GeoDrawRect(&pDesc->texture_pause, 0, 0, 8, 19, PX_COLOR(255, 128, 128, 128));
	PX_GeoDrawRect(&pDesc->texture_pause, 11, 0, 19, 19, PX_COLOR(255, 128, 128, 128));
	PX_GeoDrawRing(&pDesc->texture_reset, 10, 10, 5, 2, PX_COLOR(255, 128, 128, 128),0,270);
	PX_GeoDrawUpTriangle(&pDesc->texture_reset, 12, 5, 19, 10, PX_COLOR(255, 128, 128, 128));

	pDesc->controller_panel = PX_ObjectCreate(mp, pObject, 0, 0, 0, 0, 0,0 );
	if (!pDesc->controller_panel)
		goto _ERROR;
	pDesc->button_run= PX_Object_PushButtonCreate(mp, pDesc->controller_panel, 0, 3, 26, 26, "", 0);
	if (!pDesc->button_run)
		goto _ERROR;
	PX_Object_PushButtonSetTexture(pDesc->button_run, &pDesc->texture_run);
	PX_ObjectRegisterEvent(pDesc->button_run, PX_OBJECT_EVENT_EXECUTE, PX_Object_Debug_OnRun, pObject);

	pDesc->button_pause = PX_Object_PushButtonCreate(mp, pDesc->controller_panel, 28, 3, 26, 26, "", 0);
	if (!pDesc->button_pause)
		goto _ERROR;
	PX_Object_PushButtonSetTexture(pDesc->button_pause, &pDesc->texture_pause);
	PX_ObjectRegisterEvent(pDesc->button_pause, PX_OBJECT_EVENT_EXECUTE, PX_Object_Debug_OnPause, pObject);

	pDesc->button_step = PX_Object_PushButtonCreate(mp, pDesc->controller_panel,  28*2, 3, 26, 26, "", 0);
	if (!pDesc->button_step)
		goto _ERROR;
	PX_Object_PushButtonSetTexture(pDesc->button_step, &pDesc->texture_step);
	PX_ObjectRegisterEvent(pDesc->button_step, PX_OBJECT_EVENT_EXECUTE, PX_Object_Debug_OnStep, pObject);

	pDesc->button_stop = PX_Object_PushButtonCreate(mp, pDesc->controller_panel, 28 * 3, 3, 26, 26, "", 0);
	if (!pDesc->button_stop)
		goto _ERROR;
	PX_Object_PushButtonSetTexture(pDesc->button_stop, &pDesc->texture_stop);
	PX_ObjectRegisterEvent(pDesc->button_stop, PX_OBJECT_EVENT_EXECUTE, PX_Object_Debug_OnStop, pObject);

	pDesc->button_reset = PX_Object_PushButtonCreate(mp, pDesc->controller_panel, 28 * 4, 3, 26, 26, "", 0);
	if (!pDesc->button_reset)
		goto _ERROR;
	PX_Object_PushButtonSetTexture(pDesc->button_reset, &pDesc->texture_reset);
	PX_ObjectRegisterEvent(pDesc->button_reset, PX_OBJECT_EVENT_EXECUTE, PX_Object_Debug_OnReset, pObject);


	pDesc->debugger_state_label = PX_Object_LabelCreate(mp, pDesc->controller_panel, 28 * 6, 3, 180, 20, "", fm, PX_COLOR_BLACK);
	if (!pDesc->debugger_state_label)
		goto _ERROR;
	PX_Object_LabelSetAlign(pDesc->debugger_state_label, PX_ALIGN_LEFTMID);

	pDesc->vm_state_label = PX_Object_LabelCreate(mp, pDesc->controller_panel, (px_int)pDesc->debugger_state_label->x+ (px_int)pDesc->debugger_state_label->Width+5, 3, 180, 20, "", fm, PX_COLOR_BLACK);
	if (!pDesc->vm_state_label)
		goto _ERROR;
	PX_Object_LabelSetAlign(pDesc->vm_state_label, PX_ALIGN_LEFTMID);
	/*
	PX_OBJECT_DEBUG_STATE_DISCONNECT,
	PX_OBJECT_DEBUG_STATE_IDLE,
	PX_OBJECT_DEBUG_STATE_IDLE_WAIT,
	PX_OBJECT_DEBUG_STATE_RESET,
	PX_OBJECT_DEBUG_STATE_RESET_WAIT,
	PX_OBJECT_DEBUG_STATE_UPLOADING,
	PX_OBJECT_DEBUG_STATE_UPLOADING_WAIT,
	PX_OBJECT_DEBUG_STATE_QUERY_STATE,
	PX_OBJECT_DEBUG_STATE_QUERY_STATE_WAIT,
	PX_OBJECT_DEBUG_STATE_PAUSE,
	PX_OBJECT_DEBUG_STATE_PAUSE_WAIT,
	PX_OBJECT_DEBUG_STATE_STEP_NEXT,
	PX_OBJECT_DEBUG_STATE_STEP_NEXT_WAIT,
	PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE,
	PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE_WAIT,
	PX_OBJECT_DEBUG_STATE_RUNNING,
	PX_OBJECT_DEBUG_STATE_ERROR
	*/
	//FSM
	if (!PX_FSM_Initialize(mp, &pDesc->fsm))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_DISCONNECT, PX_Object_Debug_STATE_DISCONNECT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_RESET, PX_Object_Debug_STATE_RESET, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_RESET_WAIT, PX_Object_Debug_STATE_RESET_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_UPLOADING, PX_Object_Debug_STATE_UPLOADING, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_UPLOADING_WAIT, PX_Object_Debug_STATE_UPLOADING_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_GET_CONFIG, PX_Object_Debug_STATE_GET_CONFIG, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_GET_CONFIG_WAIT, PX_Object_Debug_STATE_GET_CONFIG_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_QUERY_STATE, PX_Object_Debug_STATE_QUERY_STATE, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_QUERY_STATE_WAIT, PX_Object_Debug_STATE_QUERY_STATE_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_PAUSE, PX_Object_Debug_STATE_PAUSE, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_PAUSE_WAIT, PX_Object_Debug_STATE_PAUSE, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_NEXT, PX_Object_Debug_STATE_STEP_NEXT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_NEXT_WAIT, PX_Object_Debug_STATE_STEP_NEXT_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE, PX_Object_Debug_STATE_STEP_NEXT_QUERY_STATE, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_QUERY_STATE_WAIT, PX_Object_Debug_STATE_STEP_NEXT_QUERY_STATE_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT, PX_Object_Debug_STATE_STEP_IR_NEXT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_IR_NEXT_WAIT, PX_Object_Debug_STATE_STEP_IR_NEXT_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE, PX_Object_Debug_STATE_STEP_IR_QUERY_STATE, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_STEP_IR_QUERY_STATE_WAIT, PX_Object_Debug_STATE_STEP_IR_QUERY_STATE_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_RUNNING, PX_Object_Debug_STATE_RUNNING, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_ERROR, PX_Object_Debug_STATE_ERROR, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_READ_MEMORY, PX_Object_Debug_STATE_READ_MEMORY, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_READ_MEMORY_WAIT, PX_Object_Debug_STATE_READ_MEMORY_WAIT, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_WRITE_MEMORY, PX_Object_Debug_STATE_WRITE_MEMORY, pObject))return PX_NULL;
	if (!PX_FSM_NewState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_WRITE_MEMORY_WAIT, PX_Object_Debug_STATE_WRITE_MEMORY_WAIT, pObject))return PX_NULL;
	PX_FSM_SetState(&pDesc->fsm, PX_OBJECT_DEBUG_STATE_DISCONNECT);

	PX_Object_Debug_RegisterTypeParser(pObject, "ix.i", PX_Object_Debug_MonitorType_ix_i);
	PX_Object_Debug_RegisterTypeParser(pObject, "ix.u", PX_Object_Debug_MonitorType_ix_u);
	PX_Object_Debug_RegisterTypeParser(pObject, "fx", PX_Object_Debug_MonitorType_fx);
	PX_Object_Debug_RegisterTypeParser(pObject, "array.ix.i.8", PX_Object_Debug_MonitorType_array_ix_iu_8);
	PX_Object_Debug_RegisterTypeParser(pObject, "array.ix.u.8", PX_Object_Debug_MonitorType_array_ix_iu_8);

	return pObject;
_ERROR:
	PX_ObjectDelete(pObject);
	return PX_NULL;
}

static px_bool PX_Object_Debug_PackDebugSource(PX_Syntax* psource, px_abi* packabi,const px_char name[])
{
	px_int i, j;
	px_string key;
	if (!PX_StringInitialize(packabi->dynamic.mp, &key))
		return PX_FALSE;

	//packet px_syntax->px_syntaxlexer->sources
	if (!PX_StringFormat1(&key, "%1_count", PX_STRINGFORMAT_STRING(name)))
	{
		PX_StringFree(&key);
		return PX_FALSE;
	}
	if (!PX_AbiSet_int(packabi, PX_StringGetText(&key), PX_VectorSize(&psource->reg_syntaxlexer.sources)))
	{
		PX_StringFree(&key);
		return PX_FALSE;
	}
	for (i = 0; i < PX_VectorSize(&psource->reg_syntaxlexer.sources); i++)
	{
		PX_SyntaxLexer_Source* src = PX_VECTORAT(PX_SyntaxLexer_Source, &psource->reg_syntaxlexer.sources, i);
		px_abi source_abi;
		if (!src)
		{
			PX_ASSERT();
			goto _ERROR;
		}
		PX_AbiCreate_DynamicWriter(&source_abi, packabi->dynamic.mp);

		//name
		if (!PX_AbiSet_string(&source_abi, "name", src->name ? src->name : (px_char*)""))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		//source
		if (!PX_AbiSet_int(&source_abi, "source_length", src->source_length))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		if (!PX_AbiSet_data(&source_abi, "source", src->source, src->source_length))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		//line info
		if (!PX_AbiSet_int(&source_abi, "line_count", src->line_count))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		if (!PX_AbiSet_int(&source_abi, "max_line_char_width", src->max_line_char_width))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		if (!PX_AbiSet_data(&source_abi, "source_index_map_to_line_index", src->source_index_map_to_line_index, src->source_length * (px_int)sizeof(px_int)))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		//cells
		if (!PX_AbiSet_int(&source_abi, "cells_count", src->cells_count))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		if (!PX_AbiSet_data(&source_abi, "cells", src->cells, src->cells_count * (px_int)sizeof(PX_SyntaxLexer_Cell)))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		if (!PX_AbiSet_data(&source_abi, "source_index_map_to_cell_index", src->source_index_map_to_cell_index, src->source_length * (px_int)sizeof(px_int)))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		//line map
		if (!PX_AbiSet_data(&source_abi, "line_begin_cell_index_map", src->line_begin_cell_index_map, src->line_count * (px_int)sizeof(PX_SyntaxLexer_LineMap)))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		//descriptor (vector of px_abi)
		if (!PX_AbiSet_int(&source_abi, "last_descriptor_index", src->last_descriptor_index))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		if (!PX_AbiSet_int(&source_abi, "descriptor_count", PX_VectorSize(&src->descriptor)))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		for (j = 0; j < PX_VectorSize(&src->descriptor); j++)
		{
			px_abi* pdesc = PX_VECTORAT(px_abi, &src->descriptor, j);
			if (!PX_StringFormat1(&key, "descriptor[%1]", PX_STRINGFORMAT_INT(j)))
			{
				PX_AbiFree(&source_abi);
				goto _ERROR;
			}
			if (!PX_AbiSet_Abi(&source_abi, PX_StringGetText(&key), pdesc))
			{
				PX_AbiFree(&source_abi);
				goto _ERROR;
			}
		}

		if (!PX_StringFormat2(&key, "%1[%2]",PX_STRINGFORMAT_STRING(name),PX_STRINGFORMAT_INT(i)))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		if (!PX_AbiSet_Abi(packabi, PX_StringGetText(&key), &source_abi))
		{
			PX_AbiFree(&source_abi);
			goto _ERROR;
		}
		PX_AbiFree(&source_abi);
	}
	PX_StringFree(&key);
	return PX_TRUE;
_ERROR:
	PX_StringFree(&key);
	return PX_FALSE;
}

px_bool PX_Object_Debug_PackDebugInfo(PX_Syntax *psource, PX_Syntax *pir, px_abi *packabi)
{
	//check packabi is dynamic or not
	PX_ASSERTIFX(!packabi->dynamic.mp, "Pack abi must be dynamic memory");
	if (!PX_Object_Debug_PackDebugSource(psource, packabi, "source"))
		return PX_FALSE;
	if (!PX_Object_Debug_PackDebugSource(pir, packabi, "ir"))
		return PX_FALSE;
	
	do{
		px_abi* pscope = PX_Syntax_GetAbiFromForward(pir, "scope");
		if (pscope)
		{
			px_dword map_size = 0, ir_map_size = 0, bin_size = 0, rdata_size=0;
			const px_char* pimport;
			px_void* psrc_map;
			px_void* pir_map;
			px_void* pbin;
			px_void* prdata;
			px_abi export_abi;

			if (!PX_AbiExist_Type(pscope, "text", PX_ABI_TYPE_ABI))
				goto _ERROR;
			psrc_map = PX_AbiExist_Type(pscope, "bin_map_to_source", PX_ABI_TYPE_DATA) ? PX_AbiGet_data(pscope, "bin_map_to_source", &map_size) : PX_NULL;
			pir_map = PX_AbiExist_Type(pscope, "bin_map_to_ir", PX_ABI_TYPE_DATA) ? PX_AbiGet_data(pscope, "bin_map_to_ir", &ir_map_size) : PX_NULL;
			prdata = PX_AbiGet_data(pscope, "rdata", &rdata_size);
			pbin = PX_AbiGet_buffer(pscope, "text", &bin_size);
			pimport = PX_AbiGet_string(pscope, "imports");
			if ( !pbin )
				goto _ERROR;
			if (!PX_AbiSet_string(packabi, "bin.module_name", PX_Syntax_GetCurrentLexerEntrySourceName(pir)))
				goto _ERROR;

			if (pimport)
			{
				if (!PX_AbiSet_string(packabi, "bin.imports", pimport))
					goto _ERROR;
			}

			if (PX_AbiGet_AbiReadOnly(pscope, &export_abi, "exports"))
			{
				if (!PX_AbiSet_Abi(packabi, "bin.exports", &export_abi))
					goto _ERROR;
			}
			if (psrc_map)
			{
				if (!PX_AbiSet_data(packabi, "bin_map_to_source", psrc_map, (px_int)map_size))
					goto _ERROR;
			}
			if (pir_map)
			{
				if (!PX_AbiSet_data(packabi, "bin_map_to_ir", pir_map, (px_int)ir_map_size))
					goto _ERROR;
			}
			
			if (prdata)
			{
				if (!PX_AbiSet_data(packabi, "bin.rdata", prdata, (px_int)rdata_size))
					goto _ERROR;
			}

			if (!PX_AbiSet_data(packabi, "bin.text", pbin, (px_int)bin_size))
				goto _ERROR;
			
		}
		else
		{
			goto _ERROR;
		}
	}while(0);

	return PX_TRUE;
_ERROR:
	return PX_FALSE;
}

static px_bool PX_Object_Debug_UnpackDebugSource(PX_Object_Debug* pDesc, px_abi* pack_abi, const px_char name[], px_vector* target, px_string* key)
{
	px_int i, j, source_count;

	if (!PX_StringFormat1(key, "%1_count", PX_STRINGFORMAT_STRING(name)))
		return PX_FALSE;
	if (!PX_AbiExist_Type(pack_abi, PX_StringGetText(key), PX_ABI_TYPE_INT))
		return PX_FALSE;
	source_count = PX_AbiGetValue_int(pack_abi, PX_StringGetText(key));

	for (i = 0; i < source_count; i++)
	{
		PX_SyntaxLexer_Source new_source;
		px_abi source_abi;
		const px_char* pname;
		px_void* pdata;
		px_dword datasize;
		px_int descriptor_count;

		PX_memset(&new_source, 0, sizeof(new_source));

		if (!PX_StringFormat2(key, "%1[%2]", PX_STRINGFORMAT_STRING(name), PX_STRINGFORMAT_INT(i)))
			return PX_FALSE;
		if (!PX_AbiGet_AbiReadOnly(pack_abi, &source_abi, PX_StringGetText(key)))
			return PX_FALSE;

		//name
		pname = PX_AbiGet_string(&source_abi, "name");
		if (!pname) return PX_FALSE;
		new_source.name = (px_char*)MP_Malloc(pDesc->mp, PX_strlen(pname) + 1);
		if (!new_source.name) goto _ERROR_ITEM;
		PX_strcpy(new_source.name, pname, PX_strlen(pname) + 1);

		//source length & data
		new_source.source_length = PX_AbiGetValue_int(&source_abi, "source_length");
		pdata = PX_AbiGet_data(&source_abi, "source", &datasize);
		if (!pdata || (px_int)datasize != new_source.source_length) goto _ERROR_ITEM;
		new_source.source = (px_char*)MP_Malloc(pDesc->mp, new_source.source_length + 1);
		if (!new_source.source) goto _ERROR_ITEM;
		PX_memcpy(new_source.source, pdata, new_source.source_length);
		new_source.source[new_source.source_length] = 0;

		//line info
		new_source.line_count = PX_AbiGetValue_int(&source_abi, "line_count");
		new_source.max_line_char_width = PX_AbiGetValue_int(&source_abi, "max_line_char_width");
		pdata = PX_AbiGet_data(&source_abi, "source_index_map_to_line_index", &datasize);
		if (!pdata || (px_int)datasize != new_source.source_length * (px_int)sizeof(px_int)) goto _ERROR_ITEM;
		new_source.source_index_map_to_line_index = (px_int*)MP_Malloc(pDesc->mp, datasize);
		if (!new_source.source_index_map_to_line_index) goto _ERROR_ITEM;
		PX_memcpy(new_source.source_index_map_to_line_index, pdata, datasize);

		//cells
		new_source.cells_count = PX_AbiGetValue_int(&source_abi, "cells_count");
		pdata = PX_AbiGet_data(&source_abi, "cells", &datasize);
		if (!pdata || (px_int)datasize != new_source.cells_count * (px_int)sizeof(PX_SyntaxLexer_Cell)) goto _ERROR_ITEM;
		new_source.cells = (PX_SyntaxLexer_Cell*)MP_Malloc(pDesc->mp, datasize);
		if (!new_source.cells) goto _ERROR_ITEM;
		PX_memcpy(new_source.cells, pdata, datasize);

		pdata = PX_AbiGet_data(&source_abi, "source_index_map_to_cell_index", &datasize);
		if (!pdata || (px_int)datasize != new_source.source_length * (px_int)sizeof(px_int)) goto _ERROR_ITEM;
		new_source.source_index_map_to_cell_index = (px_int*)MP_Malloc(pDesc->mp, datasize);
		if (!new_source.source_index_map_to_cell_index) goto _ERROR_ITEM;
		PX_memcpy(new_source.source_index_map_to_cell_index, pdata, datasize);

		//line map
		pdata = PX_AbiGet_data(&source_abi, "line_begin_cell_index_map", &datasize);
		if (!pdata || (px_int)datasize != new_source.line_count * (px_int)sizeof(PX_SyntaxLexer_LineMap)) goto _ERROR_ITEM;
		new_source.line_begin_cell_index_map = (PX_SyntaxLexer_LineMap*)MP_Malloc(pDesc->mp, datasize);
		if (!new_source.line_begin_cell_index_map) goto _ERROR_ITEM;
		PX_memcpy(new_source.line_begin_cell_index_map, pdata, datasize);

		//descriptor
		new_source.last_descriptor_index = PX_AbiGetValue_int(&source_abi, "last_descriptor_index");
		descriptor_count = PX_AbiGetValue_int(&source_abi, "descriptor_count");
		if (!PX_VectorInitialize(pDesc->mp, &new_source.descriptor, sizeof(px_abi), descriptor_count > 0 ? descriptor_count : 1))
			goto _ERROR_ITEM;
		for (j = 0; j < descriptor_count; j++)
		{
			px_abi desc_readonly;
			px_abi desc_dynamic;
			if (!PX_StringFormat1(key, "descriptor[%1]", PX_STRINGFORMAT_INT(j)))
				goto _ERROR_ITEM_DESC;
			if (!PX_AbiGet_AbiReadOnly(&source_abi, &desc_readonly, PX_StringGetText(key)))
				goto _ERROR_ITEM_DESC;
			PX_AbiCreate_DynamicWriter(&desc_dynamic, pDesc->mp);
			if (!PX_AbiCopy_FromAbi(&desc_dynamic, &desc_readonly))
			{
				PX_AbiFree(&desc_dynamic);
				goto _ERROR_ITEM_DESC;
			}
			if (!PX_VectorPushback(&new_source.descriptor, &desc_dynamic))
			{
				PX_AbiFree(&desc_dynamic);
				goto _ERROR_ITEM_DESC;
			}
		}

		if (!PX_VectorPushback(target, &new_source))
			goto _ERROR_ITEM_DESC;

		continue;
	_ERROR_ITEM_DESC:
		for (j = 0; j < PX_VectorSize(&new_source.descriptor); j++)
			PX_AbiFree(PX_VECTORAT(px_abi, &new_source.descriptor, j));
		PX_VectorFree(&new_source.descriptor);
	_ERROR_ITEM:
		if (new_source.name) MP_Free(pDesc->mp, new_source.name);
		if (new_source.source) MP_Free(pDesc->mp, new_source.source);
		if (new_source.source_index_map_to_line_index) MP_Free(pDesc->mp, new_source.source_index_map_to_line_index);
		if (new_source.cells) MP_Free(pDesc->mp, new_source.cells);
		if (new_source.source_index_map_to_cell_index) MP_Free(pDesc->mp, new_source.source_index_map_to_cell_index);
		if (new_source.line_begin_cell_index_map) MP_Free(pDesc->mp, new_source.line_begin_cell_index_map);
		return PX_FALSE;
	}
	return PX_TRUE;
}

px_bool PX_Object_Debug_UnPackDebugInfo(PX_Object *pObject, const px_byte packed_debug_info_abi[], px_int size)
{
	PX_Object_Debug* pDesc = PX_ObjectGetDesc0(PX_Object_Debug, pObject);
	px_abi pack_abi;
	px_int i;
	px_string key;
	px_int binsize=0, rdatasize=0;
	if (!pDesc)
	{
		return PX_FALSE;
	}

	//clear previous unpacked content
	for (i = 0; i < PX_VectorSize(&pDesc->sources); i++)
	{
		PX_Object_Debug_FreeSource(pDesc->mp, PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->sources, i));
	}
	PX_VectorClear(&pDesc->sources);
	for (i = 0; i < PX_VectorSize(&pDesc->ir_sources); i++)
	{
		PX_Object_Debug_FreeSource(pDesc->mp, PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->ir_sources, i));
	}
	PX_VectorClear(&pDesc->ir_sources);
	PX_MemoryClear(&pDesc->bin_map_to_source);
	PX_MemoryClear(&pDesc->bin_map_to_ir);
	PX_AbiClear(&pDesc->bin_packet_abi);

	if (!PX_AbiCheckBufferReady(packed_debug_info_abi, size))
		return PX_FALSE;

	PX_AbiCreate_StaticReader(&pack_abi, packed_debug_info_abi, size);

	if (!PX_StringInitialize(pDesc->mp, &key))
		return PX_FALSE;

	if (!PX_Object_Debug_UnpackDebugSource(pDesc, &pack_abi, "source", &pDesc->sources, &key))
		goto _ERROR;
	if (!PX_Object_Debug_UnpackDebugSource(pDesc, &pack_abi, "ir", &pDesc->ir_sources, &key))
		goto _ERROR;

	if (PX_AbiExist_Type(&pack_abi, "bin_map_to_source", PX_ABI_TYPE_DATA))
	{
		px_dword map_size = 0;
		px_void* pmap = PX_AbiGet_data(&pack_abi, "bin_map_to_source", &map_size);
		if (pmap && map_size)
		{
			if (!PX_MemoryCat(&pDesc->bin_map_to_source, pmap, (px_int)map_size))
				goto _ERROR;
		}
	}
	if (PX_AbiExist_Type(&pack_abi, "bin_map_to_ir", PX_ABI_TYPE_DATA))
	{
		px_dword map_size = 0;
		px_void* pmap = PX_AbiGet_data(&pack_abi, "bin_map_to_ir", &map_size);
		if (pmap && map_size)
		{
			if (!PX_MemoryCat(&pDesc->bin_map_to_ir, pmap, (px_int)map_size))
				goto _ERROR;
		}
	}
	if (PX_AbiExist_Type(&pack_abi, "bin", PX_ABI_TYPE_ABI))
	{
		px_abi bin_readonly;
		if (!PX_AbiGet_AbiReadOnly(&pack_abi, &bin_readonly, "bin"))
			goto _ERROR;
		
		//check text
		if (!PX_AbiExist_Type(&bin_readonly, "text", PX_ABI_TYPE_DATA))
		{
			goto _ERROR;
		}
		binsize = PX_AbiGet_PayloadDataSize(&bin_readonly, "text");
		rdatasize = PX_AbiGet_PayloadDataSize(&bin_readonly, "rdata");
		if(!PX_AbiCopy_FromBuffer(&pDesc->bin_packet_abi, PX_AbiGet_Pointer(&bin_readonly), PX_AbiGet_Size(&bin_readonly)))
		{
			goto _ERROR;
		}
	}

	PX_StringFree(&key);
	PX_Object_Debug_UpdateTabButtons(pObject);
	if (PX_VectorCheckIndex(&pDesc->sources, 0))
	{
		pDesc->current_view_source_index = 0;
		PX_Object_Code_SetSource(pDesc->source_code_viewer, PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->sources, 0));
	}
	if (PX_VectorCheckIndex(&pDesc->ir_sources, 0))
	{
		pDesc->current_ir_view_source_index = 0;
		PX_Object_Code_SetSource(pDesc->ir_code_viewer, PX_VECTORAT(PX_SyntaxLexer_Source, &pDesc->ir_sources, 0));
	}

	pDesc->machine_gp = binsize + rdatasize;
	return PX_TRUE;
_ERROR:
	PX_StringFree(&key);
	return PX_FALSE;
}
