#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "bootloader/common/protocol.h"

#include "burner/common/types.h"
#include "burner/bootloader.h"

#define DEBUG_LEVEL 4
#include "burner/common/debug.h"


#define PATH_LENGTH_MAX 1024

#define BOOTLOADER_TIMEOUT 3000


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

	struct {
		struct {
			_S32 offset;
			_S32 size;
		} read;

		struct {
			_S32 startPage;
			_S32 endPage;
		} erase;

		struct {
			_S32 offset;
		} write;
	} parameters;

	struct {
		char input[PATH_LENGTH_MAX];
		char output[PATH_LENGTH_MAX];
	} path;

	_BOOL reset;
	_BOOL commit;

	BurnerMemoryType memoryType;
} BurnerOperationDescription;


typedef struct _FlashMemoryBlock {
	_U8  *data;
	_BOOL read;
} FlashMemoryBlock;


typedef struct _FlashMemory {
	_U8              *buffer;
	_U32              blockSize;
	_U32              blocksCount;
	FlashMemoryBlock *blocks;
} FlashMemory;


typedef struct _E2promMemory {
	_U32 size;
	_U8 *buffer;
} E2promMemory;


static void debug_dump(void *buffer, int bufferSize) {
	_U32 offset = 0;
	_U32 lineElementsCount = 16;
	_U8 *buff = buffer;

	while (offset + 1 <= bufferSize) {
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


static void _getPageNumberByOffsetAndSize(_U32 pageSize, _U32 pagesCount, _U32 offset, _U32 size, _S32 *pageStart, _S32 *pageEnd) {

	{
		_U32 currentPage = 0;

		*pageStart = -1;
		*pageEnd   = -1;

		while (currentPage < pagesCount) {
			if (*pageStart < 0) {
				if (
					(offset >= currentPage * pageSize) &&
					(offset  < currentPage * pageSize + pageSize)
				) {
					*pageStart = currentPage;
				}
			}

			if (*pageEnd < 0) {
				if (
					(offset + size - 1 >= currentPage * pageSize) &&
					(offset + size - 1  < currentPage * pageSize + pageSize)
				) {
					*pageEnd = currentPage;
				}
			}

			if (*pageStart >= 0 && *pageEnd >= 0) {
				break;
			}

			currentPage++;
		}
	}
}


static CommonError _handleReadE2prom(E2promMemory *e2prom, _U32 offset, _U32 size) {
	CommonError ret = COMMON_NO_ERROR;

	{
		REPORT(("Reading %d bytes from e2prom at offset: %d", size, offset));

		ret = bootloader_e2promRead(offset, e2prom->buffer + offset, size, BOOTLOADER_TIMEOUT, NULL);
		if (ret != COMMON_NO_ERROR) {
			REPORT_ERR(("Unable to read e2prom memory!"));
		}
	}

	return ret;
}


static CommonError _handleReadFlash(FlashMemory *flash, _U32 offset, _U32 size) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 pageStart   = -1;
		_S32 pageEnd     = -1;
		_U32 lastPage    = 0;
		_U32 currentPage = 1;

		_getPageNumberByOffsetAndSize(flash->blockSize, flash->blocksCount, offset, size, &pageStart, &pageEnd);

		DBG(("_handleReadFlash(): Reading pages: %d - %d", pageStart, pageEnd));

		{
			_U32 i;

			for (i = pageStart; i <= pageEnd; i++) {
				ret = bootloader_flashPageRead(i, flash->blocks[i].data, flash->blockSize, BOOTLOADER_TIMEOUT, NULL);
				if (ret != COMMON_NO_ERROR) {
					REPORT_ERR(("Error reading from flash!"));

					break;
				}
			}
		}
	} while (0);

	return ret;
}


