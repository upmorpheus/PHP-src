/*
   +----------------------------------------------------------------------+
   | Zend JIT                                                             |
   +----------------------------------------------------------------------+
   | Copyright (c) The PHP Group                                          |
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.01 of the PHP license,      |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_01.txt                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Dmitry Stogov <dmitry@php.net>                              |
   |          Xinchen Hui <laruence@php.net>                              |
   +----------------------------------------------------------------------+
*/

#ifndef ZEND_JIT_INTERNAL_H
#define ZEND_JIT_INTERNAL_H

typedef struct _zend_jit_op_array_extension {
	zend_func_info func_info;
	const void *orig_handler;
} zend_jit_op_array_extension;

/* Profiler */
extern zend_ulong zend_jit_profile_counter;
extern int zend_jit_profile_counter_rid;

#define ZEND_COUNTER_INFO(op_array) \
	ZEND_OP_ARRAY_EXTENSION(op_array, zend_jit_profile_counter_rid)

/* Hot Counters */

#define ZEND_HOT_COUNTERS_COUNT 128

extern int16_t zend_jit_hot_counters[ZEND_HOT_COUNTERS_COUNT];

static zend_always_inline zend_long zend_jit_hash(const void *ptr)
{
	uintptr_t x;

	x = (uintptr_t)ptr >> 3;
#if SIZEOF_SIZE_T == 4
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = ((x >> 16) ^ x) * 0x45d9f3b;
	x = (x >> 16) ^ x;
#elif SIZEOF_SIZE_T == 8
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9;
	x = (x ^ (x >> 27)) * 0x94d049bb133111eb;
	x = x ^ (x >> 31);
#endif
	return x;
}

void ZEND_FASTCALL zend_jit_hot_func(zend_execute_data *execute_data, const zend_op *opline);

typedef struct _zend_jit_op_array_hot_extension {
	zend_func_info func_info;
	int16_t    *counter;
	const void *orig_handlers[1];
} zend_jit_op_array_hot_extension;

#define zend_jit_op_array_hash(op_array) \
	zend_jit_hash((op_array)->opcodes)

extern const zend_op *zend_jit_halt_op;

#ifdef HAVE_GCC_GLOBAL_REGS
# define EXECUTE_DATA_D                       void
# define EXECUTE_DATA_C
# define EXECUTE_DATA_DC
# define EXECUTE_DATA_CC
# define OPLINE_D                             void
# define OPLINE_C
# define OPLINE_DC
# define OPLINE_CC
# define ZEND_OPCODE_HANDLER_RET              void
# define ZEND_OPCODE_HANDLER_ARGS             EXECUTE_DATA_D
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU
# define ZEND_OPCODE_HANDLER_ARGS_DC
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC
# define ZEND_OPCODE_RETURN()                 return
# define ZEND_OPCODE_TAIL_CALL(handler)       do { \
		handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU); \
		return; \
	} while(0)
# define ZEND_OPCODE_TAIL_CALL_EX(handler, arg) do { \
		handler(arg ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC); \
		return; \
	} while(0)
#else
# define EXECUTE_DATA_D                       zend_execute_data* execute_data
# define EXECUTE_DATA_C                       execute_data
# define EXECUTE_DATA_DC                      , EXECUTE_DATA_D
# define EXECUTE_DATA_CC                      , EXECUTE_DATA_C
# define OPLINE_D                             const zend_op* opline
# define OPLINE_C                             opline
# define OPLINE_DC                            , OPLINE_D
# define OPLINE_CC                            , OPLINE_C
# define ZEND_OPCODE_HANDLER_RET              int
# define ZEND_OPCODE_HANDLER_ARGS             EXECUTE_DATA_D
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU    EXECUTE_DATA_C
# define ZEND_OPCODE_HANDLER_ARGS_DC          EXECUTE_DATA_DC
# define ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC EXECUTE_DATA_CC
# define ZEND_OPCODE_RETURN()                 return 0
# define ZEND_OPCODE_TAIL_CALL(handler)       do { \
		return handler(ZEND_OPCODE_HANDLER_ARGS_PASSTHRU); \
	} while(0)
# define ZEND_OPCODE_TAIL_CALL_EX(handler, arg) do { \
		return handler(arg ZEND_OPCODE_HANDLER_ARGS_PASSTHRU_CC); \
	} while(0)
