#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <xtd/forms/application.h>
#include <xtd/forms/button.h>
#include <xtd/forms/dialog_result.h>
#include <xtd/forms/folder_browser_dialog.h>
#include <xtd/forms/form.h>
#include <xtd/forms/label.h>
#include <xtd/forms/list_box.h>
#include <xtd/forms/message_box.h>
#include <xtd/forms/open_file_dialog.h>
#include <xtd/forms/progress_bar.h>
#include <xtd/forms/text_box.h>
#include <xtd/forms/timer.h>
#include <xtd/time_span.hpp>

namespace fs = std::filesystem;
using namespace xtd;
using namespace xtd::forms;

namespace {
std::string trim(const std::string& value) {
  const auto begin = value.find_first_not_of(" \t\n\r");
  if (begin == std::string::npos) return "";
  const auto end = value.find_last_not_of(" \t\n\r");
  return value.substr(begin, end - begin + 1);
}

std::string to_title_case(std::string value) {
  bool capitalize = true;
  for (char& c : value) {
    if (std::isspace(static_cast<unsigned char>(c))) {
      capitalize = true;
      continue;
    }
    c = static_cast<char>(capitalize ? std::toupper(static_cast<unsigned char>(c))
                                     : std::tolower(static_cast<unsigned char>(c)));
    capitalize = false;
  }
  return value;
}

std::string sanitize_filename(const std::string& input) {
  std::string out;
  out.reserve(input.size());
  for (char c : input) {
    if (std::isalnum(static_cast<unsigned char>(c)) || c == ' ' || c == '-' || c == '_') {
      out.push_back(c);
    }
  }
  out = trim(out);
  while (out.find("  ") != std::string::npos) {
    out.replace(out.find("  "), 2, " ");
  }
  if (out.empty()) return "Transcript";
  return out;
}

std::vector<std::string> tokenize_words(const std::string& text) {
  std::vector<std::string> tokens;
  std::string current;
  current.reserve(32);

  for (unsigned char ch : text) {
    if (std::isalnum(ch) || ch == '\'') {
      current.push_back(static_cast<char>(std::tolower(ch)));
    } else if (!current.empty()) {
      tokens.push_back(current);
      current.clear();
    }
  }

  if (!current.empty()) tokens.push_back(current);
  return tokens;
}

std::string build_content_title(const std::string& transcript) {
  static const std::set<std::string> stopwords = {
    "a",   "an",   "and", "are", "as",   "at", "be", "by", "for",  "from",
    "has", "he",   "in",  "is",  "it",   "its", "of", "on", "that", "the",
    "to",  "was",  "were", "will", "with", "we", "you", "i", "this", "they",
    "or",  "if",   "but", "so",  "not",  "do", "does", "did", "have", "had"
  };

  const auto words = tokenize_words(transcript);
  std::vector<std::string> selected;
  selected.reserve(6);

  for (size_t i = 0; i < words.size() && i < 120 && selected.size() < 6; ++i) {
    if (words[i].size() < 3) continue;
    if (stopwords.count(words[i]) > 0) continue;
    selected.push_back(words[i]);
  }

  if (selected.empty()) return "Transcript";

  std::ostringstream out;
  for (size_t i = 0; i < selected.size(); ++i) {
    if (i > 0) out << ' ';
    out << selected[i];
  }
  return to_title_case(out.str());
}

std::string shell_escape(const std::string& value) {
  std::string escaped = "'";
  for (char c : value) {
    if (c == '\'') {
      escaped += "'\\''";
    } else {
      escaped.push_back(c);
    }
  }
  escaped += "'";
  return escaped;
}

std::optional<std::string> read_all_text(const fs::path& file) {
  std::ifstream input(file);
  if (!input) return std::nullopt;
  std::ostringstream content;
  content << input.rdbuf();
  return content.str();
}

bool write_all_text(const fs::path& file, const std::string& text) {
  std::ofstream output(file, std::ios::trunc);
  if (!output) return false;
  output << text;
  return output.good();
}

std::string now_compact() {
  const auto now = std::chrono::system_clock::now();
  const auto t = std::chrono::system_clock::to_time_t(now);
  std::tm local_tm {};
#if defined(_WIN32)
  localtime_s(&local_tm, &t);
#else
  localtime_r(&t, &local_tm);
#endif
  std::ostringstream out;
  out << std::put_time(&local_tm, "%Y%m%d_%H%M%S");
  return out.str();
}

bool run_whisper(const fs::path& audio_file,
                 const std::string& whisper_cli,
                 const fs::path& model,
                 const std::string& language,
                 std::string& transcript,
                 std::string& error) {
  const auto temp_base = fs::temp_directory_path() / ("xtd_audio_loop_" + now_compact());

  std::ostringstream cmd;
  cmd << shell_escape(whisper_cli)
      << " -m " << shell_escape(model.string())
      << " -f " << shell_escape(audio_file.string())
      << " -otxt"
      << " -of " << shell_escape(temp_base.string());

  if (!trim(language).empty()) {
    cmd << " -l " << shell_escape(trim(language));
  }

  const int result = std::system(cmd.str().c_str());
  if (result != 0) {
    std::ostringstream err;
    err << "whisper-cli failed with exit code " << result;
    error = err.str();
    return false;
  }

  const auto output_txt = temp_base.string() + ".txt";
  const auto loaded = read_all_text(output_txt);
  if (!loaded.has_value()) {
    error = "Whisper completed, but no transcript file was found.";
    return false;
  }

  transcript = loaded.value();
  std::error_code ec;
  fs::remove(output_txt, ec);
  return true;
}
}  // namespace

