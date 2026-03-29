{
  description = "Kiriflux – KDE Plasma RSS reader for Miniflux";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};
    in
    {
      packages.${system}.default = pkgs.stdenv.mkDerivation {
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
          kconfig
          ki18n
          kcoreaddons
          kxmlgui
          kitemmodels
          kdbusaddons
          qtwebengine
          plasma-integration
        ];

        cmakeFlags = [
          "-DCMAKE_INSTALL_PREFIX=${placeholder "out"}"
          "-DBUILD_TESTING=ON"
        ];

        doCheck = false;

        dontWrapQtApps = true;

        # Explicitly wrap the app with required QML paths and KDE theme
        postInstall = ''
          wrapProgram $out/bin/kiriflux \
            --set QML_DISABLE_DISK_CACHE "1" \
            --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.kirigami}/lib/qt-6/qml" \
            --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.qqc2-desktop-style}/lib/qt-6/qml" \
            --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.kitemmodels}/lib/qt-6/qml" \
            --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.qtdeclarative}/lib/qt-6/qml" \
            --prefix QML2_IMPORT_PATH : "${pkgs.kdePackages.qtwebengine}/lib/qt-6/qml" \
            --prefix QT_PLUGIN_PATH : "${pkgs.kdePackages.qtbase}/lib/qt-6/plugins" \
            --prefix QT_PLUGIN_PATH : "${pkgs.kdePackages.plasma-integration}/lib/qt-6/plugins" \
            --prefix XDG_DATA_DIRS : "${pkgs.kdePackages.kirigami}/share" \
            --prefix XDG_DATA_DIRS : "$out/share" \
            --set QT_QPA_PLATFORMTHEME "kde"
        '';
      };

      devShells.${system}.default = pkgs.mkShell {
        inputsFrom = [ self.packages.${system}.default ];

        packages = with pkgs; [
          gdb
          clang-tools
        ];

        # Ensure QML modules are found at runtime during development
        shellHook = ''
          export QT_PLUGIN_PATH="${pkgs.kdePackages.qtbase}/lib/qt-6/plugins:${pkgs.kdePackages.plasma-integration}/lib/qt-6/plugins"
          export QT_QPA_PLATFORMTHEME="kde"
          echo "Kiriflux dev shell ready. Run: cmake -B build -G Ninja && cmake --build build"
        '';
      };
    };
}