#endif

/* VM handlers */
typedef ZEND_OPCODE_HANDLER_RET (ZEND_FASTCALL *zend_vm_opcode_handler_t)(ZEND_OPCODE_HANDLER_ARGS);

/* VM helpers */
ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_leave_nested_func_helper(uint32_t call_info EXECUTE_DATA_DC);
ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_leave_top_func_helper(uint32_t call_info EXECUTE_DATA_DC);
ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_leave_func_helper(uint32_t call_info EXECUTE_DATA_DC);

ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_profile_helper(ZEND_OPCODE_HANDLER_ARGS);

ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_func_counter_helper(ZEND_OPCODE_HANDLER_ARGS);
ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_loop_counter_helper(ZEND_OPCODE_HANDLER_ARGS);

void ZEND_FASTCALL zend_jit_copy_extra_args_helper(EXECUTE_DATA_D);
zend_bool ZEND_FASTCALL zend_jit_deprecated_helper(OPLINE_D);

int ZEND_FASTCALL zend_jit_get_constant(const zval *key, uint32_t flags);
int ZEND_FASTCALL zend_jit_check_constant(const zval *key);

/* Tracer */
#define zend_jit_opline_hash(opline) \
	zend_jit_hash(opline)

#define ZEND_JIT_TRACE_STOP(_) \
	_(LOOP,              "loop") \
	_(RECURSIVE_CALL,    "recursive call") \
	_(RECURSIVE_RET,     "recursive return") \
	_(RETURN,            "return") \
	_(RETURN_HALT,       "return from interpreter") \
	_(INTERPRETER,       "exit to VM interpreter") \
	_(LINK,              "link to another trace") \
	/* compilation and linking successful */ \
	_(COMPILED,          "compiled") \
	_(ALREADY_DONE,      "already prcessed") \
	/* failures */ \
	_(ERROR,             "error")                          /* not used */ \
	_(NOT_SUPPORTED,     "not supported instructions") \
	_(EXCEPTION,         "exception") \
	_(TOO_LONG,          "trace too long") \
	_(TOO_DEEP,          "trace too deep") \
	_(TOO_DEEP_RET,      "trace too deep return") \
	_(DEEP_RECURSION,    "deep recursion") \
	_(LOOP_UNROLL,       "loop unroll limit reached") \
	_(LOOP_EXIT,         "exit from loop") \
	_(BLACK_LIST,        "trace blacklisted") \
	_(INNER_LOOP,        "inner loop")                     /* trace it */ \
	_(COMPILED_LOOP,     "compiled loop") \
	_(TRAMPOLINE,        "trampoline call") \
	_(BAD_FUNC,          "bad function call") \
	_(HALT,              "exit from interpreter") \
	_(COMPILER_ERROR,    "JIT compilation error") \
	/* no recoverable error (blacklist immediately) */ \
	_(NO_SHM,            "insufficient shared memory") \
	_(TOO_MANY_TRACES,   "too many traces") \
	_(TOO_MANY_CHILDREN, "too many side traces") \
	_(TOO_MANY_EXITS,    "too many side exits") \

#define ZEND_JIT_TRACE_STOP_NAME(name, description) \
	ZEND_JIT_TRACE_STOP_ ## name,

typedef enum _zend_jit_trace_stop {
	ZEND_JIT_TRACE_STOP(ZEND_JIT_TRACE_STOP_NAME)
} zend_jit_trace_stop;

#define ZEND_JIT_TRACE_STOP_OK(ret) \
	(ret < ZEND_JIT_TRACE_STOP_COMPILED)

#define ZEND_JIT_TRACE_STOP_DONE(ret) \
	(ret < ZEND_JIT_TRACE_STOP_ERROR)

#define ZEND_JIT_TRACE_STOP_REPEAT(ret) \
	(ret == ZEND_JIT_TRACE_STOP_INNER_LOOP)

#define ZEND_JIT_TRACE_STOP_MAY_RECOVER(ret) \
	(ret <= ZEND_JIT_TRACE_STOP_COMPILER_ERROR)

#define ZEND_JIT_TRACE_START_MASK      0xf

