// Copyright (c) 2026 FrameClash contributors. MIT License.
// Layout and helpers follow frameclash-panel (sidebar + cards). Colors: palette.h. The drawing helpers (Pen, fonts,
// text layout) come from frameeyeosc.
#include "ui.h"

#include "palette.h"
#include "third_party_frameeyeosc/text_layout.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace {

constexpr double kSideW = 180;
constexpr double kX0 = 200, kX1 = kPanelWidth - 20, kCW = kX1 - kX0;  // content area
constexpr double kTop = 76;                                           // cards start (the page title is above)
constexpr double kFootY = 640, kFootH = 44;
constexpr double kGap = 16;
constexpr double kHalfW = (kCW - kGap) / 2;
constexpr double kEnvRowH = 40, kEnvHeadH = 34, kEnvBottom = kFootY - 12;
constexpr int64_t kArmMs = 4000;

/** Button text for a doctor fix. */
std::string fixLabel(const std::string& fix, bool ja) {
    if (fix == "launch-options set") return ja ? "起動オプションを設定" : "Set launch options";
    if (fix == "setup" || fix == "community") return ja ? "PCVR 版をセットアップ" : "Setup PCVR version";
    if (fix == "store") return ja ? "ストアページを開く" : "Open store page";
    if (fix == "install-deps") return ja ? "Steam 依存ツールを導入" : "Install Steam tools";
    if (fix == "packages") return ja ? "パッケージを更新" : "Update packages";
    if (fix == "official") return ja ? "公式版に戻す" : "Restore official";
    return fix;
}

/** doctor's group names (the row texts come from the CLI in English) */
std::string groupLabel(const std::string& g, bool ja) {
    if (g == "device") return ja ? "デバイス" : "Device";
    if (g == "steam") return "Steam";
    return g;
}

Color levelColor(const std::string& level) {
    return level == "ok" ? cGreen : level == "warn" ? cAmber : cRed;
}

}  // namespace

int countLevel(const Snapshot& s, const std::string& level) {
    return static_cast<int>(std::count_if(s.checks.begin(), s.checks.end(),
                                          [&](const Check& c) { return c.level == level; }));
}

PanelUi::PanelUi(Model& model, const FontSet& fonts) : model_(model), fonts_(fonts) {}

void PanelUi::addHit(const std::string& id, double x, double y, double w, double h, bool enabled) {
    hits_.push_back({id, x, y, w, h, enabled});
}

const Hit* PanelUi::hitAt(double x, double y) const {
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it)
        if (x >= it->x && x < it->x + it->w && y >= it->y && y < it->y + it->h) return &*it;
    return nullptr;
}

void PanelUi::button(const Pen& pen, const std::string& id, double x, double y, double w, double h,
                     const std::string& label, bool enabled, bool selected, double size) {
    const bool hot = enabled && (hover_ == id || pressed_ == id);
    const bool armed = isArmed(id);
    Color f = !enabled ? cCtl : armed ? cRedTint : selected ? cAccentTint : cCtl;
    if (hot) f = blendColor(hexColor(0xffffff), f, pressed_ == id ? 0.18 : 0.08);
    pen.color(f);
    pen.roundedRect(x, y, w, h, 8);
    cairo_fill(pen.cr);
    if (selected) {
        pen.color(cAccent);
        cairo_set_line_width(pen.cr, 2);
        pen.roundedRect(x + 1, y + 1, w - 2, h - 2, 8);
        cairo_stroke(pen.cr);
    }
    const bool restarts = (id == "run:community" || id == "run:setup") && last_.status.proton == "missing";
    const std::string text = !armed ? label
                             : restarts ? tr("SteamVR restarts – tap again", "SteamVR 再起動 – もう一度")
                                        : tr("Tap again to confirm", "もう一度タップで実行");
    const std::string fit = ellipsize(pen, text, size, true, w - 20, false);
    const double tw = pen.measure(fit, size, true);
    const Color tc = !enabled ? cDim : armed ? cRed : selected ? cAccent : cText;
    pen.text(x + (w - tw) / 2, y + h / 2 + size * 0.36, fit, size, tc, true);
    addHit(id, x, y, w, h, enabled);
}

void PanelUi::mainButton(const Pen& pen, const std::string& id, double x, double y, double w, double h,
                         const std::string& label, bool enabled, double size, const Color* fill) {
    const bool hot = enabled && (hover_ == id || pressed_ == id);
    const bool armed = isArmed(id);
    Color f = !enabled ? cCtl : armed ? cAmber : fill ? *fill : cAccent;
    if (hot) f = blendColor(hexColor(0xffffff), f, pressed_ == id ? 0.25 : 0.12);
    pen.color(f);
    pen.roundedRect(x, y, w, h, 10);
    cairo_fill(pen.cr);
    const std::string text = !armed ? label
                             : (id == "run:community" || id == "run:setup") && last_.status.proton == "missing"
                                 ? tr("SteamVR restarts once – tap again", "SteamVR が 1 回再起動 – もう一度タップ")
                                 : tr("Tap again to confirm", "もう一度タップで実行");
    const std::string fit = ellipsize(pen, text, size, true, w - 24, false);
    const double tw = pen.measure(fit, size, true);
    pen.text(x + (w - tw) / 2, y + h / 2 + size * 0.36, fit, size, enabled ? cOnAccent : cDim, true);
    addHit(id, x, y, w, h, enabled);
}

void PanelUi::card(const Pen& pen, double x, double y, double w, double h, const std::string& label) {
    pen.color(cCard);
    pen.roundedRect(x, y, w, h, 12);
    cairo_fill(pen.cr);
    pen.color(cLine);
    cairo_set_line_width(pen.cr, 1);
    pen.roundedRect(x + 0.5, y + 0.5, w - 1, h - 1, 12);
    cairo_stroke(pen.cr);
    if (!label.empty()) pen.text(x + 20, y + 34, label, 17, cMuted, true);
}

