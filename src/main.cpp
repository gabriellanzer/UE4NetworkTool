
// Hide Console Window
// #pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")

#include <app/app.h>
#include <fstream>

int main(int argc, char** argv)
{
	setlocale(LC_ALL, "Portuguese");

	UE4ServerBootstrap app;
	app.Run();

	return 0;
}