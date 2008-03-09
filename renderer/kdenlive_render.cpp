/****************************************************************************
**
** Copyright (C) 2006-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the tools applications of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include <stdio.h>
#include <QCoreApplication>
#include <QStringList>
#include <QString>

#include "renderjob.h"

int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    QStringList args = app.arguments();
    int in = -1;
    int out = -1;
    if (!args.isEmpty()) args.takeFirst();
    if (args.count() >= 4) {
        bool erase = false;
        if (args.at(0) == "-erase") {
            erase = true;
            args.takeFirst();
        }
        if (args.at(0).startsWith("in=")) {
            in = args.at(0).section('=', -1).toInt();
            args.takeFirst();
        }
        if (args.at(0).startsWith("out=")) {
            out = args.at(0).section('=', -1).toInt();
            args.takeFirst();
        }
        QString render = args.at(0);
        args.takeFirst();
        QString player = args.at(0);
        args.takeFirst();
        QString src = args.at(0);
        args.takeFirst();
        QString dest = args.at(0);
        args.takeFirst();
        RenderJob *job = new RenderJob(erase, render, player, src, dest, args, in, out);
        job->start();
        app.exec();
    } else {
        fprintf(stderr, "Kdenlive video renderer for MLT.\nUsage: "
                "kdenlive_render [-erase] [in=pos] [out=pos] [renderer] [player] [src] [dest] [[arg1] [arg2] ...]\n"
                "  -erase: if that parameter is present, src file will be erased at the end\n"
                "  in=pos: start rendering at frame pos\n"
                "  out=pos: end rendering at frame pos\n"
                "  render: path to inigo rendrer\n"
                "  player: path to video player to play when rendering is over, use '-' to disable playing\n"
                "  src: source file (usually westley playlist)\n"
                "  dest: destination file\n"
                "  args: space separated libavformat arguments\n");
    }
}

