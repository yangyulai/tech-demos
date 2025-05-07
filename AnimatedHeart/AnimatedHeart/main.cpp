#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/graphics.h>
#include <wx/timer.h>
#include <cmath>
#include <wx/dcbuffer.h>
static wxColour HSLtoRGB(double h, double s, double l) {
    auto hue2rgb = [](double p, double q, double t) {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1.0 / 6) return p + (q - p) * 6 * t;
        if (t < 1.0 / 2) return q;
        if (t < 2.0 / 3) return p + (q - p) * (2.0 / 3 - t) * 6;
        return p;
        };
    double r, g, b;
    if (s == 0) {
        r = g = b = l; // 灰度
    }
    else {
        double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
        double p = 2 * l - q;
        r = hue2rgb(p, q, h + 1.0 / 3);
        g = hue2rgb(p, q, h);
        b = hue2rgb(p, q, h - 1.0 / 3);
    }
    return wxColour(
        static_cast<unsigned char>(r * 255),
        static_cast<unsigned char>(g * 255),
        static_cast<unsigned char>(b * 255)
    );
}
// HeartPanel: 负责绘制和动画
class HeartPanel : public wxPanel {
public:
    HeartPanel(wxWindow* parent)
        : wxPanel(parent),
        m_timer(this),
        t(0.0)
    {
        // 双缓冲
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        Bind(wxEVT_PAINT, &HeartPanel::OnPaint, this);
        Bind(wxEVT_TIMER, &HeartPanel::OnTimer, this, m_timer.GetId());
        // 然后启动
        m_timer.Start(30, wxTIMER_CONTINUOUS);
    }

private:
    wxTimer m_timer;
    double t;  // 动画时刻
    // 呼吸动画：sin 波控制整体大小
    double GetBreathScale() const {
        return 1.0 + 0.15 * std::sin(t);
    }
    // 色相动画：随时间循环
    double GetHue() const {
        return fmod(t * 0.05, 1.0);
    }
    // 计算当前缩放系数，基于正弦函数“呼吸”
    double GetScale() const {
        // scale 在 [0.8, 1.2] 之间往返
        return 1.0 + 0.2 * std::sin(t);
    }

    void OnTimer(wxTimerEvent&) {
        t += 0.1;
        Refresh();  // 触发重绘
    }
    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        auto gc = wxGraphicsContext::Create(dc);
        if (!gc) return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        // 计算呼吸动画和色相
        double scale = std::min(GetClientSize().x, GetClientSize().y) / 4.0 * GetBreathScale();
        double cx = GetClientSize().x / 2.0;
        double cy = GetClientSize().y / 2.0;

        // 1. 构造心形参数路径（同之前）
        const int STEPS = 200;
        std::vector<wxPoint2DDouble> pts;
        pts.reserve(STEPS + 1);
        for (int i = 0; i <= STEPS; ++i) {
            double θ = 2 * M_PI * i / STEPS;
            double x = 16 * std::pow(std::sin(θ), 3);
            double y = -(13 * std::cos(θ) - 5 * std::cos(2 * θ) - 2 * std::cos(3 * θ) - std::cos(4 * θ));
            pts.emplace_back(x, y);
        }
        // 边界
        double minX = pts[0].m_x, maxX = pts[0].m_x, minY = pts[0].m_y, maxY = pts[0].m_y;
        for (auto& p : pts) {
            minX = std::min(minX, p.m_x);
            maxX = std::max(maxX, p.m_x);
            minY = std::min(minY, p.m_y);
            maxY = std::max(maxY, p.m_y);
        }
        double w = maxX - minX, h = maxY - minY;
        // 缩放和偏移
        double s = std::min(GetClientSize().x / w, GetClientSize().y / h) * 0.7 * GetBreathScale();

        // 构造路径
        wxGraphicsPath path = gc->CreatePath();
        bool first = true;
        for (auto& p : pts) {
            double px = (p.m_x - (minX + maxX) / 2.0) * s + cx;
            double py = (p.m_y - (minY + maxY) / 2.0) * s + cy;
            if (first) { path.MoveToPoint(px, py); first = false; }
            else { path.AddLineToPoint(px, py); }
        }
        path.CloseSubpath();

