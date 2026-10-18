/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * image.c - Images of the frames sent to the PSP    *
 *                                                   *
 * Copyright © 2026 Arthur Neto                      *
 *****************************************************/

/*
 * Turns a frame of the virtual display into the image of a frame of the old protocol: a JPEG, or an uncompressed bitmap (see `PSPDISP_COM_FLAG_IMAGE_IS_UNCOMPRESSED`).
 *
 * The image is never rotated: the display settings of the computer rotate the virtual display, for a PSP held vertically.
 */

#pragma once

#include <string.h>
#include <turbojpeg.h>
#include <glib.h>
#include "../../shared.h"

#define PSPDISP_DAEMON_IMAGE_PIXELS (PSPDISP_DISPLAY_WIDTH * PSPDISP_DISPLAY_HEIGHT) // Pixels of an image, the size of the PSP screen
#define PSPDISP_DAEMON_IMAGE_UNCOMPRESSED_SIZE (PSPDISP_DAEMON_IMAGE_PIXELS * 3) // Size in bytes of an uncompressed image, the biggest an image gets

/**
 * @brief Encoder of the images, with its buffers.
 */
typedef struct {
    tjhandle compressor; // The JPEG compressor of libjpeg-turbo
    uint8_t *jpeg; // Buffer of the JPEG images
    size_t jpeg_capacity; // Size of `jpeg` in bytes, enough for any JPEG of the PSP size
} PSPdispDaemonImage;

/**
 * @brief Create an encoder of the images.
 *
 * @return The encoder, free it with `pspdisp_daemon_image_destroy()`, or `NULL` if it failed.
 */
PSPdispDaemonImage *pspdisp_daemon_image_new() {
    tjhandle compressor = tj3Init(TJINIT_COMPRESS);
    if (compressor == NULL) {
        g_warning("%s(): failed to create the JPEG compressor: %s", __FUNCTION__, tj3GetErrorStr(NULL));
        return NULL;
    }

    // Baseline JPEG with 4:2:0 subsampling, as the PSP decodes (and libjpeg makes by default)
    tj3Set(compressor, TJPARAM_SUBSAMP, TJSAMP_420);
    tj3Set(compressor, TJPARAM_NOREALLOC, 1);

    PSPdispDaemonImage *image = g_new0(PSPdispDaemonImage, 1);
    image->compressor = compressor;
    image->jpeg_capacity = tj3JPEGBufSize(PSPDISP_DISPLAY_WIDTH, PSPDISP_DISPLAY_HEIGHT, TJSAMP_420);
    image->jpeg = tj3Alloc(image->jpeg_capacity);
    return image;
}

/**
 * @brief Free an encoder of the images.
 *
 * @param image The encoder.
 */
void pspdisp_daemon_image_destroy(PSPdispDaemonImage *image) {
    tj3Free(image->jpeg);
    tj3Destroy(image->compressor);
    g_free(image);
}

/**
 * @brief Encode a frame as the image of a frame of the old protocol, as the communication settings ask.
 *
 * It's a JPEG with the quality given (from the quality level of the settings), or uncompressed when `screen_no_compression` is set (or when the JPEG would be bigger).
 * An uncompressed image is 24 bpp in B,G,R order with the rows bottom-up, as the PSP reads it.
 *
 * @param image The encoder.
 * @param frame The frame, 480x272 on `PSPDISP_PIXEL_FORMAT_8888`.
 * @param com_settings The communication settings, for the compression.
 * @param quality The JPEG quality, from 1 to 100.
 * @param output Where to write the image, at least `PSPDISP_DAEMON_IMAGE_UNCOMPRESSED_SIZE` bytes.
 * @param flags The flags of the frame header, the image flags are added to them.
 * @return The size of the image in bytes.
 */
uint32_t pspdisp_daemon_image_encode(PSPdispDaemonImage *image, const uint32_t *frame, const PSPdispComSettings *com_settings, uint32_t quality, uint8_t *output, uint32_t *flags) {
    const uint32_t width = PSPDISP_DISPLAY_WIDTH;
    const uint32_t height = PSPDISP_DISPLAY_HEIGHT;

    *flags |= PSPDISP_COM_FLAG_CONTAINS_IMAGE_DATA;

    if (!com_settings->screen_no_compression) {
        unsigned char *jpeg = image->jpeg;
        size_t size = image->jpeg_capacity;

        tj3Set(image->compressor, TJPARAM_QUALITY, CLAMP(quality, 1, 100));
        if (tj3Compress8(image->compressor, (const unsigned char *) frame, width, width * sizeof(uint32_t), height, TJPF_RGBX, &jpeg, &size) == 0 && size <= PSPDISP_DAEMON_IMAGE_UNCOMPRESSED_SIZE) {
            memcpy(output, jpeg, size);
            *flags |= PSPDISP_COM_FLAG_IMAGE_IS_JPEG;
            return size;
        }

        g_warning("%s(): failed to compress the image (%s), sending it uncompressed", __FUNCTION__, tj3GetErrorStr(image->compressor));
    }

    // B,G,R with the rows bottom-up, as a Windows bitmap
    for (uint32_t y = 0; y < height; y++) {
        const uint8_t *row = (const uint8_t *) &frame[(height - 1 - y) * width];
        uint8_t *out = &output[y * width * 3];

        for (uint32_t x = 0; x < width; x++) {
            const uint8_t *pixel = &row[x * 4]; // R,G,B,X
            uint8_t *bgr = &out[x * 3];

            bgr[0] = pixel[2];
            bgr[1] = pixel[1];
            bgr[2] = pixel[0];
        }
    }

    *flags |= PSPDISP_COM_FLAG_IMAGE_IS_UNCOMPRESSED;
    return width * height * 3;
}
