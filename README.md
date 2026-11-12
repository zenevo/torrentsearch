# TorrentSearch

A small torrent meta-search client for **Sailfish OS**. It queries several public torrent
indexes, shows the results in one list and lets you copy a magnet link or save the `.torrent`
file. The app does not host, index or distribute any content itself — it only performs
searches against third-party services.


Chinese documentation: [README.zh-CN.md](README.zh-CN.md)

---

## Sources

| Source | Type | Paging | Notes |
|---|---|---|---|
| **The Pirate Bay** | JSON (`apibay.org`) | no | Returns the full result set for a query. |
| **Knaben** | JSON (`api.knaben.org`) | yes (offset) | Meta search that aggregates many indexes. |
| **TorrentsCSV** | JSON (`torrents-csv.com`) | no | Community-maintained CSV/JSON index. |
| **SolidTorrents** | JSON (`solidtorrents.eu`) | yes (page) | Includes seeder/leecher counts. |
| **DMHY** (動漫花園) | HTML | yes (page) | Chinese/Japanese anime, music and game tracker. |
| **Rutor** | HTML | yes (page) | Russian general tracker. |
| **Nyaa** | HTML | yes (page) | Anime tracker. |

The provider list is compiled into the app (`Backend::providerList()`), so adding a new source
means adding a handler in `src/backend.cpp` and a menu entry in `qml/pages/SearchPage.qml`.

### Reachability

Reachability depends on the network you are on. All sources were checked from a neutral
connection when they were added; behind filtering (for example mainland China without a VPN)
many public trackers are blocked by DNS poisoning or SNI filtering and the app will report a
network error — try another source or use a VPN. The three most reliable sources in restricted
networks are usually **The Pirate Bay** (apibay), **Knaben** and **TorrentsCSV**.

## Features

- search across seven sources from a single field;
- seeder/leecher and size columns when the source provides them;
- "Load more" paging for sources that support it;
- copy the magnet link, or download the `.torrent` file into `~/Downloads`;
- magnet hashes returned in base32 (DMHY) are converted to hex automatically;
- Sailfish Silica UI, works in portrait and landscape.

## Building

Requires the Sailfish OS SDK and a Sailfish OS target (aarch64 in the example):

```sh
mb2 -t oneplus-dumpling-aarch64 build
```

Build dependencies: `pkgconfig(sailfishapp)`, `pkgconfig(Qt5Quick)`, `pkgconfig(Qt5Qml)`,
`pkgconfig(Qt5Network)`.

Install the resulting RPM:

```sh
devel-su /bin/sh -c "zypper --allow-unsigned-rpm in ./torrentsearch-0.2.0-1.aarch64.rpm"
```

## Changelog

**0.2.0**

- add **SolidTorrents** (JSON) and **DMHY** (HTML) sources;
- "Load more" is now driven by `Backend::supportsPaging()` instead of hard-coded combo indexes,
  so adding a source cannot silently break paging;
- convert base32 info hashes (DMHY magnets) to hex for the `.torrent` download fallback;
- the QML provider menu and the backend provider list are kept in sync;
- bilingual README (English / Chinese), MIT license file added.

**0.1.0** — initial release with The Pirate Bay, Knaben, TorrentsCSV, Rutor and Nyaa.

## Disclaimer

This application only queries third-party search services and does not host or distribute any
torrent files or media. What you search for and download is your own responsibility; make sure
you have the right to access the content. The author is not affiliated with any of the services
listed above.

## License

MIT — see [LICENSE](LICENSE).
