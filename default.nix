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
pkgs.stdenv.mkDerivation {
  name = "custom-whisper-server";
  version = "0.1";
  buildInputs = with pkgs; [
    cmake
    httplib
    nlohmann_json
    whisper-cpp
    yt-dlp
  ];
  src = ./.;

  installPhase = ''
    mkdir -p $out $out/bin
    cp custom-whisper-server $out/bin
    cp ${whisper-cpp}/bin/*.so $out/bin
  '';
}
