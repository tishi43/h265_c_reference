#include "stdafx.h"
#include "bitstream_header.h"
#include <memory.h>
#include <assert.h>
#include <malloc.h>

//为什么每种类型要写一个函数，如果void ***array2D编译通不过
//(*array2D)[0][i] = init_valueerror C2036: “void *”: 未知的大小
int get_mem2Dint(int ***array2D, int rows, int columns, int init_value)
{
	int i;
	*array2D = (int **)calloc(rows, sizeof(int *));
	(*array2D)[0] = (int *)calloc(columns * rows, sizeof(int));

	if (init_value)
	{
		/* memset只把columns * rows个字节初始化成2                                   */
		/* 把columns * rows个short类型的数据初始化成2,内存的结构应为0200, 0200,......*/
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