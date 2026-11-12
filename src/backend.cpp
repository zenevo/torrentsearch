#include "backend.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QRegularExpression>

Backend::Backend(QObject *parent) : QObject(parent) {}

void Backend::setBusy(bool b)
{
    if (m_busy == b)
        return;
    m_busy = b;
    emit busyChanged();
}

void Backend::setError(const QString &e)
{
    m_error = e;
    emit errorChanged();
}

QStringList Backend::providerList() const
{
    return { QStringLiteral("The Pirate Bay"), QStringLiteral("Knaben"),
             QStringLiteral("TorrentsCSV"), QStringLiteral("SolidTorrents"),
             QStringLiteral("DMHY"), QStringLiteral("Rutor"),
             QStringLiteral("Nyaa") };
}

bool Backend::supportsPaging(const QString &provider) const
{
    return provider == QStringLiteral("Knaben")
        || provider == QStringLiteral("SolidTorrents")
        || provider == QStringLiteral("DMHY")
        || provider == QStringLiteral("Rutor")
        || provider == QStringLiteral("Nyaa");
}

// dmhy returns base32 info hashes in its magnets; convert them to the hex form
// used by torrent clients and by itorrents.
static QString base32ToHex(const QString &in)
{
    static const char *alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567";
    QString out;
    quint64 buffer = 0;
    int bits = 0;
    for (const QChar ch : in.toUpper()) {
        const int v = QString::fromLatin1(alphabet).indexOf(ch);
        if (v < 0)
            continue;
        buffer = (buffer << 5) | quint64(v);
        bits += 5;
        while (bits >= 4) {
            bits -= 4;
            out.append(QStringLiteral("%1").arg((buffer >> bits) & 0xF, 1, 16));
        }
    }
    return out;
}

static QString hashFromMagnet(const QString &magnet)
{
    const int i = magnet.indexOf(QStringLiteral("btih:"));
    if (i < 0)
        return QString();
    QString hash = magnet.mid(i + 5);
    const int amp = hash.indexOf(QLatin1Char('&'));
    if (amp >= 0)
        hash = hash.left(amp);
    if (hash.size() == 32)
        return base32ToHex(hash);
    return hash;
}

QString Backend::magnetFor(const QString &infoHash, const QString &name) const
{
    return QStringLiteral("magnet:?xt=urn:btih:%1&dn=%2")
        .arg(infoHash, QString::fromUtf8(QUrl::toPercentEncoding(name)));
}

void Backend::search(const QString &query, const QString &provider)
{
    if (query.trimmed().isEmpty())
        return;
    m_query = query.trimmed();
    m_provider = provider;
    m_offset = 0;
    m_page = 0;
    m_append = false;
    startRequest();
}

void Backend::loadMore()
{
    if (m_query.isEmpty() || m_busy)
        return;
    m_append = true;
    m_page++;
    startRequest();
}

