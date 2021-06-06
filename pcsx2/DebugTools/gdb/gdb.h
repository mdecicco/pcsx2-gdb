#pragma once
#include "libgdbstub.h"

/** Character indicating the start of a packet. */
#define GDBSTUB_PKT_START '$'
/** Character indicating the end of a packet (excluding the checksum). */
#define GDBSTUB_PKT_END '#'
/** The escape character. */
#define GDBSTUB_PKT_ESCAPE '{'
/** The out-of-band interrupt character. */
#define GDBSTUB_OOB_INTERRUPT 0x03

/** Returns the number of elements from a static array. */
#define ELEMENTS(a_Array) (sizeof(a_Array) / sizeof(a_Array[0]))
/** Returns the minimum of two given values. */
#define MIN(a_Val1, a_Val2) ((a_Val1) < (a_Val2) ? (a_Val1) : (a_Val2))
/** Sets the specified bit. */
#define BIT(a_Bit) (1 << (a_Bit))
/** Return the absolute value of a given number .*/
#define ABS(a) ((a) < 0 ? -(a) : (a))

/** Our own bool type. */
typedef uint8_t BOOLEAN;
/** true value. */
#define TRUE 1
/** false value. */
#define FALSE 0


/** Pointer to an internal PSP proxy context. */
typedef struct GDBSTUBCTXINT* PGDBSTUBCTXINT;

/**
 * GDB stub receive state.
 */
typedef enum GDBSTUBRECVSTATE
{
	/** Invalid state. */
	GDBSTUBRECVSTATE_INVALID = 0,
	/** Waiting for the start character. */
	GDBSTUBRECVSTATE_PACKET_WAIT_FOR_START,
	/** Reiceiving the packet body up until the END character. */
	GDBSTUBRECVSTATE_PACKET_RECEIVE_BODY,
	/** Receiving the checksum. */
	GDBSTUBRECVSTATE_PACKET_RECEIVE_CHECKSUM,
	/** Blow up the enum to 32bits for easier alignment of members in structs. */
	GDBSTUBRECVSTATE_32BIT_HACK = 0x7fffffff
} GDBSTUBRECVSTATE;


/**
 * Command output context.
 */
typedef struct GDBSTUBOUTCTX
{
	/** The helper structure, MUST come first!. */
	GDBSTUBCMDOUTHLP Hlp;
	/** Pointer to the owning GDB stub context. */
	PGDBSTUBCTXINT pGdbStubCtx;
	/** Current offset into the scratch buffer. */
	uint32_t offScratch;
	/** Scratch buffer. */
	uint8_t abScratch[512];
} GDBSTUBOUTCTX;
/** Pointer to a command output context. */
typedef GDBSTUBOUTCTX* PGDBSTUBOUTCTX;
/** Pointer to a const command output context. */
typedef const GDBSTUBOUTCTX* PCGDBSTUBOUTCTX;


/**
 * Internal PSP proxy context.
 */
typedef struct GDBSTUBCTXINT
{
	/** The I/O interface callback table. */
	PCGDBSTUBIOIF pIoIf;
	/** The interface callback table. */
	PCGDBSTUBIF pIf;
	/** Opaque user data passed in the callbacks. */
	void* pvUser;
	/** The current state when receiving a new packet. */
	GDBSTUBRECVSTATE enmState;
	/** Maximum number of bytes the packet buffer can hold. */
	size_t cbPktBufMax;
	/** Current offset into the packet buffer. */
	uint32_t offPktBuf;
	/** The size of the packet (minus the start, end characters and the checksum). */
	uint32_t cbPkt;
	/** Pointer to the packet buffer data. */
	uint8_t* pbPktBuf;
	/** Number of bytes left for the checksum. */
	size_t cbChksumRecvLeft;
	/** Last target state seen. */
	GDBSTUBTGTSTATE enmTgtStateLast;
	/** Number of registers this architecture has. */
	uint32_t cRegs;
	/** Overall size to return all registers. */
	size_t cbRegs;
	/** Register scratch space (for reading writing registers). */
	void* pvRegsScratch;
	/** Register index array for querying setting. */
	uint32_t* paidxRegs;
	/** Send packet checksum. */
	uint8_t uChkSumSend;
	/** Feature flags supported we negotiated with the remote end. */
	uint32_t fFeatures;
	/** Pointer to the XML target description. */
	uint8_t* pbTgtXmlDesc;
	/** Size of the XML target description. */
	size_t cbTgtXmlDesc;
	/** Flag whether the stub is in extended mode. */
	BOOLEAN fExtendedMode;
	/** Output context. */
	GDBSTUBOUTCTX OutCtx;
	/** Whether or not to stop the main loop */
	BOOLEAN doShutdown;
	/** Whether or not the main loop shutdown */
	BOOLEAN didShutdown;
} GDBSTUBCTXINT;


