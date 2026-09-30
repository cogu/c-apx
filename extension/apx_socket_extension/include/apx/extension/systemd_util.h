/*****************************************************************************
* \file      systemd_util.h
* \author    Conny Gustafsson
* \date      2026-09-30
* \brief     Systemd socket activation utility functions
*
* Copyright (c) 2026 Conny Gustafsson
* SPDX-License-Identifier: MIT
* See LICENSE in project root for full license terms.
******************************************************************************/
#ifndef APX_SYSTEMD_UTIL_H
#define APX_SYSTEMD_UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#define SD_LISTEN_FDS_START 3

/**
 * Checks for file descriptors passed by systemd socket activation.
 *
 * Inspects LISTEN_PID and LISTEN_FDS environment variables without requiring libsystemd.
 *
 * @param unset_environment If non-zero, unsets LISTEN_PID and LISTEN_FDS upon reading.
 * @return Number of inherited file descriptors starting at SD_LISTEN_FDS_START (3), or 0 if none.
 */
int apx_sd_listen_fds(int unset_environment);

#ifdef __cplusplus
}
#endif

#endif // APX_SYSTEMD_UTIL_H
