{
  description = "Example vulkan triangle";

  nixConfig = {
    extra-substituters = [
      "https://cache.nixos.org"
      "https://nix-community.cachix.org"
    ];
    extra-trusted-public-keys = [
      "cache.nixos.org-1:6NCHdD59X431o0gWypbMrAURkbJ16ZPMQFGspcDShjY="
      "nix-community.cachix.org-1:mB9FSh9qf2dCimDSUo8Zy7bkq5CX+/rkCWyvRCYg3Fs="
    ];
  };

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.05";
    #nixpkgs-unstable.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs = inputs@{ self, nixpkgs, ... }:
  let
    
    nativeBuildInputs = (pkgs: with pkgs.buildPackages; [ cmake pkg-config ]);
    
    package = { 
      lib
    , stdenv
    , cmake
    , shaderc
    , pkg-config
    , vulkan-loader
    , vulkan-headers
    , vulkan-validation-layers
    #, glfw-wayland
    , SDL2
    , glm
    }@pkgs: stdenv.mkDerivation {
      name = "Vulkan app";
      src = lib.fileset.toSource {
        root = ./.;
        fileset = lib.fileset.unions [
          ./CMakeLists.txt
          ./src
          ./shaders
        ];
      };
      #nativeBuildInputs = [buildPackages.cmake buildPackages.pkg-config];  #nativeBuildInputs pkgs;
      nativeBuildInputs = [cmake pkg-config shaderc];  #nativeBuildInputs pkgs;
      buildInputs = [
        vulkan-headers vulkan-loader.dev vulkan-validation-layers
        #glfw-wayland
        SDL2
        glm
      ];
      installPhase = ''
        mkdir $out
        cp . $out -Tr
        #cp ./main* $out
      '';
    };

    buildSystems = [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];

    #crossSystems = {
    #  x86_64-linux   = { config = "x86_64-unknown-linux-gnu";  libc = "glibc"; };
    #  i686-linux     = { config = "x86_64-unknown-linux-gnu";  libc = "glibc"; };
    #  aarch64-linux  = { config = "aarch64-unknown-linux-gnu"; libc = "glibc"; };
    #  
    #  x86_64-darwin  = { config = "x86_64-unknown-linux-gnu";  libc = "libSystem"; };
    #  aarch64-darwin = { config = "aarch64-unknown-linux-gnu"; libc = "libSystem"; };

    #  x86_64-windows = { config = "x86_64-w64-mingw32"; libc = "msvcrt"; };
    #  i686-windows   = { config = "i686-w64-mingw32";   libc = "msvcrt"; };
    #};

    forAllSystems = nixpkgs.lib.genAttrs [
      "x86_64-linux"
      "aarch64-linux"
      "x86_64-darwin"
      "aarch64-darwin"
    ];
  in {

    devShells = forAllSystems (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        default = pkgs.mkShell {
          packages = with pkgs; [
          ];
          nativeBuildInputs = with pkgs; [
            shaderc #shaderc.bin #shaderc.static shaderc.dev shaderc.lib
            cmake
            pkg-config
            clang-tools
            vulkan-tools
          ];
          buildInputs = with pkgs; [
            vulkan-headers vulkan-loader.dev vulkan-validation-layers
            #glfw-wayland
            SDL2
            glm 
          ];
          shellHook = ''
            export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
          '';
        };
      }
    );

    packages = forAllSystems (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        default = (with pkgs; callPackage package {});
      }
    );
   
  };
}
