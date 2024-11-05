{
  description = "A utilitarian bootloader for the GameCube";

  inputs = {
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    devkitNoob.url = "github:devkitNoob/devkitNoob";
    nixpkgs.follows = "devkitNoob/nixpkgs";
  };

  outputs = inputs: let
    inherit (inputs.nixpkgs) lib;
    inherit (lib) genAttrs;
    forAllSystems = genAttrs systems;
    systems = lib.systems.flakeExposed;
  in {
    formatter = forAllSystems (system: let
      devkitNoob = inputs.devkitNoob.legacyPackages.${system};
      inherit (devkitNoob) nixpkgs;
    in
      nixpkgs.alejandra);

    devShells = forAllSystems (system: let
      devkitNoob = inputs.devkitNoob.legacyPackages.${system};
      inherit (devkitNoob) nixpkgs;
    in {
      default =
        devkitNoob.callPackage
        ({
          mkShell,
          noobkitPPC,
          gamecube-tools,
          meson,
          ninja,
          clang-tools,
          p7zip,
          python3,
        }:
          mkShell {
            name = "gekkoboot";
            nativeBuildInputs = [
              # The cross toolchain
              noobkitPPC
              gamecube-tools

              # The build system
              meson
              ninja

              # clang-format
              clang-tools

              # Compressing binaries and the release archive
              p7zip

              # For build scripts
              python3
            ];

            env = {
              # For the libogc2 Makefile
              DEVKITPPC = nixpkgs.callPackage ./nix/dkppc-rules.nix {};
            };
          })
        {};
    });
  };
}
