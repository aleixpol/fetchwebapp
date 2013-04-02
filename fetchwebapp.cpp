#include "fetchwebapp.h"

#include <QTimer>
#include <QNetworkReply>
#include <QUrl>
#include <QSqlQuery>
#include <QDebug>

#include <qjson/parser.h>

FetchWebApp::FetchWebApp()
    : m_pending(0)
{
    QSqlDatabase db;
    db.setDatabaseName("bodega");
    db.setUserName("bodega");
    db.setPassword("bodega");
    bool ok = db.open();
    Q_ASSERT(ok);

    connect(&m_manager, SIGNAL(finished(QNetworkReply*)), SLOT(manifestoFetched(QNetworkReply*)));
}

FetchWebApp::~FetchWebApp()
{}

void FetchWebApp::fetch(const QUrl& url)
{
    m_pending++;
    m_manager.get(QNetworkRequest(url));
}

int FetchWebApp::userId()
{
    return 42;
}

void FetchWebApp::manifestoFetched(QNetworkReply* reply)
{
    m_pending--;
    QJson::Parser parser;
    QVariantMap data = parser.parse(reply->readAll()).toMap();

    QUrl url = reply->url();
    QVariantMap icons = data["icons"].toMap();
    QSqlQuery query("INSERT INTO assets (license, author, name, description, version, path, image, active, externid) "
                    "VALUES (:license, :author, :name, :description, :version, :path, :image, :active, :externid);");
    query.bindValue(":license", 0);
    query.bindValue(":author", userId());
    query.bindValue(":name", data["name"].toString());
    query.bindValue(":description", data["description"].toString());
    query.bindValue(":version", data["version"].toString());
    query.bindValue(":path", url.toString());
    if(icons.contains("128"))
        query.bindValue(":image", QUrl("http://"+url.host()+"/"+icons["128"].toString()));
    else
        query.bindValue(":image", QString());
    query.bindValue(":active", true);
    query.bindValue(":externid", reply->url().toString());
    query.exec();
    if(m_pending==0)
        emit done();
}
