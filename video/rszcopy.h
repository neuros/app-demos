/**
 * @file    rszcopy.h
 * @brief   
 * @version 00.11
 *
 * This module uses the H/W resizer on the DM6446 to copy a frame from
 * a source to a destination buffer.
 *
 * @verbatim
 * ============================================================================
 * Copyright (c) Texas Instruments Inc 2007
 *
 * Use of this software is controlled by the terms and conditions found in the
 * license agreement under which this software has been supplied or provided.
 * ============================================================================
 * @endverbatim
 */

#ifndef _RSZCOPY_H
#define _RSZCOPY_H

#define RSZCOPY_SUCCESS 1             //!< Indicates success of an API call.
#define RSZCOPY_FAILURE 0             //!< Indicates failure of an API call.

#define RSZCOPY_DEFAULTRSZRATE 0xe    //!< Default rate of the resizer.

/** @brief Internal structure containing the state of a resize job. */
typedef struct Rszcopy_Object {
    int rszFd;
    size_t inSize;
    size_t outSize;
} Rszcopy_Object;

/** @brief A handle representing a resize job. */
typedef Rszcopy_Object *Rszcopy_Handle;

#if defined (__cplusplus)
extern "C" {
#endif

/**
 * @brief Creates a new copy job.
 * @param rszRate The rate to use for the resizer peripheral. This will
 *                set the SDR_REQ_EXP register in the VPSS for *all* resizer
 *                operations. A negative value will bypass changing this
 *                register.
 * @return A handle to the Rszcopy job to pass to subsequent operations.
 */
extern Rszcopy_Handle Rszcopy_create(int rszRate);

/**
 * @brief Configures a copy job.
 * @param hRszcopy A handle to a previously created rszcopy job.
 * @param width The width of the image to copy in pixels.
 * @param height The height of the image to copy in pixels.
 * @param srcPitch The horizontal pitch of the input image in bytes.
 * @param dstPitch The horizontal pitch of the output image in bytes.
 * @return RSZCOPY_SUCCESS on success and RSZCOPY_FAILURE on failure.
 */
extern int Rszcopy_config(Rszcopy_Handle hRszcopy,
                          int width, int height, int srcPitch, int dstPitch);

/**
 * @brief Executes a copy job.
 * @param hRszcopy The handle to a created and configured copy job to execute
 * @param srcBuf A physical pointer to the source buffer. This buffer
 *        should be of the size srcPitch * srcHeight bytes set during the
 *        config call.
 * @param dstBuf A physical pointer to the destination buffer. This
 *        buffer should be of the size dstPitch * dstHeight bytes set during
 *        the config call.
 * @return RSZCOPY_SUCCESS on success and RSZCOPY_FAILURE on failure.
 */
extern int Rszcopy_execute(Rszcopy_Handle hRszcopy,
                          unsigned long srcBuf,
                          unsigned long dstBuf);

/**
 * @brief Deletes a resize job.
 * @param hRszcopy The handle to a previously created resize job to be deleted.
 */
void Rszcopy_delete(Rszcopy_Handle hRszcopy);

#if defined (__cplusplus)
}
#endif

#endif // _RSZCOPY_H
