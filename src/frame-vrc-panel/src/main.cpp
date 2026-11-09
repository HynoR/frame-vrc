// frame-vrc-panel: a SteamVR dashboard panel for frame-vrc (every button runs the frame-vrc CLI). Not resident:
// opened from the app list, it ends when closed from the dashboard or when SteamVR quits.
// Structure (VR connect / retry, single instance, render-when-dirty) follows frameclash-panel and frameeyeosc's panel
// (MIT, Copyright (c) 2026 konsti219; Copyright (c) 2026 sasaken1102r).
// Copyright (c) 2026 FrameClash contributors. MIT License.
#include "model.h"
#include "ui.h"
#include "vr_overlay.h"

#include <cairo.h>
#include <limits.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kExitClosed = 3;  // closed by the user / already running
constexpr int kRetryConnectMs = 5000;
constexpr const char* kInstanceSocket = "frame-vrc-panel";  // abstract unix socket name

std::atomic<bool> gStop {false};

void onSignal(int) {
    gStop = true;
}

struct Options {
    std::string cli;
    std::string dumpPng, thumbPng, iconPng;
    int iconSize = 256;
    std::vector<std::string> steps;
    bool listButtons = false;
    double scroll = 0;
    bool show = false;
    int lang = -1;  ///< -1 saved choice / LANG, 0 en, 1 ja
};

void usage() {
    std::puts(
        "frame-vrc-panel " FRAME_VRC_PANEL_VERSION " - SteamVR dashboard panel for frame-vrc\n"
        "usage: frame-vrc-panel [--show] [--cli PATH] [--lang en|ja]\n"
        "  --show: open the dashboard on the panel (a running instance is asked to do it)\n"
        "  --cli: the frame-vrc script (default: frame-vrc next to this binary)\n"
        "       frame-vrc-panel --dump-png OUT.png [--do STEP]... [--scroll PX] [--list-buttons] [--thumb-png OUT.png]\n"
        "       frame-vrc-panel --icon-png SIZE OUT.png\n"
        "steps: press:ID (see --list-buttons) | wait:MS | idle (until nothing is waiting or running)\n"
        "Exit codes: 0 SteamVR quit, 3 closed from the dashboard / another instance already running, 1 error.");
}

bool loadFonts(FontSet& fonts) {
    // The Frame (Arch: noto-fonts-cjk) and Debian/Ubuntu (fonts-noto-cjk) paths; FontSet falls back to fontconfig
    const char* dirs[] = {"/usr/share/fonts/noto-cjk/", "/usr/share/fonts/opentype/noto/"};
    for (const char* dir : dirs) {
        const std::string regular = std::string(dir) + "NotoSansCJK-Regular.ttc";
        if (::access(regular.c_str(), R_OK) == 0) {
            fonts.load(regular, std::string(dir) + "NotoSansCJK-Bold.ttc");
            return true;
        }
    }
    fonts.load("", "");
    return false;
}

std::string stateDir() {
    const char* home = std::getenv("HOME");
    return std::string(home != nullptr ? home : "/tmp") + "/.local/share/frame-vrc/panel";
}

/** Japanese when chosen on the panel before, else when LANG starts with ja. */
bool japanese() {
    std::FILE* f = std::fopen((stateDir() + "/lang").c_str(), "r");
    if (f != nullptr) {
        char buf[8] = {};
        const bool ja = std::fgets(buf, sizeof(buf), f) != nullptr && std::strncmp(buf, "ja", 2) == 0;
        std::fclose(f);
        return ja;
    }
    const char* lang = std::getenv("LANG");
    return lang != nullptr && std::strncmp(lang, "ja", 2) == 0;
}

std::string defaultCli() {
    char buf[PATH_MAX] = {};
    const ssize_t n = ::readlink("/proc/self/exe", buf, sizeof(buf) - 1);
    std::string exe = n > 0 ? std::string(buf, static_cast<size_t>(n)) : "";
    return exe.substr(0, exe.rfind('/') + 1) + "frame-vrc";
}

