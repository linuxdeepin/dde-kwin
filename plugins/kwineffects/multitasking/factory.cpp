/*
 * Copyright (C) 2019 Deepin Technology Co., Ltd.
 *
 * Author:     Sian Cao <yinshuiboy@gmail.com>
 *
 * Maintainer: Sian Cao <yinshuiboy@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "../../../accessible.h"
#include "multitasking.h"

//KWIN_EFFECT_FACTORY(MultitaskingPluginFactory, MultitaskingEffect, "multitasking.json")
//临时函数，用来输出窗口缩略图的位置，提供给自动化测试项，桌面缩略图重写以后会被删除
void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    static QMutex mutex;

    if(type == QtInfoMsg) {
        QString context_info = QString("File:(%1) Line:(%2)").arg(QString(context.file)).arg(context.line);
        QString current_date_time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss ddd");
        QString current_date = QString("(%1)").arg(current_date_time);
        QString message = QString("%1 %2 %3 %4").arg("Info:").arg(context_info).arg(msg).arg(current_date);
        QString path1 = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/winThumb-log.txt";
        mutex.lock();
        QFile file(path1);
        file.open(QIODevice::WriteOnly | QIODevice::Append);
        QTextStream text_stream(&file);
        text_stream << message << "\r\n";
        file.flush();
        file.close();
        mutex.unlock();
    }
}

class MultitaskingPluginFactory : public KWin::EffectPluginFactory
{
    Q_OBJECT
    Q_INTERFACES(KPluginFactory)
    Q_PLUGIN_METADATA(IID KPluginFactory_iid FILE "multitasking.json")

public:
    explicit MultitaskingPluginFactory() {}
    ~MultitaskingPluginFactory() {}

    KWin::Effect *createEffect() const override {
        qInstallMessageHandler(outputMessage);
        KWin::Effect *e = new  MultitaskingEffect();
        QAccessible::installFactory(accessibleFactory);
        return e;
    }
};

K_EXPORT_PLUGIN_VERSION(KWIN_EFFECT_API_VERSION)

#include "factory.moc"
