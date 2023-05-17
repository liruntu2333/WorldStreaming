#include "MeshSplitter.h"

int main(int argc, char** argv)
{
	if (argc == 1)
	{
		MeshSplitter splitter;
		splitter.Split();
	}
	else
	{
		MeshSplitter splitter(argv[1]);
		splitter.Split();
	}

	return 0;
}