void waitIdle(Model& model, int maxMs) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    const int64_t until = nowMs() + maxMs;
    while (!model.idle() && nowMs() < until) std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

bool writePng(const std::string& path, int w, int h, const std::function<void(cairo_t*)>& paint) {
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
    cairo_t* cr = cairo_create(surface);
    paint(cr);
    cairo_destroy(cr);
    const bool ok = cairo_surface_write_to_png(surface, path.c_str()) == CAIRO_STATUS_SUCCESS;
    cairo_surface_destroy(surface);
    if (!ok) std::fprintf(stderr, "can't write %s\n", path.c_str());
    return ok;
}

/** --dump-png: drive the real UI against a frame-vrc CLI (a fake one in tools/panel-preview), save a picture. */
int runDump(const Options& opt) {
    FontSet fonts;
    loadFonts(fonts);
    Model model(opt.cli, stateDir());
    PanelUi ui(model, fonts);
    ui.setLanguage(opt.lang >= 0 ? opt.lang == 1 : japanese(), "");
    cairo_surface_t* scratch = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kPanelWidth, kPanelHeight);
    auto redraw = [&] {
        cairo_t* cr = cairo_create(scratch);
        ui.draw(cr, model.snapshot());
        cairo_destroy(cr);
    };
    waitIdle(model, 120000);
    redraw();
    for (const std::string& step : opt.steps) {
        if (step.rfind("press:", 0) == 0) {
            if (!ui.press(step.substr(6))) std::fprintf(stderr, "step \"%s\": no such enabled button now\n", step.c_str());
        } else if (step == "idle") {
            waitIdle(model, 120000);
        } else if (step.rfind("wait:", 0) == 0) {
            // the Home page samples once a second meanwhile
            for (const int64_t until = nowMs() + std::atoi(step.c_str() + 5); nowMs() < until;) {
                ui.tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            }
        } else {
            std::fprintf(stderr, "unknown step: %s\n", step.c_str());
            return 2;
        }
        if (step.rfind("press:view:", 0) == 0) waitIdle(model, 120000);
        redraw();
    }
    if (opt.scroll != 0) {
        ui.scroll(opt.scroll);
        redraw();
    }
    const Snapshot snap = model.snapshot();
    const bool ok = writePng(opt.dumpPng, kPanelWidth, kPanelHeight, [&](cairo_t* cr) { ui.draw(cr, snap); });
    if (opt.listButtons)
        for (const Hit& h : ui.hits())
            std::printf("%-36s %4.0f,%-4.0f %4.0fx%-3.0f %s\n", h.id.c_str(), h.x, h.y, h.w, h.h,
                        h.enabled ? "enabled" : "disabled");
    if (!opt.thumbPng.empty())
        writePng(opt.thumbPng, kThumbnailSize, kThumbnailSize, [&](cairo_t* cr) { ui.drawThumbnail(cr, snap); });
    cairo_surface_destroy(scratch);
    std::printf("state: known=%d checks=%zu running=%d result=%s \"%s\"\n", snap.status.known ? 1 : 0, snap.checks.size(),
                snap.running ? 1 : 0, snap.haveResult ? (snap.resultOk ? "ok" : "failed") : "-", snap.resultLine.c_str());
    return ok ? 0 : 1;
}

int runIcon(const Options& opt) {
    FontSet fonts;
    loadFonts(fonts);
    return writePng(opt.iconPng, opt.iconSize, opt.iconSize, [&](cairo_t* cr) {
               const double s = opt.iconSize / static_cast<double>(kThumbnailSize);
               cairo_scale(cr, s, s);
               drawAppIcon(cr, fonts);
           })
               ? 0
               : 1;
}

