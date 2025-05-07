
#include "BeastHttp.hpp"
#include <boost/asio.hpp>
#include <boost/uuid.hpp>
#include <iostream>
#include <wx/wx.h>
#include <wx/panel.h>
#include <wx/frame.h>
#include <wx/sizer.h>
#include <wx/filepicker.h>
#include <wx/event.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/font.h>
// IDs for controls
enum {
    ID_TextInput = wxID_HIGHEST + 1,
    ID_FilePicker,
    ID_Button,
    ID_Button1,
    ID_Button2,
    ID_Button3
};

class MyFrame : public wxFrame {
public:
    MyFrame();

private:
    wxTextCtrl* m_textInput;
    wxFilePickerCtrl* m_filePicker;
    wxTextCtrl* m_outputBox;
    wxButton* m_button;
    wxButton* m_button1;
    wxButton* m_button2;
    wxButton* m_button3;

    void OnButton1Clicked(wxCommandEvent& event);
    void OnButton2Clicked(wxCommandEvent& event);
    void OnButton3Clicked(wxCommandEvent& event);
    void OnButtonClicked(wxCommandEvent& event);
    wxDECLARE_EVENT_TABLE();
};
wxBEGIN_EVENT_TABLE(MyFrame, wxFrame)
EVT_BUTTON(ID_Button, MyFrame::OnButtonClicked)
EVT_BUTTON(ID_Button1, MyFrame::OnButton1Clicked)
EVT_BUTTON(ID_Button2, MyFrame::OnButton2Clicked)
EVT_BUTTON(ID_Button3, MyFrame::OnButton3Clicked)
wxEND_EVENT_TABLE()

class MyApp : public wxApp {
public:
    bool OnInit() override {
        // 设置全局中文字体
        wxFont font(12, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false, "Microsoft YaHei");
        MyFrame* frame = new MyFrame();
        frame->SetFont(font);
        frame->Show(true);
        return true;
    }
};



wxIMPLEMENT_APP(MyApp);

MyFrame::MyFrame()
    : wxFrame(nullptr, wxID_ANY, L"xx工具", wxDefaultPosition, wxSize(500, 400))
{
    wxPanel* panel = new wxPanel(this);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    // 输入区域
    wxStaticBoxSizer* inputSizer = new wxStaticBoxSizer(wxVERTICAL, panel, L"输入");
    {
        wxFlexGridSizer* gridSizer = new wxFlexGridSizer(2, 5, 5);
        gridSizer->AddGrowableCol(1);

        // 文字输入框
        gridSizer->Add(new wxStaticText(panel, wxID_ANY, L"文本输入:"), 0, wxALIGN_CENTER_VERTICAL);
        m_textInput = new wxTextCtrl(panel, ID_TextInput);
        gridSizer->Add(m_textInput, 1, wxEXPAND);

        // 文件选择框
        gridSizer->Add(new wxStaticText(panel, wxID_ANY, L"文件选择:"), 0, wxALIGN_CENTER_VERTICAL);
        m_filePicker = new wxFilePickerCtrl(panel, ID_FilePicker, wxEmptyString,
            "选择文件...", "*.*");
        gridSizer->Add(m_filePicker, 1, wxEXPAND);

        inputSizer->Add(gridSizer, 1, wxEXPAND | wxALL, 5);
    }
    // 2. 新增的按钮区域
    wxStaticBoxSizer* buttonSizer2 = new wxStaticBoxSizer(wxHORIZONTAL, panel, L"操作按钮");
    {
        m_button1 = new wxButton(panel, ID_Button1, L"处理输入");
        m_button2 = new wxButton(panel, ID_Button2, L"清空内容");
        m_button3 = new wxButton(panel, ID_Button3, L"导出结果");

        // 设置按钮之间的间距
        buttonSizer2->Add(m_button1, 0, wxALL, 5);
        buttonSizer2->Add(m_button2, 0, wxALL, 5);
        buttonSizer2->Add(m_button3, 0, wxALL, 5);
        buttonSizer2->AddStretchSpacer(); // 让按钮靠左对齐
    }
    // 按钮
    m_button = new wxButton(panel, ID_Button, L"处理输入");
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->AddStretchSpacer();
    buttonSizer->Add(m_button, 0, wxALIGN_CENTER);
    buttonSizer->AddStretchSpacer();

    // 输出区域
    wxStaticBoxSizer* outputSizer = new wxStaticBoxSizer(wxVERTICAL, panel, L"输出");
    m_outputBox = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
    outputSizer->Add(m_outputBox, 1, wxEXPAND | wxALL, 5);

    // 组合所有控件
    mainSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 5);
    mainSizer->Add(buttonSizer, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    mainSizer->Add(buttonSizer2, 0, wxEXPAND | wxTOP | wxBOTTOM, 10);
    mainSizer->Add(outputSizer, 1, wxEXPAND | wxALL, 5);

    panel->SetSizer(mainSizer);

    // 设置窗口最小大小
    SetMinSize(wxSize(400, 300));
}

