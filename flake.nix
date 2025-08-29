{
  description = "Arg-V Transformer for C Code";

  inputs = {
    utils.url = "github:numtide/flake-utils";
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = { self, nixpkgs, utils }: utils.lib.eachDefaultSystem (system:
    let
      pkgs = nixpkgs.legacyPackages.${system};
      inherit (pkgs) lib;
      # getFetchContentFlags taken from:
      # https://github.com/FreeRTOS/Lab-Project-ota-example-for-AWS-IoT-Core/blob/c2fac602462395213e6011c47e1c9b5e81313795/flake.nix#L43
      getFetchContentFlags = file:
        let
          inherit (builtins) head elemAt match;
          parse = match
            "(.*)\nFetchContent_Declare\\(\n  ([^\n]*)\n([^)]*)\\).*"
            file;
          name = elemAt parse 1;
          content = elemAt parse 2;
          getKey = key: elemAt
            (match "(.*\n)?  ${key} ([^\n]*)(\n.*)?" content) 1;
          url = getKey "GIT_REPOSITORY";
          pkg = pkgs.fetchFromGitHub {
            owner = head (match ".*github.com/([^/]*)/.*" url);
            repo = head (match ".*/([^/]*)\\.git" url);
            rev = getKey "GIT_TAG";
            hash = getKey "# hash:";
          };
        in
        if (parse == null) then [ ] else
        ([ "-DFETCHCONTENT_SOURCE_DIR_${lib.toUpper name}=${pkg}" ] ++
          getFetchContentFlags (head parse));
    in
    rec {
      packages.default = pkgs.clangStdenv.mkDerivation {
        name = "argv-c-transformer";

        src = ./.;

        cmakeFlags = getFetchContentFlags (builtins.readFile ./CMakeLists.txt);

        nativeBuildInputs = with pkgs;
          [
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
