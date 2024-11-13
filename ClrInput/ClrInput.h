#pragma once
#include <windows.h>
using namespace System;

namespace ClrInput {
	public ref class Process
	{
		// TODO: 在此处为此类添加方法。
	public:
		static bool SendMessageToActiveWindow(System::String^ title, System::String^ message);
		static bool StartProcessAsCurrentUser(System::String^ appPath, System::String^ cmdLineArgs, System::String^ workDir, bool visible);
	private:
		static bool GetSessionUserToken(PHANDLE phUserToken);
	};
}
