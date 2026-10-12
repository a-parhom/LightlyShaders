/*
    SPDX-FileCopyrightText: 2021 Vlad Zahorodnii <vlad.zahorodnii@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "blur.h"

namespace KWin
{

// Disabled by default through metadata.json, it replaces the stock blur, which the user has to switch off
KWIN_EFFECT_FACTORY_SUPPORTED(BlurEffect, "metadata.json", return BlurEffect::supported();)

} // namespace KWin

#include "main.moc"
