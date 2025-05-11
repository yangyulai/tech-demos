
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
#include <fstream>

// 主窗口类：包含控件和事件处理函数
class MyFrame : public wxFrame {
public:
    MyFrame(const wxString& title);

private:
    // 控件成员变量
    wxFilePickerCtrl* m_filePickerCtrl;  // 文件选择器
    wxTextCtrl* m_filePathCtrl;    // 显示文件路径（只读）
    wxTextCtrl* m_inputTextCtrl;      // 输入文本框（单行）
    wxTextCtrl* m_outputTextCtrl;     // 输出结果文本框（多行只读）
    wxButton* m_toUpperButton;        // 转大写按钮
    wxButton* m_countButton;          // 统计字符按钮
    wxButton* m_clearButton;          // 清空输出按钮
    wxButton* m_exportButton;         // 导出结果按钮

    // 事件处理函数
    void OnFileSelected(wxFileDirPickerEvent& event); // 文件选择
    void OnConvertToUpper(wxCommandEvent& event);  // 将输入文本转换为大写
    void OnCountCharacters(wxCommandEvent& event); // 统计输入文本字符数
    void OnClearOutput(wxCommandEvent& event);     // 清空输出框内容
    void OnExportResult(wxCommandEvent& event);    // 导出结果到文本文件

    // 辅助函数：获取用户输入的文本并去除两端空白
    wxString GetInputText();
};

// 应用程序类
class MyApp : public wxApp {
public:
    virtual bool OnInit() {
        MyFrame* frame = new MyFrame(L"示例界面");
        frame->Show(true);
        return true;
    }
};

// 实现应用程序主类
wxIMPLEMENT_APP(MyApp);

// 构造函数：创建并布局界面
MyFrame::MyFrame(const wxString& title)
    : wxFrame(NULL, wxID_ANY, title, wxDefaultPosition, wxSize(600, 500))
{
    // 设置字体：使用微软雅黑以保证中文显示一致
    wxFont font(10, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, "Microsoft YaHei");

    // 主布局：垂直方向
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    // —— 文件选择 区域 —— 
    wxStaticBoxSizer* fileSizer = new wxStaticBoxSizer(wxHORIZONTAL, this, L"选择文件加载到输入：");
    m_filePickerCtrl = new wxFilePickerCtrl(this, wxID_ANY, wxEmptyString,
        "选择文本文件...",
        "Text files (*.txt)|*.txt|All files (*.*)|*.*",
        wxDefaultPosition, wxDefaultSize,
        wxFLP_FILE_MUST_EXIST | wxFLP_USE_TEXTCTRL);
    m_filePickerCtrl->SetFont(font);
    m_filePickerCtrl->Bind(wxEVT_FILEPICKER_CHANGED, &MyFrame::OnFileSelected, this);
    fileSizer->Add(m_filePickerCtrl, 1, wxEXPAND | wxALL, 5);
    mainSizer->Add(fileSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 10);
    // 显示所选文件路径
    //wxStaticText* pathLabel = new wxStaticText(this, wxID_ANY, "当前文件路径：");
    //pathLabel->SetFont(font);
    //fileSizer->Add(pathLabel, 0, wxLEFT | wxTOP, 5);

    //m_filePathCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
    //    wxTE_READONLY);
    //m_filePathCtrl->SetFont(font);
    //fileSizer->Add(m_filePathCtrl, 0, wxEXPAND | wxALL, 5);

    mainSizer->Add(fileSizer, 0, wxEXPAND | wxALL, 10);
    // 输入标签和文本框
    wxStaticText* inputLabel = new wxStaticText(this, wxID_ANY, L"输入文本：");
    inputLabel->SetFont(font);
    m_inputTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize);
    m_inputTextCtrl->SetFont(font);

    wxBoxSizer* inputSizer = new wxBoxSizer(wxHORIZONTAL);
    inputSizer->Add(inputLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    inputSizer->Add(m_inputTextCtrl, 1, wxEXPAND);
    mainSizer->Add(inputSizer, 0, wxEXPAND | wxALL, 10);

    // 输出标签和多行文本框
    wxStaticText* outputLabel = new wxStaticText(this, wxID_ANY, L"输出结果：");
    outputLabel->SetFont(font);
    m_outputTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
        wxTE_MULTILINE | wxTE_READONLY);
    m_outputTextCtrl->SetFont(font);

    wxBoxSizer* outputSizer = new wxBoxSizer(wxVERTICAL);
    outputSizer->Add(outputLabel, 0, wxBOTTOM, 5);
    outputSizer->Add(m_outputTextCtrl, 1, wxEXPAND);
    mainSizer->Add(outputSizer, 1, wxEXPAND | wxLEFT | wxRIGHT, 10);

    // 操作按钮
    m_toUpperButton = new wxButton(this, wxID_ANY, L"转为大写");
    m_toUpperButton->SetFont(font);
    m_toUpperButton->Bind(wxEVT_BUTTON, &MyFrame::OnConvertToUpper, this);

    m_countButton = new wxButton(this, wxID_ANY, L"统计字符");
    m_countButton->SetFont(font);
    m_countButton->Bind(wxEVT_BUTTON, &MyFrame::OnCountCharacters, this);

    m_clearButton = new wxButton(this, wxID_ANY, L"清空输出");
    m_clearButton->SetFont(font);
    m_clearButton->Bind(wxEVT_BUTTON, &MyFrame::OnClearOutput, this);

    m_exportButton = new wxButton(this, wxID_ANY, L"导出结果");
    m_exportButton->SetFont(font);
    m_exportButton->Bind(wxEVT_BUTTON, &MyFrame::OnExportResult, this);

    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    buttonSizer->Add(m_toUpperButton, 0, wxRIGHT, 10);
    buttonSizer->Add(m_countButton, 0, wxRIGHT, 10);
    buttonSizer->Add(m_clearButton, 0, wxRIGHT, 10);
    buttonSizer->Add(m_exportButton, 0);
    mainSizer->Add(buttonSizer, 0, wxALIGN_CENTER | wxALL, 10);

    // 应用布局并显示
    SetSizer(mainSizer);
    Layout();
}