void Backend::startRequest()
{
    if (m_query.isEmpty())
        return;

    setBusy(true);
    setError(QString());
    const QString q = m_query.toLower();

    QNetworkReply *reply = nullptr;

    if (m_provider == QStringLiteral("Knaben")) {
        QNetworkRequest req(QUrl(QStringLiteral("https://api.knaben.org/v1")));
        req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
        QJsonObject body;
        body.insert(QStringLiteral("query"), q);
        body.insert(QStringLiteral("size"), 100);
        body.insert(QStringLiteral("from"), m_offset);
        body.insert(QStringLiteral("order_by"), QStringLiteral("seeders"));
        body.insert(QStringLiteral("order_direction"), QStringLiteral("desc"));
        body.insert(QStringLiteral("hide_unsafe"), true);
        body.insert(QStringLiteral("hide_xxx"), false);
        qWarning() << "torrentsearch: knaben" << q << "from" << m_offset;
        reply = m_nam.post(req, QJsonDocument(body).toJson(QJsonDocument::Compact));
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                setBusy(false);
                return;
            }
            handleKnaben(reply->readAll());
            setBusy(false);
        });
    } else if (m_provider == QStringLiteral("TorrentsCSV")) {
        QUrl url(QStringLiteral("https://torrents-csv.com/service/search"));
        QUrlQuery uq;
        uq.addQueryItem(QStringLiteral("q"), q);
        url.setQuery(uq);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
        qWarning() << "torrentsearch: torrentscsv" << q;
        reply = m_nam.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                setBusy(false);
                return;
            }
            handleTorrentsCsv(reply->readAll());
            setBusy(false);
        });
    } else if (m_provider == QStringLiteral("Rutor")) {
        const QUrl url(QStringLiteral("http://rutor.info/search/%1/%2").arg(q).arg(m_page));
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
        qWarning() << "torrentsearch: rutor" << url.toString();
        reply = m_nam.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                setBusy(false);
                return;
            }
            handleRutor(reply->readAll());
            setBusy(false);
        });
    } else if (m_provider == QStringLiteral("Nyaa")) {
        QUrl url(QStringLiteral("https://nyaa.si/"));
        QUrlQuery uq;
        uq.addQueryItem(QStringLiteral("q"), q);
        uq.addQueryItem(QStringLiteral("p"), QString::number(m_page + 1));
        uq.addQueryItem(QStringLiteral("s"), QStringLiteral("seeders"));
        uq.addQueryItem(QStringLiteral("o"), QStringLiteral("desc"));
        url.setQuery(uq);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
        qWarning() << "torrentsearch: nyaa" << url.toString();
        reply = m_nam.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                setBusy(false);
                return;
            }
            handleNyaa(reply->readAll());
            setBusy(false);
        });
    } else if (m_provider == QStringLiteral("SolidTorrents")) {
        const QString u = QStringLiteral("https://solidtorrents.eu/api/v1/search?q=%1&page=%2")
                              .arg(QString::fromUtf8(QUrl::toPercentEncoding(q)))
                              .arg(m_page + 1);
        QNetworkRequest req{QUrl(u)};
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
        qWarning() << "torrentsearch: solidtorrents" << u;
        reply = m_nam.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                setBusy(false);
                return;
            }
            handleSolidTorrents(reply->readAll());
            setBusy(false);
        });
    } else if (m_provider == QStringLiteral("DMHY")) {
        const QString u = QStringLiteral("https://share.dmhy.org/topics/list?keyword=%1&page=%2")
                              .arg(QString::fromUtf8(QUrl::toPercentEncoding(q)))
                              .arg(m_page + 1);
        QNetworkRequest req{QUrl(u)};
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
        qWarning() << "torrentsearch: dmhy" << u;
        reply = m_nam.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                setBusy(false);
                return;
            }
            handleDmhy(reply->readAll());
            setBusy(false);
        });
    } else {
        QUrl url(QStringLiteral("https://apibay.org/q.php"));
        QUrlQuery uq;
        uq.addQueryItem(QStringLiteral("q"), q);
        url.setQuery(uq);
        QNetworkRequest req(url);
        req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));
        qWarning() << "torrentsearch: apibay" << q;
        reply = m_nam.get(req);
        connect(reply, &QNetworkReply::finished, this, [this, reply]() {
            reply->deleteLater();
            if (reply->error() != QNetworkReply::NoError) {
                setError(reply->errorString());
                setBusy(false);
                return;
            }
            handleApibay(reply->readAll());
            setBusy(false);
        });
    }
}

void Backend::handleApibay(const QByteArray &data)
{
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isArray()) {
        setError(QStringLiteral("Invalid response"));
        return;
    }
    QVariantList items;
    for (const QJsonValue &v : doc.array()) {
        const QJsonObject o = v.toObject();
        const QString name = o.value(QStringLiteral("name")).toString();
        if (name.isEmpty() || name == QStringLiteral("No results returned"))
            continue;
        const QString hash = o.value(QStringLiteral("info_hash")).toString();
        items.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("infoHash"), hash},
            {QStringLiteral("magnet"), magnetFor(hash, name)},
            {QStringLiteral("size"), o.value(QStringLiteral("size")).toVariant().toLongLong()},
            {QStringLiteral("seeders"), o.value(QStringLiteral("seeders")).toVariant().toInt()},
            {QStringLiteral("leechers"), o.value(QStringLiteral("leechers")).toVariant().toInt()},
        });
    }
    qWarning() << "torrentsearch: parsed" << items.size();
    m_offset += items.size();
    emit results(items, m_append);
}

