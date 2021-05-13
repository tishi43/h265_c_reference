#include "stdafx.h"
#include "bitstream_header.h"
#include <memory.h>
#include <assert.h>
#include <malloc.h>

//Ϊʲôÿ������Ҫдһ�����������void ***array2D����ͨ����
//(*array2D)[0][i] = init_valueerror C2036: ��void *��: δ֪�Ĵ�С
int get_mem2Dint(int ***array2D, int rows, int columns, int init_value)
{
	int i;
	*array2D = (int **)calloc(rows, sizeof(int *));
	(*array2D)[0] = (int *)calloc(columns * rows, sizeof(int));

	if (init_value)
	{
		/* memsetֻ��columns * rows���ֽڳ�ʼ����2                                   */
		/* ��columns * rows��short���͵����ݳ�ʼ����2,�ڴ�ĽṹӦΪ0200, 0200,......*/
		/* memset((*array2D)[0], init_value, columns * rows);                        */
		for (i = 0; i < columns * rows; i++)
		{
			(*array2D)[0][i] = init_value;
		}
	}

	for (i = 1; i < rows; i++)
	{
		(*array2D)[i] = (*array2D)[i - 1] + columns;
	}
	return rows * columns;
}

void free_mem2Dint(int **array2D)
{
	if (array2D)
	{
		if (array2D[0])
		{
			free(array2D[0]);
		}
		free(array2D);
	}
}


int get_mem2Dpixel(unsigned char ***array2D, unsigned short rows, unsigned short columns, unsigned char init_value)
{
	int i;
	*array2D = (unsigned char **)calloc(rows, sizeof(unsigned char*));
	(*array2D)[0] = (unsigned char *)calloc(columns * rows, sizeof(unsigned char));

	if ( init_value )
	{
		for (i = 0; i < columns * rows; i++)
		{
			(*array2D)[0][i] = init_value;
		}
	}

	for (i = 1; i < rows; i++)
	{
		(*array2D)[i] = (*array2D)[i - 1] + columns;
	}
	return rows * columns;
}

void free_mem2Dpixel(unsigned char** array2D)
{
	if (array2D)
	{
		if (array2D[0])
		{
			free(array2D[0]);
		}
		free(array2D);
	}
}

int get_mem2D(void ***array2D, int rows, int columns, int size)
{
	int i;
	*array2D = (void **)calloc(rows, sizeof(void *));
	(*array2D)[0] = (void *)calloc(columns * rows, size);
	for (i = 1; i < rows; i++)
	{
		(*array2D)[i] = (char *)(*array2D)[i - 1] + columns * size;
	}
	return rows * columns;
}

void free_mem2D(void **array2D)
{
	if (array2D)
	{
		if (array2D[0])
		{
			free(array2D[0]);
		}
		free(array2D);
	}
}