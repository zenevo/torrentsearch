Name:       torrentsearch
Summary:    Torrent search client for Sailfish OS
Version:    0.2.0
Release:    1
License:    MIT
Source0:    torrentsearch-0.2.0.tar.gz
BuildRequires:  pkgconfig(sailfishapp)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Network)

%description
A Sailfish OS torrent search client. Searches public torrent indexes
and lets you copy or open magnet links. The application does not host
or distribute any torrent content.

%prep
%setup -q

%build
cd %{name}-%{version}
qmake
make %{?_smp_mflags}

%install
cd %{name}-%{version}
make install INSTALL_ROOT=%{buildroot}
install -D -m 644 data/torrentsearch.desktop %{buildroot}%{_datadir}/applications/torrentsearch.desktop
for s in 86 108 128 172 256 512; do
  install -D -m 644 icons/torrentsearch-$s.png %{buildroot}%{_datadir}/icons/hicolor/${s}x${s}/apps/torrentsearch.png
done

%files
%{_bindir}/torrentsearch
%{_datadir}/torrentsearch
%{_datadir}/applications/torrentsearch.desktop
%{_datadir}/icons/hicolor/*/apps/torrentsearch.png

%changelog
* Wed Nov 12 2026 zenevo <zenevo@users.noreply.github.com> - 0.2.0
- Add SolidTorrents (JSON) and DMHY (HTML) sources
- Drive paging from Backend::supportsPaging()
- Convert base32 info hashes to hex for torrent downloads
- Bilingual README and MIT license
* Wed Nov 05 2026 zenevo <zenevo@users.noreply.github.com> - 0.1.0
- Initial release