/** Indicate support for the 'qXfer:features:read' packet to support the target description. */
#define GDBSTUBCTX_FEATURES_F_TGT_DESC BIT(0)

/**
 * Specific query packet processor callback.
 *
 * @returns Status code.
 * @param   pThis               The GDB stub context.
 * @param   pbArgs              Pointer to the arguments.
 * @param   cbArgs              Size of the arguments in bytes.
 */
typedef int(FNGDBSTUBQPKTPROC)(PGDBSTUBCTXINT pThis, const uint8_t* pbArgs, size_t cbArgs);
typedef FNGDBSTUBQPKTPROC* PFNGDBSTUBQPKTPROC;


/**
 * 'q' packet processor.
 */
typedef struct GDBSTUBQPKTPROC
{
	/** Name */
	const char* pszName;
	/** Length of name in characters (without \0 terminator). */
	uint32_t cchName;
	/** The callback to call for processing the particular query. */
	PFNGDBSTUBQPKTPROC pfnProc;
} GDBSTUBQPKTPROC;
/** Pointer to a 'q' packet processor entry. */
typedef GDBSTUBQPKTPROC* PGDBSTUBQPKTPROC;
/** Pointer to a const 'q' packet processor entry. */
typedef const GDBSTUBQPKTPROC* PCGDBSTUBQPKTPROC;


/**
 * 'v' packet processor.
 */
typedef struct GDBSTUBVPKTPROC
{
	/** Name */
	const char* pszName;
	/** Length of name in characters (without \0 terminator). */
	uint32_t cchName;
	/** Replay to a query packet (ends with ?). */
	const char* pszReplyQ;
	/** Length of the query reply (without \0 terminator). */
	uint32_t cchReplyQ;
	/** The callback to call for processing the particular query. */
	PFNGDBSTUBQPKTPROC pfnProc;
} GDBSTUBVPKTPROC;
/** Pointer to a 'q' packet processor entry. */
typedef GDBSTUBVPKTPROC* PGDBSTUBVPKTPROC;
/** Pointer to a const 'q' packet processor entry. */
typedef const GDBSTUBVPKTPROC* PCGDBSTUBVPKTPROC;


/**
 * Feature callback.
 *
 * @returns Status code.
 * @param   pThis               The GDB stub context.
 * @param   pbVal               Pointer to the value.
 * @param   cbVal               Size of the value in bytes.  
 */
typedef int(FNGDBSTUBFEATHND)(PGDBSTUBCTXINT pThis, const uint8_t* pbVal, size_t cbVal);
typedef FNGDBSTUBFEATHND* PFNGDBSTUBFEATHND;


/**
 * GDB feature descriptor.
 */
typedef struct GDBSTUBFEATDESC
{
	/** Feature name */
	const char* pszName;
	/** Length of the feature name in characters (without \0 terminator). */
	uint32_t cchName;
	/** The callback to call for processing the particular feature. */
	PFNGDBSTUBFEATHND pfnHandler;
	/** Flag whether the feature requires a value. */
	BOOLEAN fVal;
} GDBSTUBFEATDESC;
/** Pointer to a GDB feature descriptor. */
typedef GDBSTUBFEATDESC* PGDBSTUBFEATDESC;
/** Pointer to a const GDB feature descriptor. */
typedef const GDBSTUBFEATDESC* PCGDBSTUBFEATDESC;
