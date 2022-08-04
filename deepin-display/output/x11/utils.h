/*
 *  SPDX-FileCopyrightText: 2018 Daniel Vrátil <dvratil@kde.org>
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef KSCREEN_BACKEND_UTILS_H_
#define KSCREEN_BACKEND_UTILS_H_

#include "abstract_output.h"

namespace Utils
{
AbstractOutput::Type guessOutputType(const QString &type, const QString &name);

}

#endif
