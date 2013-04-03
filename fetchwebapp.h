#ifndef FETCHWEBAPP_H
#define FETCHWEBAPP_H

#include <QObject>
#include <QSet>
#include <QNetworkAccessManager>

class QUrl;

class FetchWebApp : public QObject
{
Q_OBJECT
public:
    FetchWebApp();
    virtual ~FetchWebApp();
    void fetch(const QUrl& url);

private slots:
    void manifestoFetched(QNetworkReply* reply);

signals:
    void done();

private:
    void cleanup();

    QNetworkAccessManager m_manager;
    int m_pending;
    QSet<int> m_entered;
    int m_tagId;
};

#endif // FETCHWEBAPP_H
