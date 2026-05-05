{
  description = "rust dev flake";

  inputs = {
    # taken from fenix monthly
    nixpkgs.url = "github:nixos/nixpkgs/8110df5ad7abf5d4c0f6fb0f8f978390e77f9685";
    fenix.url = "github:nix-community/fenix/380f1969f440e683333af5746caac76811b4a1a8";
    fenix.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, ... }@inputs: let
      supportedSystems = [ "aarch64-linux" "x86_64-linux" ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
  in {
    overlays.default = final: prev: rec {
      system = final.stdenv.hostPlatform.system;

      rustToolchainCI = (with inputs.fenix.packages.${system};
        combine [
          stable.rustc
          stable.cargo
          latest.rustfmt
        ]
      );
      rustToolchainDev = (with inputs.fenix.packages.${system};
        combine [
          stable.rustc
          stable.cargo
          stable.rust-analyzer
          stable.clippy
          latest.rustfmt
          prev.cargo-expand
        ]
      );
      rustToolchainStable = (with inputs.fenix.packages.${system};
        combine [
          stable.rustc
          stable.cargo  
        ]
      );
      rustToolchainNightly = (with inputs.fenix.packages.${system};
        combine [
          latest.rustc
          latest.cargo
          latest.rust-analyzer
          latest.rustfmt
          prev.cargo-expand
        ]
      );
    };

    devShells = forAllSystems (system: let
      pkgs = import inputs.nixpkgs {
        inherit system;
        overlays = [ self.overlays.default ];
      };
    in rec {
      default = dev;

      dev = pkgs.mkShell (rec {
        packages = with pkgs; [
          rustToolchainDev
          taplo
          evcxr
          vulkan-headers
          vulkan-loader
          pkg-config
          cmake
          fontconfig
          freetype
          libX11
          libXcursor
          libXrandr
          libXi
          libxkbcommon
          wayland
          wayland-protocols
          python3
        ];
        shellHook = ''
          export LD_LIBRARY_PATH="${pkgs.lib.makeLibraryPath packages}:$LD_LIBRARY_PATH"
        '';
      });
      ci = pkgs.mkShell {
        packages = with pkgs; [
          rustToolchainCI
          vulkan-headers
          vulkan-loader
          pkg-config
        ];
      };
    });
  };
}

