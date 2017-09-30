#pragma hdrfile "AST.SYM"
#include "sdlast.h"

//-------------------------------------------------------------------
// Stubs for decompilation functions

#ifdef DECOMPILE
#undef DECOMPILE
#endif

#define DECOMPILE(Typ, v)	ostream& operator<<(ostream& os, Typ &v)\
				{ (void)v; return os; }

DECOMPILE(hash_entry_t, h)
DECOMPILE(name_t, n)
DECOMPILE(system_def_t, s)
DECOMPILE(signallist_t, s)
DECOMPILE(siglist_def_t, s)
DECOMPILE(signal_def_t, s)
DECOMPILE(connect_def_t, con)
DECOMPILE(c2r_def_t, c2r)
DECOMPILE(path_t, p)
DECOMPILE(channel_def_t, c)
DECOMPILE(route_def_t, r)
DECOMPILE(subblock_def_t, s)
DECOMPILE(block_def_t, b)
DECOMPILE(ident_t, i)
DECOMPILE(array_def_t, a)
DECOMPILE(fieldgrp_t, f)
DECOMPILE(struct_def_t, s)
DECOMPILE(data_def_t, d)
DECOMPILE(import_def_t, i)
DECOMPILE(timer_def_t, t)
DECOMPILE(variable_decl_t, v)
DECOMPILE(variable_def_t, v)
DECOMPILE(view_def_t, v)
DECOMPILE(procedure_formal_param_t, p)
DECOMPILE(selector_t, t)
DECOMPILE(var_ref_t, v)
DECOMPILE(cond_expr_t, ce)
DECOMPILE(int_literal_t, il)
DECOMPILE(timer_active_expr_t, tae)
DECOMPILE(view_expr_t, ve)
DECOMPILE(unop_t, uo)
DECOMPILE(expression_t, e)
DECOMPILE(assignment_t, a)
DECOMPILE(output_arg_t, a)
DECOMPILE(output_t, o)
DECOMPILE(timer_set_t, t)
DECOMPILE(timer_reset_t, t)
DECOMPILE(invoke_node_t, i)
DECOMPILE(range_condition_t, rc)
DECOMPILE(subdecision_node_t, sd)
DECOMPILE(decision_node_t, d)
DECOMPILE(gnode_t, n)
DECOMPILE(transition_t, t)
DECOMPILE(stimulus_t, s)
DECOMPILE(input_part_t, i)
DECOMPILE(save_part_t, s)
DECOMPILE(continuous_signal_t, s)
DECOMPILE(qualifier_elt_t, s)
DECOMPILE(state_node_t, s)
DECOMPILE(procedure_def_t, p)
DECOMPILE(process_formal_param_t, p)
DECOMPILE(process_def_t, p)

