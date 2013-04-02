#include "fetchwebapp.h"

#include <QTimer>
#include <QNetworkReply>
#include <QUrl>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

#include <qjson/parser.h>

FetchWebApp::FetchWebApp()
    : m_pending(0)
    , m_channelId(-1)
{
    QSqlDatabase db(QSqlDatabase::addDatabase("QPSQL"));
    db.setDatabaseName("bodega");
    db.setUserName("bodega");
    db.setPassword("bodega");
    bool ok = db.open();
    Q_ASSERT(ok);
    
    QString ourName = "Web Apps";
    QSqlQuery queryFindTagId("SELECT id FROM channels WHERE name=:name;");
    queryFindTagId.bindValue(":name", ourName);
    ok = queryFindTagId.exec();
    if(!ok) qDebug() << "error" << queryFindTagId.lastError();
    Q_ASSERT(ok);

    if(!queryFindTagId.first()) {
        QSqlQuery queryTag( "INSERT INTO channels (partner, active, name, description) "
                            "VALUES (:partner, :active, :name, :description) "
                            "RETURNING id;");
        queryTag.bindValue(":partner", 1); //1 is KDE
        queryTag.bindValue(":active", true);
        queryTag.bindValue(":name", ourName);
        queryTag.bindValue(":description", QString());
        ok = queryTag.exec() && queryTag.first();
        Q_ASSERT(ok);
        m_channelId = queryTag.value(0).toInt();
    } else
        m_channelId = queryFindTagId.value(0).toInt();

    Q_ASSERT(m_channelId>=0);
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
    QSqlQuery assetExists("SELECT id FROM assets WHERE name=:name");
    assetExists.bindValue(":name", data["name"]);
    bool ok = assetExists.exec();
    Q_ASSERT(ok);

    QSqlQuery query;
    bool alreadyPresent = assetExists.first();
    if(alreadyPresent) {
        query.prepare("UPDATE assets "
                  "SET name = :name, license = :license, author = :author,"
                  "version = :version, externid = :externid, image = :image, description = :description,  "
                  "WHERE id = :id "
                  "RETURNING id;");
        query.bindValue(":id", assetExists.value(0));
    } else {
        query.prepare("INSERT INTO assets (license, author, name, description, version, path, image, active, externid) "
                      "VALUES (:license, :author, :name, :description, :version, :path, :image, :active, :externid) RETURNING id;");
    }

    QVariantMap icons = data["icons"].toMap();
    query.bindValue(":license", 4);
    query.bindValue(":author", 0);
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
    ok = query.exec();
    Q_ASSERT(ok);
    
    int assetId = query.value(0).toInt();
    if(!alreadyPresent) {
        QSqlQuery addQuery("INSERT INTO channelAssets (channel, asset) VALUES (:channelId, :assetId)");
        addQuery.bindValue(":channelId", m_channelId);
        addQuery.bindValue(":assetId", assetId);
        ok = addQuery.exec();
        Q_ASSERT(ok);
    }
    
    m_entered.insert(assetId);
    if(m_pending==0)
        cleanup();
}

void FetchWebApp::cleanup()
{
    QSqlQuery removeOldIds("SELECT asset FROM channelAssets WHERE channel=:channelId;");
    removeOldIds.bindValue(":channelId", m_channelId);
    bool ok = removeOldIds.exec();
    Q_ASSERT(ok);
    
    while(removeOldIds.next()) {
        int id = removeOldIds.value(0).toInt();
        if(m_entered.contains(id)) {
            QSqlQuery removeQuery("DELETE * FROM channelAssets WHERE asset=:id;");
            ok = removeQuery.exec();
            Q_ASSERT(ok);

            QSqlQuery removeAssetQuery("DELETE * FROM assets WHERE id=:id;");
            ok = removeAssetQuery.exec();
            Q_ASSERT(ok);
        }
    }
    
    emit done();
}
