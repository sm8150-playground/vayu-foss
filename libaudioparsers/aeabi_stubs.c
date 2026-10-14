/*
 * Copyright (C) The LineageOS Project
 *
 * SPDX-License-Identifier: Apache-2.0
 */



__asm__(
    ".arm\n"
    ".balign 4\n"

    ".global __aeabi_memcpy\n"
    ".type __aeabi_memcpy, %function\n"
    ".global __aeabi_memcpy4\n"
    ".type __aeabi_memcpy4, %function\n"
    ".global __aeabi_memcpy8\n"
    ".type __aeabi_memcpy8, %function\n"
    "__aeabi_memcpy:\n"
    "__aeabi_memcpy4:\n"
    "__aeabi_memcpy8:\n"
    "  b memcpy\n"

    ".balign 4\n"
    ".global __aeabi_memset\n"
    ".type __aeabi_memset, %function\n"
    ".global __aeabi_memset4\n"
    ".type __aeabi_memset4, %function\n"
    ".global __aeabi_memset8\n"
    ".type __aeabi_memset8, %function\n"
    "__aeabi_memset:\n"
    "__aeabi_memset4:\n"
    "__aeabi_memset8:\n"
    "  mov r3, r1\n"
    "  mov r1, r2\n"
    "  mov r2, r3\n"
    "  b memset\n"

    ".balign 4\n"
    ".global __aeabi_memclr\n"
    ".type __aeabi_memclr, %function\n"
    ".global __aeabi_memclr4\n"
    ".type __aeabi_memclr4, %function\n"
    ".global __aeabi_memclr8\n"
    ".type __aeabi_memclr8, %function\n"
    "__aeabi_memclr:\n"
    "__aeabi_memclr4:\n"
    "__aeabi_memclr8:\n"
    "  mov r2, r1\n"
    "  movs r1, #0\n"
    "  b memset\n"

    ".thumb\n");