#define ZEND_JIT_TRACE_START_LOOP   (1<<0)
#define ZEND_JIT_TRACE_START_ENTER  (1<<1)
#define ZEND_JIT_TRACE_START_RETURN (1<<2)
#define ZEND_JIT_TRACE_START_SIDE   (1<<3) /* used for side traces */

#define ZEND_JIT_TRACE_JITED        (1<<4)
#define ZEND_JIT_TRACE_BLACKLISTED  (1<<5)
#define ZEND_JIT_TRACE_UNSUPPORTED  (1<<6)

#define ZEND_JIT_TRACE_SUPPORTED    0

#define ZEND_JIT_EXIT_JITED         (1<<0)
#define ZEND_JIT_EXIT_BLACKLISTED   (1<<1)
#define ZEND_JIT_EXIT_TO_VM         (1<<2) /* exit to VM without attempt to create a side trace */
#define ZEND_JIT_EXIT_RESTORE_CALL  (1<<3) /* deoptimizer should restore EX(call) chain */
#define ZEND_JIT_EXIT_POLYMORPHISM  (1<<4) /* exit because of polymorphic call */
#define ZEND_JIT_EXIT_FREE_OP1      (1<<5)
#define ZEND_JIT_EXIT_FREE_OP2      (1<<6)
#define ZEND_JIT_EXIT_PACKED_GUARD  (1<<7)
#define ZEND_JIT_EXIT_DYNAMIC_CALL  (1<<8) /* exit because of polymorphic INTI_DYNAMIC_CALL call */

typedef union _zend_op_trace_info {
	zend_op dummy; /* the size of this structure must be the same as zend_op */
	struct {
		const void *orig_handler;
		const void *call_handler;
		int16_t    *counter;
		uint8_t     trace_flags;
	};
} zend_op_trace_info;

typedef struct _zend_jit_op_array_trace_extension {
	zend_func_info func_info;
	const zend_op_array *op_array;
	size_t offset; /* offset from "zend_op" to corresponding "op_info" */
	zend_op_trace_info trace_info[1];
} zend_jit_op_array_trace_extension;

#define ZEND_OP_TRACE_INFO(opline, offset) \
	((zend_op_trace_info*)(((char*)opline) + offset))

/* Recorder */
typedef enum _zend_jit_trace_op {
	ZEND_JIT_TRACE_VM,
	ZEND_JIT_TRACE_OP1_TYPE,
	ZEND_JIT_TRACE_OP2_TYPE,
	ZEND_JIT_TRACE_INIT_CALL,
	ZEND_JIT_TRACE_DO_ICALL,
	ZEND_JIT_TRACE_ENTER,
	ZEND_JIT_TRACE_BACK,
	ZEND_JIT_TRACE_END,
	ZEND_JIT_TRACE_START,
} zend_jit_trace_op;

#define IS_UNKNOWN 255 /* may be used for zend_jit_trace_rec.op?_type */
#define IS_TRACE_PACKED    (1<<4)
#define IS_TRACE_REFERENCE (1<<5)
#define IS_TRACE_INDIRECT  (1<<6)

#define ZEND_JIT_TRACE_FAKE_INIT_CALL    0x00000100
#define ZEND_JIT_TRACE_RETRUN_VALUE_USED 0x00000100

#define ZEND_JIT_TRACE_MAX_SSA_VAR       0x7ffffe
#define ZEND_JIT_TRACE_SSA_VAR_SHIFT     9

#define ZEND_JIT_TRACE_FAKE_LEVEL_MASK   0xffff0000
#define ZEND_JIT_TRACE_FAKE_LEVEL_SHIFT  16

#define ZEND_JIT_TRACE_FAKE_LEVEL(info) \
	(((info) & ZEND_JIT_TRACE_FAKE_LEVEL_MASK) >> ZEND_JIT_TRACE_FAKE_LEVEL_SHIFT)

#define ZEND_JIT_TRACE_FAKE_INFO(level) \
	(((level) << ZEND_JIT_TRACE_FAKE_LEVEL_SHIFT) | ZEND_JIT_TRACE_FAKE_INIT_CALL)

#define ZEND_JIT_TRACE_SET_FIRST_SSA_VAR(_info, var) do { \
		_info |= (var << ZEND_JIT_TRACE_SSA_VAR_SHIFT); \
	} while (0)