void Backend::handleTorrentsCsv(const QByteArray &data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonArray arr = doc.object().value(QStringLiteral("torrents")).toArray();
    QVariantList items;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QString name = o.value(QStringLiteral("name")).toString();
        const QString hash = o.value(QStringLiteral("infohash")).toString();
        if (name.isEmpty() || hash.isEmpty())
            continue;
        items.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("infoHash"), hash},
            {QStringLiteral("magnet"), magnetFor(hash, name)},
            {QStringLiteral("size"), o.value(QStringLiteral("size_bytes")).toVariant().toLongLong()},
            {QStringLiteral("seeders"), o.value(QStringLiteral("seeders")).toVariant().toInt()},
            {QStringLiteral("leechers"), o.value(QStringLiteral("leechers")).toVariant().toInt()},
        });
    }
    qWarning() << "torrentsearch: parsed" << items.size();
    m_offset += items.size();
    emit results(items, m_append);
}

void Backend::handleKnaben(const QByteArray &data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonArray arr = doc.object().value(QStringLiteral("hits")).toArray();
    QVariantList items;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QString name = o.value(QStringLiteral("title")).toString();
        const QString magnet = o.value(QStringLiteral("magnetUrl")).toString();
        if (name.isEmpty())
            continue;
        items.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("infoHash"), o.value(QStringLiteral("hash")).toString()},
            {QStringLiteral("magnet"), magnet},
            {QStringLiteral("size"), o.value(QStringLiteral("bytes")).toVariant().toLongLong()},
            {QStringLiteral("seeders"), o.value(QStringLiteral("seeders")).toVariant().toInt()},
            {QStringLiteral("leechers"), o.value(QStringLiteral("peers")).toVariant().toInt()},
        });
    }
    qWarning() << "torrentsearch: parsed" << items.size();
    m_offset += items.size();
    emit results(items, m_append);
}

void Backend::handleRutor(const QByteArray &data)
{
    const QString html = QString::fromUtf8(data);
    QVariantList items;
    static const QRegularExpression rowRe(
        QStringLiteral("<tr class=\"(?:gai|tum)\">(.*?)</tr>"),
        QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression titleRe(
        QStringLiteral("<a href=\"/torrent/[^\"]+\">([^<]+)</a>"));
    static const QRegularExpression magnetRe(
        QStringLiteral("href=\"(magnet:[^\"]+)\""));
    static const QRegularExpression sizeRe(
        QStringLiteral("<td align=\"right\">([^<]+)</td>"));

    auto it = rowRe.globalMatch(html);
    while (it.hasNext()) {
        const QString row = it.next().captured(1);
        const auto tm = titleRe.match(row);
        const auto mm = magnetRe.match(row);
        if (!tm.hasMatch() || !mm.hasMatch())
            continue;
        QString magnet = mm.captured(1);
        magnet.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        int hash = magnet.indexOf(QStringLiteral("btih:"));
        QString ih = hash >= 0 ? magnet.mid(hash + 5, 40) : QString();
        QString size = sizeRe.match(row).captured(1);
        items.append(QVariantMap{
            {QStringLiteral("name"), tm.captured(1).trimmed()},
            {QStringLiteral("infoHash"), ih},
            {QStringLiteral("magnet"), magnet},
            {QStringLiteral("size"), 0},
            {QStringLiteral("sizeText"), size},
            {QStringLiteral("seeders"), 0},
            {QStringLiteral("leechers"), 0},
        });
    }
    qWarning() << "torrentsearch: parsed" << items.size();
    m_offset += items.size();
    emit results(items, m_append);
}

void Backend::handleNyaa(const QByteArray &data)
{
    const QString html = QString::fromUtf8(data);
    QVariantList items;
    static const QRegularExpression rowRe(
        QStringLiteral("<tr class=\"default\">(.*?)</tr>"),
        QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression titleRe(
        QStringLiteral("<a href=\"/view/[0-9]+\" title=\"([^\"]+)\">"));
    static const QRegularExpression magnetRe(
        QStringLiteral("href=\"(magnet:[^\"]+)\""));

    auto it = rowRe.globalMatch(html);
    while (it.hasNext()) {
        const QString row = it.next().captured(1);
        const auto tm = titleRe.match(row);
        const auto mm = magnetRe.match(row);
        if (!tm.hasMatch() || !mm.hasMatch())
            continue;
        QString magnet = mm.captured(1);
        magnet.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        int hash = magnet.indexOf(QStringLiteral("btih:"));
        QString ih = hash >= 0 ? magnet.mid(hash + 5, 40) : QString();
        items.append(QVariantMap{
            {QStringLiteral("name"), tm.captured(1).trimmed()},
            {QStringLiteral("infoHash"), ih},
            {QStringLiteral("magnet"), magnet},
            {QStringLiteral("size"), 0},
            {QStringLiteral("sizeText"), QString()},
            {QStringLiteral("seeders"), 0},
            {QStringLiteral("leechers"), 0},
        });
    }
    qWarning() << "torrentsearch: parsed" << items.size();
    m_offset += items.size();
    emit results(items, m_append);
}

void Backend::handleSolidTorrents(const QByteArray &data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    const QJsonArray arr = doc.object().value(QStringLiteral("results")).toArray();
    QVariantList items;
    for (const QJsonValue &v : arr) {
        const QJsonObject o = v.toObject();
        const QString name = o.value(QStringLiteral("title")).toString();
        const QString hash = o.value(QStringLiteral("infohash")).toString();
        if (name.isEmpty() || hash.isEmpty())
            continue;
        items.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("infoHash"), hash.toLower()},
            {QStringLiteral("magnet"), magnetFor(hash.toLower(), name)},
            {QStringLiteral("size"), o.value(QStringLiteral("size")).toVariant().toLongLong()},
            {QStringLiteral("seeders"), o.value(QStringLiteral("seeders")).toVariant().toInt()},
            {QStringLiteral("leechers"), o.value(QStringLiteral("leechers")).toVariant().toInt()},
        });
    }
    qWarning() << "torrentsearch: parsed" << items.size();
    m_offset += items.size();
    emit results(items, m_append);
}