        // 填充与描边（彩虹渐变呼吸效果）
        wxColour fillCol = HSLtoRGB(GetHue(), 0.8, 0.5);
        wxColour strokeCol = HSLtoRGB(fmod(GetHue() + 0.5, 1.0), 1.0, 0.4);

        gc->SetBrush(wxBrush(fillCol));
        gc->FillPath(path);

        gc->SetPen(wxPen(strokeCol, s * 0.03, wxPENSTYLE_SOLID));
        gc->StrokePath(path);

        // —— 在这里添加浪漫文字 —— 
        // 文本内容
        wxString txt = L"我爱你";

        // 选一款大号加粗字体
        wxFont font(36, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD);
        double textHue = fmod(GetHue() + 0.5, 1.0);
        wxColour textCol = HSLtoRGB(textHue, 1.0, 0.8);
        gc->SetFont(font, textCol);

        // 测量文本尺寸
        double tw, th;
        gc->GetTextExtent(txt, &tw, &th);

        // 文字放在心形中心偏上位置
        double tx = cx - tw / 2.0;
        double ty = cy - (h / 2.0) * s / (std::max(w, h)) - th;
        // 上面 ty 计算：cy - 心形半高*缩放 - 文本高度

        gc->DrawText(txt, tx, ty);
    }

    void OnPaint2(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        dc.Clear();

        auto gc = wxGraphicsContext::Create(dc);
        if (!gc) return;
        gc->SetAntialiasMode(wxANTIALIAS_DEFAULT);

        // 1. 采样心形参数方程
        const int STEPS = 200;
        std::vector<wxPoint2DDouble> pts;
        pts.reserve(STEPS + 1);
        for (int i = 0; i <= STEPS; ++i) {
            double theta = 2 * M_PI * i / STEPS;
            double x = 16 * std::pow(std::sin(theta), 3);
            double y = -(13 * std::cos(theta)
                - 5 * std::cos(2 * theta)
                - 2 * std::cos(3 * theta)
                - 1 * std::cos(4 * theta));
            pts.emplace_back(x, y);
        }

        // 2. 找到边界
        double minX = pts[0].m_x, maxX = pts[0].m_x;
        double minY = pts[0].m_y, maxY = pts[0].m_y;
        for (auto& p : pts) {
            minX = std::min(minX, p.m_x);
            maxX = std::max(maxX, p.m_x);
            minY = std::min(minY, p.m_y);
            maxY = std::max(maxY, p.m_y);
        }
        double w = maxX - minX;
        double h = maxY - minY;

        // 3. 计算缩放和平移到中心
        auto sz = GetClientSize();
        double W = sz.x, H = sz.y;
        double scale = std::min(W / w, H / h) * 0.4 * GetBreathScale();
        double cx = W / 2.0, cy = H / 2.0;

        // 4. 构造路径
        wxGraphicsPath path = gc->CreatePath();
        bool first = true;
        for (auto& p : pts) {
            double px = (p.m_x - (minX + maxX) / 2.0) * scale + cx;
            double py = (p.m_y - (minY + maxY) / 2.0) * scale + cy;
            if (first) {
                path.MoveToPoint(px, py);
                first = false;
            }
            else {
                path.AddLineToPoint(px, py);
            }
        }
        path.CloseSubpath();

        // 5. 填充
        wxColour fillCol = HSLtoRGB(GetHue(), 0.8, 0.5);
        gc->SetBrush(wxBrush(fillCol));
        gc->FillPath(path);

        // 6. 描边
        wxColour strokeCol = HSLtoRGB(fmod(GetHue() + 0.5, 1.0), 1.0, 0.4);
        double penWidth = scale * 0.03;
        gc->SetPen(wxPen(strokeCol, penWidth, wxPENSTYLE_SOLID));
        gc->StrokePath(path);
    }
};

// 主窗口
class MyFrame : public wxFrame {
public:
    MyFrame()
        : wxFrame(nullptr, wxID_ANY, "杨舒华我爱你", wxDefaultPosition, wxSize(500, 500))
    {
        auto* panel = new HeartPanel(this);
        auto* s = new wxBoxSizer(wxVERTICAL);
        s->Add(panel, 1, wxEXPAND);
        SetSizer(s);
    }
};

// 应用类
class MyApp : public wxApp {
public:
    bool OnInit() override {
        auto* f = new MyFrame();
        f->Show();
        return true;
    }
};

wxIMPLEMENT_APP(MyApp);
