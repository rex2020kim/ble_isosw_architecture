#include <iostream>
#include "iso_types.h"

using namespace std;

/* Pre-questions
 * 1. If an earlier packet has an earlier time stamp value?
 * 2. If 1 is not guarantted, the packet sequence number represents the order of PDU?
 */

/* 1. LE Iso modules
 * 1) LL_Iso_Admin:
 *  - Receive LE Iso HCIs
 *  - Retrive corresponding ACL & check resource
 *  - Calculate bandwidth and determine CIG & CIS parameters
 *  - Request Create/Terminate ISO to LL_ISO_Control
 *
 * 2) LL_Iso_Control:
 *  - Receive Message from LL_Iso_Admin
 *   . LL_ISO_CREATE_CIS
 *   . LL_ISO_REMOVE_CIS
 *  - Manage LL Iso Connection Disconnection procedure
 *
 * 3) LL_Iso_Isoal:
 *  - Fragmention/recombination or Segmentation/reassembly
 *  - Interface between HCI
 *
 * 4) ll_iso_scheduler:
 *  - It depends existing ll_scheduler and link layer firmware architecture.
 *  - Option 1: ll_scheduler task register a CIG event and keep it until the CIG is removed.
 *   . Send LL_CREATE_CIG_EVENT to ll_scheduler
 *   . ll_scheduler keep scheduling until the CIG is removed by REMOVE_CIG_CMD
 *   . ll_scheduler schedule baseband packets based on provided CIG info.
 *   . ll_cig_scheduling function can be registered by ll_iso_scheduler
 *   . ll_scheduler use callback to update next anchor point.
 *   . Static memory or HW handling buffer pointer will be used to share PDU buffer
 *  - Option 2: Use ll_iso_scheduler and register ll_scheduling instance with a CIG event object
 *    . Use a function to register the iso link to ll_scheduler
 *    . ll_scheduler calls a callback when it completes a CIG event and pass TX/Rx done status
 *    . A CIG event is an instance being registered by ll_iso_scheduler for each CIG event and ll_scheduler
 *      only consume one CIG event and removed from ll_scheduler pending schedule.
 *    . Registering the next CIG event is reposible on ll_iso_scheduler
 *    . Re-registering a next CIG event may be done in the callback function provided by ll_iso_scheduler or
 *      in ll_iso_scheduler task when it receives TXACK and RX DONE message from ll_scheduler.
 *    . ll_iso_scheduler provide the CIG or BIG scheduler function to the ll_scheduler for scheduling a CIG event
 *
 */

