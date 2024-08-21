#include "display_ffmpeg.hpp"

#include "bee/file_path.hpp"
#include "bee/filesystem.hpp"
#include "bee/format.hpp"
#include "bee/sub_process.hpp"

namespace heaven_ice {
namespace {

struct DisplayFfmpegImpl final : public DisplayIntf {
  DisplayFfmpegImpl(int scale) : _scale(scale) {}
  virtual ~DisplayFfmpegImpl()
  {
    if (_input_pipe) {
      _input_pipe->close();
      must_unit(_ffmpeg_proc->wait());
    }
  }

  void update(const pixel::Image& img) override
  {
    if (_scale != 1) {
      _update_impl(img.upscale(_scale));
    } else {
      _update_impl(img);
    }
  }

  std::vector<sdl::Event> get_events() override { return {}; }

 private:
  void _init_proc(int height, int width)
  {
    auto input_pipe = bee::SubProcess::Pipe::create();
    must_assign(
      _ffmpeg_proc,
      bee::SubProcess::spawn({
        .cmd = bee::FilePath("ffmpeg"),
        .args =
          {
            "-f",
            "rawvideo",
            "-pixel_format",
            "rgb24",
            "-video_size",
            F("$x$", width, height),
            "-framerate",
            "60",
            "-i",
            "-",
            "-vcodec",
            "libx264",
            "-crf",
            "22",
            "-preset",
            "veryslow",
            "-y",
            "test.mkv",
          },
        .stdin_spec = input_pipe,
      }));
    _input_pipe = input_pipe->fd();
  }

  void _update_impl(const pixel::Image& img)
  {
    if (_input_pipe == nullptr) { _init_proc(img.height(), img.width()); }
    must_unit(_input_pipe->write(
      reinterpret_cast<const char*>(img.data()),
      img.height() * img.width() * 3));
  }

  bee::FD::shared_ptr _input_pipe;
  bee::SubProcess::ptr _ffmpeg_proc;

  int _scale;
};

} // namespace

DisplayIntf::ptr DisplayFfmpeg::create(int scale)
{
  return std::make_shared<DisplayFfmpegImpl>(scale);
}

} // namespace heaven_ice