void MyFrame::OnButtonClicked(wxCommandEvent& WXUNUSED(event)) {
    wxString text = m_textInput->GetValue();
    wxString file = m_filePicker->GetPath();

    wxString output;
    output << L"文本输入内容: " << text << L"\n\n";
    output << L"选择的文件路径: " << file;

    m_outputBox->SetValue(output);
    m_outputBox->SetInsertionPointEnd();  // 滚动到末尾
}
void MyFrame::OnButton1Clicked(wxCommandEvent& event) {
    wxString text = m_textInput->GetValue();
    wxString file = m_filePicker->GetPath();

    wxString output;
    output << "文本输入内容: " << text << "\n\n";
    output << "选择的文件路径: " << file;

    m_outputBox->SetValue(output);
    m_outputBox->SetInsertionPointEnd();
}

void MyFrame::OnButton2Clicked(wxCommandEvent& event) {
    m_textInput->Clear();
    m_filePicker->SetPath("");
    m_outputBox->Clear();
}

void MyFrame::OnButton3Clicked(wxCommandEvent& event) {
    wxString message = "此处应实现导出功能\n当前内容:\n" + m_outputBox->GetValue();
    wxMessageBox(message, "导出结果", wxOK | wxICON_INFORMATION, this);
}
//
//int main() {
//    boost::asio::io_context ioc;
//	HttpClient client(ioc);
//	auto result = client.get("175.24.114.29", "19880", "/token.php");
//	if (result.empty()) {
//		std::cerr << "Failed to get response from server.\n";
//		return 1;
//	}
//	std::cout << "Server response: " << result << "\n";
//    return 0;
//}



// launcher.cpp
//#include <boost/beast.hpp>
//#include <boost/asio.hpp>
//#include <boost/process.hpp>
//#include <nlohmann/json.hpp>
//#include <iostream>
//#include <string>
//#include <boost/process/v1/child.hpp>
//#include <boost/process/v1/exe.hpp>
//#include <boost/process/v1/args.hpp>
//#include <boost/process/v1/io.hpp>
//#include <boost/algorithm/string/predicate.hpp>
//namespace beast   = boost::beast;     // from <boost/beast.hpp>
//namespace http    = beast::http;      // from <boost/beast/http.hpp>
//namespace asio    = boost::asio;      // from <boost/asio.hpp>
//namespace bp      = boost::process::v1;   // from <boost/process.hpp>
//using json        = nlohmann::json;
//
//int main(int argc, char* argv[]) {
//    try {
//        // 1. 构造 I/O 上下文
//        asio::io_context ioc;
//
//        // 2. 解析地址和端口
//        const std::string host = "175.24.114.29";
//        const std::string port = "19880";
//        const std::string target = "/token.php";
//        const int version = 11; // HTTP/1.1
//
//        // 3. 建立 TCP 连接
//        asio::ip::tcp::resolver resolver{ioc};
//        auto const results = resolver.resolve(host, port);
//        beast::tcp_stream stream{ioc};
//        stream.connect(results);
//
//        // 4. 发送 HTTP GET
//        http::request<http::string_body> req{http::verb::get, target, version};
//        req.set(http::field::host, host);
//        req.set(http::field::user_agent, "MyLauncher/1.0");
//        http::write(stream, req);
//
//        // 5. 读取响应
//        beast::flat_buffer buffer;
//        http::response<http::string_body> res;
//        http::read(stream, buffer, res);
//
//        // 6. 关闭连接
//        beast::error_code ec;
//        stream.socket().shutdown(asio::ip::tcp::socket::shutdown_both, ec);
//        // (忽略 shutdown 错误)
//
//        if (res.result() != http::status::ok) {
//            std::cerr << "Config server returned " << res.result_int() << "\n";
//            return 1;
//        }
//        auto args = res.body();
//
//
//        // 9. 启动子进程
//        bp::child server_proc(
//            bp::exe = R"(F:\SkillRoad\SVR\gameserver.exe)",
//            bp::args = args
//        );
//
//        std::cout << "Launched server (pid=" << server_proc.id() << ")\n";
//
//        // 10. 等待服务器退出（也可以不等待，直接返回）
//        server_proc.wait();
//        return server_proc.exit_code();
//    }
//    catch (std::exception& e) {
//        std::cerr << "Error: " << e.what() << "\n";
//        return 1;
//    }
//}
