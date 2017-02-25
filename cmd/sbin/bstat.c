
#include <bioctl.h>
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv)
{
	int cnt;
	int i;
	
	cnt = _bdev_max();
	if (cnt < 0)
		err(1, NULL);
	
	struct bdev_stat bs[cnt];
	
	if (_bdev_stat(bs))
		err(1, NULL);
	
	puts("NAME             READS    WRITES    ERRORS");
	puts("------------ --------- --------- ---------");
	
	for (i = 0; i < cnt; i++)
		if (*bs[i].name)
		{
			if (!bs[i].read_cnt && !bs[i].write_cnt && !bs[i].error_cnt)
				continue;
			
			printf("%-12s %9ji %9ji %9ji\n",
				bs[i].name,
				(uintmax_t)bs[i].read_cnt,
				(uintmax_t)bs[i].write_cnt,
				(uintmax_t)bs[i].error_cnt);
		}
	
	return 0;
}
