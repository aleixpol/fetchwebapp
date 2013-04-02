#ifndef FETCHWEBAPP_H
#define FETCHWEBAPP_H

#include <QObject>
#include <QNetworkAccessManager>

class QUrl;

class FetchWebApp : public QObject
{
Q_OBJECT
public:
    FetchWebApp();
    virtual ~FetchWebApp();
    void fetch(const QUrl& url);
    int userId();

private slots:
    void manifestoFetched(QNetworkReply* reply);

signals:
    void done();

private:
    QNetworkAccessManager m_manager;
    int m_pending;
};

#endif // FETCHWEBAPP_H