void PanelUi::hint(const Pen& pen, double x, double y, double w, const std::string& text, int lines) {
    for (const std::string& line : wrapText(pen, text, 15, false, w, static_cast<size_t>(lines))) {
        pen.text(x, y, line, 15, cMuted);
        y += 21;
    }
}

bool PanelUi::needsConfirm(const std::string& id) const {
    if (id == "run:uninstall" || id == "run:uninstall --purge") return true;
    // changes VRChat's compatibility tool (Steam downloads the other build); the first setup restarts Steam once
    if (id == "run:official" || id == "run:community" || id == "run:setup") return true;
    // replaces options the user wrote (kept, and put back by "clear")
    return id == "run:launch-options set" && last_.status.launch == "other";
}

bool PanelUi::isArmed(const std::string& id) const {
    return armed_ == id && nowMs() - armedAt_ < kArmMs;
}

std::string PanelUi::setting(const Snapshot& s, const std::string& key, const std::string& current) const {
    const auto it = s.pending.find(key);
    return it != s.pending.end() ? it->second : current;
}

int PanelUi::settingInt(const Snapshot& s, const std::string& key, int current) const {
    const auto it = s.pending.find(key);
    return it != s.pending.end() ? std::atoi(it->second.c_str()) : current;
}

bool PanelUi::canRun(const std::string& args) const {
    if (last_.running) return false;
    const bool swaps = args == "setup" || args == "community" || args == "official" || args.rfind("uninstall", 0) == 0 ||
                       args == "update";
    return !(swaps && last_.status.gameRunning);
}

void PanelUi::draw(cairo_t* cr, const Snapshot& s) {
    last_ = s;
    hits_.clear();
    const Pen pen {cr, &fonts_};
    pen.color(cBg);
    cairo_paint(cr);
    drawSidebar(pen, s);
    // the first answer: a headset not ready yet starts on the Install page
    if (!viewChosen_ && s.status.known) {
        viewChosen_ = true;
        const std::string& step = s.status.nextStep;
        if (view_ == View::Home && !step.empty() && step != "ready" && step != "official") view_ = View::Install;
    }
    // the official version, or VRChat not installed: Home and Install only
    const bool few = s.status.known && (s.status.edition == "official" || s.status.edition.empty());
    if (few && (view_ == View::Environment || view_ == View::Advanced)) view_ = View::Home;
    const std::string titles[] = {tr("Home", "ホーム"), tr("Install", "インストール"), tr("Environment", "環境チェック"),
                                  tr("Advanced", "詳細設定"), tr("Log", "ログ")};
    pen.text(kX0, 52, titles[static_cast<int>(view_)], 28, cText, true);
    switch (view_) {
        case View::Home: drawHome(pen, s); break;
        case View::Install: drawInstall(pen, s); break;
        case View::Environment: drawEnvironment(pen, s); break;
        case View::Advanced: drawAdvanced(pen, s); break;
        case View::Log: drawLog(pen, s); break;
    }
    drawFooter(pen, s);
}

void PanelUi::drawSidebar(const Pen& pen, const Snapshot& s) {
    pen.color(cSide);
    cairo_rectangle(pen.cr, 0, 0, kSideW, kPanelHeight);
    cairo_fill(pen.cr);
    pen.color(cLine);
    cairo_rectangle(pen.cr, kSideW - 1, 0, 1, kPanelHeight);
    cairo_fill(pen.cr);
    pen.color(cAccent);
    pen.roundedRect(22, 33, 14, 14, 3);
    cairo_fill(pen.cr);
    pen.text(44, 48, "frame-vrc", 21, cText, true);
    const struct {
        View v;
        const char* id;
        std::string label;
    } items[] = {{View::Home, "view:home", tr("Home", "ホーム")},
                 {View::Install, "view:install", tr("Install", "インストール")},
                 {View::Environment, "view:env", tr("Environment", "環境チェック")},
                 {View::Advanced, "view:advanced", tr("Advanced", "詳細設定")},
                 {View::Log, "view:log", tr("Log", "ログ")}};
    // the official version (Valve's setup) or VRChat not installed: nothing of the PC build's to show
    const bool android = s.status.known && (s.status.edition == "official" || s.status.edition.empty());
    const int fails = countLevel(s, "fail");
    double y = 92;
    for (const auto& it : items) {
        // the official Android build: Home and Install only (the PC build's checks and settings do not apply); the log
        // while a switch runs
        if (android && it.v != View::Home && it.v != View::Install && !(it.v == View::Log && (s.running || view_ == View::Log)))
            continue;
        const bool sel = view_ == it.v;
        const bool hot = hover_ == it.id || pressed_ == it.id;
        if (sel || hot) {
            pen.color(sel ? cAccentTint : cCtl);
            pen.roundedRect(12, y, kSideW - 24, 50, 8);
            cairo_fill(pen.cr);
        }
        if (sel) {
            pen.color(cAccent);
            pen.roundedRect(12, y + 12, 4, 26, 2);
            cairo_fill(pen.cr);
        }
        pen.text(32, y + 32, it.label, 18, sel ? cAccent : cText, sel);
        if (it.v == View::Environment && fails > 0) pen.dot(160, y + 25, cRed);
        if (it.v == View::Install && !s.status.nextStep.empty() && s.status.nextStep != "ready" && !android)
            pen.dot(160, y + 25, cAmber);
        if (it.v == View::Log && s.running) pen.dot(160, y + 25, cAccent);
        addHit(it.id, 12, y, kSideW - 24, 50, true);
        y += 58;
    }
    button(pen, "lang", 12, kPanelHeight - 100, kSideW - 24, 40, ja_ ? "English" : "日本語", true, false, 17);
    pen.text(22, kPanelHeight - 28, "v" + (s.status.version.empty() ? std::string(FRAME_VRC_PANEL_VERSION) : s.status.version),
             15, cMuted);
}

