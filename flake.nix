{
  description = "Kiriflux – KDE Plasma RSS reader for Miniflux";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    nix2flatpak.url = "github:neobrain/nix2flatpak";
  };

  outputs = { self, nixpkgs, nix2flatpak }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

      # Kirigami's qmldir files contain 'prefer :/qt/qml/...' directives that
      # force the QML engine to load from QRC resource paths. When KF6::Kirigami
      # is linked statically, those resources exist in the binary. On NixOS we
      # don't link it (it breaks qt6_import_qml_plugins), so we strip the
      # 'prefer' lines to let the engine use the filesystem paths instead.
      kirigamiQml = pkgs.runCommand "kirigami-qml-patched" {} ''
        cp -r ${pkgs.kdePackages.kirigami.unwrapped}/lib/qt-6/qml $out
        chmod -R u+w $out
        find $out -name qmldir -exec sed -i -e '/^prefer /d' -e '/^linktarget /d' {} \;
      '';
    in
    {
      packages.${system} = {
        default = pkgs.stdenv.mkDerivation {
          pname = "kiriflux";
          version = "0.1.1";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            makeWrapper
            kdePackages.extra-cmake-modules
          ];

          buildInputs = with pkgs.kdePackages; [
            qtbase
            qtdeclarative
            qtsvg
            kirigami
            qqc2-desktop-style
            qqc2-breeze-style
            kconfig
            ki18n
            kcoreaddons
            kxmlgui
            kitemmodels
            kdbusaddons
            qtwebengine
            plasma-integration
            breeze
            breeze-icons
          ];

          cmakeFlags = [
            "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
            "-DBUILD_TESTING=ON"
            "-DNIX_BUILD=ON"
          ];

          doCheck = false;

          dontWrapQtApps = true;

          # Explicitly wrap the app with required QML paths and KDE theme
          postInstall = ''
            wrapProgram $out/bin/kiriflux \
              --set QML_DISABLE_DISK_CACHE "1" \
              --set QTWEBENGINEPROCESS_PATH "${pkgs.kdePackages.qtwebengine}/libexec/QtWebEngineProcess" \
              --set QTWEBENGINE_RESOURCES_PATH "${pkgs.kdePackages.qtwebengine}/resources" \
              --set QTWEBENGINE_LOCALES_PATH "${pkgs.kdePackages.qtwebengine}/translations/qtwebengine_locales" \
              --prefix QML2_IMPORT_PATH : "${kirigamiQml}" \
              --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.qqc2-desktop-style}/lib/qt-6/qml" \
              --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.qqc2-breeze-style}/lib/qt-6/qml" \
              --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.kitemmodels}/lib/qt-6/qml" \
              --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.qtdeclarative}/lib/qt-6/qml" \
              --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.qtwebengine}/lib/qt-6/qml" \
              --prefix QT_PLUGIN_PATH : "${pkgs.kdePackages.qtbase}/lib/qt-6/plugins" \
              --prefix QT_PLUGIN_PATH : "${pkgs.kdePackages.plasma-integration}/lib/qt-6/plugins" \
              --prefix QT_PLUGIN_PATH : "${pkgs.kdePackages.breeze}/lib/qt-6/plugins" \
              --prefix XDG_DATA_DIRS : "${pkgs.kdePackages.kirigami}/share" \
              --prefix XDG_DATA_DIRS : "${pkgs.kdePackages.breeze-icons}/share" \
              --prefix XDG_DATA_DIRS : "$out/share" \
              --set QT_QPA_PLATFORMTHEME "kde" \
              --set QT_QUICK_CONTROLS_STYLE "org.kde.desktop"
          '';
        };

        flatpak = nix2flatpak.lib.${system}.mkFlatpak {
          appId = "org.kde.kiriflux";
          package = self.packages.${system}.default;
          runtime = "org.kde.Platform/6.10";
          permissions = {
            share = [ "ipc" "network" ];
            sockets = [ "x11" "wayland" "pulseaudio" "session-bus" ];
            devices = [ "dri" ];
            filesystems = [
              "xdg-run/dconf"
              "xdg-config/kdeglobals:ro"
            ];
            talk-names = [
              "org.freedesktop.Notifications"
              "org.kde.StatusNotifierWatcher"
              "com.canonical.AppMenu.Registrar"
              "org.kde.kappmenu"
            ];
          };
          skipAbiChecks = true;
        };
      };

      devShells.${system}.default = pkgs.mkShell {
        inputsFrom = [ self.packages.${system}.default ];

        packages = with pkgs; [
          gdb
          clang-tools
        ];

        # Mirror the postInstall wrapper environment for dev builds
        shellHook = ''
          export QML_DISABLE_DISK_CACHE=1
          export QML2_IMPORT_PATH="${kirigamiQml}:${pkgs.kdePackages.qqc2-desktop-style}/lib/qt-6/qml:${pkgs.kdePackages.qqc2-breeze-style}/lib/qt-6/qml:${pkgs.kdePackages.kitemmodels}/lib/qt-6/qml:${pkgs.kdePackages.qtdeclarative}/lib/qt-6/qml:${pkgs.kdePackages.qtwebengine}/lib/qt-6/qml"
          export QT_PLUGIN_PATH="${pkgs.kdePackages.qtbase}/lib/qt-6/plugins:${pkgs.kdePackages.plasma-integration}/lib/qt-6/plugins:${pkgs.kdePackages.breeze}/lib/qt-6/plugins"
          export XDG_DATA_DIRS="${pkgs.kdePackages.kirigami}/share:${pkgs.kdePackages.breeze-icons}/share:$XDG_DATA_DIRS"
          export QT_QPA_PLATFORMTHEME="kde"
          export QT_QUICK_CONTROLS_STYLE="org.kde.desktop"
          echo "Kiriflux dev shell ready. Run: cmake -B build -G Ninja -DNIX_BUILD=ON && cmake --build build"
        '';
      };
    };
}
