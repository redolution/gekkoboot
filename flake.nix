{
  description = "A utilitarian bootloader for the GameCube";

  inputs = {
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    devkitNoob.url = "github:devkitNoob/devkitNoob";
    nixpkgs.follows = "devkitNoob/nixpkgs";
    flake-parts = {
      url = "github:hercules-ci/flake-parts";
      inputs.nixpkgs-lib.follows = "nixpkgs";
    };
    systems.flake = false;
  };

  outputs = { ... } @ inputs: inputs.flake-parts.lib.mkFlake {
    inherit inputs;
  } ({ config, flake-parts-lib, getSystem, inputs, lib, options, ... }:
    let
      rootConfig = config;
      rootOptions = options;
    in
    {
      _file = ./flake.nix;
      imports = [ ];
      config.perSystem = { config, inputs', nixpkgs, options, pkgs, system, ... }:
        let
          systemConfig = config;
          systemOptions = options;
          inherit (inputs') devkitNoob;
        in
        {
          _file = ./flake.nix;
          config = {
            devShells.default = devkitNoob.legacyPackages.callPackage
              ({ mkShell
              , noobkitPPC
              , gamecube-tools
              , meson
              , ninja
              , p7zip
              , python3
              }: mkShell {
                name = "gekkoboot";
                nativeBuildInputs = [
                  # The cross toolchain
                  noobkitPPC
                  gamecube-tools

                  # The build system
                  meson ninja

                  # Compressing binaries and the release archive
                  p7zip

                  # For build scripts
                  python3
                ];

                env = {
                  # For the libogc2 Makefile
                  DEVKITPPC = pkgs.callPackage ./nix/dkppc-rules.nix {};
                };
              })
              { };
          };
        };
      config.systems = import inputs.systems;
  });
}