void PanelUi::drawHome(const Pen& pen, const Snapshot& s) {
    const Status& st = s.status;
    if (monitorAt_ == 0) tick();  // a picture before the first second passed (--dump-png)
    const MonitorSample& m = monitor_.last();
    char buf[160];
    auto gb = [](double mb) { return mb / 1024; };
    const double rx = kX0 + kHalfW + kGap;
    double y = kTop;
    const double topH = 250;

    // ---- memory: the limit on this headset ----
    card(pen, kX0, y, kHalfW, topH, tr("Memory", "メモリ"));
    const double usedMb = m.memTotalMb - m.memAvailMb;
    const Color memC = m.memAvailMb < 1000 ? cRed : m.memAvailMb < 2000 ? cAmber : cAccent;
    std::snprintf(buf, sizeof(buf), "%.1f / %.1f GB", gb(usedMb), gb(m.memTotalMb));
    pen.text(kX0 + 20, y + 80, buf, 30, cText, true);
    std::snprintf(buf, sizeof(buf), ja_ ? "空き %.2f GB" : "%.2f GB free", gb(m.memAvailMb));
    pen.text(kX0 + kHalfW - 20, y + 78, buf, 20, memC, true, true);
    std::snprintf(buf, sizeof(buf), ja_ ? "スワップ %.1f / %.1f GB" : "swap %.1f / %.1f GB", gb(m.swapUsedMb), gb(m.swapTotalMb));
    pen.text(kX0 + 20, y + 112, buf, 16, cMuted);
    graph(pen, kX0 + 20, y + 128, kHalfW - 40, topH - 148, monitor_.memHistory(), memC);

    // ---- CPU per core, temperature, GPU clock ----
    card(pen, rx, y, kHalfW, topH, "CPU");
    std::snprintf(buf, sizeof(buf), "%.0f%%", m.cpuPct);
    pen.text(rx + 20, y + 80, buf, 30, cText, true);
    std::snprintf(buf, sizeof(buf), ja_ ? "最高温度 %.0f °C" : "max temperature %.0f °C", m.tempC);
    if (m.tempC > 0)
        pen.text(rx + kHalfW - 20, y + 78, buf, 20, m.tempC >= 90 ? cRed : m.tempC >= 75 ? cAmber : cText, true, true);
    if (m.gpuMaxMhz > 0) {
        std::snprintf(buf, sizeof(buf), ja_ ? "GPU クロック %.0f / %.0f MHz" : "GPU clock %.0f / %.0f MHz", m.gpuMhz,
                      m.gpuMaxMhz);
        pen.text(rx + 20, y + 112, buf, 16, cMuted);
    }
    const size_t n = m.corePct.size();
    if (n > 0) {
        const double gap = 6, cw = (kHalfW - 40 - gap * (n - 1)) / n, ch = topH - 148;
        for (size_t i = 0; i < n; ++i) {
            const double bx = rx + 20 + i * (cw + gap), by = y + 128;
            pen.color(cCtl);
            pen.roundedRect(bx, by, cw, ch, 4);
            cairo_fill(pen.cr);
            const double fill = ch * std::clamp(m.corePct[i], 0.0, 100.0) / 100;
            pen.color(m.corePct[i] >= 90 ? cAmber : cAccent);
            pen.roundedRect(bx, by + ch - fill, cw, fill, 4);
            cairo_fill(pen.cr);
        }
    }

    // ---- quality: the one setting on this page (DFR auto and video hardware: Advanced) ----
    y += topH + kGap;
    const double lowH = kFootY - 12 - y;
    card(pen, kX0, y, kHalfW, lowH, tr("Render Resolution", "レンダリング解像度"));
    const bool android = st.edition == "official";
    const struct {
        int res;
        std::string sub;
    } res[] = {{1056, tr("Performance", "パフォーマンス")}, {1440, tr("Balanced", "バランス")}, {1920, tr("Quality", "高画質")}};
    const double bw = (kHalfW - 40 - 20) / 3;
    const int resNow = settingInt(s, "resolution", st.resolution);
    for (int i = 0; i < 3 && !android; ++i) {
        const double bx = kX0 + 20 + i * (bw + 10);
        const std::string id = "set:resolution=" + std::to_string(res[i].res);
        button(pen, id, bx, y + 56, bw, 70, std::to_string(res[i].res), st.resolution > 0, resNow == res[i].res, 24);
        const double tw = pen.measure(res[i].sub, 15);
        pen.text(bx + (bw - tw) / 2, y + 150, res[i].sub, 15, cMuted);
    }
    if (android)
        hint(pen, kX0 + 20, y + 80, kHalfW - 40,
             tr("The official Android build uses VRChat's in-game graphics settings.",
                "公式 Android 版の画質は VRChat 内の設定に従います。"), 2);
    else hint(pen, kX0 + 20, y + lowH - 52, kHalfW - 40,
         st.resolution > 0 ? tr("Applies on next VRChat launch. Eye-tracked DFR is enabled automatically.",
                                "次回の VRChat 起動時に適用されます。視線追跡 DFR は自動で有効になります。")
                           : tr("Start SteamVR to change resolution.", "解像度を変更するには SteamVR を起動してください。"), 2);

    // ---- VRChat: how it runs, and launch ----
    card(pen, rx, y, kHalfW, lowH, "VRChat");
    const std::string& step = st.nextStep;
    if (m.gamePid > 0) {
        std::snprintf(buf, sizeof(buf), ja_ ? "メモリ %.2f GB · %.1f コア分" : "%.2f GB memory · %.1f cores", gb(m.gameRssMb), m.gameCores);
        pen.text(rx + 20, y + 80, buf, 22, cText, true);
        std::snprintf(buf, sizeof(buf), "pid %d · %s", m.gamePid,
                      ("DFR " + st.dfrEffective).c_str());
        pen.text(rx + 20, y + 108, buf, 15, cMuted);
    } else if (step == "official") {
        pen.text(rx + 20, y + 80, tr("Official Version", "公式版"), 22, cGreen, true);
        hint(pen, rx + 20, y + 108, kHalfW - 40, tr("Running on Lepton. Switch to the PCVR version on the Install page.",
                                                    "標準構成（Lepton）。PCVR 版への切り替えはインストール画面から行えます。"), 2);
    } else if (step == "update") {
        pen.text(rx + 20, y + 80, tr("Patched: update and launch", "パッチ済み：更新して起動"), 22, cGreen, true);
        hint(pen, rx + 20, y + 108, kHalfW - 40, tr("Steam will update VRChat to the PC build on launch.",
                                                    "起動時に Steam が PC ビルドへ自動更新します。"), 1);
    } else if (step == "download") {
        pen.text(rx + 20, y + 80, tr("Downloading VRChat in Steam", "Steam でダウンロード中"), 22, cAmber, true);
        hint(pen, rx + 20, y + 108, kHalfW - 40, tr("Check download progress in Steam.",
                                                    "Steam でダウンロードの進行状況を確認してください。"), 1);
    } else if (!step.empty() && step != "ready") {
        pen.text(rx + 20, y + 80, tr("Setup Required", "セットアップが必要"), 22, cAmber, true);
        hint(pen, rx + 20, y + 108, kHalfW - 40, tr("Open the Install page to continue setup.", "インストール画面からセットアップを進めてください。"), 1);
    } else {
        pen.text(rx + 20, y + 80, tr("Ready", "準備完了"), 22, cGreen, true);
        std::snprintf(buf, sizeof(buf), "%s · %d · DFR %s", st.build == "android" ? (ja_ ? "公式 Android 版" : "Official Android build")
                                                                                   : (ja_ ? "PCVR 版 · PC ビルド" : "PCVR version · PC build"),
                      st.resolution, st.dfrEffective.c_str());
        pen.text(rx + 20, y + 108, buf, 15, cMuted);
    }
    const bool ready = step.empty() || step == "ready" || step == "update" || step == "official";
    if (ready || st.gameRunning)
        mainButton(pen, "run:launch", rx + 20, y + lowH - 90, kHalfW - 40, 70,
                   st.gameRunning ? tr("VRChat is running", "VRChat 実行中") : tr("Launch VRChat", "VRChat を起動"),
                   canRun("") && !st.gameRunning, 26);
    else
        mainButton(pen, "view:install", rx + 20, y + lowH - 90, kHalfW - 40, 70, tr("Go to Install", "インストールへ"), true, 24);
}