void Backend::handleDmhy(const QByteArray &data)
{
    const QString html = QString::fromUtf8(data);
    QVariantList items;
    static const QRegularExpression rowRe(
        QStringLiteral("<tr[^>]*>(.*?)</tr>"),
        QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression titleRe(
        QStringLiteral("<td class=\"title\">.*?<a href=\"/topics/view/[^\"]+\"[^>]*>(.*?)</a>"),
        QRegularExpression::DotMatchesEverythingOption);
    static const QRegularExpression magnetRe(
        QStringLiteral("href=\"(magnet:[^\"]+)\""));

    auto it = rowRe.globalMatch(html);
    while (it.hasNext()) {
        const QString row = it.next().captured(1);
        const auto tm = titleRe.match(row);
        const auto mm = magnetRe.match(row);
        if (!tm.hasMatch() || !mm.hasMatch())
            continue;
        QString title = tm.captured(1);
        title.remove(QRegularExpression(QStringLiteral("<[^>]*>")));
        title = title.simplified();
        QString magnet = mm.captured(1);
        magnet.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
        const QString hash = hashFromMagnet(magnet);
        items.append(QVariantMap{
            {QStringLiteral("name"), title},
            {QStringLiteral("infoHash"), hash},
            {QStringLiteral("magnet"), magnet},
            {QStringLiteral("size"), 0},
            {QStringLiteral("sizeText"), QString()},
            {QStringLiteral("seeders"), 0},
            {QStringLiteral("leechers"), 0},
        });
    }
    qWarning() << "torrentsearch: parsed" << items.size();
    m_offset += items.size();
    emit results(items, m_append);
}

void Backend::downloadTorrent(const QString &infoHash, const QString &name, const QString &torrentUrl)
{
    QString hash = infoHash;
    if (hash.size() == 32)
        hash = base32ToHex(hash);

    QString url = torrentUrl;
    if (url.isEmpty() && !hash.isEmpty())
        url = QStringLiteral("https://itorrents.org/torrent/%1.torrent").arg(hash);

    if (url.isEmpty()) {
        emit torrentFailed(QStringLiteral("No torrent URL available"));
        return;
    }

    QNetworkRequest req((QUrl(url)));
    req.setHeader(QNetworkRequest::UserAgentHeader, QStringLiteral("Mozilla/5.0"));

    QNetworkReply *reply = m_nam.get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, name]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit torrentFailed(reply->errorString());
            return;
        }
        const QByteArray data = reply->readAll();
        if (data.isEmpty() || data[0] != 'd') {
            emit torrentFailed(QStringLiteral("Not a torrent file"));
            return;
        }

        QString dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        if (dir.isEmpty())
            dir = QDir::homePath();
        QDir().mkpath(dir);

        QString safe = name;
        safe.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9_. -]")), QStringLiteral("_"));
        if (safe.isEmpty())
            safe = QStringLiteral("torrent");
        const QString path = dir + QStringLiteral("/") + safe.left(100) + QStringLiteral(".torrent");

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            emit torrentFailed(f.errorString());
            return;
        }
        f.write(data);
        f.close();
        qWarning() << "torrentsearch: saved" << path;
        emit torrentSaved(path);
    });
}
