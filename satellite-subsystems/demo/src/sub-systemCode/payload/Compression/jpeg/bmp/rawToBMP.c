
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <sys/types.h>	// for f_open
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h> 	// for memcpy
#include <stdio.h>		// for printf
#include <stdlib.h>		// for exit

#include "../../../Misc/FileSystem.h"

#include "rawToBMP.h"

#define IMG_WIDTH(compfact)	(2048/(compfact))
#define IMG_HEIGHT(compfact) (1088/(compfact))

#define MAX_IMG_WIDTH 2048
#define MAX_IMG_HEIGHT 1088

#define getPixelColor(row, col)		(row % 2 == 0 ? (col % 2 == 0 ? RED : GREEN) : (col % 2 == 0 ? GREEN : BLUE))

typedef enum
{
	RED,
	GREEN,
	BLUE,
} RGB;

void interpolate(F_FILE* input_fd, int row, int col, int compfact);
int getInputAt(F_FILE* input_fd, int row, int col, int compfact);
unsigned char f_writeF_FILEHeader(F_FILE* output_fd, int compfact);
unsigned char f_writeOutputRow(F_FILE* output_fd, int compfact);

unsigned char row1Data[MAX_IMG_WIDTH], row2Data[MAX_IMG_WIDTH], row3Data[MAX_IMG_WIDTH];
unsigned char * rowsInMem[3] = { row1Data, row2Data,row3Data };
int rowIndexInArray1;
unsigned char rowOutput[MAX_IMG_WIDTH * 3];		// B,G,R

void TransformRawToBMP(char * inputF_FILE, char * outputF_FILE, int compfact)
{
	F_FILE* input_fd = NULL;
	f_managed_open(inputF_FILE, "r+", &input_fd);
	if (input_fd == NULL)
	{
		printf("Failed to f_open intput F_FILE\n");
		return;
	}
	F_FILE* output_fd = NULL;
	f_managed_open(outputF_FILE, "w", &output_fd);
	if (output_fd == NULL)
	{
		printf("Failed to f_open output F_FILE\n");
		f_managed_close(&input_fd);
		return;
	}

	vTaskDelay(SYSTEM_DEALY);

	rowIndexInArray1 = -1;

	// Writing File Header:
	if (0 != f_writeF_FILEHeader(output_fd, compfact))	// for true BMP F_FILE format, if just image data is required, comment this out. take note that the BMP F_FILE format expects data in BGR format, which is not intuitive
	{
		f_managed_close(&output_fd);
		f_managed_close(&input_fd);
		return;
	}

	int row, col;
	for (row = 0; row < IMG_HEIGHT(compfact); ++row)
	{
		for (col = 0; col < IMG_WIDTH(compfact); ++col)
		{
			interpolate(input_fd, row, col, compfact);
		}
		if (0 != f_writeOutputRow(output_fd, compfact))
		{
			f_managed_close(&output_fd);
			f_managed_close(&input_fd);
			return;
		}

		vTaskDelay(SYSTEM_DEALY);
	}

	f_managed_close(&output_fd);
	f_managed_close(&input_fd);

	printf("Done!\n");
}