void PanelUi::drawInstall(const Pen& pen, const Snapshot& s) {
    const Status& st = s.status;
    const bool official = st.edition == "official", community = st.edition == "community";
    const std::string& step = st.nextStep;
    // ---- what is set now ----
    double y = kTop;
    const double topH = 300;
    card(pen, kX0, y, kCW, topH, tr("Installed", "インストール状況"));
    button(pen, "refresh", kX1 - 150, y + 12, 130, 36, s.refreshing ? tr("Checking…", "確認中…") : tr("Check again", "再チェック"),
           !s.refreshing, false, 16);
    struct Row {
        std::string label, value;
        Color color;
    };
    const std::string fexValue = st.fex == "ok" ? tr("Patched (EAC, VRChat only)", "パッチ版（EAC・VRChat 専用）")
                                 : st.fex == "old" ? tr("Update available", "更新あり") : tr("Not configured", "未設定");
    const std::string protonValue = st.proton == "ok" ? "Proton Experimental (Ji10me PCVR Edit)"
                                    : st.proton == "old" ? tr("Update available", "更新あり") : tr("Not configured", "未設定");
    const std::string launch = st.launch == "set" ? tr("Configured", "設定済み")
                               : st.launch == "stale" ? tr("Outdated", "要更新")
                               : st.launch == "other" ? tr("Custom", "カスタム") : tr("Not set", "未設定");
    const int fails = countLevel(s, "fail"), warns = countLevel(s, "warn");
    const std::string env = !s.checked ? tr("Checking…", "確認中…")
                            : fails > 0 ? std::to_string(fails) + tr(" to fix", " 件要修正")
                            : warns > 0 ? std::to_string(warns) + tr(warns > 1 ? " warnings" : " warning", " 件の警告")
                            : tr("OK", "正常");
    const std::string version = official ? tr("Official version (Lepton)", "公式版（Lepton）")
                                : community ? tr("PCVR version (frame-vrc)", "PCVR 版（frame-vrc）")
                                : st.edition == "other" ? tr("Unsupported Proton (EAC fails)", "非対応 Proton（EAC 不可）")
                                : tr("Not installed", "未インストール");
    const std::string build = step == "download" ? tr("Downloading in Steam", "Steam でダウンロード中")
                              : st.build == "pc" ? tr("PC build", "PC ビルド")
                              : st.build == "android" ? tr("Official Android build", "公式 Android ビルド") : "—";
    const Row rows[] = {
        {"VRChat", version, community || official ? cGreen : cAmber},
        {tr("Game files", "ゲーム本体"), build, step == "download" ? cAmber : cText},
        {"FEX", st.known ? fexValue : "—", st.fex == "ok" ? cGreen : cAmber},
        {"Proton", st.known ? protonValue : "—", st.proton == "ok" ? cGreen : cAmber},
        {tr("Launch options", "起動オプション"), launch, st.launch == "set" ? cGreen : cAmber},
        {tr("Environment", "環境"), env, !s.checked ? cMuted : fails > 0 ? cRed : warns > 0 ? cAmber : cGreen},
    };
    // the official version: Valve's setup, only VRChat and its files; the PCVR version: frame-vrc's parts too
    double ry = y + 76;
    for (const Row& r : rows) {
        // the official version or nothing installed: VRChat and its files only (frame-vrc's parts do not apply)
        if ((official || st.edition.empty()) && &r > &rows[1]) continue;
        pen.text(kX0 + 24, ry, r.label, 18, cMuted);
        pen.text(kX1 - 24, ry, ellipsize(pen, r.value, 19, true, kCW - 260, false), 19,
                 st.known || &r == &rows[5] ? r.color : cMuted, true, true);
        if (&r == &rows[5] && fails > 0) addHit("view:env", kX0, ry - 26, kCW, 34, true);
        ry += 34;
    }
    // Advanced is hidden then: frame-vrc's own update and uninstall are here
    if (st.known && (official || st.edition.empty())) {
        const double bw = (kCW - 58) / 2;
        button(pen, "run:update", kX0 + 24, y + topH - 70, bw, 48, tr("Update frame-vrc", "frame-vrc を更新"), canRun("update"));
        button(pen, "run:uninstall --purge", kX0 + 34 + bw, y + topH - 70, bw, 48, tr("Uninstall frame-vrc", "frame-vrc を削除"),
               canRun("uninstall"));
    }

    // ---- the one thing to do: switch versions, or what Steam needs first ----
    y += topH + kGap;
    const double lowH = kFootY - 12 - y;
    card(pen, kX0, y, kCW, lowH, "");
    std::string id, label, text;
    const Color* fill = nullptr;  // neutral accent: something Steam needs first
    bool enabled = st.known && canRun("setup");
    if (step == "steam") {
        id = "refresh", label = tr("Start Steam, then check again", "Steam を起動してから再チェック"), enabled = !s.refreshing;
        text = tr("Steam is not running or not responding.", "Steam が起動していないか応答していません。");
    } else if (step == "own") {
        id = "run:store", label = tr("Open VRChat Store Page", "VRChat のストアページを開く"), enabled = st.known && canRun("store");
        text = tr("Add VRChat (free) to your Steam library, then check again.", "VRChat（無料）を Steam ライブラリに追加してから再チェックしてください。");
    } else if (step == "deps") {
        id = "run:install-deps", label = tr("Install Steam Dependencies", "Steam 依存ツールをインストール"), enabled = st.known && canRun("install-deps");
        text = tr("Installs FEX-Emu, Proton Experimental, and Proton EasyAntiCheat Runtime via Steam.",
                  "Steam から FEX-Emu、Proton Experimental、EasyAntiCheat Runtime を導入します。");
    } else if (community && step != "patch") {
        static const Color red = cRed;
        id = "run:official", label = tr("Recover to Official Android Version", "公式 Android 版に戻す"), fill = &red;
        enabled = st.known && canRun("official");
        text = step == "update" ? tr("patched: update and launch VRChat (Steam updates it to the PC build first).",
                                     "パッチ済み：VRChat を更新して起動（起動時に Steam が PC ビルドへ更新します）。")
                                : tr("PCVR version is active. Recover switches back to the official Android build (Lepton) and "
                                     "restores your previous launch options and resolution.",
                                     "PCVR 版が有効です。公式 Android 版（Lepton）および元の起動オプション・解像度に戻す場合は上のボタンをタップしてください。");
    } else {
        static const Color green = cGreen;
        id = "run:setup", label = tr("Setup to PCVR Version", "PCVR 版をセットアップ"), fill = &green;
        text = st.edition.empty()
                   ? tr("Configures the PCVR runtime and prompts Steam to install VRChat (~1.1 GB).",
                        "PCVR ランタイムを構成し、Steam で VRChat（約 1.1 GB）のインストールを開始します。")
               : community ? tr("PCVR setup is incomplete. Tap above to finish configuration.",
                                "PCVR 版のセットアップが未完了です。タップして設定を完了してください。")
                           : tr("Configures VRChat's PC build with patched FEX (EAC), eye-tracked DFR, and H.264 hardware decoding "
                                "(isolated to VRChat). Restarts Steam once on first setup.",
                                "パッチ版 FEX（EAC）、視線追跡 DFR、H.264 ハードウェアデコードを VRChat 専用に構成します。初回は Steam が "
                                "1 回再起動します。");
    }
    mainButton(pen, id, kX0 + 24, y + 28, kCW - 48, 84, label, enabled, 28, fill);
    // the setup is done and VRChat only waits for Steam's update: say so plainly, in green
    if (step == "update")
        pen.text(kX0 + 24, y + 156, ellipsize(pen, text, 20, true, kCW - 48, false), 20, cGreen, true);
    else
        hint(pen, kX0 + 24, y + 146, kCW - 48, text, 3);
}