// 辅助函数：获取并返回用户输入的文本，去除首尾空白
wxString MyFrame::GetInputText() {
    wxString text = m_inputTextCtrl->GetValue();
    text.Trim(true).Trim(false);
    return text;
}

// 事件处理：将输入文本转换为大写并显示在输出框
void MyFrame::OnConvertToUpper(wxCommandEvent& event) {
    wxString text = GetInputText();
    text.MakeUpper();  // 转大写
    m_outputTextCtrl->SetValue(text);
}
// 文件选择事件：将所选文件的 UTF-8 文本内容加载到输入框
void MyFrame::OnFileSelected(wxFileDirPickerEvent& event) {
    wxString path = event.GetPath();
    //m_filePathCtrl->SetValue(path);
    std::ifstream inFile(path.utf8_str(), std::ios::binary);
    if (!inFile) {
        wxMessageBox(L"无法打开所选文件", "错误", wxOK | wxICON_ERROR);
        return;
    }
    std::string content((std::istreambuf_iterator<char>(inFile)),
        std::istreambuf_iterator<char>());
    inFile.close();
    // 将读取的内容设到多行输入框
    m_inputTextCtrl->SetValue(wxString::FromUTF8(content));
}

// 事件处理：统计输入文本字符数并显示在输出框
void MyFrame::OnCountCharacters(wxCommandEvent& event) {
    wxString text = GetInputText();
    wxString result = wxString::Format("字符数：%llu", (unsigned long long)text.length());
    m_outputTextCtrl->SetValue(result);
}

// 事件处理：清空输出框内容
void MyFrame::OnClearOutput(wxCommandEvent& event) {
    m_outputTextCtrl->Clear();
}

// 事件处理：将输出框内容导出为 UTF-8 编码的文本文件
void MyFrame::OnExportResult(wxCommandEvent& event) {
    wxFileDialog saveFileDialog(this, L"保存结果", L"", L"",
        L"Text files (*.txt)|*.txt|All files (*.*)|*.*",
        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    if (saveFileDialog.ShowModal() == wxID_CANCEL) {
        return; // 用户取消
    }
    wxString path = saveFileDialog.GetPath();
    wxString content = m_outputTextCtrl->GetValue();

    // 使用 std::ofstream 保存为 UTF-8 编码
    std::ofstream outFile(path.utf8_str(), std::ios::binary);
    if (outFile) {
        std::string data = std::string(content.utf8_str());
        outFile << data;
        outFile.close();
    }
    else {
        wxMessageBox(L"无法打开文件进行写入", L"错误", wxOK | wxICON_ERROR);
    }
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
