
#include <systat.h>
#include <stdio.h>
#include <err.h>

int main(int argc, char **argv)
{
	struct systat st;
	
	if (_systat(&st))
		err(1, "_systat");
	
	printf("task_avail = %i\n",	(int)st.task_avail);
	printf("task_max   = %i\n",	(int)st.task_max);
	printf("core_avail = %lli\n",	(long long)st.core_avail);
	printf("core_max   = %lli\n",	(long long)st.core_max);
	printf("kva_avail  = %lli\n",	(long long)st.kva_avail);
	printf("kva_max    = %lli\n",	(long long)st.kva_max);
	printf("file_avail = %i\n",	(int)st.file_avail);
	printf("file_max   = %i\n",	(int)st.file_max);
	printf("fso_avail  = %i\n",	(int)st.fso_avail);
	printf("fso_max    = %i\n",	(int)st.fso_max);
	printf("blk_dirty  = %i\n",	(int)st.blk_dirty);
	printf("blk_valid  = %i\n",	(int)st.blk_valid);
	printf("blk_avail  = %i\n",	(int)st.blk_avail);
	printf("blk_max    = %i\n",	(int)st.blk_max);
	printf("sw_freq    = %i\n",	(int)st.sw_freq);
	printf("hz         = %i\n",	(int)st.hz);
	printf("uptime     = %i\n",	(int)st.uptime);
	printf("cpu        = %i\n",	(int)st.cpu);
	printf("cpu_max    = %i\n",	(int)st.cpu_max);
	
	return 0;
}
