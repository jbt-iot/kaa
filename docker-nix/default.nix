let
  nixpkgs-bootstrap = import <nixpkgs> { };

  # How to update nixpkgs version:
  # - go to https://github.com/NixOS/nixpkgs-channels
  # - select branch you want to track
  # - get latest commit hash -- this goes to `rev` field
  # - execute `nix-prefetch-url --unpack https://github.com/NixOS/nixpkgs-channels/archive/<rev>.tar.gz`
  # - output of the previous command goes to `sha256` field
  nixpkgs-16_09 = import (nixpkgs-bootstrap.fetchFromGitHub {
    owner = "NixOS";
    repo = "nixpkgs-channels";
    rev = "4e14fd5d5aac14a17c28465104b7ffacf27d9579";
    sha256 = "0mz62lg25hwvhb85yfhn0vy7biws7j4dq9bg33dbam5kj17f0lca";
  }) { };

in

{ pkgs ? nixpkgs-16_09
}:
let
  callPackage = pkgs.lib.callPackageWith (pkgs // self);

  self = {
    # We use a custom Kaa C++ SDK (various fixes, etc.)
    #
    # To update SDK on the server, put result file into
    # /usr/lib/kaa-node/sdk/cpp/ directory and restart Kaa node.
    #
    # NOTE: this build requires network, so does not work with
    # sandboxing. Use the following command to disable it:
    # nix-build -A kaa-cpp-sdk --option build-use-sandbox false
    kaa-cpp-sdk = callPackage ./kaa-c++-sdk { };

    # NOTE: this build requires network, so does not work with
    # sandboxing. Use the following command to disable it:
    # nix-build -A kaa --option build-use-sandbox false
    kaa = callPackage ./kaa { };
  };

in self
