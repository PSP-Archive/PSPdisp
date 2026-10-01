/*****************************************************
 * SPDX-License-Identifier: BSD-3-Clause             *
 *                                                   *
 * edid.c - Display identification (EDID) functions  *
 *                                                   *
 * Copyright © 2026 Net-0                            *
 *****************************************************/

/*
 * EDID (Extended Display Identification Data) is an existing standard by VESA, not something made for this project.
 *
 * Every monitor gives one to the computer, to describe itself: its manufacturer, model, physical size, colours and supported modes.
 *
 * A real monitor stores it in a chip and sends it through the cable, while a virtual display hands it to its driver (e.g. EVDI) when connecting.
 *
 * This file builds the EDID of the PSP display, from the model, region and serial number of the PSP (see `edid.h` for its types).
 */

#pragma once

#include <stdint.h>
#include <string.h>
#include <glib.h>
#include "../../shared.h"
#include "./edid.h"

/**
 * @brief Create an EDID display descriptor that holds a text (e.g. the product name).
 *
 * The text is ASCII, cut at 13 characters, and when shorter it's terminated by `0x0A` (`'\\n'`; line-break) and padded with `0x20` (`' '`; space).
 *
 * @param tag The kind of text, `PSPDISP_EDID_TAG_SERIAL_NUMBER`, `PSPDISP_EDID_TAG_ALPHANUMERIC_DATA` or `PSPDISP_EDID_TAG_PRODUCT_NAME`.
 * @param text The text of the descriptor.
 * @return The display descriptor.
 */
PSPdispEDIDDescriptor pspdisp_display_edid_text_descriptor(PSPdispEDIDDescriptorTag tag, const char *text) {
    PSPdispEDIDDisplayDescriptor descriptor = { .tag = tag };
    const uint8_t size = sizeof(descriptor.text);

    uint8_t length = 0;
    for (; length < size && text[length] != '\0'; length++); // Just count up to the max allowed size

    // Copy the text up to the max allowed size
    memcpy(descriptor.text, text, length);

    // Put the termination '\n' and maybe the padding ' '
    if (length < size) {
        descriptor.text[length] = '\n'; // Termination
        memset(descriptor.text + length + 1, ' ', size - length - 1); // Fill the empty positions with the padding
    }

    return (PSPdispEDIDDescriptor) { .display = descriptor };
}

/**
 * @brief Get the model number of a PSP, as printed on its label (e.g. "PSP-3004"), to name its display.
 *
 * The model number is the series of the generation (e.g. PSP-3000 for the 03g, 04g, 07g and 09g boards) with the region in its last digits.
 *
 * Names follow that pattern even for combinations that were never sold (e.g. "PSP-E1001", the PSP-E1000 was only sold in Europe).
 *
 * Development units get the model number of the first kit of their kind (e.g. "DTP-T1000" for a DevKit), whatever the generation.
 *
 * Never released generations get "?" as their series (e.g. "PSP-?001"), and unknown regions get the generation instead (e.g. "PSP-01g?").
 *
 * @param model The PSP generation.
 * @param region The PSP sales region.
 * @return The model number, a static string short enough for an EDID text descriptor (up to 13 characters).
 */