#define ZEND_JIT_TRACE_GET_FIRST_SSA_VAR(_info) \
	(_info >> ZEND_JIT_TRACE_SSA_VAR_SHIFT)

struct _zend_jit_trace_rec {
	union {
		struct { ZEND_ENDIAN_LOHI(
			uint8_t   op,    /* zend_jit_trace_op */
			union {
				struct {
					uint8_t op1_type;/* recorded zval op1_type for ZEND_JIT_TRACE_VM */
					uint8_t op2_type;/* recorded zval op2_type for ZEND_JIT_TRACE_VM */
					uint8_t op3_type;/* recorded zval for op_data.op1_type for ZEND_JIT_TRACE_VM */
				};
				struct {
					uint8_t  start;  /* ZEND_JIT_TRACE_START_MASK for ZEND_JIT_TRACE_START/END */
					uint8_t  stop;   /* zend_jit_trace_stop for ZEND_JIT_TRACE_START/END */
					uint8_t  level;  /* recursive return level for ZEND_JIT_TRACE_START */
				};
			})
		};
		uint32_t last;
		uint32_t info; /* "first_ssa_var" for ZEND_JIT_TRACE_ENTER and ZEND_JIT_TRACE_BACK,
		                * "return_value_used" for ZEND_JIT_TRACE_ENTER,
		                * "fake" for ZEND_JIT_TRACE_INIT_CALL */
	};
	union {
		const void             *ptr;
		const zend_function    *func;
		const zend_op_array    *op_array;
		const zend_op          *opline;
		const zend_class_entry *ce;
	};
};

#define ZEND_JIT_TRACE_START_REC_SIZE 2

typedef struct _zend_jit_trace_exit_info {
	const zend_op       *opline;     /* opline where VM should continue execution */
	const zend_op_array *op_array;
	uint32_t             flags;      /* set of ZEND_JIT_EXIT_... */
	uint32_t             stack_size;
	uint32_t             stack_offset;
} zend_jit_trace_exit_info;

typedef union _zend_jit_trace_stack {
	int32_t      ssa_var;
	uint32_t     info;
	struct {
		uint8_t  type;
		int8_t   reg;
		uint16_t flags;
	};
} zend_jit_trace_stack;

#define STACK_VAR(_stack, _slot) \
	(_stack)[_slot].ssa_var
#define STACK_INFO(_stack, _slot) \
	(_stack)[_slot].info
#define STACK_TYPE(_stack, _slot) \
	(_stack)[_slot].type
#define STACK_REG(_stack, _slot) \
	(_stack)[_slot].reg
#define SET_STACK_VAR(_stack, _slot, _ssa_var) do { \
		(_stack)[_slot].ssa_var = _ssa_var; \
	} while (0)
#define SET_STACK_INFO(_stack, _slot, _info) do { \
		(_stack)[_slot].info = _info; \
	} while (0)
#define SET_STACK_TYPE(_stack, _slot, _type) do { \
		(_stack)[_slot].type = _type; \
		(_stack)[_slot].reg = ZREG_NONE; \
		(_stack)[_slot].flags = 0; \
	} while (0)
#define SET_STACK_REG(_stack, _slot, _reg) do { \
		(_stack)[_slot].reg = _reg; \
	} while (0)

/* trace info flags */
#define ZEND_JIT_TRACE_CHECK_INTERRUPT (1<<0)
#define ZEND_JIT_TRACE_LOOP            (1<<1)
#define ZEND_JIT_TRACE_USES_INITIAL_IP (1<<2)

typedef struct _zend_jit_trace_info {
	uint32_t                  id;            /* trace id */
	uint32_t                  root;          /* root trace id or self id for root traces */
	uint32_t                  parent;        /* parent trace id or 0 for root traces */
	uint32_t                  link;          /* link trace id or self id for loop) */
	uint32_t                  exit_count;    /* number of side exits */
	uint32_t                  child_count;   /* number of side traces for root traces */
	uint32_t                  code_size;     /* size of native code */
	uint32_t                  exit_counters; /* offset in exit counters array */
	uint32_t                  stack_map_size;
	uint32_t                  flags;         /* See ZEND_JIT_TRACE_... defines above */
	uint32_t                  polymorphism;  /* Counter of polymorphic calls */
	uint32_t                  jmp_table_size;/* number of jmp_table slots */
	const zend_op            *opline;        /* first opline */
	const void               *code_start;    /* address of native code */
	zend_jit_trace_exit_info *exit_info;     /* info about side exits */
	zend_jit_trace_stack     *stack_map;
	//uint32_t    loop_offset;
} zend_jit_trace_info;