class loop_form : public form {
public:
  ~loop_form() override {
    if (worker_.joinable()) worker_.join();
  }

  loop_form() {
    text("Audio Batch Convert Loop (xtd + whisper.cpp)");
    client_size({980, 680});
    minimum_size({980, 680});

    lbl_cli_.parent(*this);
    lbl_cli_.text("whisper-cli path:");
    lbl_cli_.location({16, 16});
    lbl_cli_.auto_size(true);

    txt_cli_.parent(*this);
    txt_cli_.location({160, 12});
    txt_cli_.size({790, 28});
    txt_cli_.text("whisper-cli");

    lbl_model_.parent(*this);
    lbl_model_.text("Model path:");
    lbl_model_.location({16, 52});
    lbl_model_.auto_size(true);

    txt_model_.parent(*this);
    txt_model_.location({160, 48});
    txt_model_.size({680, 28});

    btn_model_.parent(*this);
    btn_model_.text("Browse");
    btn_model_.location({850, 48});
    btn_model_.size({100, 28});
    btn_model_.click += [&] { choose_model(); };

    lbl_lang_.parent(*this);
    lbl_lang_.text("Language:");
    lbl_lang_.location({16, 88});
    lbl_lang_.auto_size(true);

    txt_lang_.parent(*this);
    txt_lang_.location({160, 84});
    txt_lang_.size({100, 28});
    txt_lang_.text("en");

    lbl_source_.parent(*this);
    lbl_source_.text("Source audio files:");
    lbl_source_.location({16, 126});
    lbl_source_.auto_size(true);

    btn_source_.parent(*this);
    btn_source_.text("Select Files");
    btn_source_.location({160, 122});
    btn_source_.size({140, 34});
    btn_source_.click += [&] { choose_audio_files(); };

    lbl_dest_.parent(*this);
    lbl_dest_.text("Destination folder:");
    lbl_dest_.location({316, 130});
    lbl_dest_.auto_size(true);

    txt_dest_.parent(*this);
    txt_dest_.location({454, 126});
    txt_dest_.size({386, 28});

    btn_dest_.parent(*this);
    btn_dest_.text("Browse");
    btn_dest_.location({850, 126});
    btn_dest_.size({100, 28});
    btn_dest_.click += [&] { choose_destination(); };

    btn_convert_.parent(*this);
    btn_convert_.text("Convert");
    btn_convert_.location({16, 166});
    btn_convert_.size({140, 34});
    btn_convert_.click += [&] { convert_all(); };

    progress_.parent(*this);
    progress_.location({160, 171});
    progress_.size({790, 24});
    progress_.minimum(0);
    progress_.maximum(100);
    progress_.value(0);

    lbl_files_.parent(*this);
    lbl_files_.text("Queued source files:");
    lbl_files_.location({16, 210});
    lbl_files_.auto_size(true);

    list_files_.parent(*this);
    list_files_.location({16, 236});
    list_files_.size({934, 190});

    lbl_done_.parent(*this);
    lbl_done_.text("Converted files:");
    lbl_done_.location({16, 440});
    lbl_done_.auto_size(true);

    list_done_.parent(*this);
    list_done_.location({16, 466});
    list_done_.size({934, 100});

    lbl_log_.parent(*this);
    lbl_log_.text("Log:");
    lbl_log_.location({16, 574});
    lbl_log_.auto_size(true);

    txt_log_.parent(*this);
    txt_log_.location({16, 598});
    txt_log_.size({934, 70});
    txt_log_.multiline(true);
    txt_log_.read_only(true);

    ui_timer_.interval(xtd::time_span::from_milliseconds(100));
    ui_timer_.tick += [&] { on_ui_tick(); };
    ui_timer_.enabled(true);

    append_log("Select source files and destination folder, then click Convert.");
  }

private:
  struct worker_result {
    int success = 0;
    int total = 0;
  };

