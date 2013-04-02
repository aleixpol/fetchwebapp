#include <QCoreApplication>
#include <QFile>
#include <QUrl>
#include <QStringList>
#include "fetchwebapp.h"

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    Q_ASSERT(app.argc()==2);
    FetchWebApp fetcher;
    QString source = app.arguments().last();
    QObject::connect(&fetcher, SIGNAL(done()), &app, SLOT(quit()));
    if (source.endsWith("/manifest.webapp")) {
        fetcher.fetch(QUrl(source));
    } else {
        QFile f(source);
        for(;;) {
            QByteArray line = f.readLine();
            fetcher.fetch(QUrl(line));
            if(f.atEnd())
                break;
        }
    }
    return app.exec();
}