void PanelUi::drawEnvironment(const Pen& pen, const Snapshot& s) {
    button(pen, "refresh", kX1 - 170, 22, 170, 40, s.refreshing ? tr("Checking…", "確認中…") : tr("Check again", "再チェック"), !s.refreshing);
    if (!s.checked) {
        card(pen, kX0, kTop, kCW, 120, "");
        pen.text(kX0 + 24, kTop + 66, s.error.empty() ? tr("Checking the environment…", "環境を確認中…") : s.error, 20, cMuted);
        return;
    }
    // flat list: group headers and rows
    struct Item {
        bool head;
        const Check* c;
        std::string group;
    };
    std::vector<Item> items;
    std::string group;
    for (const Check& c : s.checks) {
        if (c.group != group) items.push_back({true, nullptr, group = c.group});
        items.push_back({false, &c, ""});
    }
    // how many items fit from each offset decides the last offset
    double total = 0;
    int fitFromEnd = 0;
    for (int i = static_cast<int>(items.size()) - 1; i >= 0; --i) {
        total += items[i].head ? kEnvHeadH : kEnvRowH;
        if (total > kEnvBottom - kTop) break;
        ++fitFromEnd;
    }
    maxEnvOffset_ = std::max(0, static_cast<int>(items.size()) - fitFromEnd);
    envOffset_ = std::min(envOffset_, maxEnvOffset_);
    double y = kTop;
    for (size_t i = envOffset_; i < items.size(); ++i) {
        const Item& it = items[i];
        const double h = it.head ? kEnvHeadH : kEnvRowH;
        if (y + h > kEnvBottom) break;
        if (it.head) {
            pen.text(kX0 + 4, y + 24, groupLabel(it.group, ja_), 16, cMuted, true);
        } else {
            const Check& c = *it.c;
            pen.color(cCard);
            pen.roundedRect(kX0, y, kCW, h - 4, 8);
            cairo_fill(pen.cr);
            const Color lc = levelColor(c.level);
            pen.color(lc);
            pen.roundedRect(kX0 + 12, y + 8, 52, h - 20, 5);
            cairo_fill(pen.cr);
            const std::string tag = c.level == "ok" ? "OK" : c.level == "warn" ? "WARN" : "FAIL";
            const double tw = pen.measure(tag, 13, true);
            pen.text(kX0 + 38 - tw / 2, y + h / 2 + 3, tag, 13, cOnAccent, true);
            const double textW = kCW - 90 - (c.fix.empty() ? 0 : 220);
            pen.text(kX0 + 78, y + h / 2 + 4, ellipsize(pen, c.text, 17, false, textW, false), 17, cText);
            if (!c.fix.empty())
                button(pen, "run:" + c.fix, kX1 - 214, y + 3, 206, h - 10, fixLabel(c.fix, ja_), canRun(c.fix), false, 16);
        }
        y += h;
    }
    if (maxEnvOffset_ > 0) {
        const std::string more = std::to_string(envOffset_ + 1) + "–" +
                                 std::to_string(std::min<size_t>(items.size(), envOffset_ + fitFromEnd)) + " / " +
                                 std::to_string(items.size()) + tr(" · scroll", " · スクロール");
        pen.text(kX1 - 190, 52, more, 14, cMuted, false, true);
    }
}

