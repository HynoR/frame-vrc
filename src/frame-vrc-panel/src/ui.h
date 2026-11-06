// frame-vrc panel: the 1200x700 SteamVR dashboard (Home / Environment / Advanced) and laser hit-testing.
// Every button runs one frame-vrc command; the panel holds no settings of its own.
// Copyright (c) 2026 FrameClash contributors. MIT License.
#pragma once

#include "model.h"
#include "monitor.h"
#include "third_party_frameeyeosc/draw.h"

#include <cairo.h>

#include <string>
#include <vector>

constexpr int kPanelWidth = 1200;
constexpr int kPanelHeight = 700;
constexpr int kThumbnailSize = 256;

struct Hit {
    std::string id;   ///< "view:home", "refresh", "run:<frame-vrc arguments>" or "set:KEY=VALUE[,KEY=VALUE]"
    double x, y, w, h;
    bool enabled;
};

enum class View { Home, Environment, Advanced, Monitor, Log };

class PanelUi {
public:
    PanelUi(Model& model, const FontSet& fonts);

    /** Draw the panel for this snapshot (also rebuilds the click targets). */
    void draw(cairo_t* cr, const Snapshot& s);
    void drawThumbnail(cairo_t* cr, const Snapshot& s);

    /** Pointer events (panel px, origin top-left). @return true if the panel must be redrawn */
    bool pointerMove(double x, double y);
    bool pointerDown(double x, double y);
    bool pointerUp(double x, double y);
    bool pointerLeave();
    bool scroll(double dy);
    /** Press a target by id (as a click on it would). @return false if there's no such enabled target */
    bool press(const std::string& id);
    const std::vector<Hit>& hits() const { return hits_; }
    /** English or Japanese; the file keeps the choice for the next start. */
    void setLanguage(bool ja, std::string file) {
        ja_ = ja;
        langFile_ = std::move(file);
    }
    /** Called every frame: the Monitor page samples once a second. @return true if the panel must be redrawn */
    bool tick();

private:
    Model& model_;
    const FontSet& fonts_;
    View view_ = View::Home;
    std::vector<Hit> hits_;
    std::string hover_, pressed_;
    std::string armed_;          ///< a button waiting for its confirming second tap
    int64_t armedAt_ = 0;
    int envOffset_ = 0, maxEnvOffset_ = 0;
    double scrollAccum_ = 0;
    Snapshot last_;
    bool ja_ = false;
    std::string langFile_;
    SystemMonitor monitor_;
    int64_t monitorAt_ = 0;

    std::string tr(const std::string& en, const std::string& ja) const { return ja_ ? ja : en; }
    void drawMonitor(const Pen& pen, const Snapshot& s);
    void drawLog(const Pen& pen, const Snapshot& s);
    int logOffset_ = 0, maxLogOffset_ = 0;  ///< lines scrolled up from the newest
    void graph(const Pen& pen, double x, double y, double w, double h, const std::deque<double>& values, Color c);

    void activate(const std::string& id);
    void addHit(const std::string& id, double x, double y, double w, double h, bool enabled);
    const Hit* hitAt(double x, double y) const;
    /** A button; selected = the current value (accent outline and text). */
    void button(const Pen& pen, const std::string& id, double x, double y, double w, double h,
                const std::string& label, bool enabled, bool selected = false, double size = 19);
    void mainButton(const Pen& pen, const std::string& id, double x, double y, double w, double h,
                    const std::string& label, bool enabled, double size = 22);
    void card(const Pen& pen, double x, double y, double w, double h, const std::string& label);
    void hint(const Pen& pen, double x, double y, double w, const std::string& text, int lines = 2);
    /** A command button that asks for a second tap first. */
    bool needsConfirm(const std::string& id) const;
    bool isArmed(const std::string& id) const;
    /** Buttons that change FEX or Proton (not while the game runs). */
    bool canRun(const std::string& args) const;
    /** A setting as the user last chose it: a pending tap over the status value. */
    std::string setting(const Snapshot& s, const std::string& key, const std::string& current) const;
    int settingInt(const Snapshot& s, const std::string& key, int current) const;
    void drawSidebar(const Pen& pen, const Snapshot& s);
    void drawHome(const Pen& pen, const Snapshot& s);
    void drawEnvironment(const Pen& pen, const Snapshot& s);
    void drawAdvanced(const Pen& pen, const Snapshot& s);
    void drawFooter(const Pen& pen, const Snapshot& s);
    void tuneRow(const Pen& pen, double x, double y, double w, const std::string& label, const std::string& key,
                 int value, int down, int up, const std::string& unit);
};

/** The app icon (the .desktop launcher): the dashboard thumbnail without the status dot. */
void drawAppIcon(cairo_t* cr, const FontSet& fonts);

/** How many doctor rows are at this level ("fail" / "warn"). */
int countLevel(const Snapshot& s, const std::string& level);
