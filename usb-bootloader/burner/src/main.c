#include <getopt.h>
#include <string.h>

#include "bootloader/common/protocol.h"

#include "burner/common/types.h"

#include "burner/bootloader.h"

#define DEBUG_LEVEL 5
#include "burner/common/debug.h"


#define PATH_LENGTH_MAX 1024


typedef enum _BurnerOperation {
	BURNER_OPERATION_NONE,
	BURNER_OPERATION_ERASE,
	BURNER_OPERATION_READ,
	BURNER_OPERATION_WRITE
} BurnerOperationType;

typedef enum _BurnerMemoryType {
	BURNER_MEMORY_TYPE_NONE,
	BURNER_MEMORY_TYPE_FLASH,
	BURNER_MEMORY_TYPE_E2PROM
} BurnerMemoryType;


typedef struct _BurnerOperationDescription {
	BurnerOperationType type;

	union {
		struct {
			_S32 offset;
			_S32 size;
		} read;

		struct {
			_S32 startPage;
			_S32 endPage;
		} erase;

		struct {
			_U32 offset;
		} write;
	} parameters;

	struct {
		char input[PATH_LENGTH_MAX];
		char output[PATH_LENGTH_MAX];
	} path;

	BurnerMemoryType memoryType;
} BurnerOperationDescription;

#if 0
static CommonError _handleErase(usb_dev_handle *devHandle, BurnerOperationDescription *description, McuParameters *mcuParameters) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 libUsbRet;
		_U8 responseBuffer[1024];

		if (description->parameters.erase.startPage == -1) {
			description->parameters.erase.startPage = 0;
		}

		if (description->memoryType == BURNER_MEMORY_TYPE_FLASH) {
			_U32 availablePagesCount = (mcuParameters->flash.size - mcuParameters->flash.bootloaderSectionSize) / mcuParameters->flash.pageSize;
			_U32 i;

			if (description->parameters.erase.endPage == -1) {
				description->parameters.erase.endPage = availablePagesCount - 1;
			}

			if (description->parameters.erase.endPage >= availablePagesCount) {
				REPORT_ERR(("Erase: last page index is out of range! (%d > %d-%d)", description->parameters.erase.endPage, 0, availablePagesCount - 1));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}

			if (description->parameters.erase.startPage > description->parameters.erase.endPage) {
				REPORT_ERR(("Erase: First page must must have index smaller/equal last! (%d > %d)", description->parameters.erase.startPage, description->parameters.erase.endPage));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}

			REPORT(("Erase: Erasing FLASH memory pages from %d to %d", description->parameters.erase.startPage, description->parameters.erase.endPage));

			for (i = description->parameters.erase.startPage; i <= description->parameters.erase.endPage; i++) {
				_U16 pageNumber;

				libUsbRet = _commandRead(devHandle, BOOTLOADER_COMMON_COMMAND_FLASH_ERASE_PAGE, 0, i, responseBuffer, sizeof(responseBuffer));
				if (libUsbRet != 3) {
					REPORT_ERR(("Erase: Unable to erase page: %d!", i));

					ret = COMMON_ERROR;
					break;
				}

				pageNumber = responseBuffer[1] | (responseBuffer[2] << 8);
				if (pageNumber != i) {
					REPORT_ERR(("Erase: Unable to erase page: %d!", i));

					ret = COMMON_ERROR;
					break;
				}
			}

		} else if (description->memoryType == BURNER_MEMORY_TYPE_E2PROM) {
			_U32 availablePagesCount = mcuParameters->e2prom.size;
			_U32 i;

			if (description->parameters.erase.endPage == -1) {
				description->parameters.erase.endPage = availablePagesCount - 1;
			}

			if (description->parameters.erase.endPage >= availablePagesCount) {
				REPORT_ERR(("Erase: last page index is out of range! (%d > %d-%d)", description->parameters.erase.endPage, 0, availablePagesCount - 1));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}

			if (description->parameters.erase.startPage > description->parameters.erase.endPage) {
				REPORT_ERR(("Erase: First page must must have index smaller/equal last! (%d > %d)", description->parameters.erase.startPage, description->parameters.erase.endPage));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}

			DBG(("Erase: Erasing E2PROM memory bytes from %d to %d", description->parameters.erase.startPage, description->parameters.erase.endPage));

			for (i = description->parameters.erase.startPage; i <= description->parameters.erase.endPage; i++) {
				libUsbRet = _commandRead(devHandle, BOOTLOADER_COMMON_COMMAND_E2PROM_WRITE, 0xff, i, responseBuffer, sizeof(responseBuffer));
				if (libUsbRet != 1) {
					ret = COMMON_ERROR;
					break;
				}
			}

			if (ret != COMMON_NO_ERROR) {
				REPORT_ERR(("Erase: Error erasing E2PROM memory!"));

				break;
			}

		} else {
			REPORT_ERR(("ERASE: Not supported memory type! %d", description->memoryType));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}
	} while (0);

	return ret;
}


