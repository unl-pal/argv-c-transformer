# SPDX-FileCopyrightText: Copyright (C) 2026 The ARG-V Project
#
# SPDX-License-Identifier: Apache-2.0

CC.Arch := 64
# Newer clang warns about -c being redundant with -fsyntax-only (SYNTAX_ONLY=1 default)
# getdelim benchmark defensively checks stream/lineptr/n for NULL; clang flags these as
# tautological because glibc's stdio.h declares getdelim with __nonnull__ attributes
CLANG_WARNINGS := -Wno-unused-command-line-argument -Wno-tautological-pointer-compare
LEVEL := ../

include $(LEVEL)/Makefile.config
