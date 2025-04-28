{
  description = "Arg-V Transformer for C Code";

  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, utils }: utils.lib.eachDefaultSystem (system:
    let pkgs = nixpkgs.legacyPackages.${system};
    in rec {
      packages = rec {
        argv-argc-transformer = pkgs.clangStdenv.mkDerivation {
          name = "argv-argc-transformer";

          src = ./.;

          nativeBuildInputs = with pkgs; [
            cmake
            ninja
            pkg-config
            clang
            gitMinimal
            llvmPackages.bintools
          ];
          buildInputs = with pkgs; [
            boost
            clang-tools
            clang
            libclang
            libllvm
          ];
        };
        default = argv-argc-transformer;
      };
      devShells.default = pkgs.mkShell {
        stdenv = pkgs.clangStdenv;
        inputsFrom = [ packages.argv-argc-transformer ];
      };
    });
}
