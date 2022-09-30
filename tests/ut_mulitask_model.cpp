// Copyright (C) 2012 Martin Gräßlin <mgraesslin@kde.org>
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QObject>
#include <QtTest>
#if defined(CMAKE_SAFETYTEST_ARG_ON)
#include <sanitizer/asan_interface.h>
#endif

class ut_test : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void test();
};

void ut_test::test()
{
#if defined(CMAKE_SAFETYTEST_ARG_ON)
    __sanitizer_set_report_path("asan.log");
#endif
}

QTEST_MAIN(ut_test)
#include "ut_mulitask_model.moc"
