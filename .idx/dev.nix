{ pkgs, ... }: {
  channel = "stable-23.11";
  packages = [
    pkgs.gcc
    pkgs.cmake
    pkgs.gnumake
    pkgs.gtest
  ];
  idx.extensions = [
    "ms-vscode.cpptools"
    "twxs.cmake"
    "ms-vscode.cmake-tools"
  ];
}