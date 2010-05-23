#include "jansson.h"
#include "stdio.h"

int main()
{
	json_t* arr = json_array();
	json_array_append_new(arr, json_string("test"));

	printf("%s", json_dumps(arr, JSON_INDENT(2)));
	getchar();
	return 0;
}