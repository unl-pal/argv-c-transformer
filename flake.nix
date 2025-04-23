{
  description = "Arg-V Transformer for C Code";

  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, utils }: utils.lib.eachDefaultSystem (system:
    let pkgs = nixpkgs.legacyPackages.${system};
    in rec {
      packages.default = pkgs.clangStdenv.mkDerivation {
        name = "argv-c-transformer";

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
      devShells.default = pkgs.mkShell {
        stdenv = pkgs.clangStdenv;
        inputsFrom = [ packages.default ];
      };
    });
}