static CommonError _handleRead(BurnerOperationDescription *operation, FlashMemory *flash, E2promMemory *e2prom) {
	CommonError ret = COMMON_NO_ERROR;

	{
		_S32 outputFile = -1;

		do {
			_U32 memorySize;

			// Open output file if needed
			if (strlen(operation->path.output) > 0) {
				outputFile = open(operation->path.output, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
				if (outputFile < 0) {
					REPORT_ERR(("Unable to open output file. (%m)"));

					ret = COMMON_ERROR;
					break;
				}
			}

			// Check memory constraints
			if (operation->memoryType == BURNER_MEMORY_TYPE_FLASH) {
				memorySize = flash->blockSize * flash->blocksCount;

			} else {
				memorySize = e2prom->size;
			}

			if (operation->parameters.read.offset >= memorySize) {
				REPORT_ERR(("Offset is out of bounds!"));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}

			if (operation->parameters.read.offset + operation->parameters.read.size > memorySize) {
				REPORT_ERR(("Offset is out of bounds!"));

				ret = COMMON_ERROR_BAD_PARAMETER;
				break;
			}

			REPORT(("Reading '%s' memory from %d to %d...",
				operation->memoryType == BURNER_MEMORY_TYPE_FLASH ? "flash" : "e2prom",
				operation->parameters.read.offset, operation->parameters.read.offset + operation->parameters.read.size
			));

			// Read memory
			if (operation->memoryType == BURNER_MEMORY_TYPE_FLASH) {
				ret = _handleReadFlash(flash, operation->parameters.read.offset, operation->parameters.read.size);

			} else {
				ret = _handleReadE2prom(e2prom, operation->parameters.read.offset, operation->parameters.read.size);
			}
			if (ret != COMMON_NO_ERROR) {
				break;
			}

			// Save in file or dump on console
			if (outputFile >= 0) {
				if (operation->memoryType == BURNER_MEMORY_TYPE_FLASH) {
					write(outputFile, flash->buffer + operation->parameters.read.offset, operation->parameters.read.size);

				} else {
					write(outputFile, e2prom + operation->parameters.read.offset, operation->parameters.read.size);
				}

			} else {
				if (operation->memoryType == BURNER_MEMORY_TYPE_FLASH) {
					debug_dump(flash->buffer + operation->parameters.read.offset, operation->parameters.read.size);

				} else {
					debug_dump(e2prom->buffer + operation->parameters.read.offset, operation->parameters.read.size);
				}
			}
		} while (0);

		if (outputFile >= 0) {
			close(outputFile);
		}
	}

	return ret;
}


static CommonError _handleErase(BurnerOperationDescription *operation, FlashMemory *flash, E2promMemory *e2prom) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		if (operation->parameters.erase.endPage >= flash->blocksCount) {
			REPORT_ERR(("Last page is out of flash memory bounds!"));

			ret = COMMON_ERROR_BAD_PARAMETER;
			break;
		}

		{
			_U32 i;

			for (i = operation->parameters.erase.startPage; i <= operation->parameters.erase.endPage; i++) {
				ret = bootloader_flashPageErase(i, BOOTLOADER_TIMEOUT);
				if (ret != COMMON_NO_ERROR) {
					REPORT_ERR(("Unable to erase page %d!", i));

					break;
				}

				REPORT(("Page %d erased.", i));
			}
		}
	} while (0);

	return ret;
}