/** Single instance: listen on an abstract socket; a second launch asks the first to show the panel. */
int claimInstance(bool sendShow, bool& otherRunning) {
    otherRunning = false;
    sockaddr_un addr {};
    addr.sun_family = AF_UNIX;
    const size_t n = std::strlen(kInstanceSocket);
    std::memcpy(addr.sun_path + 1, kInstanceSocket, n);  // sun_path[0] = 0: abstract (nothing on disk)
    const socklen_t len = static_cast<socklen_t>(offsetof(sockaddr_un, sun_path) + 1 + n);
    const int fd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (fd < 0) return -1;
    if (::bind(fd, reinterpret_cast<sockaddr*>(&addr), len) == 0 && ::listen(fd, 4) == 0) return fd;
    ::close(fd);
    otherRunning = true;
    const int c = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sendShow && c >= 0 && ::connect(c, reinterpret_cast<sockaddr*>(&addr), len) == 0) {
        ssize_t ignored = ::write(c, "show\n", 5);
        (void)ignored;
    }
    if (c >= 0) ::close(c);
    return -1;
}

bool showRequested(int listenFd) {
    bool show = false;
    for (;;) {
        const int c = ::accept4(listenFd, nullptr, nullptr, SOCK_CLOEXEC);
        if (c < 0) break;
        char buf[16] = {};
        struct timeval tv {0, 200000};
        ::setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        if (::read(c, buf, sizeof(buf) - 1) > 0 && std::strncmp(buf, "show", 4) == 0) show = true;
        ::close(c);
    }
    return show;
}

int runVr(const Options& opt) {
    bool otherRunning = false;
    const int instanceFd = claimInstance(opt.show, otherRunning);
    if (otherRunning) {
        std::fprintf(stderr, "frame-vrc-panel is already running: asked it to show the panel\n");
        return kExitClosed;
    }
    FontSet fonts;
    loadFonts(fonts);
    Model model(opt.cli, stateDir());
    VrOverlay vr;
    PanelUi ui(model, fonts);
    // for the language choice (~/.local/share exists; frame-vrc/ may not yet)
    ::mkdir(stateDir().substr(0, stateDir().rfind('/')).c_str(), 0700);
    ::mkdir(stateDir().c_str(), 0700);
    ui.setLanguage(opt.lang >= 0 ? opt.lang == 1 : japanese(), stateDir() + "/lang");
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kPanelWidth, kPanelHeight);
    cairo_surface_t* thumb = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, kThumbnailSize, kThumbnailSize);
    std::vector<uint8_t> rgba, thumbRgba;
    int exitCode = 0;
    int64_t nextConnect = 0, drawnAt = 0;
    std::string lastConnectMessage;
    bool showPending = opt.show;
    uint64_t drawnSeq = ~0ull;
    bool dirty = true, wasVisible = false, thumbSent = false;
    int thumbState = -1;
    while (!gStop) {
        if (instanceFd >= 0 && showRequested(instanceFd)) showPending = true;
        if (!vr.connected()) {
            if (nowMs() < nextConnect) {
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
                continue;
            }
            std::string message;
            if (vr.connect(kPanelWidth, kPanelHeight, message) != VrOverlay::ConnectResult::Ok) {
                if (message != lastConnectMessage) std::fprintf(stderr, "[VR] not connected: %s\n", message.c_str());
                lastConnectMessage = message;
                nextConnect = nowMs() + kRetryConnectMs;
                continue;
            }
            std::fprintf(stderr, "[VR] connected\n");
            lastConnectMessage.clear();
            dirty = true;
            thumbSent = false;
        }
        const VrEvents events = vr.pollEvents();
        if (events.quit || !vr.steamVrAlive()) break;
        if (events.closeRequested) {
            exitCode = kExitClosed;
            break;
        }
        for (const PointerInput& p : events.pointer) {
            switch (p.type) {
                case PointerInput::Type::Move: dirty |= ui.pointerMove(p.x, p.y); break;
                case PointerInput::Type::Down: dirty |= ui.pointerDown(p.x, p.y); break;
                case PointerInput::Type::Up: dirty |= ui.pointerUp(p.x, p.y); break;
                case PointerInput::Type::Leave: dirty |= ui.pointerLeave(); break;
                case PointerInput::Type::Scroll: dirty |= ui.scroll(p.y); break;
            }
        }
        const bool visible = vr.panelVisible();
        if (visible) dirty |= ui.tick();
        if (visible != wasVisible) {
            model.setInterval(visible ? 10000 : 60000);
            if (visible) model.refresh();
            vr.setPanelScroll(visible);
            wasVisible = visible;
            dirty = true;
        }
        const Snapshot snap = model.snapshot();
        if (snap.seq != drawnSeq) dirty = true;
        // the elapsed seconds and the apply countdown
        if ((snap.running || snap.applyAtMs != 0) && nowMs() - drawnAt >= 500) dirty = true;
        if (dirty && (visible || showPending || drawnSeq == ~0ull)) {
            cairo_t* cr = cairo_create(surface);
            ui.draw(cr, snap);
            cairo_destroy(cr);
            surfaceToRgba(surface, rgba);
            vr.submitPanel(rgba.data());
            drawnSeq = snap.seq;
            drawnAt = nowMs();
            dirty = false;
        }
        const int state = !snap.checked ? 0 : countLevel(snap, "fail") > 0 ? 1 : snap.running ? 2 : 3;
        if (!thumbSent || state != thumbState) {
            cairo_t* cr = cairo_create(thumb);
            ui.drawThumbnail(cr, snap);
            cairo_destroy(cr);
            surfaceToRgba(thumb, thumbRgba);
            thumbSent = vr.submitThumbnail(thumbRgba.data(), kThumbnailSize);
            thumbState = state;
        }
        if (showPending) {
            vr.showPanel();  // an image was submitted above, so it never opens blank
            showPending = false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(visible ? 33 : 250));
    }
    vr.shutdown();
    cairo_surface_destroy(thumb);
    cairo_surface_destroy(surface);
    if (instanceFd >= 0) ::close(instanceFd);
    return exitCode;
}

}  // namespace

