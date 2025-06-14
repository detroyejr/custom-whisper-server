let
  pkgs = import <nixpkgs> { };
  whisper-cpp = pkgs.whisper-cpp.overrideAttrs (attrs: {
    src = pkgs.fetchFromGitHub {
      owner = "ggml-org";
      repo = "whisper.cpp";
      rev = "b175baa665bc35f97a2ca774174f07dfffb84e19";
      hash = "sha256-GtGzUNpIbS81z7SXFolT866fGfdSjdyf9R+PKlK6oYs=";
    };
  });
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    ffmpeg
    gdb
    httplib
    meson
    miniaudio
    ninja
    nlohmann_json
    openssl
    openvino
    pkg-config
    whisper-cpp
    yt-dlp
  ];
}