  void append_log(const std::string& message) {
    std::ostringstream line;
    line << "[" << now_compact() << "] " << message << "\r\n";
    txt_log_.append_text(line.str());
  }

  void queue_log(const std::string& message) {
    std::lock_guard<std::mutex> lock(worker_mutex_);
    pending_logs_.push_back(message);
  }

  void on_ui_tick() {
    std::deque<std::string> logs;
    std::deque<std::string> done_files;
    bool done = false;
    int progress = 0;
    std::optional<worker_result> result;

    {
      std::lock_guard<std::mutex> lock(worker_mutex_);
      logs.swap(pending_logs_);
      done_files.swap(done_files_pending_);
      done = worker_done_;
      progress = worker_progress_;
      if (worker_result_.has_value()) {
        result = worker_result_;
        worker_result_.reset();
      }
    }

    for (const auto& log : logs) append_log(log);
    for (const auto& f : done_files) list_done_.items().push_back(f);

    if (progress_.maximum() > 0) {
      progress_.value(std::clamp(progress, progress_.minimum(), progress_.maximum()));
    }

    if (done && result.has_value()) finish_worker_ui(result.value());
  }

  void choose_model() {
    open_file_dialog dialog;
    dialog.filter("Whisper model (*.bin)|*.bin|All files (*.*)|*.*");
    if (dialog.show_dialog() == dialog_result::ok) txt_model_.text(dialog.file_name());
  }

  void choose_audio_files() {
    open_file_dialog dialog;
    dialog.multiselect(true);
    dialog.filter(
      "Audio files (*.wav;*.mp3;*.m4a;*.flac;*.ogg)|*.wav;*.mp3;*.m4a;*.flac;*.ogg|All files (*.*)|*.*");

    if (dialog.show_dialog() != dialog_result::ok) return;

    selected_files_.clear();
    list_files_.items().clear();

    for (const auto& file_name : dialog.file_names()) {
      const fs::path p = std::string(file_name.c_str());
      selected_files_.push_back(p);
      list_files_.items().push_back(p.string());
    }

    append_log("Selected " + std::to_string(selected_files_.size()) + " source file(s).");
  }

  void choose_destination() {
    folder_browser_dialog dialog;
    dialog.description("Select destination folder for transcript files");
    if (dialog.show_dialog() != dialog_result::ok) return;
    txt_dest_.text(dialog.selected_path());
    append_log("Destination set: " + std::string(txt_dest_.text().c_str()));
  }

  void finish_worker_ui(const worker_result& result) {
    if (worker_.joinable()) worker_.join();

    btn_source_.enabled(true);
    btn_dest_.enabled(true);
    btn_model_.enabled(true);
    btn_convert_.enabled(true);

    std::ostringstream summary;
    summary << "Completed. " << result.success << "/" << result.total << " file(s) converted.";
    append_log(summary.str());

    const auto icon = result.success == result.total ? message_box_icon::information : message_box_icon::warning;
    message_box::show(*this, summary.str(), "Conversion Result", message_box_buttons::ok, icon);

    const auto loop_choice = message_box::show(
      *this,
      "Do you want to select more files and run another conversion batch?",
      "Convert More?",
      message_box_buttons::yes_no,
      message_box_icon::question
    );

    if (loop_choice == dialog_result::yes) {
      choose_audio_files();
    }
  }

