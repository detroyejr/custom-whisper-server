// Custom Whisper Server

#include "common-whisper.h"
#include "httplib.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ggml-backend.h>
#include <map>
#include <nlohmann/json.hpp>
#include <regex>
#include <vector>
#include <whisper.h>

httplib::Server server;

std::string download_url(std::string url, std::string paths) {

  std::string cmd = {"yt-dlp "
                     "--audio-quality 0 "
                     "--audio-format wav "
                     "--extract-audio "
                     "--paths "};

  cmd.append(paths + " " + url);

  FILE *pipe = popen(cmd.c_str(), "r");
  char buffer[128];
  std::string out;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    std::cout << buffer << std::endl;
    out.append(buffer + std::string("\n"));
  };

  std::regex re{"\\[ExtractAudio\\] Destination: (.*?).wav"};
  auto words_begin = std::sregex_iterator(out.begin(), out.end(), re);
  auto fname = std::regex_replace((*words_begin).str(), re, "$1");
  return fname.c_str();
}

void transcribe(httplib::Request req, whisper_context_params cparams,
                whisper_full_params wparams,
                std::map<std::string, std::string> args) {
  auto j = nlohmann::json::parse(req.body.c_str());
  if (j.contains("url")) {

    auto fname = download_url(j.at("url"), args["paths"]);
    std::vector<float> pcmf32;
    std::vector<std::vector<float>> pcmf32s;
    std::ofstream transcript{fname + ".txt"};

    transcript.clear();
    read_audio_data(fname + ".wav", pcmf32, pcmf32s, false);

    struct whisper_context *ctx =
        whisper_init_from_file_with_params(args["model"].c_str(), cparams);

    if (whisper_full(ctx, wparams, pcmf32.data(), pcmf32.size()) != 0) {
      fprintf(stderr, "failed to process audio\n");
    }

    const int n_segments = whisper_full_n_segments(ctx);

    std::string result;
    for (int i = 0; i < n_segments; ++i) {
      const char *text = whisper_full_get_segment_text(ctx, i);
      transcript << text << std::endl;
    }

    // Cleanup
    transcript.close();
    whisper_free(ctx);

    if (std::filesystem::exists(fname + ".wav")) {
      std::filesystem::remove(fname + ".wav");
    }

    if (std::filesystem::exists(fname + ".mkv")) {
      std::filesystem::remove(fname + ".mkv");
    }

    if (std::filesystem::exists(fname + ".txt")) {
      std::filesystem::copy_file(fname + ".txt",
                                 args["out-directory"] + "/" + fname + ".md");
      std::filesystem::remove(fname + ".txt");
    }

    std::cout << "Completed: " << fname.c_str() << std::endl;
  }
}

int main(const int argc, const char *argv[]) {

  std::map<std::string, std::string> args;

  // Defaults
  args["model"] = "";
  args["out-directory"] = "";
  args["paths"] = ".";
  args["port"] = "8080";
  args["threads"] = "4";
  args["url"] = "";

  for (int i = 0; i < argc; i++) {
    std::string arg = argv[i];
    if (arg.substr(0, 2) == "--") {
      if (i + 1 < argc) {
        if (args.find(arg.substr(2)) == args.end()) {
          std::cout << "Error: " << arg.substr(2) + " argument not found."
                    << std::endl;
          exit(1);
        };
        std::string option = argv[i + 1];
        if (i < argc && option.substr(0, 2) != "--") {
          args[arg.substr(2)] = option;
        } else {
          args[arg.substr(2)] = "";
        }
      }
    }
  }

  if (args["model"] == "") {
    std::cout << "Error: model " << args["model"] << " not found." << std::endl;
    exit(1);
  }

  ggml_backend_load_all();
  const whisper_context_params cparams = whisper_context_default_params();

  whisper_full_params wparams =
      whisper_full_default_params(WHISPER_SAMPLING_GREEDY);
  wparams.n_threads = std::max(2, std::stoi(args["threads"]));
  wparams.strategy = WHISPER_SAMPLING_GREEDY;

  server.Post("/", [&](const httplib::Request &req, httplib::Response &res) {
    res.status = httplib::StatusCode::Created_201;
    std::thread t(transcribe, req, cparams, wparams, args);
    if (!t.joinable()) {
      res.status = httplib::StatusCode::InternalServerError_500;
    }
    t.detach();
    return 0;
  });

  server.listen("0.0.0.0", std::stoi(args["port"]));
  return 0;
}