void interpolate(F_FILE* input_fd, int row, int col, int compfact)
{
	int red = 0;
	int green = 0;
	int blue = 0;

	int dividerBlue = 0, dividerRed = 0, dividerGreen = 0;
	RGB posColor = getPixelColor(row, col);

	switch (posColor)
	{
	case RED:
		red = getInputAt(input_fd, row, col, compfact);
		if (row > 0)
		{
			green += getInputAt(input_fd, row - 1, col, compfact);
			++dividerGreen;

			if (col > 0)
			{
				blue += getInputAt(input_fd, row - 1, col - 1, compfact);
				++dividerBlue;
			}
			if (col < IMG_WIDTH(compfact) - 1)
			{
				blue += getInputAt(input_fd, row - 1, col + 1, compfact);
				++dividerBlue;
			}
		}
		if (row < IMG_HEIGHT(compfact) - 1)
		{
			green += getInputAt(input_fd, row + 1, col, compfact);
			++dividerGreen;

			if (col > 0)
			{
				blue += getInputAt(input_fd, row + 1, col - 1, compfact);
				++dividerBlue;
			}
			if (col < IMG_WIDTH(compfact) - 1)
			{
				blue += getInputAt(input_fd, row + 1, col + 1, compfact);
				++dividerBlue;
			}
		}
		if (col > 0)
		{
			green += getInputAt(input_fd, row, col - 1, compfact);
			++dividerGreen;
		}
		if (col < IMG_WIDTH(compfact) - 1)
		{
			green += getInputAt(input_fd, row, col + 1, compfact);
			++dividerGreen;
		}

		blue /= dividerBlue;
		green /= dividerGreen;

		break;
	case GREEN:
		green = getInputAt(input_fd, row, col, compfact);

		int horizontal;
		if (col == 0)       // only option is red/blue to the right
			horizontal = getInputAt(input_fd, row, col + 1, compfact);
		else if (col == IMG_WIDTH(compfact) - 1)   // only option is red/blue to the left
			horizontal = getInputAt(input_fd, row, col - 1, compfact);
		else                         // red/blue is both to the right and left
			horizontal = (getInputAt(input_fd, row, col - 1, compfact) + getInputAt(input_fd, row, col + 1, compfact)) / 2;

		if (row == 0)       // only option is blue below and red to the left and right
		{
			blue = getInputAt(input_fd, row + 1, col, compfact);
			red = horizontal;
		}
		else if (row == IMG_HEIGHT(compfact) - 1) // only option is red above and blue to the left and right
		{
			red = getInputAt(input_fd, row - 1, col, compfact);
			blue = horizontal;
		}
		else                    // might be blue above and below and red to the sides, or red above and below and blue to the sides
		{
			int vertical = (getInputAt(input_fd, row - 1, col, compfact) + getInputAt(input_fd, row + 1, col, compfact)) / 2;
			if (getPixelColor(row - 1, col) == BLUE)
			{
				blue = vertical;
				red = horizontal;
			}
			else
			{
				red = vertical;
				blue = horizontal;
			}
		}

		break;
	case BLUE:
		blue = getInputAt(input_fd, row, col, compfact);
		if (row > 0)
		{
			green += getInputAt(input_fd, row - 1, col, compfact);
			++dividerGreen;

			if (col > 0)
			{
				red += getInputAt(input_fd, row - 1, col - 1, compfact);
				++dividerRed;
			}
			if (col < IMG_WIDTH(compfact) - 1)
			{
				red += getInputAt(input_fd, row - 1, col + 1, compfact);
				++dividerRed;
			}
		}
		if (row < IMG_HEIGHT(compfact) - 1)
		{
			green += getInputAt(input_fd, row + 1, col, compfact);
			++dividerGreen;

			if (col > 0)
			{
				red += getInputAt(input_fd, row + 1, col - 1, compfact);
				++dividerRed;
			}
			if (col < IMG_WIDTH(compfact) - 1)
			{
				red += getInputAt(input_fd, row + 1, col + 1, compfact);
				++dividerRed;
			}
		}
		if (col > 0)
		{
			green += getInputAt(input_fd, row, col - 1, compfact);
			++dividerGreen;
		}
		if (col < IMG_WIDTH(compfact) - 1)
		{
			green += getInputAt(input_fd, row, col + 1, compfact);
			++dividerGreen;
		}

		red /= dividerRed;
		green /= dividerGreen;
		break;
	default:
		break;
	}

	rowOutput[col * 3 + 0] = (unsigned char)red;
	rowOutput[col * 3 + 1] = (unsigned char)green;
	rowOutput[col * 3 + 2] = (unsigned char)blue;
}

int IThinkItsRow = 0;
int getInputAt(F_FILE* input_fd, int row, int col, int compfact)
{
	if (rowIndexInArray1 == -1)	// no data f_read alf_ready
	{
		if (row > 2)
		{
			printf("ERROR - first request of data was not from row 0,1  or 2\n");
			return 0;
		}

		int test = f_read(row1Data, 1, IMG_WIDTH(compfact), input_fd);
		if (test != IMG_WIDTH(compfact))
		{
			printf("get input at %d, %d\n", row, col);
			printf("Failed to f_read a full row of data\n");
			exit(1);
			return 0;
		}
		if (f_read(row2Data, 1, IMG_WIDTH(compfact), input_fd) != IMG_WIDTH(compfact))
		{
			printf("get input at %d, %d\n", row, col);
			printf("Failed to f_read a full row of data\n");
			exit(2);
			return 0;
		}
		if (f_read(row3Data, 1, IMG_WIDTH(compfact), input_fd) != IMG_WIDTH(compfact))
		{
			printf("get input at %d, %d\n", row, col);
			printf("Failed to f_read a full row of data\n");
			exit(3);
			return 0;
		}
		rowIndexInArray1 = 0;
		rowsInMem[0] = row1Data;
		rowsInMem[1] = row2Data;
		rowsInMem[2] = row3Data;
	}
	else if (row == rowIndexInArray1 + 3)		// advance row
	{
		unsigned char * temp = rowsInMem[0];
		rowsInMem[0] = rowsInMem[1];
		rowsInMem[1] = rowsInMem[2];
		rowsInMem[2] = temp;

		if (f_read(rowsInMem[2], 1, IMG_WIDTH(compfact), input_fd) != IMG_WIDTH(compfact))
		{
			printf("get input at %d, %d\n", row, col);
			printf("Failed to f_read a full row of data\n");
			exit(4);
			return 0;
		}
		++rowIndexInArray1;
	}
	else if (row < rowIndexInArray1 || rowIndexInArray1 > rowIndexInArray1 + 2)	// not in the 3 loaded rows
	{
		printf("Attempted to get a row of data not loaded to memory\n");
		return 0;
	}

	return (int)(rowsInMem[row - rowIndexInArray1][col]);
}

