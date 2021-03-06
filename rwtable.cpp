rw_entry_t rwtable[] = {
	{ "active",			TOKEN(T_ACTIVE) },
	{ "adding",			TOKEN(T_ADDING) },
	{ "all",			TOKEN(T_ALL) },
	{ "alternative",		TOKEN(T_ALTERNATIVE) },
	{ "and",			TOKEN(T_AND) },
	{ "array",			TOKEN(T_ARRAY) },	/* NON STANDARD! */
	{ "axioms",			TOKEN(T_AXIOMS) },
	{ "block",			TOKEN(T_BLOCK) },
	{ "call",			TOKEN(T_CALL) },
	{ "channel",			TOKEN(T_CHANNEL) },
	{ "comment",			TOKEN(T_COMMENT) },
	{ "connect",			TOKEN(T_CONNECT) },
	{ "constant",			TOKEN(T_CONSTANT) },
	{ "constants",			TOKEN(T_CONSTANTS) },
	{ "create",			TOKEN(T_CREATE) },
	{ "dcl",			TOKEN(T_DCL) },
	{ "decision",			TOKEN(T_DECISION) },
	{ "default",			TOKEN(T_DEFAULT) },
	{ "else",			TOKEN(T_ELSE) },
	{ "endalternative",		TOKEN(T_ENDALTERNATIVE) },
	{ "endblock",			TOKEN(T_ENDBLOCK) },
	{ "endchannel",			TOKEN(T_ENDCHANNEL) },
	{ "enddecision",		TOKEN(T_ENDDECISION) },
	{ "endgenerator",		TOKEN(T_ENDGENERATOR) },
	{ "endmacro",			TOKEN(T_ENDMACRO) },
	{ "endnewtype",			TOKEN(T_ENDNEWTYPE) },
	{ "endprocedure",		TOKEN(T_ENDPROCEDURE) },
	{ "endprocess",			TOKEN(T_ENDPROCESS) },
	{ "endrefinement",		TOKEN(T_ENDREFINEMENT) },
	{ "endselect",			TOKEN(T_ENDSELECT) },
	{ "endservice",			TOKEN(T_ENDSERVICE) },
	{ "endstate",			TOKEN(T_ENDSTATE) },
	{ "endsubstructure",		TOKEN(T_ENDSUBSTRUCTURE) },
	{ "endsyntype",			TOKEN(T_ENDSYNTYPE) },
	{ "endsystem",			TOKEN(T_ENDSYSTEM) },
	{ "env",			TOKEN(T_ENV) },
	{ "error",			TOKEN(T_ERROR) },
	{ "export",			TOKEN(T_EXPORT) },
	{ "exported",			TOKEN(T_EXPORTED) },
	{ "external",			TOKEN(T_EXTERNAL) },
	{ "false",			TOKEN(T_FALSE) },
	{ "fi",				TOKEN(T_FI) },
	{ "fix",  			TOKEN(T_FIX) },
	{ "float",			TOKEN(T_FLOAT) },
	{ "for",			TOKEN(T_FOR) },
	{ "fpar",			TOKEN(T_FPAR) },
	{ "from",			TOKEN(T_FROM) },
	{ "generator",			TOKEN(T_GENERATOR) },
	{ "if",				TOKEN(T_IF) },
	{ "import",			TOKEN(T_IMPORT) },
	{ "imported",			TOKEN(T_IMPORTED) },
	{ "in",				TOKEN(T_IN) },
	{ "inherits",			TOKEN(T_INHERITS) },
	{ "input",			TOKEN(T_INPUT) },
	{ "join",			TOKEN(T_JOIN) },
	{ "literal",			TOKEN(T_LITERAL) },
	{ "literals",			TOKEN(T_LITERALS) },
	{ "macro",			TOKEN(T_MACRO) },
	{ "macrodefinition",		TOKEN(T_MACRODEFINITION) },
	{ "macroid",			TOKEN(T_MACROID) },
	{ "map",			TOKEN(T_MAP) },
	{ "mod",			TOKEN(T_MOD) },
	{ "nameclass",			TOKEN(T_NAMECLASS) },
	{ "newtype",			TOKEN(T_NEWTYPE) },
	{ "nextstate",			TOKEN(T_NEXTSTATE) },
	{ "not",			TOKEN(T_NOT) },
	{ "now",			TOKEN(T_NOW) },
	{ "of",				TOKEN(T_OF) },	/* NON STANDARD! */
	{ "offspring",			TOKEN(T_OFFSPRING) },
	{ "operator",			TOKEN(T_OPERATOR) },
	{ "operators",			TOKEN(T_OPERATORS) },
	{ "or",				TOKEN(T_OR) },
	{ "ordering",			TOKEN(T_ORDERING) },
	{ "out",			TOKEN(T_OUT) },
	{ "output",			TOKEN(T_OUTPUT) },
	{ "parent",			TOKEN(T_PARENT) },
	{ "priority",			TOKEN(T_PRIORITY) },
	{ "procedure",			TOKEN(T_PROCEDURE) },
	{ "process",			TOKEN(T_PROCESS) },
	{ "provided",			TOKEN(T_PROVIDED) },
	{ "read",			TOKEN(T_READ) },
	{ "referenced",			TOKEN(T_REFERENCED) },
	{ "refinement",			TOKEN(T_REFINEMENT) },
	{ "rem",			TOKEN(T_REM) },
	{ "reset",			TOKEN(T_RESET) },
	{ "return",			TOKEN(T_RETURN) },
	{ "revealed",			TOKEN(T_REVEALED) },
	{ "reverse",			TOKEN(T_REVERSE) },
	{ "save",			TOKEN(T_SAVE) },
	{ "select",			TOKEN(T_SELECT) },
	{ "self",			TOKEN(T_SELF) },
	{ "sender",			TOKEN(T_SENDER) },
	{ "service",			TOKEN(T_SERVICE) },
	{ "set",			TOKEN(T_SET) },
	{ "signal",			TOKEN(T_SIGNAL) },
	{ "signallist",			TOKEN(T_SIGNALLIST) },
	{ "signalroute",		TOKEN(T_SIGNALROUTE) },
	{ "signalset",			TOKEN(T_SIGNALSET) },
	{ "spelling",			TOKEN(T_SPELLING) },
	{ "start",			TOKEN(T_START) },
	{ "state",			TOKEN(T_STATE) },
	{ "stop",			TOKEN(T_STOP) },
	{ "struct",			TOKEN(T_STRUCT) },
	{ "substructure",		TOKEN(T_SUBSTRUCTURE) },
	{ "synonym",			TOKEN(T_SYNONYM) },
	{ "syntype",			TOKEN(T_SYNTYPE) },
	{ "system",			TOKEN(T_SYSTEM) },
	{ "task",			TOKEN(T_TASK) },
	{ "then",			TOKEN(T_THEN) },
	{ "timer",			TOKEN(T_TIMER) },
	{ "to",				TOKEN(T_TO) },
	{ "true",			TOKEN(T_TRUE) },
	{ "type",			TOKEN(T_TYPE) },
	{ "via",			TOKEN(T_VIA) },
	{ "view",			TOKEN(T_VIEW) },
	{ "viewed",			TOKEN(T_VIEWED) },
	{ "with",			TOKEN(T_WITH) },
	{ "write",			TOKEN(T_WRITE) },
	{ "xor",			TOKEN(T_XOR) }
};
     