  void convert_all() {
    if (worker_.joinable() && !worker_running_) worker_.join();

    if (worker_running_) {
      message_box::show(*this, "A conversion job is already running.", "Busy", message_box_buttons::ok,
                        message_box_icon::information);
      return;
    }

    if (selected_files_.empty()) {
      message_box::show(*this, "Please select one or more source audio files.", "Source missing",
                        message_box_buttons::ok, message_box_icon::warning);
      return;
    }

    const fs::path destination = trim(std::string(txt_dest_.text().c_str()));
    if (destination.empty() || !fs::exists(destination) || !fs::is_directory(destination)) {
      message_box::show(*this, "Please select a valid destination folder.", "Destination missing",
                        message_box_buttons::ok, message_box_icon::warning);
      return;
    }

    const fs::path model_path = trim(std::string(txt_model_.text().c_str()));
    if (model_path.empty() || !fs::exists(model_path)) {
      message_box::show(*this, "Please set a valid whisper model (.bin) path.", "Model missing",
                        message_box_buttons::ok, message_box_icon::warning);
      return;
    }

    const std::string whisper_cli = trim(std::string(txt_cli_.text().c_str()));
    if (whisper_cli.empty()) {
      message_box::show(*this, "Please set whisper-cli executable path.", "CLI missing",
                        message_box_buttons::ok, message_box_icon::warning);
      return;
    }

    btn_source_.enabled(false);
    btn_dest_.enabled(false);
    btn_model_.enabled(false);
    btn_convert_.enabled(false);

    progress_.minimum(0);
    progress_.maximum(static_cast<int>(selected_files_.size()));
    progress_.value(0);

    {
      std::lock_guard<std::mutex> lock(worker_mutex_);
      pending_logs_.clear();
      worker_progress_ = 0;
      worker_done_ = false;
      worker_result_.reset();
    }

    worker_running_ = true;
    queue_log("Starting asynchronous conversion...");

    const auto files = selected_files_;
    const auto cli = whisper_cli;
    const auto model = model_path;
    const auto language = std::string(txt_lang_.text().c_str());
    const auto dest = destination;

    worker_ = std::thread([this, files, cli, model, language, dest] {
      int success = 0;

      for (size_t i = 0; i < files.size(); ++i) {
        const auto& audio = files[i];
        queue_log("Converting: " + audio.string());

        std::string transcript;
        std::string error;
        if (!run_whisper(audio, cli, model, language, transcript, error)) {
          queue_log("Failed: " + error);
          std::lock_guard<std::mutex> lock(worker_mutex_);
          worker_progress_ = static_cast<int>(i + 1);
          continue;
        }

        const auto content_title = sanitize_filename(build_content_title(transcript));
        const auto out_name = audio.stem().string() + " - " + content_title + ".txt";
        const auto out_path = dest / out_name;

        if (!write_all_text(out_path, transcript)) {
          queue_log("Failed to write: " + out_path.string());
          std::lock_guard<std::mutex> lock(worker_mutex_);
          worker_progress_ = static_cast<int>(i + 1);
          continue;
        }

        ++success;
        queue_log("Converted: " + audio.filename().string() + " -> " + out_path.string());
        list_done_safe_add(out_path.string());

        std::lock_guard<std::mutex> lock(worker_mutex_);
        worker_progress_ = static_cast<int>(i + 1);
      }

      {
        std::lock_guard<std::mutex> lock(worker_mutex_);
        worker_result_ = worker_result{success, static_cast<int>(files.size())};
        worker_done_ = true;
      }
      worker_running_ = false;
    });
  }

  void list_done_safe_add(const std::string& file) {
    std::lock_guard<std::mutex> lock(worker_mutex_);
    done_files_pending_.push_back(file);
  }

  label lbl_cli_;
  text_box txt_cli_;

  label lbl_model_;
  text_box txt_model_;
  button btn_model_;

  label lbl_lang_;
  text_box txt_lang_;

  label lbl_source_;
  button btn_source_;

  label lbl_dest_;
  text_box txt_dest_;
  button btn_dest_;

  button btn_convert_;
  progress_bar progress_;

  label lbl_files_;
  list_box list_files_;

  label lbl_done_;
  list_box list_done_;

  label lbl_log_;
  text_box txt_log_;

  std::vector<fs::path> selected_files_;
  timer ui_timer_;

  std::thread worker_;
  std::mutex worker_mutex_;
  std::deque<std::string> pending_logs_;
  std::deque<std::string> done_files_pending_;
  std::optional<worker_result> worker_result_;
  std::atomic<bool> worker_running_ {false};
  int worker_progress_ = 0;
  bool worker_done_ = false;

};

int main() {
  application::enable_visual_styles();

  loop_form form;
  form.text("Audio Batch Convert Loop (xtd + whisper.cpp)");
  application::run(form);
}
