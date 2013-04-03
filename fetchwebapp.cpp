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
    , m_tagId(-1)
{
    QSqlDatabase db(QSqlDatabase::addDatabase("QPSQL"));
    db.setDatabaseName("bodega");
    db.setUserName("bodega");
    db.setPassword("bodega");
    bool ok = db.open();
    Q_ASSERT(ok);
    
    QSqlQuery("UPDATE batchJobsInProgress SET doWork = true WHERE job = 'webapps'");
    
    QString ourName = "Web Apps";
    QSqlQuery queryFindTagId;
    queryFindTagId.prepare("SELECT id FROM tags WHERE title=:title;");
    queryFindTagId.bindValue(":title", ourName);
    ok = queryFindTagId.exec();
    Q_ASSERT(ok);

    if(!queryFindTagId.first()) {
        QSqlQuery queryTag;
        queryTag.prepare(   "INSERT INTO channels (partner, active, name, description) "
                            "VALUES (:partner, :active, :name, :description) "
                            "RETURNING id;");
        queryTag.bindValue(":partner", 1); //1 is KDE
        queryTag.bindValue(":active", true);
        queryTag.bindValue(":name", ourName);
        queryTag.bindValue(":description", ourName);
        ok = queryTag.exec() && queryTag.first();
        Q_ASSERT(ok);
        int channelId = queryTag.value(0).toInt();
        
        QSqlQuery channelToDevice;
        channelToDevice.prepare("INSERT INTO deviceChannels (device, channel) VALUES ('VIVALDI-1', :channelId)");
        channelToDevice.bindValue(":channelId", channelId);
        ok = channelToDevice.exec();
        Q_ASSERT(ok);

        // 1->KDE, 4->category
        QSqlQuery addTag;
        addTag.prepare("INSERT INTO tags (partner, type, title) VALUES (1, 4, :title) RETURNING id;");
        addTag.bindValue(":title", ourName);
        ok = addTag.exec() && addTag.first();
        Q_ASSERT(ok);
        m_tagId = addTag.value(0).toInt();

        QSqlQuery addTagToChannel;
        addTagToChannel.prepare("INSERT INTO channelTags (channel, tag) VALUES (:channelId, :tagId)");
        addTagToChannel.bindValue(":channelId", channelId);
        addTagToChannel.bindValue(":tagId", m_tagId);
        ok = addTagToChannel.exec();
        Q_ASSERT(ok);
    } else
        m_tagId = queryFindTagId.value(0).toInt();

    Q_ASSERT(m_tagId>0);
    connect(&m_manager, SIGNAL(finished(QNetworkReply*)), SLOT(manifestoFetched(QNetworkReply*)));
}

FetchWebApp::~FetchWebApp()
{}

void FetchWebApp::fetch(const QUrl& url)
{
    m_pending++;
    m_manager.get(QNetworkRequest(url));
}

void FetchWebApp::manifestoFetched(QNetworkReply* reply)
{
    m_pending--;
    QJson::Parser parser;
    QVariantMap data = parser.parse(reply->readAll()).toMap();

    QUrl url = reply->url();
    QSqlQuery assetExists;
    assetExists.prepare("SELECT id FROM assets WHERE name=:name;");
    assetExists.bindValue(":name", data["name"]);
    bool ok = assetExists.exec();
    Q_ASSERT(ok);

    QSqlQuery query;
    bool alreadyPresent = assetExists.first();
    int assetId = -1;
    if(alreadyPresent) {
        query.prepare("UPDATE assets "
                  "SET name = :name, license = :license, author = :author, path = :path, active = :active, "
                  "version = :version, externid = :externid, image = :image, description = :description "
                  "WHERE id = :id;");
        assetId = assetExists.value(0).toInt();
        query.bindValue(":id", assetExists.value(0));
    } else {
        query.prepare("INSERT INTO assets (license, author, name, description, version, path, image, active, externid) "
                      "VALUES (:license, :author, :name, :description, :version, :path, :image, :active, :externid) RETURNING id;");
    }

    QVariantMap icons = data["icons"].toMap();
    query.bindValue(":license", 4);
    query.bindValue(":author", 0);
    query.bindValue(":name", data["name"]);
    query.bindValue(":description", data["description"]);
    query.bindValue(":version", data["version"]);
    query.bindValue(":path", url.toString());
    if(icons.contains("128"))
        query.bindValue(":image", QString("http://"+url.host()+"/"+icons["128"].toString()));
    else
        query.bindValue(":image", QString());
    query.bindValue(":active", true);
    query.bindValue(":externid", reply->url().toString());
    ok = query.exec();
    Q_ASSERT(ok);
    if(!alreadyPresent) {
        ok = query.first();
        Q_ASSERT(ok);
        assetId = query.value(0).toInt();
        
        QSqlQuery addQuery;
        addQuery.prepare("INSERT INTO assetTags (asset, tag) VALUES (:assetId, :tagId);");
        addQuery.bindValue(":assetId", assetId);
        addQuery.bindValue(":tagId", m_tagId);
        ok = addQuery.exec();
        Q_ASSERT(ok);
    }
    qDebug() << (alreadyPresent ? "updated" : "added") << assetId;
    
    m_entered.insert(assetId);
    if(m_pending==0)
        cleanup();
}

void FetchWebApp::cleanup()
{
    QSqlQuery removeOldIds;
    removeOldIds.prepare("SELECT asset FROM assetTags WHERE tag=:tagId;");
    removeOldIds.bindValue(":tagId", m_tagId);
    bool ok = removeOldIds.exec();
    Q_ASSERT(ok);
    
    while(removeOldIds.next()) {
        int id = removeOldIds.value(0).toInt();
        if(!m_entered.contains(id)) {
            qDebug() << "removing..." << id;
            QSqlQuery removeQuery;
            removeQuery.prepare("DELETE * FROM channelAssets WHERE asset=:id;");
            ok = removeQuery.exec();
            Q_ASSERT(ok);

            QSqlQuery removeAssetQuery;
            removeAssetQuery.prepare("DELETE * FROM assets WHERE id=:id;");
            ok = removeAssetQuery.exec();
            Q_ASSERT(ok);
        }
    }
    
    QSqlQuery("update batchJobsInProgress set doWork = false where job = 'webapps'");
    emit done();
}