static CommonError _handleWriteFlash(FlashMemory *flash, _U32 offset, _U8 *buffer, _U32 bufferSize) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_S32 pageStart = -1;
		_S32 pageEnd   = -1;

		DBG(("_handleWriteFlash(): Call for offset: %d, size: %d", offset, bufferSize));

		_getPageNumberByOffsetAndSize(flash->blockSize, flash->blocksCount, offset, bufferSize, &pageStart, &pageEnd);

		DBG(("_handleWriteFlash(): Writing pages: %d - %d", pageStart, pageEnd));

		if (offset % flash->blockSize != 0) {
			DBG(("_handleWriteFlash(): Reading %d page.", pageStart));

			ret = bootloader_flashPageRead(pageStart, flash->blocks[pageStart].data, flash->blockSize, BOOTLOADER_TIMEOUT, NULL);
			if (ret != COMMON_NO_ERROR) {
				ERR(("_handleWriteFlash(): Error reading from flash!"));

				break;
			}

			flash->blocks[pageStart].read = TRUE;
		}

		if (
			((offset + bufferSize) % flash->blockSize != 0) &&
			(pageStart != pageEnd)
		) {
			DBG(("_handleWriteFlash(): Reading %d page.", pageEnd));

			ret = bootloader_flashPageRead(pageEnd, flash->blocks[pageEnd].data, flash->blockSize, BOOTLOADER_TIMEOUT, NULL);
			if (ret != COMMON_NO_ERROR) {
				ERR(("_handleWriteFlash(): Error reading from flash!"));

				break;
			}

			flash->blocks[pageEnd].read = TRUE;
		}

		memcpy(flash->buffer + offset, buffer, bufferSize);

		{
			_U8 *pageBuffer = NULL;

			pageBuffer = malloc(flash->blockSize);
			if (pageBuffer == NULL) {
				ERR(("_handleWriteFlash(): No more free memory!"));

				ret = COMMON_ERROR_NO_FREE_RESOURCES;
				break;
			}

			while (pageStart <= pageEnd) {
				REPORT(("Erasing page   %d.", pageStart));
				ret |= bootloader_flashPageErase(pageStart, BOOTLOADER_TIMEOUT);

				REPORT(("Writing page   %d.", pageStart));
				ret |= bootloader_flashPageWrite(pageStart, flash->blocks[pageStart].data, flash->blockSize, BOOTLOADER_TIMEOUT, NULL);

				REPORT(("Verifying page %d.", pageStart));
				ret |= bootloader_flashPageRead(pageStart, pageBuffer, flash->blockSize, BOOTLOADER_TIMEOUT, NULL);

				if (ret != COMMON_NO_ERROR) {
					REPORT_ERR(("Unable to write page %d!", pageStart));

					break;
				}

				if (memcmp(pageBuffer, flash->blocks[pageStart].data, flash->blockSize) != 0) {
					REPORT_ERR(("Verification failed!"));

					ret = COMMON_ERROR;
					break;
				}

				REPORT(("Page %d has written and verified.", pageStart));

				flash->blocks[pageStart].read = TRUE;

				pageStart++;
			}

			free(pageBuffer);
		}
	} while (0);

	return ret;
}


static CommonError _handleWriteE2prom(E2promMemory *e2prom, _U32 offset, _U8 *buffer, _U32 bufferSize) {
	CommonError ret = COMMON_NO_ERROR;

	{
		REPORT(("Writing %d bytes to e2prom at offset: %d", bufferSize, offset));

		ret = bootloader_e2promWrite(offset, buffer, bufferSize, BOOTLOADER_TIMEOUT, NULL);
		if (ret != COMMON_NO_ERROR) {
			REPORT_ERR(("Unable to write e2prom memory!"));
		}
	}

	return ret;
}


static CommonError _handleWrite(BurnerOperationDescription *operation, FlashMemory *flash, E2promMemory *e2prom) {
	CommonError ret = COMMON_NO_ERROR;

	{
		_S32 inputFile       = -1;
		_U8 *inputFileBuffer = NULL;

		do {
			_U32 memorySize;
			_U32 inputFileSize;

			inputFile = open(operation->path.input, O_RDONLY);
			if (inputFile < 0) {
				REPORT_ERR(("Unable to open input file! (%m)"));

				ret = COMMON_ERROR;
				break;
			}

			{
				struct stat stats = { 0 };

				if (fstat(inputFile, &stats) < 0) {
					REPORT_ERR(("Unable to stat input file! (%m)"));

					ret = COMMON_ERROR;
					break;
				}

				inputFileSize = stats.st_size;
			}

			if (operation->memoryType == BURNER_MEMORY_TYPE_FLASH) {
				// Prereserve memory for checksum
				memorySize = flash->blockSize * flash->blocksCount - 2;

			} else {
				memorySize = e2prom->size;
			}

			if (inputFileSize + operation->parameters.write.offset > memorySize) {
				REPORT_ERR(("Input file doesn't fit into memory!"));

				ret = COMMON_ERROR;
				break;
			}

			inputFileBuffer = malloc(inputFileSize);
			if (inputFileBuffer == NULL) {
				ERR(("_handleWrite(): No more free memory!"));

				ret = COMMON_ERROR_NO_FREE_RESOURCES;
				break;
			}

			if (read(inputFile, inputFileBuffer, inputFileSize) != inputFileSize) {
				REPORT_ERR(("Error reading input file! (%m)"));

				ret = COMMON_ERROR;
				break;
			}

			if (operation->memoryType == BURNER_MEMORY_TYPE_FLASH) {
				ret = _handleWriteFlash(flash, operation->parameters.write.offset, inputFileBuffer, inputFileSize);

			} else {
				ret = _handleWriteE2prom(e2prom, operation->parameters.write.offset, inputFileBuffer, inputFileSize);
			}
			if (ret != COMMON_NO_ERROR) {
				REPORT_ERR(("Error writing input file to '%s' memory!.", operation->memoryType == BURNER_MEMORY_TYPE_FLASH ? "flash" : "e2prom"));

				break;
			}
		} while (0);

		if (inputFileBuffer != NULL) {
			free(inputFileBuffer);
		}

		if (inputFile >= 0) {
			close(inputFile);
		}
	}

	return ret;
}