void PanelUi::drawAdvanced(const Pen& pen, const Snapshot& s) {
    const Status& st = s.status;
    const double rx = kX0 + kHalfW + kGap, bw = (kHalfW - 50) / 2;
    const int resNow = settingInt(s, "resolution", st.resolution);
    double y = kTop;
    // what the community version runs on (setup sets it up; nothing to switch here)
    card(pen, kX0, y, kHalfW, 150, tr("PCVR Version", "PCVR 版"));
    pen.text(kX0 + 20, y + 72, "FEX", 17, cMuted, true);
    pen.text(kX0 + 110, y + 72, ellipsize(pen, st.fexBuild.empty() ? "—" : st.fexBuild, 17, false, kHalfW - 130, false), 17, cText);
    pen.text(kX0 + 20, y + 102, "Proton", 17, cMuted, true);
    pen.text(kX0 + 110, y + 102, st.proton == "ok" ? "Experimental (Ji10me PCVR Edit)" : st.proton.empty() ? "—" : st.proton, 17, cText);
    hint(pen, kX0 + 20, y + 134, kHalfW - 40, tr("VRChat only; Steam's FEX-Emu and other games are unaffected.",
                                                  "VRChat 専用。Steam の FEX-Emu や他のゲームに影響なし。"), 1);
    // swap: SteamOS's 8 GB zram, then the swap file (1 GB as shipped, 8 GB set up for VRChat); root only, so the buttons
    // show the commands in the log
    card(pen, rx, y, kHalfW, 150, tr("Swap File", "スワップファイル"));
    const bool big = st.swapFileMb > 4096;
    button(pen, "run:swap 8g", rx + 20, y + 52, bw, 48, tr("8 GB (VRChat)", "8 GB（VRChat 向け）"), canRun("swap"), st.known && big);
    button(pen, "run:swap 1g", rx + 30 + bw, y + 52, bw, 48, tr("1 GB (Default)", "1 GB（標準）"), canRun("swap"),
           st.known && st.swapFileMb > 0 && !big);
    hint(pen, rx + 20, y + 126, kHalfW - 40, tr("Plus 8 GB zram. Requires root (commands shown in Log).",
                                                  "zram 8 GB に追加。root が必要（コマンドをログに表示）。"), 1);

    y += 150 + kGap;
    const double bw3 = (kHalfW - 60) / 3;
    card(pen, kX0, y, kHalfW, 150, tr("Additional Resolutions", "その他の解像度"));
    button(pen, "set:resolution=1600", kX0 + 20, y + 52, bw3, 48, "1600", st.resolution > 0, resNow == 1600);
    button(pen, "set:resolution=2160", kX0 + 30 + bw3, y + 52, bw3, 48, "2160", st.resolution > 0, resNow == 2160);
    button(pen, "run:resolution reset", kX0 + 40 + 2 * bw3, y + 52, bw3, 48, tr("Reset", "元に戻す"), canRun("") && st.resolution > 0);
    hint(pen, kX0 + 20, y + 126, kHalfW - 40, tr("Restores the SteamVR resolution set prior to frame-vrc.", "frame-vrc 導入前の SteamVR 解像度設定に戻します。"), 1);
    const double h3 = kFootY - 12 - y;
    card(pen, rx, y, kHalfW, h3, tr("Maintenance", "メンテナンス"));
    button(pen, "run:launch-options set", rx + 20, y + 52, bw, 44, tr("Set Launch Options", "起動オプションを設定"), canRun("launch-options set"));
    button(pen, "run:update", rx + 30 + bw, y + 52, bw, 44, tr("Update frame-vrc", "frame-vrc を更新"), canRun("update"));
    button(pen, "run:uninstall", rx + 20, y + 106, bw, 44, tr("Uninstall frame-vrc", "frame-vrc を削除"), canRun("uninstall"));
    button(pen, "run:uninstall --purge", rx + 30 + bw, y + 106, bw, 44, tr("Uninstall & Purge", "データごと完全削除"), canRun("uninstall"));
    hint(pen, rx + 20, y + 176, kHalfW - 40, tr("Restores the official Android version, then removes frame-vrc and this panel.",
                                                "公式 Android 版へ戻した上で、frame-vrc とこのパネルを削除します。"), 2);

    // ji10me's tested settings: fixed, nothing to tune
    y += 150 + kGap;
    card(pen, kX0, y, kHalfW, kFootY - 12 - y, tr("Runtime", "ランタイム"));
    pen.text(kX0 + 20, y + 72, ellipsize(pen, st.runtime.empty() ? "—" : st.runtime, 17, false, kHalfW - 40, false), 17, cText);
    hint(pen, kX0 + 20, y + 104, kHalfW - 40,
         tr("ji10me's preset: eye-tracked DFR, 2 DXVK threads, 2 GB VRAM cap, VRChat thread affinity.",
            "ji10me プリセット：視線追跡 DFR、DXVK 2 スレッド、VRAM 2 GB 制限、スレッド配置最適化。"), 2);
}

