#ifndef ISO_TYPES_H
#define ISO_TYPES_H

#define MAX_NOF_CIG         1
#define MAX_CIS_PER_CIG     2
#define MAX_NOF_CIS         (MAX_CIS_PER_CIG * MAX_NOF_CIG)

//This is a premitive to keep memory order by compiler.
#define ALIGNED


typedef struct AclObj_T {
    uint16_t handle;
    /* one ACL (for central device) can have multiple 
     * CISes with same or different peripherals.
     */
    LINK_T *cisObjQ;
} AclObj_T;

// For unframed, from information bytes would be placed by Isoal in HCI buffer, above HCI payload, to minimize copy
// For framed, Isoal shall use a separate temporary buffer to serment and add Isoal header.
typedef struct UnframedPDU_T {
    uint8_t     header; //ll, sn, nesn, ..
    uint8_t     length;
    uint8_t     pdu[MAX_PDU - 2];
} UnframedPDU_T;

typedef struct FramedPDU_T {
    uint16_t    header;
    uint8_t     pdu[MAX_PDU - 2];
} FramedPDU_T;

typedef struct cisPdu_T {
    uint64_t    cisPayloadCounter;
    void        *ptHciBuff;
    uint16_t    remainingBytes;
    union {
        UnframedPDU_T unframed;
        framedPDU_T   framed;
    }
} cisPdu_T;

typedef struct CisEvent_T{
    uint64_t    cisEventCount;
    uint32_t    anchor;
    uint8_t     sn:1;       //depends on ll_scheduler
    uint8_t     nesn:1;
    LINK_T      *txPduQ; //# of tx pdu shall be NB
    LINK_T      *rxPduQ; //# of rx pdu buffer shall be NB
} CisEvent_T;

typedef struct CisObj_T {
    //by host
    uint8_t     cigId;
    uint8_t     cisId;
    uint8_t     maxSdu[2];
    uint8_t     maxPdu[2];
    uint8_t     phy[2];
    uint8_t     rtn[2];
    //by controller
    AclObj_T    *parent;
    uint16_t    cisHandle;
    uint8_t     bn[2];
    uint8_t     nse;
    uint32_t    sub_interval;
    uint8_t     state; //STBY -> ALLOCED -> CONNECTED
    uint32_t    cisOffsetMin;
    uint32_t    cisOffsetMax;
    uint16_t    connEventCount;
    CisEvent_T  cisEvent;
} CisObj_T;

typedef struct CigEvent_T {
    uint8_t     cisCount;
    uint8_t     packing;
    uint32_t    isoInterval;
    CisObj_T    *cisObj;
} CigEvent_T;

typedef struct CigObj_T {
    //by host
    uint8_t     cigId;
    uint32_t    sduInterval[2];
    uint8_t     worstSca;
    uint8_t     framing;
    uint8_t     packing;
    uint32_t    maxTransportLatency[2];
    uint8_t     cisCount;
    //by controller
    //Make an array to get fixed order because the order of 
    //CIS is fixed when it is created. If any of one is disconnected
    //reconnected shall be assigned into the disconnected cis
    uint8_t     state: // STBY -> ACTIVE
    uint32_t    isoInterval;
    uint8_t     ft[2];
    CigEvent_T  cigEvent;
} CigObj_T;

typedef struct CIGMGM_T {
    uint8_t     noCigs;
    uint8_t     noCises;
    CigObj_T    IsoCig[MAX_NOF_CIG];
} CIGMGM_T;


typedef struct LeCommand_T {
    uint16_t opcode;
    uint8_t  params[256];
} LeCommand_T;

typedef struct VOS_TMessage_T {
    uint32_t tId;
} VOS_TMessage_T;

#endif