static _U8 crc8_getForByte(_U8 byte, _U8 polynomial, _U8 start) {
	_U8 remainder = start;

	remainder ^= byte;

	{
		_U8 bit;

		for (bit = 0; bit < 8; bit++) {
			if (remainder & 0x01) {
				remainder = (remainder >> 1) ^ polynomial;

			} else {
				remainder = (remainder >> 1);
			}

		}
	}

    return remainder;
}


static _U8 crc8_get(_U8 *buffer, _U16 bufferSize, _U8 polynomial, _U8 start) {
	_U8  remainder = start;
	_U16 byte;

	// Perform modulo-2 division, a byte at a time.
	for (byte = 0; byte < bufferSize; byte++) {
		remainder = crc8_getForByte(buffer[byte], polynomial, remainder);
	}

    return remainder;
}


static CommonError _handleCommit(FlashMemory *flash) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		_U32 i;

		for (i = 0; i < flash->blocksCount; i++) {
			if (! flash->blocks[i].read) {
				ret = bootloader_flashPageRead(i, flash->blocks[i].data, flash->blockSize, BOOTLOADER_TIMEOUT, NULL);
				if (ret != COMMON_NO_ERROR) {
					REPORT_ERR(("Unable to read page %d!", i));

					break;
				}

				DBG(("Page: %d read.", i));
			}
		}

		if (ret != COMMON_NO_ERROR) {
			break;
		}

		{
			_U8  checksum   = crc8_get(flash->buffer, (flash->blocksCount * flash->blockSize) - 1, IMAGE_CHECKSUM_POLYNOMIAL, 0);
			_U32 pageNumber = flash->blocksCount - 1;

			DBG(("CRC8: %x", checksum));

			flash->blocks[pageNumber].data[flash->blockSize - 1] = checksum;

			ret |= bootloader_flashPageErase(pageNumber, BOOTLOADER_TIMEOUT);
			ret |= bootloader_flashPageWrite(pageNumber, flash->blocks[pageNumber].data, flash->blockSize, BOOTLOADER_TIMEOUT, NULL);
			if (ret != COMMON_NO_ERROR) {
				REPORT_ERR(("Error writing checksum to flash memory!"));

				break;
			}
		}
	} while (0);

	return ret;
}


