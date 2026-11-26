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
    in
    {
      packages.${system} = {
        default = pkgs.stdenv.mkDerivation {
          pname = "kiriflux";
          version = "0.1.2";
          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            kdePackages.wrapQtAppsHook
            kdePackages.extra-cmake-modules
          ];

          buildInputs = with pkgs.kdePackages; [
            qtbase
            qtdeclarative
            qtsvg
            qtwayland
            kirigami
            qqc2-desktop-style
            qqc2-breeze-style
            sonnet
            kconfig
            ki18n
            kcoreaddons
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

          preFixup = ''
            qtWrapperArgs=(
              --unset NIXPKGS_QT6_QML_IMPORT_PATH
              --unset QML2_IMPORT_PATH
              --set QML_DISABLE_DISK_CACHE "1"
              --set QTWEBENGINEPROCESS_PATH "${pkgs.kdePackages.qtwebengine}/libexec/QtWebEngineProcess"
              --set QTWEBENGINE_RESOURCES_PATH "${pkgs.kdePackages.qtwebengine}/resources"
              --set QTWEBENGINE_LOCALES_PATH "${pkgs.kdePackages.qtwebengine}/translations/qtwebengine_locales"
              --prefix XDG_DATA_DIRS : "${pkgs.kdePackages.breeze-icons}/share"
              --set QT_QPA_PLATFORMTHEME "kde"
              --set QT_QUICK_CONTROLS_STYLE "org.kde.desktop"
              "''${qtWrapperArgs[@]}"
            )
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

        # Mirror the wrapQtAppsHook environment for dev builds
        shellHook = ''
          unset QML2_IMPORT_PATH
          export QML_DISABLE_DISK_CACHE=1
          export NIXPKGS_QT6_QML_IMPORT_PATH="${pkgs.lib.makeSearchPath "lib/qt-6/qml" (with pkgs.kdePackages; [
            kirigami.unwrapped
            qqc2-desktop-style
            qqc2-breeze-style
            sonnet
            kitemmodels
            qtdeclarative
            qtwebengine
          ])}"
          export QT_PLUGIN_PATH="${pkgs.lib.makeSearchPath "lib/qt-6/plugins" (with pkgs.kdePackages; [
            qtbase
            qtwayland
            plasma-integration
            breeze
          ])}:$QT_PLUGIN_PATH"
          export XDG_DATA_DIRS="${pkgs.kdePackages.kirigami}/share:${pkgs.kdePackages.breeze-icons}/share:$XDG_DATA_DIRS"
          export QT_QPA_PLATFORMTHEME="kde"
          export QT_QUICK_CONTROLS_STYLE="org.kde.desktop"
          echo "Kiriflux dev shell ready. Run: cmake -B build -G Ninja -DNIX_BUILD=ON && cmake --build build"
        '';
      };
    };
}