const char *pspdisp_display_edid_name(PSPdispModel model, PSPdispRegion region) {
    switch (region) {
        case PSPDISP_REGION_JAPAN:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1000";
                case PSPDISP_MODEL_02G: return "PSP-2000";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3000";
                case PSPDISP_MODEL_05G: return "PSP-N1000";
                case PSPDISP_MODEL_11G: return "PSP-E1000";
                default: return "PSP-?000"; // Never released
            }
        case PSPDISP_REGION_NORTH_AMERICA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1001";
                case PSPDISP_MODEL_02G: return "PSP-2001";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3001";
                case PSPDISP_MODEL_05G: return "PSP-N1001";
                case PSPDISP_MODEL_11G: return "PSP-E1001";
                default: return "PSP-?001"; // Never released
            }
        case PSPDISP_REGION_AUSTRALIA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1002";
                case PSPDISP_MODEL_02G: return "PSP-2002";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3002";
                case PSPDISP_MODEL_05G: return "PSP-N1002";
                case PSPDISP_MODEL_11G: return "PSP-E1002";
                default: return "PSP-?002"; // Never released
            }
        case PSPDISP_REGION_UK:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1003";
                case PSPDISP_MODEL_02G: return "PSP-2003";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3003";
                case PSPDISP_MODEL_05G: return "PSP-N1003";
                case PSPDISP_MODEL_11G: return "PSP-E1003";
                default: return "PSP-?003"; // Never released
            }
        case PSPDISP_REGION_EUROPE:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1004";
                case PSPDISP_MODEL_02G: return "PSP-2004";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3004";
                case PSPDISP_MODEL_05G: return "PSP-N1004";
                case PSPDISP_MODEL_11G: return "PSP-E1004";
                default: return "PSP-?004"; // Never released
            }
        case PSPDISP_REGION_KOREA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1005";
                case PSPDISP_MODEL_02G: return "PSP-2005";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3005";
                case PSPDISP_MODEL_05G: return "PSP-N1005";
                case PSPDISP_MODEL_11G: return "PSP-E1005";
                default: return "PSP-?005"; // Never released
            }
        case PSPDISP_REGION_HONG_KONG:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1006";
                case PSPDISP_MODEL_02G: return "PSP-2006";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3006";
                case PSPDISP_MODEL_05G: return "PSP-N1006";
                case PSPDISP_MODEL_11G: return "PSP-E1006";
                default: return "PSP-?006"; // Never released
            }
        case PSPDISP_REGION_TAIWAN:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1007";
                case PSPDISP_MODEL_02G: return "PSP-2007";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3007";
                case PSPDISP_MODEL_05G: return "PSP-N1007";
                case PSPDISP_MODEL_11G: return "PSP-E1007";
                default: return "PSP-?007"; // Never released
            }
        case PSPDISP_REGION_RUSSIA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1008";
                case PSPDISP_MODEL_02G: return "PSP-2008";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3008";
                case PSPDISP_MODEL_05G: return "PSP-N1008";
                case PSPDISP_MODEL_11G: return "PSP-E1008";
                default: return "PSP-?008"; // Never released
            }
        case PSPDISP_REGION_CHINA:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1009";
                case PSPDISP_MODEL_02G: return "PSP-2009";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3009";
                case PSPDISP_MODEL_05G: return "PSP-N1009";
                case PSPDISP_MODEL_11G: return "PSP-E1009";
                default: return "PSP-?009"; // Never released
            }
        case PSPDISP_REGION_MEXICO:
            switch (model) {
                case PSPDISP_MODEL_01G: return "PSP-1010";
                case PSPDISP_MODEL_02G: return "PSP-2010";
                case PSPDISP_MODEL_03G:
                case PSPDISP_MODEL_04G:
                case PSPDISP_MODEL_07G:
                case PSPDISP_MODEL_09G: return "PSP-3010";
                case PSPDISP_MODEL_05G: return "PSP-N1010";
                case PSPDISP_MODEL_11G: return "PSP-E1010";
                default: return "PSP-?010"; // Never released
            }
        case PSPDISP_REGION_TEST_UNIT:  return "PSP-0000";
        case PSPDISP_REGION_DEVKIT:     return "DTP-T1000";
        case PSPDISP_REGION_TESTKIT:    return "DTP-H1500";
        default: break;
    }

    // Unknown region
    switch (model) {
        case PSPDISP_MODEL_01G: return "PSP-01g?";
        case PSPDISP_MODEL_02G: return "PSP-02g?";
        case PSPDISP_MODEL_03G: return "PSP-03g?";
        case PSPDISP_MODEL_04G: return "PSP-04g?";
        case PSPDISP_MODEL_05G: return "PSP-05g?";
        case PSPDISP_MODEL_06G: return "PSP-06g?";
        case PSPDISP_MODEL_07G: return "PSP-07g?";
        case PSPDISP_MODEL_08G: return "PSP-08g?";
        case PSPDISP_MODEL_09G: return "PSP-09g?";
        case PSPDISP_MODEL_10G: return "PSP-10g?";
        case PSPDISP_MODEL_11G: return "PSP-11g?";
    }
    return "PSP-?"; // Unknown model
}

/**
 * @brief Get the release date of a PSP model in a region, as an EDID manufacture date (ISO 8601 week and year).
 *
 * The PSP-3000 board revisions (04g, 07g and 09g) get the PSP-3000 dates, and the UK gets the dates of the rest of Europe, where it launched at the same time.
 *
 * When the date in the region is not known (e.g. a development unit), the week is unspecified (0) and the year is the first launch of the model (e.g. 2004 for the PSP-1000).
 *
 * @param model The PSP generation.
 * @param region The PSP sales region.
 * @return The release date, as the week and year of manufacture.
 */
