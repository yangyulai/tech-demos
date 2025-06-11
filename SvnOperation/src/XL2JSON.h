#pragma once
#include <string>
#include <xlnt/xlnt.hpp>
#include <filesystem>
#include <fstream>
#include <format>
#include "ConfigLoader.h"
#include <sol/sol.hpp>


struct ColumnInfo
{
	std::string name;
	std::string type;
	std::string desc;
	std::string to_json(std::string_view field_name)
	{
		if (type == "std::string")
		{
			return std::format(R"("{}": "{}")", name, field_name);
		}
		else if (type == "int" || type == "int8_t" || type == "int16_t" || type == "int32_t"
			|| type == "int64_t" || type == "uint8_t" || type == "uint16_t" || type == "uint32_t"
			|| type == "uint64_t" || type == "short" || type == "char" || type == "float" || type == "bool")
		{
			if (field_name.empty())
			{
				return std::format(R"("{}": 0)", name);
			}
			else
			{
				return std::format(R"("{}": {})", name, field_name);
			}
		}
		else if (type.starts_with("std::vector"))
		{
			if (field_name.empty() || field_name == "0")
			{
				return std::format(R"("{}": [])", name);

			}
			else
			{
				return std::format(R"("{}": [{}])", name, field_name);

			}
		}
		return "";
	}
};

class XL2JSON
{
	xlnt::workbook wb;
	xlnt::worksheet ws;
	std::string table_name;
	std::unordered_map<std::string,ColumnInfo> column_info;
	std::vector<std::filesystem::path> paths;
	std::string cpp_path;
	sol::state lua;
	std::vector<std::pair<xlnt::column_t,ColumnInfo>> _columns;

public:
	explicit XL2JSON();
	void set_paths(std::string str);
	void analyze_excel_columns();
	void excel_to_json();
	void export_struct();
	void init(const std::filesystem::path& path);
	void execute();
	void active_sheet(const std::wstring& path);
	std::string suffix = ".json";
	std::string cpp_file_name = "JsonStruct.h";
};