void PanelUi::drawFooter(const Pen& pen, const Snapshot& s) {
    std::string text;
    Color c = cMuted;
    const bool restartArmed = (isArmed("run:community") || isArmed("run:setup")) && s.status.proton == "missing";
    if (restartArmed) {
        text = tr("SteamVR will restart once (~1 min) while setup continues in the background. Reopen frame-vrc afterward.",
                  "SteamVR が 1 回再起動します（約 1 分）。設定はバックグラウンドで継続されます。");
        c = cAmber;
    } else if (s.applyAtMs != 0 && !s.running) {
        std::string what;
        for (const auto& kv : s.pending) what += (what.empty() ? "" : ", ") + kv.first + " " + kv.second;
        const int64_t secs = std::max<int64_t>(0, (s.applyAtMs - nowMs() + 999) / 1000);
        text = tr("Applying in ", "適用まで ") + std::to_string(secs) + tr(" s: ", " 秒：") + what;
        c = cAccent;
    } else if (s.running) {
        const int64_t secs = (nowMs() - s.runSinceMs) / 1000;
        text = tr("Running frame-vrc ", "実行中 frame-vrc ") + s.runLabel + " (" + std::to_string(secs) + tr(" s)", " 秒)") +
               (s.runLine.empty() ? "" : ": " + s.runLine);
        c = cAccent;
    } else if (s.haveResult) {
        text = (s.resultOk ? tr("Done: ", "完了：") : tr("Failed: ", "失敗：")) + s.resultLabel + (s.resultLine.empty() ? "" : " — " + s.resultLine);
        c = s.resultOk ? cGreen : cRed;
    } else if (!s.error.empty()) {
        text = s.error;
        c = cRed;
    } else if (s.status.gameRunning) {
        text = tr("VRChat is running: versions can be switched after it exits.",
                  "VRChat 実行中：バージョンは終了後に切り替えられます。");
    }
    if (text.empty()) return;
    const bool toLog = view_ != View::Log && (s.running || s.haveResult) && !restartArmed;
    pen.color(toLog && (hover_ == "view:log" || pressed_ == "view:log") ? cCtlHot : cCard);
    pen.roundedRect(kX0, kFootY, kCW, kFootH, 8);
    cairo_fill(pen.cr);
    const double more = toLog ? pen.measure(tr("Log ›", "ログ ›"), 16, true) + 24 : 0;
    pen.text(kX0 + 16, kFootY + 28, ellipsize(pen, text, 16, false, kCW - 32 - more, false), 16, c);
    if (toLog) {
        pen.text(kX1 - 16, kFootY + 28, tr("Log ›", "ログ ›"), 16, cText, true, true);
        addHit("view:log", kX0, kFootY, kCW, kFootH, true);
    }
}

void PanelUi::drawThumbnail(cairo_t* cr, const Snapshot& s) {
    drawAppIcon(cr, fonts_);
    const Pen pen {cr, &fonts_};
    const Color c = !s.checked ? cDim : countLevel(s, "fail") > 0 ? cRed : s.running ? cAccent : cGreen;
    cairo_arc(cr, 214, 42, 24, 0, 2 * M_PI);
    pen.color(c);
    cairo_fill(cr);
}

void drawAppIcon(cairo_t* cr, const FontSet& fonts) {
    const Pen pen {cr, &fonts};
    pen.color(cCard);
    pen.roundedRect(0, 0, kThumbnailSize, kThumbnailSize, 48);
    cairo_fill(cr);
    const double tw = pen.measure("VRC", 92, true);
    pen.text((kThumbnailSize - tw) / 2, 160, "VRC", 92, cText, true);
    pen.color(cAccent);
    cairo_rectangle(cr, 48, 200, kThumbnailSize - 96, 10);
    cairo_fill(cr);
}

bool PanelUi::pointerMove(double x, double y) {
    const Hit* h = hitAt(x, y);
    const std::string id = h != nullptr && h->enabled ? h->id : "";
    if (id == hover_) return false;
    hover_ = id;
    return true;
}

bool PanelUi::pointerDown(double x, double y) {
    const Hit* h = hitAt(x, y);
    pressed_ = h != nullptr && h->enabled ? h->id : "";
    hover_ = pressed_;
    return true;
}

bool PanelUi::pointerUp(double x, double y) {
    const Hit* h = hitAt(x, y);
    const std::string id = h != nullptr && h->enabled ? h->id : "";
    const std::string was = pressed_;
    pressed_.clear();
    if (!id.empty() && id == was) activate(id);  // only a press and release on the same target counts
    return true;
}

bool PanelUi::pointerLeave() {
    const bool changed = !hover_.empty() || !pressed_.empty();
    hover_.clear();
    pressed_.clear();
    return changed;
}

bool PanelUi::scroll(double dy) {
    if (view_ == View::Log) {  // up = older lines
        scrollAccum_ += dy;
        bool changed = false;
        for (; scrollAccum_ >= 26; scrollAccum_ -= 26)
            if (logOffset_ > 0) --logOffset_, changed = true;
        for (; scrollAccum_ <= -26; scrollAccum_ += 26)
            if (logOffset_ < maxLogOffset_) ++logOffset_, changed = true;
        return changed;
    }
    if (view_ != View::Environment) return false;
    scrollAccum_ += dy;
    bool changed = false;
    for (; scrollAccum_ >= kEnvRowH; scrollAccum_ -= kEnvRowH)
        if (envOffset_ < maxEnvOffset_) ++envOffset_, changed = true;
    for (; scrollAccum_ <= -kEnvRowH; scrollAccum_ += kEnvRowH)
        if (envOffset_ > 0) --envOffset_, changed = true;
    return changed;
}

bool PanelUi::press(const std::string& id) {
    for (const Hit& h : hits_)
        if (h.id == id && h.enabled) {
            activate(id);
            return true;
        }
    return false;
}