static CommonError _handleRead(usb_dev_handle *devHandle, BurnerOperationDescription *description, McuParameters *mcuParameters) {
	CommonError ret = COMMON_NO_ERROR;

	{
		_S32 outputFileFd = -1;
		_U8 *outputBuffer = NULL;

		do {
			_U32 toReadDataSize = 0;

			if (strlen(description->path.output) == 0) {
				REPORT_ERR(("Read: Output file not specified!"));

				ret = COMMON_ERROR;
				break;
			}

			if (description->parameters.read.offset <= 0) {
				description->parameters.read.offset = 0;
			}

			if (description->memoryType == BURNER_MEMORY_TYPE_FLASH) {
				if (description->parameters.read.size <= 0) {
					description->parameters.read.size =
							mcuParameters->flash.size - mcuParameters->flash.bootloaderSectionSize - description->parameters.read.offset;
				} else {
					if (
						(description->parameters.read.size + description->parameters.read.offset) >
						(mcuParameters->flash.size - mcuParameters->flash.bootloaderSectionSize - description->parameters.read.offset)
					) {
						REPORT_ERR(("Read: trying to read data out of available flash memory!"));

						ret = COMMON_ERROR;
						break;
					}
				}

			} else if (description->memoryType == BURNER_MEMORY_TYPE_E2PROM) {
				if (description->parameters.read.size <= 0) {
					description->parameters.read.size = mcuParameters->e2prom.size;

				} else {
					if ((description->parameters.read.size + description->parameters.read.offset) > mcuParameters->e2prom.size) {
						REPORT_ERR(("Read: trying to read data out of available e2prom memory!"));

						ret = COMMON_ERROR;
						break;
					}
				}
			} else {
				REPORT_ERR(("Read: Not supported memory type: %d!", description->memoryType));
			}

			REPORT(("Read: Opening output file: '%s'", description->path.output));

			outputFileFd = open(description->path.output, O_RDWR | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
			if (outputFileFd < 0) {
				REPORT_ERR(("Read: Error opening output file! (%s, '%m')", description->path.output));

				ret = COMMON_ERROR;
				break;
			}

			REPORT(("Read: Reading %d bytes of data from offset: %#08x", description->parameters.read.size, description->parameters.read.offset));

			if (description->memoryType == BURNER_MEMORY_TYPE_FLASH) {
				_U32 pageStart           = 0;
				_U32 pageEnd             = 0;
				_U32 outputBufferWritten = 0;

				pageStart = description->parameters.read.offset / mcuParameters->flash.pageSize;
				pageEnd   = (description->parameters.read.offset + description->parameters.read.size) / mcuParameters->flash.pageSize;

				DBG(("Reading pages: %d - %d", pageStart, pageEnd));

				outputBuffer = calloc(pageEnd - pageStart + 1, mcuParameters->flash.pageSize);
				if (outputBuffer == NULL) {
					REPORT_ERR(("Read: Unable to allocate memory for output buffer!"));

					ret = COMMON_ERROR_NO_FREE_RESOURCES;
					break;
				}

				{
					_U32 currentPage = pageStart;
					_S32 libUsbRet   = 0;
					_U8 *responseBuffer = NULL;

					responseBuffer = calloc(mcuParameters->flash.pageSize + 1, 1);
					if (responseBuffer == NULL) {
						ERR(("Read: Error allocating temporary memory!"));

						ret = COMMON_ERROR_NO_FREE_RESOURCES;
						break;
					}

					while (currentPage <= pageEnd) {
						libUsbRet = _commandRead(
							devHandle,
							BOOTLOADER_COMMON_COMMAND_FLASH_READ_PAGE,
							0,
							currentPage,
							responseBuffer,
							mcuParameters->flash.pageSize + 1
						);
						if (libUsbRet != mcuParameters->flash.pageSize + 1) {
							REPORT_ERR(("Read: Error reading data from target!"));

							ret = COMMON_ERROR;
							break;
						}

						memcpy(outputBuffer + (currentPage - pageStart) * mcuParameters->flash.pageSize, responseBuffer, mcuParameters->flash.pageSize);

						currentPage++;
					}

					free(responseBuffer);

					if (ret != COMMON_NO_ERROR) {
						break;
					}
				}

				write(outputFileFd, outputBuffer + (description->parameters.read.offset % mcuParameters->flash.pageSize), description->parameters.read.size);

			} else {

			}
		} while (0);

		if (outputBuffer != NULL) {
			free(outputBuffer);

			outputBuffer = NULL;
		}

		if (outputFileFd >= 0) {
			close(outputFileFd);

			outputFileFd = -1;
		}
	}

	return ret;
}
#endif


static void debug_dump(void *buffer, int bufferSize) {
	_U32 offset = 0;
	_U32 lineElementsCount = 16;
	_U8 *buff = buffer;

	while (offset + 1 < bufferSize) {
		_U32 lineLength = (bufferSize - offset >= lineElementsCount) ? lineElementsCount : bufferSize - offset;
		_U32 i;

		printf("[ %04x ]:  ", offset);

		for (i = 0; i < lineLength; i++) {
			printf("%02X ", buff[offset + i]);
		}

		while (i < lineElementsCount) {
			printf("   ");

			i++;
		}

		printf("   ");

		for (i = 0; i < lineLength; i++) {
			if (isalnum(buff[i])) {
				printf("%c", buff[i]);

			} else {
				printf(".");
			}
		}

		offset += i;

		printf("\n");
	}
}


int main(int argc, char *argv[]) {
	CommonError ret = COMMON_NO_ERROR;

	{
		BurnerOperationDescription operation = { 0 };

		DBG(("START"));

		operation.type = BURNER_OPERATION_NONE;

		operation.parameters.erase.endPage   = -1;
		operation.parameters.erase.startPage = -1;

		do {
			struct option longOptions[] = {
				{ "erase",       no_argument,       NULL, 'e' },
				{ "page-start",  required_argument, NULL,  1  },
				{ "page-end",    required_argument, NULL,  2  },
				{ "read",        no_argument,       NULL, 'r' },
				{ "offset",      required_argument, NULL,  3  },
				{ "size",        required_argument, NULL,  4  },
				{ "write",       no_argument,       NULL, 'w' },
				{ "in",          required_argument, NULL, 'i' },
				{ "out",         required_argument, NULL, 'o' },
				{ "memory-type", required_argument, NULL, 'm' },
				{ NULL,          0,                 NULL,  0  }
			};
			char *shortOptions = "erwi:o:m:";

			bootloader_initialize();

			while (1) {
				int longOptionIndex = -1;
				int getOptRet;

				getOptRet = getopt_long(argc, argv, shortOptions, longOptions, &longOptionIndex);
				if (getOptRet < 0) {
					DBG(("all options parsed."));

					break;
				}

				DBG(("Option: %d, long option index: %d", getOptRet, longOptionIndex));

				switch (getOptRet) {
					case 'e':
						{
							if (operation.type != BURNER_OPERATION_NONE) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							operation.type = BURNER_OPERATION_ERASE;
						}
						break;

					case 'r':
						{
							if (operation.type != BURNER_OPERATION_NONE) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							operation.type = BURNER_OPERATION_READ;
						}
						break;

					case 'w':
						{
							if (operation.type != BURNER_OPERATION_NONE) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							operation.type = BURNER_OPERATION_WRITE;
						}
						break;

					case 'i':
						{
							if (operation.path.input[0] != '\0') {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							strncpy(operation.path.input, optarg, sizeof(operation.path.input));
						}
						break;

					case 'o':
						{
							if (operation.path.output[0] != '\0') {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							strncpy(operation.path.output, optarg, sizeof(operation.path.output));
						}
						break;

					case 'm':
						{
							if (strcasecmp("flash", optarg) == 0) {
								operation.memoryType = BURNER_MEMORY_TYPE_FLASH;

							} else if (strcasecmp("e2prom", optarg) == 0) {
								operation.memoryType = BURNER_MEMORY_TYPE_E2PROM;

							} else {
								REPORT_ERR(("Not supported memory type '%s'! Supported are only: flash|e2prom", optarg));

								ret = COMMON_ERROR_BAD_PARAMETER;
							}
						}
						break;

					case 1:
						{
							if (operation.type != BURNER_OPERATION_ERASE) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							operation.parameters.erase.startPage = atoi(optarg);
						}
						break;

					case 2:
						{
							if (operation.type != BURNER_OPERATION_ERASE) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							operation.parameters.erase.endPage = atoi(optarg);
						}
						break;

					case 3:
						{
							if (operation.type != BURNER_OPERATION_READ) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							operation.parameters.read.offset = atoi(optarg);
						}
						break;

					case 4:
						{
							if (operation.type != BURNER_OPERATION_READ) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

							operation.parameters.read.size = atoi(optarg);
						}
						break;

					default:
						break;
				}
			}

			if (ret != COMMON_NO_ERROR) {
				// TODO: Show usage

				break;
			}

			if (operation.memoryType == BURNER_MEMORY_TYPE_NONE) {
				operation.memoryType = BURNER_MEMORY_TYPE_FLASH;
			}
		} while (0);

		REPORT(("Connecting..."));

		{
			BootloaderTargetInformation targetInformation = { 0 };

			bootloader_connect(&targetInformation, 8000);

			REPORT(("Found MCU '%s' with bootloader version: %d.%d.",
				targetInformation.mcu.name, targetInformation.bootloader.versionMajor, targetInformation.bootloader.versionMinor
			));

			REPORT(("MCU flash size: %d (%d pages), page size: %d, e2prom size: %d",
				targetInformation.flash.pageSize * targetInformation.flash.pagesCount, targetInformation.flash.pagesCount,
				targetInformation.flash.pageSize, targetInformation.e2prom.size
			));

	//		bootloader_reset(8000);

			_U8 buffer[] = {
				0x12, 'c', 0x43, 0x98, 0xff, 'd', 'a', 0xaa,
				0x12, 'c', 0x43, 0x98, 0xff, 'd', 'a', 0xaa,
				0x12, 'c', 0x43, 0x98, 0xff, 'd', 'a', 0xaa,
				0x12, 'c', 0x43, 0x98, 0xff, 'd', 'a', 0xaa,
				0x12, 'c', 0x43, 0x98, 0xff, 'd', 'a', 0xaa
			};

			debug_dump(buffer, sizeof(buffer));


			switch (operation.type) {
				case BURNER_OPERATION_ERASE:
					{
						ret = bootloader_flashPageErase(0, 1000);

	//					ret = _handleErase(devHandle, &operation, params);
					}
					break;

				case BURNER_OPERATION_READ:
					{
						_U8 pageBuffer[128] = { 0 };
						_U32 written;

						ret = bootloader_flashPageRead(0, pageBuffer, 128, 1000, &written);

						debug_dump(pageBuffer, 128);
	//					ret = _handleRead(devHandle, &operation, params);
					}
					break;

				case BURNER_OPERATION_WRITE:
					{
						_U8 pageBuffer[128];

						_U32 i;

						for (i = 0; i < 128; i++) {
							pageBuffer[i] = i;
						}

						bootloader_flashPageWrite(0, pageBuffer, 128, 1000, &i);
					}
					break;

				default:
					break;
			}
		}
	}

	REPORT(("Exiting..."));

	return ret;
}
