let
  pkgs =
    import
      (
        fetchTarball (
          builtins.fromJSON (
            builtins.readFile ./nixpkgs.json
          )
        )
      )
      { };
in
pkgs.mkShell {
  packages = [
    pkgs.cacert
    pkgs.cmake
    pkgs.docker
    pkgs.dpkg
    pkgs.fakeroot
    pkgs.gitFull
    pkgs.gnumake
    pkgs.nodejs
    pkgs.perl
    pkgs.pre-commit
    pkgs.which
    pkgs.zlib
  ];
}
