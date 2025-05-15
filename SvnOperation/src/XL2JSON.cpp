#include "stringex.h"
#include "XL2JSON.h"
#include "AppConfig.h"
#include <ranges>

XL2JSON::XL2JSON() : config(std::make_unique<ConfigLoader>())
{
	lua.open_libraries(sol::lib::base, sol::lib::io, sol::lib::string);
	lua.script_file(utf8ToString(app_config::ToJsonConfig));
}

void XL2JSON::set_paths(std::string str)
{
	std::string input(utf8ToString(str));
	std::stringstream iss(input);
	std::string word;
	while (iss >> word)
	{
		paths.emplace_back(word);
	}
}

void XL2JSON::analyze_excel_columns()
{
	try
	{
		std::vector<xlnt::row_t> empty_rows;
		const auto& const_ws = ws;
		auto max_row = const_ws.highest_row();
		auto max_col = const_ws.highest_column();
		for (xlnt::row_t r = 2; r <= max_row; ++r) {
			bool has_data = false;
			for (xlnt::column_t c = 1; c <= max_col; ++c) {
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
		sol::optional<sol::table> maybe_tbl = lua[table_name];
		if (!maybe_tbl) {
			throw std::runtime_error("ToJsonConfig.lua未配置"+table_name);
		}
		sol::table tbl = *maybe_tbl;
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
		max_col = const_ws.highest_column();
		for (xlnt::column_t c = 1; c <= max_col; ++c) {
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
	std::string full_path = app_config::json_path + table_name + suffix;
	std::ofstream out_file(full_path, std::ios_base::out);
	if (!out_file.is_open()) {
		throw std::runtime_error(std::format("打开文件失败：{}", full_path));
	}
	out_file << "[\n";
	bool first_row = true;
	std::vector<xlnt::row_t> empty_rows;
	const auto& const_ws = ws;
	auto max_row = const_ws.highest_row();
	for (xlnt::row_t r = 2; r <= max_row; ++r) {
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
			}else
			{
				auto value = info.to_json("");
				if (it!=--_columns.end())
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
			//export_struct();
		}
		catch (const std::exception& e) {
			throw std::runtime_error(std::format("处理文件时出错:{}\n", e.what()));
		}

	}
}

void XL2JSON::active_sheet(const std::wstring& path)
{
	wb.clear();
	wb.load(path);
	ws = wb.active_sheet();
}
