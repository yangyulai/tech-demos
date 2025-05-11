#include <print>
#include "stringex.h"
#include "XL2JSON.h"

#include <ranges>

XL2JSON::XL2JSON(const std::string& file_name, const std::wstring& luaScriptPath) : config(std::make_unique<ConfigLoader>())
{
	if (auto result = config->Load(file_name); !result)
	{
		std::println("加载失败: {}", result.error());
	}
	lua.open_libraries(sol::lib::base, sol::lib::io, sol::lib::string);
	lua.script_file(wstring_to_ansi(luaScriptPath));
	server_path = config->GetString("server_path").value();
	cpp_path = config->GetString("cpp_path").value();
}

void XL2JSON::set_paths(std::string_view str)
{
	std::string input(str);
	std::stringstream iss(input);
	std::string word;
	while (iss >> word)
	{
		paths.emplace_back(word);
	}
}
void XL2JSON::cin_paths()
{
	paths = str::cin();
}

void XL2JSON::show_tips() const
{
	std::print("表格转前后端JSON配置工具\n");
	std::print("将文件拖入（支持多文件同时拖入）\n");
	std::print("当前后端文件路径：{}\n", server_path);
}

void XL2JSON::analyze_excel_columns()
{
	try
	{
		std::vector<xlnt::row_t> empty_rows;
		const auto& const_ws = ws;
		for (xlnt::row_t r = 2; r <= const_ws.highest_row(); ++r) {
			bool has_data = false;
			for (xlnt::column_t c = 1; c <= const_ws.highest_column(); ++c) {
				if (const_ws.has_cell(xlnt::cell_reference(c, r))) {
					const auto& cell = const_ws.cell(xlnt::cell_reference(c, r));
					if (!cell.to_string().empty()) {
						has_data = true;
						break;
					}
				}
			}
			if (!has_data) {
				empty_rows.push_back(r);
			}
		}
		std::for_each(empty_rows.rbegin(), empty_rows.rend(), [&](xlnt::row_t r) {
			ws.delete_rows(r, 1);
			});

		sol::table tbl = lua[table_name];
		for (auto& [_,v]:tbl)
		{
			if (v.is<sol::table>())
			{
				sol::table value = v.as<sol::table>();
				std::string column_name = value.get<std::string>("column_name");
				ColumnInfo info{
					.name = value.get<std::string>("name"),
					.type = value.get<std::string>("type"),
					.desc = value.get<std::string>("desc")
				};
				column_info[column_name] = info;
			}

		}
		for (xlnt::column_t c = 1; c <= const_ws.highest_column(); ++c) {
 			if (const_ws.has_cell(xlnt::cell_reference(c, 1))) {
				const auto& cell = const_ws.cell(xlnt::cell_reference(c, 1));
				auto cell_string = cell.to_string();
				auto sub_tbl = tbl[cell_string];
				if (column_info.contains(cell_string))
				{
					_columns.emplace_back(c ,column_info[cell_string]);
				}
			}
		}
	}
	catch (const std::exception& e) {
		throw std::runtime_error(e.what());
	}
}

void XL2JSON::excel_to_json() {
	std::ofstream out_file(server_path + table_name + suffix, std::ios_base::out);
	out_file << "[\n";
	bool first_row = true;
	std::vector<xlnt::row_t> empty_rows;
	const auto& const_ws = ws;
	for (xlnt::row_t r = 2; r <= const_ws.highest_row(); ++r) {
		std::ostringstream row_stream;
		row_stream << "  {\n";
		size_t col_idx = 0;
		for (auto it = _columns.begin();it!=_columns.end();++it)
		{
			auto& [c, info] = *it;
			if (const_ws.has_cell(xlnt::cell_reference(c, r))) {
				const auto& cell = const_ws.cell(xlnt::cell_reference(c, r));

				auto value = info.to_json(cell.to_string());
				if (it != --_columns.end())
				{
					value.append(",");
				}
				if (col_idx++ > 0) row_stream << "\n";
				row_stream << "    " << value;
			}
		}
		row_stream << "\n  }";

		// 写入文件
		if (col_idx > 0) {
			if (!first_row) out_file << ",\n";
			first_row = false;
			out_file << row_stream.str();
		}
		row_stream.str().pop_back();
	}
	out_file << "\n]";
	out_file.close();
}

void XL2JSON::export_struct() {
	sol::function export_struct = lua["export_struct"];
	export_struct.call();
	return;
	if (cpp_path.empty()) return;
	std::ofstream out_file(cpp_path + cpp_file_name, std::ios_base::out);
	out_file << table_name << " = {\n";
	for (auto& info : _columns | std::views::values)
	{
		out_file << "	" << info.type << " " << info.name << ";";
	}
	out_file << "\n};";
	out_file.close();
}
void XL2JSON::init(const std::filesystem::path& path)
{
	column_info.clear();
	_columns.clear();
	table_name = path.stem().string();
	active_sheet(path);
}


void XL2JSON::execute()
{
	for (auto& path : paths)
	{
		try
		{
			init(path);
			analyze_excel_columns();
			excel_to_json();
			export_struct();
			std::print("导入数据库成功\n");
			std::print("请拖拽文件\n");
		}
		catch (const std::exception& e) {
			std::print("处理文件时出错:{}\n", e.what());
		}

	}
}

void XL2JSON::active_sheet(const std::wstring& path)
{
	wb.clear();
	wb.load(path);
	ws = wb.active_sheet();
}
