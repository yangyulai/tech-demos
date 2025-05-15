#include <iostream>
#include "CommonUtils/SvnWrapper.hpp"
int main()
{
	SVNController svn;
	auto logResult = svn.executeCommand(
		SVNController::Operation::Log,
		{ "-l", "5" },  // ��ʾ���5����־
		"./"
	);
	std::cout << "Hello, World!" << std::endl;
	return 0;
}
