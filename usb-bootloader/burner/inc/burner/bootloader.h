#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_

#include "common/types.h"


#define BOOTLOADER_TIMEOUT_INFINITY 0xffffffff


typedef struct _BootloaderTargetInformation {
	struct {
		_U32 pagesCount;
		_U32 pageSize;
	} flash;

	struct {
		_U32 size;
	} e2prom;

	struct {
		_U8 versionMajor;
		_U8 versionMinor;
	} bootloader;

	struct {
		char *name;
	} mcu;
} BootloaderTargetInformation;


CommonError bootloader_initialize(void);

CommonError bootloader_terminate(void);

CommonError bootloader_connect(BootloaderTargetInformation *targetInformation, _U32 timeout);

CommonError bootloader_disconnect(void);

CommonError bootloader_reset(_U32 timeout);

CommonError bootloader_flashPageErase(_U32 pageumber, _U32 timeout);

CommonError bootloader_flashPageRead(_U32 pageumber, _U8 *pageBuffer, _U32 pageBufferSize, _U32 timeout, _U32 *pageBufferReadSize);

CommonError bootloader_flashPageWrite(_U32 pageumber, _U8 *pageBuffer, _U32 pageBufferSize, _U32 timeout, _U32 *pageBufferWritten);

CommonError bootloader_e2promRead(_U32 address, _U8 *e2promBuffer, _U32 e2promBufferSize, _U32 timeout, _U32 *e2promBufferReadSize);

CommonError bootloader_e2promWrite(_U32 address, _U8 *e2promBuffer, _U32 e2promBufferSize, _U32 timeout, _U32 *e2promBufferWritten);

#endif /* BOOTLOADER_H_ */
