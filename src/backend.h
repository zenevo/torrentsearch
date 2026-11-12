#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QNetworkAccessManager>

class Backend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
public:
    explicit Backend(QObject *parent = nullptr);

    bool busy() const { return m_busy; }
    QString error() const { return m_error; }

    Q_INVOKABLE void search(const QString &query, const QString &provider);
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE QStringList providerList() const;
    Q_INVOKABLE bool supportsPaging(const QString &provider) const;
    Q_INVOKABLE QString magnetFor(const QString &infoHash, const QString &name) const;
    Q_INVOKABLE void downloadTorrent(const QString &infoHash, const QString &name, const QString &torrentUrl);

signals:
    void busyChanged();
    void errorChanged();
    void results(const QVariantList &items, bool append);
    void torrentSaved(const QString &path);
    void torrentFailed(const QString &reason);

private:
    void setBusy(bool b);
    void setError(const QString &e);
    void handleApibay(const QByteArray &data);
    void handleTorrentsCsv(const QByteArray &data);
    void handleKnaben(const QByteArray &data);
    void handleRutor(const QByteArray &data);
    void handleNyaa(const QByteArray &data);
    void handleSolidTorrents(const QByteArray &data);
    void handleDmhy(const QByteArray &data);
    void startRequest();

    QNetworkAccessManager m_nam;
    bool m_busy = false;
    QString m_error;
    QString m_query;
    QString m_provider;
    int m_offset = 0;
    int m_page = 0;
    bool m_append = false;
};

#endif
