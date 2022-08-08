// app.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <iomanip>
#include <Windows.h>
#include <ViGEm/Client.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <bitset>

#pragma comment(lib, "setupapi.lib")

static std::string hexStr(unsigned char* data, int len)
{
	std::stringstream ss;
	ss << std::hex;
	for (int i = 0; i < len; ++i)
		ss << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]) << ' ';
	return ss.str();
}

int main()
{
	const auto client = vigem_alloc();

	auto error = vigem_connect(client);

	const auto ds4 = vigem_target_ds4_alloc();

	error = vigem_target_add(client, ds4);

	DS4_OUTPUT_BUFFER out;

	while (TRUE) 
	{
		error = vigem_target_ds4_await_output_report(client, ds4, INFINITE, &out);
		
		if (VIGEM_SUCCESS(error))
		{
			std::cout << hexStr(out.Buffer, sizeof(DS4_OUTPUT_BUFFER)) << std::endl;
		}
		else if (error != VIGEM_ERROR_TIMED_OUT)
		{
			auto win32 = GetLastError();

			auto t = 0;
		}
	}
}