int main(int argc, char** argv) {
    // Steam starts apps with LD_LIBRARY_PATH on its own runtime (Steam/steamrtarm64). Since the Steam client of
    // 2026-11-08 that holds an older libopenvr_api.so than SteamVR's: it wins over our RUNPATH and the panel crashes in
    // VR_Init ("free(): invalid size"). Start again without it: SteamVR's own copy (RUNPATH), the system's libvulkan,
    // and the frame-vrc commands the panel runs see no Steam runtime either.
    if (const char* p = std::getenv("LD_LIBRARY_PATH"); p != nullptr && *p != '\0') {
        unsetenv("LD_LIBRARY_PATH");
        execv("/proc/self/exe", argv);
        std::perror("frame-vrc-panel: restart without LD_LIBRARY_PATH");  // carries on as before
    }
    Options opt;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::fprintf(stderr, "%s needs a value\n", a.c_str());
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--version") {
            std::puts("frame-vrc-panel " FRAME_VRC_PANEL_VERSION);
            return 0;
        } else if (a == "--help" || a == "-h") {
            usage();
            return 0;
        } else if (a == "--cli") {
            opt.cli = next();
        } else if (a == "--dump-png") {
            opt.dumpPng = next();
        } else if (a == "--thumb-png") {
            opt.thumbPng = next();
        } else if (a == "--icon-png") {
            opt.iconSize = std::atoi(next().c_str());
            opt.iconPng = next();
        } else if (a == "--do") {
            opt.steps.push_back(next());
        } else if (a == "--list-buttons") {
            opt.listButtons = true;
        } else if (a == "--scroll") {
            opt.scroll = std::atof(next().c_str());
        } else if (a == "--lang") {
            opt.lang = next() == "ja" ? 1 : 0;
        } else if (a == "--show") {
            opt.show = true;
        } else {
            std::fprintf(stderr, "unknown option %s\n", a.c_str());
            usage();
            return 2;
        }
    }
    if (opt.cli.empty()) opt.cli = defaultCli();
    if (!opt.iconPng.empty()) return runIcon(opt);
    if (!opt.dumpPng.empty()) return runDump(opt);
    struct sigaction sa {};
    sa.sa_handler = onSignal;
    ::sigaction(SIGTERM, &sa, nullptr);
    ::sigaction(SIGINT, &sa, nullptr);
    ::signal(SIGPIPE, SIG_IGN);
    return runVr(opt);
}