PSPdispEDIDManufactureDate pspdisp_display_edid_release_date(PSPdispModel model, PSPdispRegion region) {
    switch (model) {
        case PSPDISP_MODEL_01G: // PSP-1000
            switch (region) {
                case PSPDISP_REGION_JAPAN:         return (PSPdispEDIDManufactureDate) { .week = 50, .year = 2004 - 1990 }; // 2004-12-12
                case PSPDISP_REGION_NORTH_AMERICA: return (PSPdispEDIDManufactureDate) { .week = 12, .year = 2005 - 1990 }; // 2005-03-24
                case PSPDISP_REGION_KOREA:         return (PSPdispEDIDManufactureDate) { .week = 18, .year = 2005 - 1990 }; // 2005-05-02
                case PSPDISP_REGION_TAIWAN:        return (PSPdispEDIDManufactureDate) { .week = 19, .year = 2005 - 1990 }; // 2005-05-12
                case PSPDISP_REGION_HONG_KONG:     return (PSPdispEDIDManufactureDate) { .week = 19, .year = 2005 - 1990 }; // 2005-05-12 in Singapore, Hong Kong was the same month
                case PSPDISP_REGION_EUROPE:
                case PSPDISP_REGION_UK:
                case PSPDISP_REGION_AUSTRALIA:     return (PSPdispEDIDManufactureDate) { .week = 35, .year = 2005 - 1990 }; // 2005-09-01
                default:                           return (PSPdispEDIDManufactureDate) { .week =  0, .year = 2004 - 1990 }; // Unknown day, the year of the first launch
            }
        case PSPDISP_MODEL_02G: // PSP-2000
            switch (region) {
                case PSPDISP_REGION_HONG_KONG:     return (PSPdispEDIDManufactureDate) { .week = 35, .year = 2007 - 1990 }; // 2007-08-30
                case PSPDISP_REGION_EUROPE:
                case PSPDISP_REGION_UK:            return (PSPdispEDIDManufactureDate) { .week = 36, .year = 2007 - 1990 }; // 2007-09-05
                case PSPDISP_REGION_NORTH_AMERICA: return (PSPdispEDIDManufactureDate) { .week = 36, .year = 2007 - 1990 }; // 2007-09-06
                case PSPDISP_REGION_KOREA:         return (PSPdispEDIDManufactureDate) { .week = 36, .year = 2007 - 1990 }; // 2007-09-07
                case PSPDISP_REGION_AUSTRALIA:     return (PSPdispEDIDManufactureDate) { .week = 37, .year = 2007 - 1990 }; // 2007-09-12
                case PSPDISP_REGION_JAPAN:         return (PSPdispEDIDManufactureDate) { .week = 38, .year = 2007 - 1990 }; // 2007-09-20
                default:                           return (PSPdispEDIDManufactureDate) { .week =  0, .year = 2007 - 1990 }; // Unknown day, the year of the first launch
            }
        case PSPDISP_MODEL_03G:
        case PSPDISP_MODEL_04G:
        case PSPDISP_MODEL_07G:
        case PSPDISP_MODEL_09G: // PSP-3000
            switch (region) {
                case PSPDISP_REGION_NORTH_AMERICA: return (PSPdispEDIDManufactureDate) { .week = 42, .year = 2008 - 1990 }; // 2008-10-14
                case PSPDISP_REGION_JAPAN:         return (PSPdispEDIDManufactureDate) { .week = 42, .year = 2008 - 1990 }; // 2008-10-16
                case PSPDISP_REGION_EUROPE:
                case PSPDISP_REGION_UK:            return (PSPdispEDIDManufactureDate) { .week = 42, .year = 2008 - 1990 }; // 2008-10-17
                case PSPDISP_REGION_AUSTRALIA:     return (PSPdispEDIDManufactureDate) { .week = 43, .year = 2008 - 1990 }; // 2008-10-23
                default:                           return (PSPdispEDIDManufactureDate) { .week =  0, .year = 2008 - 1990 }; // Unknown day, the year of the first launch
            }
        case PSPDISP_MODEL_05G: // PSP Go
            switch (region) {
                case PSPDISP_REGION_NORTH_AMERICA:
                case PSPDISP_REGION_EUROPE:
                case PSPDISP_REGION_UK:            return (PSPdispEDIDManufactureDate) { .week = 40, .year = 2009 - 1990 }; // 2009-10-01
                case PSPDISP_REGION_JAPAN:         return (PSPdispEDIDManufactureDate) { .week = 44, .year = 2009 - 1990 }; // 2009-11-01
                default:                           return (PSPdispEDIDManufactureDate) { .week =  0, .year = 2009 - 1990 }; // Unknown day, the year of the first launch
            }
        case PSPDISP_MODEL_11G: // PSP-E1000
            switch (region) {
                case PSPDISP_REGION_EUROPE:
                case PSPDISP_REGION_UK:            return (PSPdispEDIDManufactureDate) { .week = 43, .year = 2011 - 1990 }; // 2011-10-26
                default:                           return (PSPdispEDIDManufactureDate) { .week =  0, .year = 2011 - 1990 }; // Unknown day, the year of the first launch
            }
        default: return (PSPdispEDIDManufactureDate) { .week =  0, .year = 2004 - 1990 }; // Never released, the year of the first PSP
    }
}