unsigned char f_writeF_FILEHeader(F_FILE* output_fd, int compfact)
{
	if (output_fd == NULL)
	{
		printf("output F_FILE was not f_open, so failed to f_write header\n");
		return 1;
	}

	unsigned int 	uintVar;
	int 			intVar;
	unsigned short	ushortVar;

	unsigned char headerBuf[14 + 40];
	headerBuf[0] = 'B';
	headerBuf[1] = 'M';
	uintVar = 14 + 40 + IMG_WIDTH(compfact)*IMG_HEIGHT(compfact) * 3;
	memcpy(headerBuf + 2, (unsigned char *)&uintVar, sizeof(uintVar));
	uintVar = 0;
	memcpy(headerBuf + 6, (unsigned char *)&uintVar, sizeof(uintVar));
	uintVar = 14 + 40;
	memcpy(headerBuf + 10, (unsigned char *)&uintVar, sizeof(uintVar));

	uintVar = 40;
	memcpy(headerBuf + 14, (unsigned char *)&uintVar, sizeof(uintVar));		// size of image info header
	intVar = IMG_WIDTH(compfact);
	memcpy(headerBuf + 18, (unsigned char *)&intVar, sizeof(intVar));			// bitmap width
	intVar = IMG_HEIGHT(compfact);
	memcpy(headerBuf + 22, (unsigned char *)&intVar, sizeof(intVar));			// bitmap height
	ushortVar = 1;
	memcpy(headerBuf + 26, (unsigned char *)&ushortVar, sizeof(ushortVar));	// number of color planes
	ushortVar = 8 * 3;
	memcpy(headerBuf + 28, (unsigned char *)&ushortVar, sizeof(ushortVar));	// bits per pixel
	uintVar = 0;
	memcpy(headerBuf + 30, (unsigned char *)&uintVar, sizeof(uintVar));		// compression version - default 0
	uintVar = 3 * IMG_WIDTH(compfact)*IMG_HEIGHT(compfact);
	memcpy(headerBuf + 34, (unsigned char *)&uintVar, sizeof(uintVar));		// image size
	uintVar = 1;
	memcpy(headerBuf + 38, (unsigned char *)&uintVar, sizeof(uintVar));		// horizontal resolution - less than 1, so set as 1
	memcpy(headerBuf + 42, (unsigned char *)&uintVar, sizeof(uintVar));		// vertical resolution - less than 1, so set as 1
	uintVar = 0;
	memcpy(headerBuf + 46, (unsigned char *)&uintVar, sizeof(uintVar));		// colors in pallete - default 0
	memcpy(headerBuf + 50, (unsigned char *)&uintVar, sizeof(uintVar));		// important colors used - default 0

	if (f_write(headerBuf, 1, 14 + 40, output_fd) != 54)
	{
		printf("Failed to f_write header to F_FILE\n");
		return 1;
	}
	return 0;
}

unsigned char f_writeOutputRow(F_FILE* output_fd, int compfact)
{
	if (output_fd == NULL)
	{
		printf("output F_FILE was not f_open, so failed to f_write data row\n");
		return 1;
	}

	if (f_write(rowOutput, 1, IMG_WIDTH(compfact) * 3, output_fd) != IMG_WIDTH(compfact) * 3)
	{
		printf("Failed to f_write data row to F_FILE\n");
		return 1;
	}
	return 0;
}