struct _zend_jit_trace_stack_frame {
	zend_jit_trace_stack_frame *call;
	zend_jit_trace_stack_frame *prev;
	const zend_function        *func;
	const zend_op              *call_opline;
	uint32_t                    call_level;
	uint32_t                    _info;
	zend_jit_trace_stack        stack[1];
};

#define TRACE_FRAME_SHIFT_NUM_ARGS            16
#define TRACE_FRAME_MAX_NUM_ARGS              32767

#define TRACE_FRAME_MASK_NUM_ARGS             0xffff0000
#define TRACE_FRAME_MASK_NESTED               0x00000001
#define TRACE_FRAME_MASK_LAST_SEND_BY_REF     0x00000002
#define TRACE_FRAME_MASK_LAST_SEND_BY_VAL     0x00000004
#define TRACE_FRAME_MASK_RETURN_VALUE_USED    0x00000008
#define TRACE_FRAME_MASK_RETURN_VALUE_UNUSED  0x00000010
#define TRACE_FRAME_MASK_THIS_CHECKED         0x00000020
#define TRACE_FRAME_MASK_UNKNOWN_RETURN       0x00000040
#define TRACE_FRAME_MASK_NO_NEED_RELEASE_THIS 0x00000080


#define TRACE_FRAME_INIT(frame, _func, _flags, num_args) do { \
		zend_jit_trace_stack_frame *_frame = (frame); \
		_frame->call = NULL; \
		_frame->prev = NULL; \
		_frame->func = (const zend_function*)_func; \
		_frame->call_opline = NULL; \
		_frame->call_level = 0; \
		_frame->_info = (((uint32_t)(num_args)) << TRACE_FRAME_SHIFT_NUM_ARGS) & TRACE_FRAME_MASK_NUM_ARGS; \
		_frame->_info |= _flags; \
	} while (0)

#define TRACE_FRAME_RETURN_SSA_VAR(frame) \
	((int)(frame)->_info)
#define TRACE_FRAME_NUM_ARGS(frame) \
	((int)((frame)->_info) >> TRACE_FRAME_SHIFT_NUM_ARGS)
#define TRACE_FRAME_IS_NESTED(frame) \
	((frame)->_info & TRACE_FRAME_MASK_NESTED)
#define TRACE_FRAME_IS_LAST_SEND_BY_REF(frame) \
	((frame)->_info & TRACE_FRAME_MASK_LAST_SEND_BY_REF)
#define TRACE_FRAME_IS_LAST_SEND_BY_VAL(frame) \
	((frame)->_info & TRACE_FRAME_MASK_LAST_SEND_BY_VAL)
#define TRACE_FRAME_IS_RETURN_VALUE_USED(frame) \
	((frame)->_info & TRACE_FRAME_MASK_RETURN_VALUE_USED)
#define TRACE_FRAME_IS_RETURN_VALUE_UNUSED(frame) \
	((frame)->_info & TRACE_FRAME_MASK_RETURN_VALUE_UNUSED)
#define TRACE_FRAME_IS_THIS_CHECKED(frame) \
	((frame)->_info & TRACE_FRAME_MASK_THIS_CHECKED)
#define TRACE_FRAME_IS_UNKNOWN_RETURN(frame) \
	((frame)->_info & TRACE_FRAME_MASK_UNKNOWN_RETURN)
#define TRACE_FRAME_NO_NEED_REKEASE_THIS(frame) \
	((frame)->_info & TRACE_FRAME_MASK_NO_NEED_RELEASE_THIS)

#define TRACE_FRAME_SET_RETURN_SSA_VAR(frame, var) do { \
		(frame)->_info = var; \
	} while (0)
#define TRACE_FRAME_SET_LAST_SEND_BY_REF(frame) do { \
		(frame)->_info |= TRACE_FRAME_MASK_LAST_SEND_BY_REF; \
		(frame)->_info &= ~TRACE_FRAME_MASK_LAST_SEND_BY_VAL; \
	} while (0)
