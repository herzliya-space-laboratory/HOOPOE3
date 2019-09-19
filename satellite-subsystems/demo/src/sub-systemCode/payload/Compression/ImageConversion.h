/*!
 * @file ImageConversion.h
 * @brief This module contains all image conversions integrated into the Image DataBase
 * @date 25 July 2012
 * @author Roy Harel
 */


#ifndef IMAGECONVERSION_H_
#define IMAGECONVERSION_H_

#include "../DataBase/DataBase.h"
#include "../Misc/Macros.h"

ImageDataBaseResult CreateImageThumbnail_withoutSearch(imageid id, fileType reductionLevel, Boolean Skipping, uint32_t image_address, ImageMetadata image_metadata);

/*!
 * Reduces the resolution of an image.
 * There are 6 available levels, each smaller by a factor of 4 than the previous (a factor of 2 foe each dimention).
 * Original image resolution: 2048x1088.
 * Thumbnailing algorithms: Binning Thumbnails, Skipping Thumbnails.
 *
 * @brief Image resolution reducement
 *
 * @see the enum fileType at the file "DataBase.h"
 *
 * @param[in] database			The memory area to copy to.
 * @param[in] id  				Indicates the requested image to be converted
 * @param[in] reductionLevel	Indicates the requested file type to convert the image to
 * @param[in] Skipping			determines whether the thumbnail will be created using the Binning or the Skipping algorithm
 *
 * @return An enum indicating error type and reason for exit
 */
ImageDataBaseResult CreateImageThumbnail(imageid id, fileType reductionLevel, Boolean Skipping);

/*!
 * @brief Creates an image using the JPEG algorithm
 *
 * @see the enum fileType at the file "DataBase.h"
 *
 * @param[in] database			The memory area to copy to.
 * @param[in] id  				Indicates the requested image to be converted
 * @param[in] quality_factor	must range from 1-100
 * @param[in] reductionLevel	Indicates the requested file type to convert the image to
 *
 * @return An enum indicating error type and reason for exit
 */
ImageDataBaseResult compressImage(imageid id, unsigned int quality_factor);

#endif /* IMAGECONVERSION_H_ */
