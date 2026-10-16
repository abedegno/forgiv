/* usb.h: ST's USB OTG host library (core, HCD, host core, MSC and CDC classes) and the
   application's USB code (usb/usb_task.c, usb/usbh_usr.c, usb/usb_bsp.c).
   name: descriptive (readability step 1). */
#ifndef USB_H
#define USB_H

#include "ghidra_types.h"
#include "fatfs_types.h"
#include "usb_otg_types.h"
#include "usbh_types.h"

/* data */
extern uint BSP_delay;
extern uchar CDC_Desc[];
extern uchar CDC_GetLineCode[];
extern uchar CDC_SetLineCode[];
extern uchar CDC_cb[];
extern uchar TxBuf[];
extern uchar USBH_CfgDesc[];
extern uchar USBH_HCD_INT_fops[];
extern uchar USBH_MSC_cb[];
extern uchar USBH_USR_ApplicationState;
extern uchar USB_Host[];
extern uchar USB_OTG_Core[];
extern uchar USR_cb[];
extern uchar UserCb[];
extern ushort cdc_rx_count;
extern uchar cdc_tx_busy_count;
extern uchar cdc_tx_len[];
extern uchar cdc_tx_stalled[];
extern FF_FIL fil_arm_image;
extern FF_FIL fil_msn;
extern FF_FIL fil_tmp;
extern FF_FIL fil_update;
extern uchar g_2000030D;
extern uchar g_2000030E;
extern uchar usb_class_prev;
extern uchar prev_ep_size[];
extern uchar prev_itf[];
extern uint BOTStallErrorCount;
extern uchar BOTErrorCount;
extern uint remainingDataLength;
extern uchar *datapointer;
extern uchar *datapointer_prev;
extern uchar error_direction;
extern uchar MSCErrorCount[];
extern uchar maxLunExceed[];
extern uchar RX_Enabled;
extern uchar RxBuf[];
extern uchar msc_scsi_status[];
extern ushort usb_cdc_timer;      /* 500 ms passes since the dongle's last good request */
extern uchar usb_class_code;      /* bInterfaceClass: 8 mass storage, 2 CDC */
extern uchar usb_device_inserted; /* HR22: 8 stick, 2 dongle, 0 none */
extern uchar usb_fatfs[];
extern uchar usb_file_buf[];
extern ushort usb_line_same_count;
extern int usb_line_state;
extern int usb_line_state_prev;
extern uchar usb_line_stuck;
extern uchar usb_reset_state; /* the dongle watchdog's USB restart sequence */
extern uchar usb_sn_written;  /* a stick set the serial number (fault LED on) */
extern uchar usb_start_delay; /* counts to 20 (10 s) before task_usb starts the host */
extern uchar usb_usr_init_done;
extern uchar usbh_ctrl_timeout[];

/* functions */
void BSP_Delay(int duration, int unit);
void BSP_SetTime(int unit);
void CDC_ChangeStateToIssueSetConfig(uint param_1, undefined1 *param_2);
uchar CDC_ClassRequest(void *pdev, void *phost);
uchar CDC_GETLineCoding(void *pdev, USBH_HOST *phost);
int CDC_Handle(int param_1, int param_2);
void CDC_InitTxRxParam();
void CDC_InterfaceDeInit(void *pdev, void *phost);
uchar CDC_InterfaceInit(void *pdev, void *phost);
void CDC_IssueGetConfig(uint param_1, undefined1 *param_2);
void CDC_ProcessReception(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost);
void CDC_ProcessTransmission(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost);
void CDC_ReceiveData(int param_1);
uchar CDC_SETControlLineState(void *pdev, USBH_HOST *phost);
uchar CDC_SETLineCoding(void *pdev, USBH_HOST *phost);
void CDC_SendData(int param_1, ushort param_2);
void CDC_StartReception(int p_x1);
void CDC_StopReception(int param_1);
uint HCD_GetCurrentFrame(int param_1);
int HCD_GetCurrentSpeed(int param_1);
uchar HCD_GetHCState(int param_1, int param_2);
uchar HCD_GetURB_State(int param_1, int param_2);
uint HCD_GetXferCnt(int param_1, int param_2);
void HCD_HC_Init(int param_1, uint param_2);
uint HCD_Init(USB_OTG_CORE_HANDLE *pdev, int coreID);
uint HCD_IsDeviceConnected(int param_1);
uint HCD_IsPortEnabled(int param_1);
uint HCD_ResetPort(int param_1);
int HCD_SubmitRequest(int param_1, int param_2);
uchar USBH_Alloc_Channel(USB_OTG_CORE_HANDLE *pdev, uchar ep_addr);
#ifndef OWN_USBH_BulkReceiveData
int USBH_BulkReceiveData(int param_1, int param_2, int param_3, int param_4);
#endif
#ifndef OWN_USBH_BulkSendData
int USBH_BulkSendData(int param_1, int param_2, int param_3, int param_4);
#endif
uchar USBH_ClearFeature(void *pdev, USBH_HOST *phost, ushort wValue, ushort wIndex);
uchar USBH_ClrFeature(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, uchar ep_num, uchar hc_num);
int USBH_Connected(int param_1);
int USBH_CtlReceiveData(int param_1, int param_2, int param_3, int param_4);
#ifndef OWN_USBH_CtlReq
int USBH_CtlReq(void *pdev, USBH_HOST *phost, uchar *buff, ushort length);
#endif
int USBH_CtlSendData(int param_1, int param_2, int param_3, int param_4);
uchar USBH_CtlSendSetup(int param_1, int param_2, int param_3);
uint USBH_DeAllocate_AllChannel(int param_1);
int USBH_DeInit(void *pdev, USBH_HOST *phost);
uint USBH_Disconnected(USB_OTG_CORE_HANDLE *pdev);
void USBH_ErrorHandle(undefined1 *param_1, int param_2);
uint USBH_Free_Channel(int param_1, int param_2);
uchar USBH_GetDescriptor(void *pdev, USBH_HOST *phost, uchar req_type, ushort value_idx,
                         uchar *buff, ushort length);
