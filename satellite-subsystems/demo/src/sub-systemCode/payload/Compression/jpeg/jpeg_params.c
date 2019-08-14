/*
 * params.c
 *
 *  Created on: 12 αιεμ 2019
 *      Author: Tamir
 */

#include <sys/types.h>
#include "jpeg_params.h"
BooleanJpeg check(const jpeg_params_t * params)
{
	if (params == NULL)
		return FALSE_JPEG;
	if ((params->m_quality < 1) || (params->m_quality > 100)) return FALSE_JPEG;
	if ((uint)(params->m_subsampling) > (uint)H2V2) return FALSE_JPEG;
	return TRUE_JPEG;
}