/**
 * @brief Create the EDID of a PSP, the identification its virtual display gives to the computer.
 *
 * It's an EDID 1.4 base block (no extension blocks) of a digital display with only the PSP native mode: 480x272 at 59.94 Hz (9 MHz pixel clock), sRGB colours and DPMS support.
 *
 * The model gives the screen size (3.8" on the PSP Go, 4.3" on the others), and with the region the product name (e.g. "PSP-3004").
 *
 * The model and the region also give the manufacture date, as the release date (see `pspdisp_display_edid_release_date()`).
 *
 * @param model The PSP generation.
 * @param region The PSP sales region.
 * @param serial_number The serial number of the display, not 0: without a serial, a DE may prefix the product name with the connector name (e.g. "DVI-I-1-PSP-3004").
 * @return The EDID, with its checksum set.
 */
PSPdispEDID pspdisp_display_edid_new(PSPdispModel model, PSPdispRegion region, uint32_t serial_number) {
    // most PSP screen dimensions -> width = 9.50cm, height = 5.38cm
    // PSP GO screen dimensions -> width = 8.40cm, height = 4.76cm
    uint16_t width_mm = model != PSPDISP_MODEL_05G ? 95 : 84;
    uint16_t height_mm = model != PSPDISP_MODEL_05G ? 54 : 47;

    PSPdispEDID edid = {
        .header = PSPDISP_EDID_HEADER,
        .manufacturer_id = PSPDISP_PNP_ID('L', 'N', 'X'), // The Linux Foundation (LNX), not the best choice but I don't have a PNP ID, and if I used Sony's (SNY) I might get some legal issue, and it's *most* of the time just a identification info
        .product_code = 0x5350, // "PS", arbitrary
        .serial_number = serial_number,
        .manufacture_date = pspdisp_display_edid_release_date(model, region),
        .version = { .major = 1, .minor = 4 }, // EDID v1.4
        .video_input = { .digital = { .is_digital = true, .color_depth = PSPDISP_EDID_COLOR_DEPTH_8, .interface_type = PSPDISP_EDID_INTERFACE_UNDEFINED } }, // No real cable
        .screen_size = { .horizontal = (width_mm + 5) / 10, .vertical = (height_mm + 5) / 10 },
        .gamma = 120, // 2.2, the sRGB gamma
        .features = { .dpms_active_off = true, .color_type = PSPDISP_EDID_COLOR_TYPE_RGB444, .srgb_default = true, .preferred_timing_native = true },
        .chromaticity = PSPDISP_EDID_CHROMATICITY(0.640, 0.330, 0.300, 0.600, 0.150, 0.060, 0.3127, 0.3290), // sRGB primaries, D65 white point
        .established_timings = {}, // No VGA-era mode fits the PSP
        .descriptors = {
            // 480x272 at 59.94 Hz: 525 x 286 pixels in total at 9 MHz, as the PSP LCD
            { .detailed_timing = PSPDISP_EDID_DETAILED_TIMING(9000, PSPDISP_DISPLAY_WIDTH, 45, PSPDISP_DISPLAY_HEIGHT, 14, 2, 41, 2, 10, width_mm, height_mm, .sync_type = PSPDISP_EDID_SYNC_TYPE_DIGITAL_SEPARATE) },
            { .display = {
                .tag = PSPDISP_EDID_TAG_RANGE_LIMITS,
                .range_limits = {
                    .vertical_rate_min = 59,
                    .vertical_rate_max = 61,
                    .horizontal_rate_min = 17, // 17.14 kHz
                    .horizontal_rate_max = 18,
                    .pixel_clock_max = 1, // 10 MHz
                    .timing_support = PSPDISP_EDID_TIMING_SUPPORT_RANGE_LIMITS_ONLY,
                    .timing_data = { '\n', ' ', ' ', ' ', ' ', ' ', ' ' },
                },
            } },
            pspdisp_display_edid_text_descriptor(PSPDISP_EDID_TAG_PRODUCT_NAME, pspdisp_display_edid_name(model, region)),
            pspdisp_display_edid_text_descriptor(PSPDISP_EDID_TAG_ALPHANUMERIC_DATA, "PSPdisp"), // Just some extra descriptor info ;V
        },
        .extension_count = 0,
    };

    // 480x272 has no standard aspect ratio, so all the standard timings are unused
    for (size_t i = 0; i < G_N_ELEMENTS(edid.standard_timings); i++)
        edid.standard_timings[i] = PSPDISP_EDID_STANDARD_TIMING_UNUSED;

    // All the bytes must sum to 0 (mod 256)
    const uint8_t *bytes = (const uint8_t *) &edid;
    uint8_t sum = 0;
    for (size_t i = 0; i < sizeof(edid) - 1; i++)
        sum += bytes[i];
    edid.checksum = (uint8_t) (0x100 - sum);

    return edid;
}