USBH_DescHeader_t *USBH_GetNextDesc(uchar *pbuf, ushort *ptr);
uchar USBH_Get_CfgDesc(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, ushort length);
uchar USBH_Get_DevDesc(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, uchar length);
uchar USBH_Get_StringDesc(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost, uchar string_index,
                          uchar *buff, ushort length);
uchar USBH_HandleControl(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost);
uchar USBH_HandleEnum(void *pdev, USBH_HOST *phost);
void USBH_Init(byte *param_1, int param_2, undefined1 *param_3, uint param_4, uint param_5);
int USBH_InterruptReceiveData(int param_1, int param_2, int param_3, int param_4);
int USBH_InterruptSendData(int param_1, int param_2, int param_3, int param_4);
int USBH_IsocReceiveData(int param_1, int param_2, int param_3, int param_4);
int USBH_IsocSendData(int param_1, int param_2, int param_3, int param_4);
uchar USBH_MSC_BOT_Abort(void *pdev, void *phost, uchar direction);
int USBH_MSC_ClassRequest(int param_1, int p_x1);
uchar USBH_MSC_DecodeCSW(void *pdev, void *phost);
void USBH_MSC_ErrorHandle(uchar status);
uchar USBH_MSC_GETMaxLUN(void *pdev, USBH_HOST *phost);
uchar USBH_MSC_Handle(void *pdev, void *phost);
void USBH_MSC_HandleBOTXfer(void *pdev, void *phost);
void USBH_MSC_Init(void *pdev);
void USBH_MSC_InterfaceDeInit(int param_1, uint param_2);
uchar USBH_MSC_InterfaceInit(void *pdev, void *phost);
uchar USBH_MSC_ModeSense6(void *pdev);
uchar USBH_MSC_Read10(void *pdev, uchar *dataBuffer, uint address, uint nbOfbytes);
uchar USBH_MSC_ReadCapacity10(void *pdev);
uchar USBH_MSC_RequestSense(void *pdev);
uchar USBH_MSC_TestUnitReady(void *pdev);
uchar USBH_MSC_Write10(void *pdev, uchar *dataBuffer, uint address, uint nbOfbytes);
int USBH_Modify_Channel(int param_1, int param_2, int param_3, int param_4, int param_5,
                        int param_6);
int USBH_OTG_ISR_Handler(int param_1);
uchar USBH_Open_Channel(USB_OTG_CORE_HANDLE *pdev, uchar hc_num, uchar dev_address, uchar speed,
                        uchar ep_type, ushort mps);