static void _showUsage(char *fileName) {
	REPORT(("Usage: "));
	REPORT((" $ %s [edwiomrc] [--page-start] [--page-end] [--offset] [--size] <inFile/outFile>", fileName));
	REPORT((" Where:"));
	REPORT(("  -e [--erase]      erase memory - only for flash."));
	REPORT(("     [--page-start] first page to erase - default: 0."));
	REPORT(("     [--page-end]   last page to erase  - default: the last page."));
	REPORT((" "));
	REPORT(("  -d [--dump]   dump memory."));
	REPORT(("     [--offset] start offset in memory - default: 0."));
	REPORT(("     [--size]   size of memory to dump - default: all writable memory size."));
	REPORT((" "));
	REPORT(("  -w [--write]  write memory."));
	REPORT(("     [--offset] start offset - default: 0."));
	REPORT((" "));
	REPORT(("  -i [--in]  input file path."));
	REPORT(("  -o [--out] output file path."));
	REPORT((" "));
	REPORT(("     [--memory-type] Type of memory to read/write. Available are: 'flash' and 'e2prom' - default: 'flash."));
	REPORT(("     [--reset]       Reset MCU after all operation performed."));
	REPORT(("     [--commit]      Compute and write checksum of flash memory to allow bootloader start main application."))
}


