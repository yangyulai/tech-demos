#include <iostream>
#include "CommonUtils/SvnWrapper.hpp"
int main()
{
	SVNController svn;
	auto logResult = svn.executeCommand(
		SVNController::Operation::Log,
		{ "-l", "5" },  // 显示最后5条日志
		"./"
	);
	std::cout << "Hello, World!" << std::endl;
	return 0;
}
