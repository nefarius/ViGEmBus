// app.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ViGEm/Client.h>
#include <iostream>

#pragma comment(lib, "setupapi.lib")

int main()
{
	const auto client = vigem_alloc();

	auto error = vigem_connect(client);

	const auto ds4 = vigem_target_ds4_alloc();

	error = vigem_target_add(client, ds4);

	DS4_OUTPUT_BUFFER out;

	while (TRUE) {
		error = vigem_target_ds4_await_output_report(client, ds4, &out);
	}
}
