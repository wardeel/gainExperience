static int config_emmc_read(char *emmc, char *buf, unsigned int offset, unsigned int size)
{
	int ret = -1;
	int fd = -1;
	
	if(NULL == emmc || NULL == buf||0 == size)
	{
		printf("emmc_read: NULL pointer err\r\n");
		ret = -1;
		goto out;
	}
	fd = open(emmc, O_RDONLY);
	if(fd < 0)
	{
		printf("emmc_read open<%s> failed, message<%s>!\r\n", emmc, strerror(errno));
		ret = -1;
		goto out;
	}
	if(lseek(fd, offset, SEEK_SET) == (off_t) - 1)
	{
		printf("Lseek sector error: errno=%d\r\n", errno);
		ret = -1;
		goto out;
	}
	ret = read(fd, buf, size);
	if(ret != size)
	{
		printf("emmc_read: read <%s> failed, message<%s>!\r\n", emmc, strerror(errno));
		ret = -1;
		goto out;
	}		
out:
	if(fd >= 0)
		close(fd);
	return ret;
}

static int config_emmc_write(char *emmc, char *buf, unsigned int offset, unsigned int size)
{
	int ret = -1;
	int fd = -1;
	
	if(NULL == emmc||NULL ==buf|| (size <= 0))
	{
		printf("emmc_write: NULL pointer err\r\n");
		ret = 1;
		goto out;
	}
	
	fd = open(emmc, O_RDWR);
	if(fd < 0)
	{
		printf("emmc_write: open <%s> failed, message<%s>!\r\n", emmc, strerror(errno));
		ret =-1;
		goto out;
	}
	if(lseek(fd, offset, SEEK_SET) == (off_t) - 1)
	{
		printf("Lseek sector error: errno = %d\r\n", errno);
		ret = -1;
		goto out;
	}
	ret = write(fd, buf, size);
	if(ret != size)
	{
		printf("emmc_write: write <%s> failed, message<%s>\r\n", emmc, strerror(errno));
		ret = -1;
		goto out;
	}
	if (ERROR == fsync(fd))
	{
		printf("fsync error: errno = %d\r\n", errno);
	}
	
out:
	if(fd >= 0)
		close(fd);
	
	return ret;
	
}