void PanelUi::activate(const std::string& id) {
    if (id == "view:home") {
        view_ = View::Home;
        monitorAt_ = 0;  // sample now
    } else if (id == "view:env") {
        view_ = View::Environment;
        model_.refresh();
    }
    else if (id == "view:advanced") view_ = View::Advanced;
    else if (id == "view:log") {
        view_ = View::Log;
        logOffset_ = 0;
    } else if (id == "view:install") {
        view_ = View::Install;
        model_.refresh();
    } else if (id == "lang") {
        ja_ = !ja_;
        if (!langFile_.empty()) {
            std::FILE* f = std::fopen(langFile_.c_str(), "w");
            if (f != nullptr) {
                std::fputs(ja_ ? "ja\n" : "en\n", f);
                std::fclose(f);
            }
        }
    }
    else if (id == "refresh") model_.refresh();
    model_.setFullChecks(view_ == View::Environment);
    if (id.rfind("set:", 0) == 0) {  // settings: shown at once, applied together once the taps settle
        const std::string list = id.substr(4);
        for (size_t from = 0; from < list.size();) {
            const size_t comma = std::min(list.find(',', from), list.size());
            const std::string kv = list.substr(from, comma - from);
            const size_t eq = kv.find('=');
            if (eq != std::string::npos) model_.stage(kv.substr(0, eq), kv.substr(eq + 1));
            from = comma + 1;
        }
        return;
    }
    if (id.rfind("run:", 0) != 0) return;
    if (needsConfirm(id) && !isArmed(id)) {
        armed_ = id;
        armedAt_ = nowMs();
        return;
    }
    armed_.clear();
    std::vector<std::string> args;
    const std::string cmd = id.substr(4);
    for (size_t from = 0; from < cmd.size();) {
        const size_t sp = std::min(cmd.find(' ', from), cmd.size());
        args.push_back(cmd.substr(from, sp - from));
        from = sp + 1;
    }
    model_.run(args);
    // long, many-step ones: show their output as it comes
    if (args[0] == "setup" || args[0] == "community" || args[0] == "official" || args[0] == "update" || args[0] == "uninstall" ||
        args[0] == "install-deps" || args[0] == "swap") {
        view_ = View::Log;
        logOffset_ = 0;
    }
}

bool PanelUi::tick() {
    if (view_ != View::Home || nowMs() - monitorAt_ < 1000) return false;
    monitorAt_ = nowMs();
    monitor_.sample();
    return true;
}

void PanelUi::graph(const Pen& pen, double x, double y, double w, double h, const std::deque<double>& values,
                    Color c) {
    pen.color(cCtl);
    pen.roundedRect(x, y, w, h, 6);
    cairo_fill(pen.cr);
    if (values.size() < 2) return;
    const double step = w / (SystemMonitor::kHistory - 1);
    const double x0 = x + w - step * (values.size() - 1);  // newest at the right edge
    cairo_move_to(pen.cr, x0, y + h);
    for (size_t i = 0; i < values.size(); ++i)
        cairo_line_to(pen.cr, x0 + step * i, y + h - h * std::clamp(values[i], 0.0, 100.0) / 100);
    cairo_line_to(pen.cr, x + w, y + h);
    cairo_close_path(pen.cr);
    pen.color(c, 0.25);
    cairo_fill(pen.cr);
    for (size_t i = 0; i < values.size(); ++i) {
        const double px = x0 + step * i, py = y + h - h * std::clamp(values[i], 0.0, 100.0) / 100;
        if (i == 0) cairo_move_to(pen.cr, px, py);
        else cairo_line_to(pen.cr, px, py);
    }
    pen.color(c);
    cairo_set_line_width(pen.cr, 2);
    cairo_stroke(pen.cr);
}

void PanelUi::drawLog(const Pen& pen, const Snapshot& s) {
    const std::string label = s.running ? s.runLabel : s.resultLabel;
    std::string state;
    Color sc = cMuted;
    if (s.running) {
        state = tr("running ", "実行中 ") + std::to_string((nowMs() - s.runSinceMs) / 1000) + tr(" s", " 秒");
        sc = cAccent;
    } else if (s.haveResult) {
        state = s.resultOk ? tr("done", "完了") : tr("failed", "失敗");
        sc = s.resultOk ? cGreen : cRed;
    }
    if (!label.empty()) {
        const double w = pen.measure(state, 18, true);
        pen.text(kX1, 50, state, 18, sc, true, true);
        pen.text(kX1 - w - 16, 50, ellipsize(pen, "frame-vrc " + label, 18, false, 420, false), 18, cMuted, false, true);
    }
    const double y0 = kTop, h = kFootY - 12 - kTop, lineH = 24;
    pen.color(hexColor(0x0E0E0E));
    pen.roundedRect(kX0, y0, kCW, h, 10);
    cairo_fill(pen.cr);
    if (s.runLog.empty()) {
        pen.text(kX0 + 20, y0 + 40, label.empty() ? tr("Nothing has run yet.", "まだ何も実行していません。")
                                                  : tr("Waiting for output…", "出力を待っています…"), 17, cMuted);
        return;
    }
    const int fit = static_cast<int>((h - 24) / lineH);
    const int n = static_cast<int>(s.runLog.size());
    maxLogOffset_ = std::max(0, n - fit);
    logOffset_ = std::min(logOffset_, maxLogOffset_);
    const int first = std::max(0, n - fit - logOffset_);
    double y = y0 + 30;
    for (int i = first; i < n && i < first + fit; ++i) {
        const std::string& line = s.runLog[i];
        const bool head = line.rfind("==> ", 0) == 0;
        const bool bad = line.find("FAIL") != std::string::npos || line.rfind("frame-vrc: ", 0) == 0;
        const Color c = head ? cAccent : bad ? cRed : line.find("WARN") != std::string::npos ? cAmber : cText;
        if (head) pen.text(kX0 + 16, y, ellipsize(pen, line, 16, true, kCW - 32, false), 16, c, true);
        else pen.textMono(kX0 + 16, y, line.size() > 110 ? line.substr(0, 109) + "…" : line, 15, c);
        y += lineH;
    }
    if (logOffset_ > 0) pen.text(kX1 - 16, y0 + h - 10, tr("scrolled up · newest below", "スクロール中 · 最新は下"), 13,
                                 cMuted, false, true);
}