int USBH_PortDisabled(int param_1);
int USBH_PortEnabled(int param_1);
void USBH_Process(USB_OTG_CORE_HANDLE *pdev, USBH_HOST *phost);
longlong USBH_SOF(uint param_1);
uchar USBH_SetAddress(void *pdev, USBH_HOST *phost, uchar DeviceAddress);
uchar USBH_SetCfg(void *pdev, USBH_HOST *phost, ushort cfg_idx);
uchar USBH_SetFeature(void *pdev, USBH_HOST *phost, ushort wValue, ushort wIndex);
uchar USBH_SetInterface(void *pdev, USBH_HOST *phost, uchar ep_num, uchar altSetting);
int USBH_SubmitSetupRequest(USBH_HOST *phost, uchar *buff, ushort length);
void USBH_USR_Configuration_DescAvailable(int unused_host, int config_desc, int unused_len);
void USBH_USR_DeInit();
void USBH_USR_DeviceAddressAssigned();
void USBH_USR_DeviceAttached();
void USBH_USR_DeviceDisconnected();
void USBH_USR_DeviceNotSupported();
void USBH_USR_DeviceSpeedDetected(int speed);
void USBH_USR_Device_DescAvailable();
void USBH_USR_EnumerationDone();
void USBH_USR_Init();
int USBH_USR_MSC_Application();
void USBH_USR_Manufacturer_String();
void USBH_USR_OverCurrentDetected();
void USBH_USR_Product_String();
void USBH_USR_ResetDevice();
void USBH_USR_SerialNum_String();
void USBH_USR_UnrecoveredError();
uint USBH_USR_UserInput();
void USB_OTG_BSP_ConfigVBUS(uint unused_core);
void USB_OTG_BSP_DriveVBUS();
void USB_OTG_BSP_EnableInterrupt(uint unused_core);
void USB_OTG_BSP_Init(uint unused_core);
void USB_OTG_BSP_TimeInit();
void USB_OTG_BSP_TimerIRQ();
void USB_OTG_BSP_mDelay(int ms);
void USB_OTG_BSP_uDelay(int us);
int USB_OTG_CoreInit(int param_1);
int USB_OTG_CoreInitHost(USB_OTG_CORE_HANDLE *pdev);
int USB_OTG_CoreReset(int param_1);
int USB_OTG_DisableGlobalInt(int param_1);
void USB_OTG_DriveVbus(USB_OTG_CORE_HANDLE *pdev, uchar state);
void USB_OTG_EnableCommonInt(int param_1);
int USB_OTG_EnableGlobalInt(int param_1);
int USB_OTG_EnableHostInt(int param_1);
int USB_OTG_FlushRxFifo(int param_1);
int USB_OTG_FlushTxFifo(int param_1, int param_2);
uint USB_OTG_GetMode(int param_1);
int USB_OTG_HC_DoPing(USB_OTG_CORE_HANDLE *pdev, uchar hc_num);
int USB_OTG_HC_Halt(USB_OTG_CORE_HANDLE *pdev, uchar hc_num);
#ifndef OWN_USB_OTG_HC_Init
int USB_OTG_HC_Init(USB_OTG_CORE_HANDLE *pdev, uchar hc_num);
#endif
#ifndef OWN_USB_OTG_HC_StartXfer
int USB_OTG_HC_StartXfer(USB_OTG_CORE_HANDLE *pdev, uchar hc_num);
#endif
void USB_OTG_InitFSLSPClkSel(USB_OTG_CORE_HANDLE *pdev, uchar freq);
uchar USB_OTG_IsDeviceMode(int param_1);
uint USB_OTG_IsEvenFrame(int param_1);
uchar USB_OTG_IsHostMode(int param_1);
int USB_OTG_ReadCoreItr(int param_1);
int USB_OTG_ReadHPRT0(int param_1);
uint USB_OTG_ReadHostAllChannels_intr(int param_1);
uint USB_OTG_ReadOtgItr(int param_1);
int USB_OTG_ReadPacket(int param_1, int param_2, int param_3);
uint USB_OTG_ResetPort(USB_OTG_CORE_HANDLE *pdev);
int USB_OTG_SelectCore(int param_1, int param_2);
int USB_OTG_SetCurrentMode(int param_1, int param_2);
void USB_OTG_StopHost(USB_OTG_CORE_HANDLE *pdev);
uint USB_OTG_USBH_handle_Disconnect_ISR(USB_OTG_CORE_HANDLE *pdev);
uint USB_OTG_USBH_handle_IncompletePeriodicXfer_ISR(USB_OTG_CORE_HANDLE *pdev);
uint USB_OTG_USBH_handle_hc_ISR(USB_OTG_CORE_HANDLE *pdev);
uint USB_OTG_USBH_handle_hc_n_In_ISR(USB_OTG_CORE_HANDLE *pdev, uint num);
uint USB_OTG_USBH_handle_hc_n_Out_ISR(USB_OTG_CORE_HANDLE *pdev, uint num);
uint USB_OTG_USBH_handle_nptxfempty_ISR(USB_OTG_CORE_HANDLE *pdev);
uint USB_OTG_USBH_handle_port_ISR(USB_OTG_CORE_HANDLE *pdev);
uint USB_OTG_USBH_handle_ptxfempty_ISR(USB_OTG_CORE_HANDLE *pdev);
uint USB_OTG_USBH_handle_rx_qlvl_ISR(USB_OTG_CORE_HANDLE *pdev);
int USB_OTG_USBH_handle_sof_ISR(int param_1);
int USB_OTG_WritePacket(uchar *pdev, uchar *src, uchar ch_ep_num, ushort len);
void arm_fw_update_arm_bootflag();
uchar disk_initialize(int param_1);
int disk_ioctl(int param_1, int param_2, int param_3);
int disk_read(int param_1, int param_2, int param_3, int param_4);
int disk_status(int param_1);
int disk_write(int param_1, int param_2, int param_3, int param_4);
void task_usb();
void usb_cdc_line_coding_init();
void usb_check_update_files();
int usb_disk_getfree(int path, int *free_kb, int *total_kb);
void usb_host_port_resume(USB_OTG_CORE_HANDLE *pdev);
void usb_host_port_suspend(USB_OTG_CORE_HANDLE *pdev);
void usb_read_msn_file();
int usbh_line_state_stuck(int param_1);

#endif
