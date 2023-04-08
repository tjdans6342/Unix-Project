#define MAX 50
#define START_IDX 1

struct locker {
	int is_used;
	char id[100];
	char pw[100];
	
	int num;
	char contents[MAX][30];
};
