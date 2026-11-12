# TorrentSearch

一个面向 **Sailfish OS** 的种子元搜索客户端。它同时查询多个公开种子索引站，把结果汇总到一个列表，
可以复制磁力链接或把 `.torrent` 文件保存到本地。应用本身不托管、不索引、不分发任何内容，只向第三方
服务发起搜索请求。


English documentation: [README.md](README.md)

---

## 搜索源

| 源 | 类型 | 分页 | 说明 |
|---|---|---|---|
| **The Pirate Bay** | JSON（`apibay.org`） | 否 | 一次返回该查询的全部结果。 |
| **Knaben** | JSON（`api.knaben.org`） | 是（offset） | 聚合多个索引站的元搜索。 |
| **TorrentsCSV** | JSON（`torrents-csv.com`） | 否 | 社区维护的 CSV/JSON 索引。 |
| **SolidTorrents** | JSON（`solidtorrents.eu`） | 是（page） | 带做种/下载人数。 |
| **DMHY**（動漫花園） | HTML | 是（page） | 中文/日文动漫、音乐、游戏资源站。 |
| **Rutor** | HTML | 是（page） | 俄语综合站。 |
| **Nyaa** | HTML | 是（page） | 动漫站。 |

搜索源列表编译在应用内（`Backend::providerList()`），新增一个源需要在 `src/backend.cpp`
里加处理函数，并在 `qml/pages/SearchPage.qml` 里加菜单项。

### 可用性

可用性取决于你所在的网络。所有源在加入时都从不受限的网络验证过；在受限网络下（例如中国大陆
未使用 VPN 时），很多公开站会被 DNS 污染或 SNI 阻断，应用会报网络错误——换一个源或挂 VPN 即可。
受限网络下相对稳定的是 **The Pirate Bay**（apibay）、**Knaben** 和 **TorrentsCSV**。

## 功能

- 一个输入框搜索七个源；
- 源提供数据时显示做种数/下载数和体积；
- 支持分页的源可以「加载更多」；
- 复制磁力链接，或把 `.torrent` 文件下载到 `~/Downloads`；
- DMHY 返回的 base32 磁力哈希会自动转成十六进制；
- Sailfish Silica 界面，横竖屏都可用。

## 编译

需要 Sailfish OS SDK 和对应的 target（示例为 aarch64）：

```sh
mb2 -t oneplus-dumpling-aarch64 build
```

编译依赖：`pkgconfig(sailfishapp)`、`pkgconfig(Qt5Quick)`、`pkgconfig(Qt5Qml)`、
`pkgconfig(Qt5Network)`。

安装生成的 RPM：

```sh
devel-su /bin/sh -c "zypper --allow-unsigned-rpm in ./torrentsearch-0.2.0-1.aarch64.rpm"
```

## 更新记录

**0.2.0**

- 新增 **SolidTorrents**（JSON）和 **DMHY**（HTML）两个源；
- 「加载更多」改为由 `Backend::supportsPaging()` 判断，不再依赖下拉框的硬编码下标，
  以后加源不会悄悄把分页弄坏；
- DMHY 的 base32 哈希自动转十六进制，`.torrent` 下载回退可用；
- QML 的源菜单与后端列表保持同步；
- 新增中英双语 README 和 MIT 许可证文件。

**0.1.0** — 首个版本，包含 The Pirate Bay、Knaben、TorrentsCSV、Rutor、Nyaa。

## 免责声明

本应用只向第三方搜索服务发起查询，不托管、不分发任何种子文件或媒体内容。搜索和下载什么内容由你
自己负责，请确保你有权访问相关内容。作者与上述任何站点均无关联。

## 许可证

MIT — 见 [LICENSE](LICENSE)。