int main(int argc, char *argv[]) {
	CommonError ret = COMMON_NO_ERROR;

	do {
		BurnerOperationDescription operation    = { 0 };
		FlashMemory                flashMemory  = { 0 };
		E2promMemory               e2promMemory = { 0 };

		DBG(("START"));

		operation.type = BURNER_OPERATION_NONE;

		operation.parameters.erase.endPage   = -1;
		operation.parameters.erase.startPage = -1;
		operation.parameters.read.size       = -1;
		operation.parameters.read.offset     = -1;
		operation.parameters.write.offset    = -1;

		do {
			struct option longOptions[] = {
				{ "erase",       no_argument,       NULL, 'e' },
				{ "page-start",  required_argument, NULL,  1  },
				{ "page-end",    required_argument, NULL,  2  },
				{ "dump",        no_argument,       NULL, 'd' },
				{ "offset",      required_argument, NULL,  3  },
				{ "size",        required_argument, NULL,  4  },
				{ "write",       no_argument,       NULL, 'w' },
				{ "in",          required_argument, NULL, 'i' },
				{ "out",         required_argument, NULL, 'o' },
				{ "memory-type", required_argument, NULL, 'm' },
				{ "reset",       no_argument,       NULL, 'r' },
				{ "commit",      no_argument,       NULL, 'c' },
				{ NULL,          0,                 NULL,  0  }
			};
			char *shortOptions = "edwi:o:m:rc";

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

					case 'd':
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
							if (operation.memoryType != BURNER_MEMORY_TYPE_NONE) {
								ret = COMMON_ERROR_BAD_PARAMETER;

								break;
							}

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

					case 'r':
						{
							operation.reset = TRUE;
						}
						break;

					case 'c':
						{
							operation.commit = TRUE;
						}
						break;

					case 1:
						{
							operation.parameters.erase.startPage = atoi(optarg);
						}
						break;

					case 2:
						{
							operation.parameters.erase.endPage = atoi(optarg);
						}
						break;

					case 3:
						{
							operation.parameters.read.offset  = atoi(optarg);
							operation.parameters.write.offset = atoi(optarg);
						}
						break;

					case 4:
						{
							operation.parameters.read.size = atoi(optarg);
						}
						break;

					case '?':
						ret = COMMON_ERROR_BAD_PARAMETER;
						break;

					default:
						break;
				}
			}

			if (operation.type == BURNER_OPERATION_NONE) {
				ret = COMMON_ERROR_BAD_PARAMETER;
			}

			if (ret != COMMON_NO_ERROR) {
				_showUsage(argv[0]);

				break;
			}
		} while (0);

		if (ret != COMMON_NO_ERROR) {
			break;
		}

		REPORT(("Connecting..."));

		{
			BootloaderTargetInformation targetInformation = { 0 };

			// Try connect to target
			ret = bootloader_connect(&targetInformation, BOOTLOADER_TIMEOUT);
			if (ret != COMMON_NO_ERROR) {
				REPORT_ERR(("No device with active bootloader found!"));

				ret = COMMON_ERROR_NO_DEVICE;
				break;
			}

			REPORT(("Found MCU '%s' with bootloader version: %d.%d.",
				targetInformation.mcu.name, targetInformation.bootloader.versionMajor, targetInformation.bootloader.versionMinor
			));

			REPORT(("MCU flash size: %d (%d pages), page size: %d, e2prom size: %d",
				targetInformation.flash.pageSize * targetInformation.flash.pagesCount, targetInformation.flash.pagesCount,
				targetInformation.flash.pageSize, targetInformation.e2prom.size
			));

			// Set default memory type
			if (operation.memoryType == BURNER_MEMORY_TYPE_NONE) {
				operation.memoryType = BURNER_MEMORY_TYPE_FLASH;
			}

			// Allocate structure for flash memory blocks
			if (operation.memoryType == BURNER_MEMORY_TYPE_FLASH) {
				_U32 i;

				flashMemory.blockSize   = targetInformation.flash.pageSize;
				flashMemory.blocksCount = targetInformation.flash.pagesCount;

				flashMemory.blocks = malloc(flashMemory.blocksCount * sizeof(FlashMemoryBlock));
				if (flashMemory.blocks == NULL) {
					ERR(("main(): Unable to allocate memory!"));

					ret = COMMON_ERROR_NO_FREE_RESOURCES;
					break;
				}

				flashMemory.buffer = malloc(flashMemory.blockSize * flashMemory.blocksCount);
				if (flashMemory.buffer == NULL) {
					ERR(("main(): Unable to allocate memory!"));

					ret = COMMON_ERROR_NO_FREE_RESOURCES;
					break;
				}

				memset(flashMemory.buffer, 0, flashMemory.blockSize * flashMemory.blocksCount);

				for (i = 0; i < flashMemory.blocksCount; i++) {
					flashMemory.blocks[i].read = FALSE;
					flashMemory.blocks[i].data = flashMemory.buffer + i * flashMemory.blockSize;
				}

			// Allocate memory for e2prom map
			} else {
				e2promMemory.size = targetInformation.e2prom.size;

				e2promMemory.buffer = malloc(e2promMemory.size);
				if (e2promMemory.buffer == NULL) {
					ERR(("main(): Unable to allocate memory!"));

					ret = COMMON_ERROR_NO_FREE_RESOURCES;
					break;
				}
			}

			// Perform operation
			switch (operation.type) {
				case BURNER_OPERATION_ERASE:
					{
						if (operation.parameters.erase.startPage < 0) {
							operation.parameters.erase.startPage = 0;
						}

						if (operation.parameters.erase.endPage < 0) {
							operation.parameters.erase.endPage = targetInformation.flash.pagesCount - 1;
						}

						ret = _handleErase(&operation, &flashMemory, &e2promMemory);
					}
					break;

				case BURNER_OPERATION_READ:
					{
						// Set default parameters if needed
						{
							if (operation.parameters.read.offset < 0) {
								operation.parameters.read.offset = 0;
							}

							if (operation.parameters.read.size < 0) {
								if (operation.memoryType == BURNER_MEMORY_TYPE_FLASH) {
									operation.parameters.read.size = targetInformation.flash.pageSize * targetInformation.flash.pagesCount;

								} else {
									operation.parameters.read.size = targetInformation.e2prom.size;
								}
							}
						}

						ret = _handleRead(&operation, &flashMemory, &e2promMemory);
					}
					break;

				case BURNER_OPERATION_WRITE:
					{
						// Set default parameters if needed
						if (operation.parameters.write.offset < 0) {
							operation.parameters.write.offset = 0;
						}

						ret = _handleWrite(&operation, &flashMemory, &e2promMemory);
					}
					break;

				default:
					break;
			}
			if (ret != COMMON_NO_ERROR) {
				REPORT_ERR(("Operation failed!"));

				break;
			}

			if (operation.commit) {
				ret = _handleCommit(&flashMemory);
				if (ret != COMMON_NO_ERROR) {
					REPORT_ERR(("Unable to write flash checksum!"));

					break;
				}
			}
		}

		if (e2promMemory.buffer != NULL) {
			free(e2promMemory.buffer);
		}

		if (flashMemory.buffer != NULL) {
			free(flashMemory.buffer);
		}

		if (flashMemory.blocks != NULL) {
			free(flashMemory.blocks);
		}
	} while (0);

	REPORT(("Exiting..."));

	return ret;
}
