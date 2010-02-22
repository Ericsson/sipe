#
# OBS SPEC file to generate a RPM for pidgin-sipe.
# It should work on Fedora 10/11, openSUSE 11.x, RHEL5/CentOS 5, SLES/D 10/11 and Mandriva 2009.1.
#

%define purple_plugin libpurple-plugin-sipe

%define purple_develname libpurple-devel

%if 0%{?mandriva_version} >= 200910
%ifarch x86_64
%define purple_develname lib64purple-devel
%endif
%endif

%if 0%{?suse_version} || 0%{?sles_version}
%define pkg_group Productivity/Networking/Instant Messenger
%endif
%if 0%{?fedora_version}
%define pkg_group Applications/Internet
%endif
%if 0%{?mandriva_version}
%define pkg_group Networking/Instant messaging
%else
%define pkg_group Applications/Internet
%endif

Name:           pidgin-sipe
Version:        1.8.1
Release:        1
License:        GPLv2+
Summary:        Pidgin third-party plugin for Microsoft LCS/OCS
Url:            http://sipe.sourceforge.net/
Group:          %{pkg_group}
Source:         %{name}-%{version}.tar.gz
BuildRequires:  gettext-devel
BuildRequires:  intltool
# Configurable components
%if !0%{?_without_kerberos:1}
%define config_krb5 --with-krb5
BuildRequires:  krb5-devel
%endif
BuildRequires:  %{purple_develname} >= 2.3.1
BuildRequires:  libtool
# For directory ownership
BuildRequires:  pidgin
Requires:       %{purple_plugin} = %{version}-%{release}
Requires:       pidgin
%if 0%{?sles_version} == 10
BuildRequires:  gnome-keyring-devel
%endif
BuildRoot:      %{_tmppath}/%{name}-%{version}-build

%description
A third-party plugin for the Pidgin multi-protocol instant messenger. It
implements the extended version of SIP/SIMPLE used by various products:

    * Microsoft Office Communications Server (OCS 2007 and newer)
    * Microsoft Live Communications Server (LCS 2003/2005)
    * Reuters Messaging

This package provides the icon set for Pidgin.


%package -n %{purple_plugin}
License:        GPLv2+
Summary:        Libpurple third-party plugin for Microsoft LCS/OCS
Group:          %{pkg_group}
Obsoletes:      purple-sipe

%description -n %{purple_plugin}
A third-party plugin for the libpurple multi-protocol instant messaging core.
It implements the extended version of SIP/SIMPLE used by various products:

    * Microsoft Office Communications Server (OCS 2007 and newer)
    * Microsoft Live Communications Server (LCS 2003/2005)
    * Reuters Messaging

%prep
%setup -q

%build
%define config_params %{?config_krb5:%{config_krb5}}
%if 0%{?sles_version} == 10
export CFLAGS="%optflags -I%{_includedir}/gssapi"
%endif
%configure %{config_params}
make %{_smp_mflags}

%install
%makeinstall
find %{buildroot} -type f -name "*.la" -delete -print
%find_lang %{name}

%clean
rm -rf %{buildroot}

%files -n %{purple_plugin} -f %{name}.lang
%defattr(-,root,root,-)
%doc AUTHORS ChangeLog COPYING NEWS README TODO
%{_libdir}/purple-2/libsipe.so

%files
%defattr(-,root,root,-)
%doc AUTHORS COPYING
%{_datadir}/pixmaps/pidgin/protocols/*/sipe.png

%changelog
* Sun Feb 08 2010 pier11 <pier11@operamail.com> 1.8.0
- source is an original 1.8.0 with patch: git(upstream) 9c34cc3557daa3d61a002002492c71d0343c8cae
- temp hack - renamed source in spec from .bz2 to .gz as the latter was prepared with the patch. 

* Sun Nov 22 2009 pier11 <pier11@operamail.com> 1.7.1
- reinstated enable-quality-check

* Wed Nov 04 2009 John Beranek <john@redux.org.uk> 1.7.0
- Spec file modifications to allow SLES/D 10 and Mandriva 2009.1 builds

* Tue Nov 03 2009 John Beranek <john@redux.org.uk> 1.7.0
- Spec file modifications for openSUSE build service

* Sun Oct 11 2009 J. D. User <jduser@noreply.com> 1.6.3-*git*
- move non-Pidgin files to new sub-package purple-sipe

* Sun Oct 11 2009 J. D. User <jduser@noreply.com> 1.6.3-*git*
- remove directory for emoticon theme icons

* Sun Oct 11 2009 J. D. User <jduser@noreply.com> 1.6.3-*git*
- libpurple protocol plugins are located under %%{_libdir}/purple-2

* Mon Sep 28 2009 J. D. User <jduser@noreply.com> 1.6.3-*git*
- added directory for emoticon theme icons

* Wed Sep 09 2009 J. D. User <jduser@noreply.com> 1.6.3
- update to 1.6.3

* Fri Aug 28 2009 J. D. User <jduser@noreply.com> 1.6.2-*git*
- reduce libpurple-devel requirement to >= 2.3.1

* Mon Aug 24 2009 J. D. User <jduser@noreply.com> 1.6.2
- update to 1.6.2

* Fri Aug 21 2009 J. D. User <jduser@noreply.com> 1.6.1-*git*
- reduce libpurple-devel requirement to >= 2.4.1

* Mon Aug 17 2009 J. D. User <jduser@noreply.com> 1.6.1-*git*
- com_err.h only required for kerberos

* Tue Aug 11 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- require libpurple-devel >= 2.5.0

* Sun Aug 09 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- refactor configure parameters
- make kerberos configurable
- don't hard code prefix for git builds

* Sun Aug 09 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- removed unnecessary zlib-devel

* Sat Aug 08 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- fix prefix for git builds

* Sat Aug 01 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- append -Wno-unused-parameter for GCC <4.4 compilation errors

* Thu Jul 30 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- remove duplicate GPL2

* Thu Jul 30 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- use "--with git" to build from git
- corrected download URL for release archive
- add missing BR gettext-devel

* Wed Jul 29 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- use default rpmbuild CFLAGS also for git builds
- merge with SPEC files created by mricon & jberanek

* Tue Jul 28 2009 J. D. User <jduser@noreply.com> 1.6.0-*git*
- initial RPM SPEC example generated