/* HCI */
/*
VOS_PROCESS(hci_receiver) {
    VOS_TMessage_T *ptMsg = vos_receive_message();

    switch (ptMsg->tId)
    {
        case HCI_CMD:
            if (ptMsg->opCode ==
                    HCI_LE_READ_ISO_TX_SYNC ||
                    HCI_LE_SET_CIG_PARAMETER ||
                    HCI_LE_CREATE_CIS ||
                    HCI_LE_REMOVE_CIS ||
                    HCI_LE_ACCEPT_CIS_REQ)
            {
                //1. check parameters
                //2. Send command to LL_Admision
            }
        break;

        case HCI_ACL:
        break;

        case HCI_SCO:
        break;

        case HCI_EVT:
        break;

        case HCI_ISO:
        {
            *** important function: Handle Time_Stamp
            - There could be two option of time stamp handling method.
            1. Host uses an original time stamp from the media decorder.
             - controller calculate average time of offset between controller
             time stamp and a time stamp in HCI.
             (Ex, calculate an average of accumulated 12 SDUs after discarding
                 fastest and slowest one)
             - convert host time stamp to controller time stamp && calculate
             time offset for a framed PDU.
            2. Host uses controller time stamp by synchronizing host's to controller's
             (Recommended)
             - Controller calcuate initial time offset by capturing controller time stamp
             when it receives HCI iso data + HCI delay + processing delay.
             - After host reads synchronization information, controller can use the time
             stamp in HCI directly. 
             - Controller may need to detect if host uses host's time stamp or controller's
            *******************

            HCI_LE_ISO_DATA_T *ptIsoData = (HCI_LE_ISO_DATA_T *)ptMsg;
            LE_ISOTX_DATA_T *ptSig = VOS_Alloc(sizeof(LE_ISOTX_DATA_T));
            ptSig->tId = LE_ISOTX_DATA;
            ptSig->connHandle <- copy connHandle from HCI buffer;
            ptSig->pbFlag, tsFlag <- copy from HCI buffer;
            ptSig->length <- copy from HCI buffer;
            ptSig->pData <- copy start address of ISO_Data_Load;

            VOS_Send(ptSig, TO(VOS_PROCESS_ID(ll_Isoal)));
        }
        break;
    }
}

VOS_PROCESS(ll_admision) {
    VOS_TMessage_T *ptMsg = vos_receive_message();

    switch (ptMsg->tId)
    {
        case HCI_DISCONNECT:
        {
            if (connHandle > NOFACLHANDLE) {
                VOS_Send(ptMsg, TO(VOS_PROCES(ll_iso_admin)))
            }
        }
        break;

        case LL_SET_CREATE_CIS:
        {
            1. check if all CIS params:
                - duplication check: use a kind of hash map(int visited[handle - MIN_CIS_HANDLE])
            2. try creating each CIS:
             - There are two options to create multiple CISes:
             1) pass create cis command to ll_iso_control and ll_iso_control create each CIS sequentially.
             2) create each cis at ll_admision and pass create cis message to ll_iso_control for only one CIS.
              . pros:
               - ll_admision can serialize each CIS connection request (depends on existing architecture)
                 -> if ll_admision uses a serializer queue, it can process a prioritized message earlier than next CIS creation
               - when ACL link is disconnected, may easy to handle stopping ongoing CIS connection or aborting remaining CIS here
                 -> There is no CIS connection cancel command
            if (role != central) {
               VOS_Send(LL_SET_CREATE_CIS_NEG, ERROR_CODE, TO(hci_receiver));
               break;
            }

            for (int i=0; i<ptCmd->cisCount; i++)
            {
                AclObj_T *ptAcl = getLeAclObject(ptCmd->connHandle[i]);
                if (ptAcl != NULL)
                {
                    CisObj_T *ptCis = getCisInAcl(ptCmd->cisHandle[i]);
                    if (ptCis != NULL) {
                        pushLLSerializer(CREATE_CIS_MSG);
                    }
                    else {
                        VOS_Send(LL_SET_CREATE_CIS_NEG, ERROR_CODE, TO(hci_receiver));
                    }
                    revokeNextSerializer();
                }
                else {
                  VOS_Send(LL_SET_CREATE_CIS_NEG, ERROR_CODE, TO(hci_receiver));
                }
            }
        }
        break;

        case LL_SET_CIG_PARAMETERS:
        case LL_READ_ISO_TX_SYNC:
        case LL_REMOVE_CIG:
        case LL_ACEPT_CIS_REQ:
        {
            VOS_Send(ptMsg, TO(VOS_PROCES(ll_iso_admin)))
        }
        break;
    }
}

- l_iso_admin :
 1. manage CIG/BIG control data
 2. check bandwidth & resource
 3. determine CIS parameters
 4. request CIS create/remove to ll_iso_control

VOS_PROCESS(ll_iso_admin) {
    VOS_TMessage_T *ptMsg = vos_receive_message();

    switch (ptMsg->tId)
    {
        case LL_SET_CIG_PARAMETERS:
            LL_CMD_SET_CIG_PARAM_T *ptCmd = (LL_CMD_SET_CIG_PARAM_T *)ptMsg;
            CigObj_T *ptCig = getIsoCigObject(ptCmd->cigId);
            if (ptCig) {
                if (!hasActiveCis(ptCig)) {
                    LL_Iso_UpdateCig(ptCig);
                }
                else {
                  VOS_Send(LL_SET_CREATE_CIG_NEG, ERROR_CODE, TO(hci_receiver));
                }
            }
            else {
                if((Error = LL_Iso_CreateCig(ptCig)) == SUCCESS) {
                    VOS_Send(LL_ISO_CREATE_CIG_POS, TO(hci_receiver);
                }
                else {
                  VOS_Send(LL_SET_CREATE_CIG_NEG, Error, TO(hci_receiver));
                }
            }
            break;

        case LL_CREATE_CIS:
            LL_CMD_CREATE_CIS_T *ptCmd = (LL_CMD_CREATE_CIS_T *)ptMsg;
            LL_Iso_CreateCis_checkParams(ptCmd); //check if all CIS and ACL handles are valid or not already connected.

            CisObj_T *ptCis = getIsoCisObject(ptCis->cisHandle);
            if (ptCis) {
                CigObj_T *ptCig = getIsoCigObject(ptCis->cigId);
                if (ptCig) {
                    if (ptCig->state == STBY) {
                        IsoCigCalculateCisParams(ptCig, ptCis);
                        VOS_Send(LL_CREATE_CIS, TO(ll_iso_control));
                    }
                }
                else {
                    VOS_Send(LL_CREATE_CIS_NEG, Error, TO(ll_admision));
                }
            }
            else {
              VOS_Send(LL_CREATE_CIS_NEG, Error, TO(ll_admision));
            }
            break;

        case LL_READ_ISO_TX_SYNC:
            break;

        case LL_REMOVE_CIG:
            break;

        case LL_ACCEPT_CIS_REQ:
            break;
    }
}

VOS_PROCES(ll_iso_control)
{
    VOS_TMessage_T *ptMsg = vos_receive_message();

    switch(ptMsg->tId)
    {
        case LL_CREATE_CIS:
            sendLLControlPDu(LL_CREATE_CIS);
        break;

        case LL_RX_CREATE_CIS_RSP:
            sendLLControlPdu(LL_CREATE_CIS_IND);
        break;

        case TXACK_LL_CREATE_CIS_IND:
           sendLLSchedulerCommand(LL_CREATE_CIS_SCHED, ptCig, ptCis);
           VOS_Send(LL_CIS_ESTABLISHED_EVENT, SUCCESS, TO(hci_receiver));
        break;
    }
}

VOS_PROCESS(ll_iso_scheduler)
{
    VOS_TMessage_T *ptMsg = vos_receive_message();

    switch(ptMsg->tId)
    {
        case LL_CREATE_CIS_SCHED:
            generateLlCigEventScheduling(ptAcl, ptCig, ptCis);
        break;

        case LL_CIS_EVENT_DONE:
            rescheduleCigEvent(ptCig, ptCis);
        break;
    }
}

VOS_PROCESS(ll_isoal)
{
    VOS_TMessage_T *ptMsg = vos_receive_message();

    switch(ptMsg->tId)
    {
        case ISO_TX_DATA:
            LL_ISO_HCI_DATA_T *ptIsoHciData = (LL_ISO_HCI_DATA_T *)pMsg;
            CisObj_T *ptCis = getIsoCisObject(ptIsoHciData->cisHandle);
            CigObj_T *ptCig = getIsoCigObject(ptCis->cigId);

            if (ptCig->framing == UNFRAMED) {
                1. ptIsoPdu = generate unframed Iso PDU header into Hci buffer header.
                2. addLinkedList(ptCis->cisEvent.txPduQ, ptIsoPdu)
            }
            else {
                1. ptIsoPdu = VOS_Alloc(sizeof(cisPdu_T))
                2. packet Iso framed PDU
            }
        break;
    }
}

void generateLlCigEventScheduling(ptAcl, ptCig, ptCis)
{
    1. calculate an anchor point for the first CIS event based on:
        - connEventCount, cisOffsetMin/Max 
    2. set time stamp in framed PDU
}

void ll_iso_scheduler_callback()
{
    1. if TX is ACked, free ptCisEvent->txPduQ not to be retransmitted or move to TxAck signal event queue
    2. send TxACK to ll_iso_scheduler;
    3. send RxDone to ll_isoal;
}

void rescheduleCigEvent()
{
    1. calculate an anchor point for the next CIS event based on
        - previous anchor point, iso_interval
    2. set time offset for framed PDU
    3. register next schedule into ll_scheduler
}

void ll_iso_scheduler_scheduling_fn()
{
    1. read cigEvent in ll_scheduling queue;
    2. based on packing, transmit/receive schedule
}

//can be changed to dynamic allocation
static CIGMGM_T CigDb;
static CisObj_T CisObj[MAX_NOF_CIS];

void IsoCisCalculateCisParams(ptCig, ptCis)
{
    //If this is a first CIS connection, 
    1. check bandwidth
    2. determine:
     1) find default iso interval: iso interval should be > min_cig_sync_delay + margin for scheduling other ll
        . if LE has another CIG, same or integer multiple sdu interval may be the best to minimize collision
        . choose integer multiple of sdu interval temporarily
        . best candidate may be closest value to SDU interval which is equal to or greater than (min_cig_sync_delay*2)
     2) max_pdu = (MAX_SDU <= MAX_PDU(251)) ? MAX_SDU : MAX_SDU / ((MAX_SDU + (MAX_PDU-1))/MAX_PDU);
     3) MPT = (max_pdu + overhead)*8*us/symbol.
     4) min_se_interal = MPT*max_pdu_m2s + MPT*max_pdu_s2m+T_IFS+T_MSS
     5) choose minBN = CEIL(SDU interval / iso interval)
     4) nse = minBN*RTN
     5) min_cis_event_len = se_interval * nse
     6) min_cig_sync_delay = sum(min_cis_event_len of all CISes)
     7) max_ft = (maxTransportLatency - min_cis_sync_delay - sdu_interval) / iso interval candidate;
     8) if max_ft >= 1, use chosen value.
        else if max_ft < 1, next
     9) change params and recalculate:
        increase BN >> increase MAX_PDU >> decrease Iso interval

    3. choose best iso interval:
     - get information from ll_scheduler to check which one is best regular interval minimizing collision
     - rule:
      . CIS prefer 
      1) iso interval == sdu interval (Because we will use same Iso HCI packet max payload size to MAX_PDU)
      2) calculate minimum required iso interval based on given CIG timing.
      3) choose at least required bandwidth * 2 and longest value satisfying following conditions:
       - unframed: integer multiple of SDU interval
         . if already another CIG is active and the iso interval of the CIG is integer multiple of SDU interval: choose this
           if the iso interval of the CIG is not an integer multiple, choose closest value
       - framed: choose same value with the existing CIG's iso interval
}

CisObj_T *getIsoCigObject(uint8_t cigId)
{
    for (int i=0; i<MAX_NOF_CIG; i++) {
        if (CigDb.CigObj[i].cigId == cigId) {
            return &CigDb.CigObj[i];
        }
    }

    return NULL;
}

CisObj_T *getFreeCis(void)
{
    for (int i=0; i<MAX_NOF_CIS; i++) {
        if (CisObj[i].state == STBY) {
            return &CisObj[i];
        }
    }
    return NULL;
}

void LL_Iso_setCig(ptCmd, ptCig)
{
    ptCig->cigId = ptCmd->cigId;
    ptCig->sduInterval[ISO_PATH_M2S] = ptCmd->SduInterval_CtoP;
    ptCig->sduInterval[ISO_PATH_S2M] = ptCmd->SduInterval_PtoC;
    ptCig->worstSca = ptCmd->worstSca;
    ptCig->framing = ptCmd->framing;
    ptCig->packing = ptCmd->packing;
    ptCig->maxTransportLatency[ISO_PATH_M2S] = ptCmd->maxTransportLatency_M2S;
    ptCig->maxTransportLatency[ISO_PATH_S2M] = ptCmd->maxTransportLatency_S2M;
    ptCig->cisCount = ptCmd->cisCount;

    for (int i=0; i<ptCmd->cisCount; i++) {
        CisObj_T *ptCis = getFreeCis();
        ** copy command params from ptCmd;
        ptCis->cigId = ptCig->cigId;
        ptCig->cisObj[i] = ptCis;
    }
}

CigObj_T * getFreeCig(voi)
{
    CigObj_T *IsoCig = CigDb.IsoCig;
    for (i=0; i<MAX_NOF_CIG; i++)
    {
        if (IsoCig[i].state == STBY) {
            return &IsoCig[i];
        }
    }
    return NULL;
}

uint8_t LL_Iso_CreateCig(LL_CMD_SET_CIG_PARAM_T *ptCmd)
{
    CigObj_T * ptCig = getFreeCig();
    if (ptCig && (ptCmd->cisCount + CigDb.noCises) < MAX_NOF_CIS) {
        LL_Iso_setCig(ptCmd, ptCig);
        return SUCCESS;
    }
    else {
        return ERROR_LIMITED_RESOURCES;
    }
}
*/

int main(int argc, char* argv[])
{

    return 0;
}


