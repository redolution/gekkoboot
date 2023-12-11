{
  description = "A utilitarian bootloader for the GameCube";

  inputs = {
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
    nixpkgs.url = "github:9ary/nixpkgs/devkitPPC";
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
        in
        {
          _file = ./flake.nix;
          config = {
            devShells.default = pkgs.callPackage
              ({ mkShell
              , devkitPPC
              , gamecube-tools
              , meson
              , ninja
              , p7zip
              , python3
              }: mkShell {
                name = "iplboot";
                nativeBuildInputs = [
                  # The cross toolchain
                  devkitPPC
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
                  DEVKITPPC = devkitPPC;
                };
              })
              { };
          };
        };
      config.systems = import inputs.systems;
  });
}