#define TRACE_FRAME_SET_LAST_SEND_BY_VAL(frame) do { \
		(frame)->_info |= TRACE_FRAME_MASK_LAST_SEND_BY_VAL; \
		(frame)->_info &= ~TRACE_FRAME_MASK_LAST_SEND_BY_REF; \
	} while (0)
#define TRACE_FRAME_SET_RETURN_VALUE_USED(frame) do { \
		(frame)->_info |= TRACE_FRAME_MASK_RETURN_VALUE_USED; \
		(frame)->_info &= ~TRACE_FRAME_MASK_RETURN_VALUE_UNUSED; \
	} while (0)
#define TRACE_FRAME_SET_RETURN_VALUE_UNUSED(frame) do { \
		(frame)->_info |= TRACE_FRAME_MASK_RETURN_VALUE_UNUSED; \
		(frame)->_info &= ~TRACE_FRAME_MASK_RETURN_VALUE_USED; \
	} while (0)
#define TRACE_FRAME_SET_THIS_CHECKED(frame) do { \
		(frame)->_info |= TRACE_FRAME_MASK_THIS_CHECKED; \
	} while (0)
#define TRACE_FRAME_SET_NO_NEED_RELEASE_THIS(frame) do { \
		(frame)->_info |= TRACE_FRAME_MASK_NO_NEED_RELEASE_THIS; \
	} while (0)

ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_func_trace_helper(ZEND_OPCODE_HANDLER_ARGS);
ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_ret_trace_helper(ZEND_OPCODE_HANDLER_ARGS);
ZEND_OPCODE_HANDLER_RET ZEND_FASTCALL zend_jit_loop_trace_helper(ZEND_OPCODE_HANDLER_ARGS);

int ZEND_FASTCALL zend_jit_trace_hot_root(zend_execute_data *execute_data, const zend_op *opline);
int ZEND_FASTCALL zend_jit_trace_exit(uint32_t exit_num, zend_jit_registers_buf *regs);
zend_jit_trace_stop ZEND_FASTCALL zend_jit_trace_execute(zend_execute_data *execute_data, const zend_op *opline, zend_jit_trace_rec *trace_buffer, uint8_t start, uint32_t is_megamorphc);

static zend_always_inline const zend_op* zend_jit_trace_get_exit_opline(zend_jit_trace_rec *trace, const zend_op *opline, zend_bool *exit_if_true)
{
	if (trace->op == ZEND_JIT_TRACE_VM || trace->op == ZEND_JIT_TRACE_END) {
		if (trace->opline == opline + 1) {
			/* not taken branch */
			*exit_if_true = opline->opcode == ZEND_JMPNZ;
			return OP_JMP_ADDR(opline, opline->op2);
		} else if (trace->opline == OP_JMP_ADDR(opline, opline->op2)) {
			/* taken branch */
			*exit_if_true = opline->opcode == ZEND_JMPZ;
			return opline + 1;
		} else {
			ZEND_UNREACHABLE();
		}
	} else  {
		ZEND_UNREACHABLE();
	}
	*exit_if_true = 0;
	return NULL;
}

static zend_always_inline zend_bool zend_jit_may_be_polymorphic_call(const zend_op *opline)
{
	if (opline->opcode == ZEND_INIT_FCALL
	 || opline->opcode == ZEND_INIT_FCALL_BY_NAME
	 || opline->opcode == ZEND_INIT_NS_FCALL_BY_NAME) {
		return 0;
	} else if (opline->opcode == ZEND_INIT_METHOD_CALL
     || opline->opcode == ZEND_INIT_DYNAMIC_CALL) {
		return 1;
	} else if (opline->opcode == ZEND_INIT_STATIC_METHOD_CALL) {
		return (opline->op1_type != IS_CONST || opline->op2_type != IS_CONST);
	} else if (opline->opcode == ZEND_INIT_USER_CALL) {
		return (opline->op2_type != IS_CONST);
	} else if (opline->opcode == ZEND_NEW) {
		return (opline->op1_type != IS_CONST);
	} else {
		ZEND_UNREACHABLE();
		return 0;
	}
}

#endif /* ZEND_JIT_INTERNAL_H */
