#include "stdio.h"
#include "string.h"
#include "utility.h"
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/sysinfo.h>


static bool g_signal_registered = false;
static bool g_exit = false;
static std::vector<AW_RECT> last_rects;
int __attribute__((weak)) save_frame = 0;
int __attribute__((weak)) gpio_num_register = -1;
int __attribute__((weak)) gpio_do_register = 0;

AW_RECT intersect(AW_RECT r1, AW_RECT r2)
{
	AW_RECT z = {0, 0, 0, 0};
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;
	
	if (r1.x < r2.x)
	{
		if (r1.x+r1.w-1 >= r2.x)
			x1 = r2.x;
		else
			return z;
	}
	else
	{
		if (r2.x+r2.w-1 >= r1.x)
			x1 = r1.x;
		else
			return z;
	}
	
	if (r1.x+r1.w-1 > r2.x+r2.w-1)
	{
		if (r1.x < r2.x+r2.w-1)
			x2 = r2.x+r2.w-1;
		else
			return z;
	}
	else
	{
		if (r1.x+r1.w-1 >= r2.x)
			x2 = r1.x+r1.w-1;
		else
			return z;
	}

	if (r1.y < r2.y)
	{
		if (r1.y+r1.h-1 >= r2.y)
			y1 = r2.y;
		else
			return z;
	}
	else
	{
		if (r2.y+r2.h-1 >= r1.y)
			y1 = r1.y;
		else
			return z;
	}

	if (r1.y+r1.h-1 > r2.y+r2.h-1)
	{
		if (r1.y < r2.y+r2.h-1)
			y2 = r2.y+r2.h-1;
		else
			return z;
	}
	else
	{
		if (r1.y+r1.h-1 >= r2.y)
			y2 = r1.y+r1.h-1;
		else
			return z;
	}

	if (x2 < x1 || y2 < y1)
		return z;
	else
	{
		z.x = x1;
		z.w = x2 - x1 + 1;
		z.y = y1;
		z.h = y2 - y1 + 1;
		return z;
	}
}

float IOU(AW_RECT r1, AW_RECT r2)
{
	AW_RECT ri = intersect(r1, r2);
	return float(ri.w*ri.h)/(r1.w*r1.h + r2.w*r2.h - ri.w*ri.h);
}

std::vector<AW_RECT>& refine_rects(std::vector<AW_RECT>& rects)
{
	std::vector<AW_RECT> result;

	for(auto r : rects)
	{
		int i = 0;
		for (i=0; i<last_rects.size(); i++)
		{
			if (IOU(last_rects[i], r) > 0.8)
			{
				result.push_back(last_rects[i]);
				break;
			}
		}

		if (i >= last_rects.size())
			result.push_back(r);
	}

	last_rects = result;

	return last_rects;
}

#define IS_WHITE(c) ((c) == '\r' || (c) == '\n' || (c) == ' ' || (c) == '\t')

char *trim(char *s)
{
	while (IS_WHITE(*s))
		s++;

	char *end = s + strlen(s);
	while (--end >= s)
	{
		if (IS_WHITE(*end))
			*end = '\0';
		else
			break;
	}

	return s;
}

char *read_line(FILE *fp, char *buf, int len)
{
	for (;;)
	{
		char *s = fgets(buf, len, fp);
		if (NULL == s)
			return NULL;

		s = trim(s);

		if (s[0] != '#')
			return s;
	}
}

unsigned char *read_ppm(const char *filename, int *w, int *h, int *is_rgb)
{
	if (NULL == w || NULL == h)
		return NULL;
	
	FILE *fp = fopen(filename, "rb");
	if (NULL == fp)
	{
		printf("open %s failed\n", filename);
		return NULL;
	}

	char *line;
	line = (char *)malloc(1024);
	if (NULL == line)
	{
		printf("malloc failed\n");
		goto EXIT1;
	}

	char *s;
	if (NULL == (s = read_line(fp, line, 1024)))
	{
		printf("ppm format read error\n");
		goto EXIT2;
	}

	if (0 != strcmp(s, "P6"))
	{
		printf("only format P6 of ppm is supported\n");
		goto EXIT2;
	}

	if (NULL == (s = read_line(fp, line, 1024)))
	{
		printf("ppm width/height read error\n");
		goto EXIT2;
	}

	if (2 != sscanf(s, "%d %d", w, h))
	{
		printf("parse ppm \"width height\" failed\n");
		goto EXIT2;
	}

	if (NULL == (s = read_line(fp, line, 1024)))
	{
		printf("ppm depth read error\n");
		goto EXIT2;
	}

	if (0 != strcmp(s, "255"))
	{
		printf("ppm depth must be 255\n");
		goto EXIT2;
	}

	if (NULL != is_rgb)
		*is_rgb = 1;

	unsigned char *data;
	data = (unsigned char *)malloc((*w)*(*h)*3);
	if (NULL == data)
	{
		printf("malloc failed\n");
		goto EXIT2;
	}

	if ((*w)*(*h)*3 != fread(data, 1, (*w)*(*h)*3, fp))
	{
		printf("fread failed\n");
		goto EXIT3;
	}

	// convert rgb to rgba
	unsigned char *data2;
	data2 = (unsigned char *)malloc((*w)*(*h)*4);
	if (NULL == data2)
	{
		printf("malloc failed\n");
		goto EXIT3;
	}

	unsigned char *src;
	unsigned char *dst;
	src = data;
	dst = data2;
	for (int i=(*w)*(*h); i>0; i--)
	{
		*dst++ = *src++;	// r
		*dst++ = *src++;	// g
		*dst++ = *src++;	// b
		*dst++ = 255;		// alpha
	}

	free(data);
	free(line);
	fclose(fp);

	return data2;
	
EXIT3:
	free(data);

EXIT2:
	free(line);
	
EXIT1:
	fclose(fp);

	return NULL;
}

unsigned char *read_image(const char *filename, int *w, int *h, int *is_rgb)
{
	const char *s2 = strrchr(filename, '.');
	
	if (NULL != s2 && 0 == strcmp(s2, ".ppm"))
		return read_ppm(filename, w, h, is_rgb);
	
	if (NULL == s2 || (0 != strcmp(s2, ".rgba") && (0 != strcmp(s2, ".yuv"))))
	{
		printf("filename must be end with \".ppm\" or \".rgba\" or \".yuv\" (of type xxxx.###_###.argb or xxxx.###_###.yuv)\n");
		return NULL;
	}

	const char *s = s2 - 1;
	while (s >= filename && *s != '.')
		s--;

	if (s < filename || 2 != sscanf(s, ".%d_%d.", w, h))
	{
		printf("filename must be of type xxxx.###_###.rgba or xxxx.###_###.yuv\n");
		return NULL;
	}

	int rgb = (0 == strcmp(s2, ".rgba")) ? 1 : 0;
	if (NULL != is_rgb)
		*is_rgb = rgb;

	unsigned char *data = (unsigned char *)malloc(rgb ? ((*w)*(*h)*4) : ((*w)*(*h)*3/2));
	if (NULL == data)
	{
		printf("malloc failed\n");
		return NULL;
	}
	
	FILE *fp = fopen(filename, "rb");
	if (NULL == fp)
	{
		printf("open %s failed\n", filename);
		return NULL;
	}

	if ((rgb && (*w)*(*h)*4 != fread(data, 1, (*w)*(*h)*4, fp)) ||
		(!rgb && (*w)*(*h)*3/2 != fread(data, 1, (*w)*(*h)*3/2, fp)))
	{
		printf("fread failed\n");
		return NULL;
	}

	fclose(fp);

	return data;
}

std::vector<std::string> list_dir(std::string path)
{
	DIR *dirp; 
	struct dirent *dp;
	std::vector<std::string> result;
	
	dirp = opendir(path.c_str());
	if (NULL == dirp)
		return result;
	
	while ((dp = readdir(dirp)) != NULL)
	{
		if (dp->d_name[0] == '.')
			continue;
		
		// printf("%s\n", dp->d_name );
		result.push_back(path + "/" + dp->d_name);
	}
	
	closedir(dirp);
	
	return result;
}

void handle_signal(int signo)
{
	if (signo == SIGINT)
	{
		printf("catch Ctrl + C, exit normally\n");
		g_exit = true;
	}
}

int export_gpio(int gpio_num)
{
	FILE *fp;
	int ret = -1;
	int u8strlen;
	char str[10];

	if ((fp = fopen("/sys/class/gpio/export", "ab")) == NULL)
	{
		printf("Cannot open export file.\n");
		return ret;
	}

	rewind(fp);

	memset(str, 0x00, sizeof(str));
	sprintf(str, "%d", gpio_num);
	u8strlen = strlen(str);
	fwrite(str, sizeof(char), u8strlen, fp);

	fclose(fp);
	printf("user export gpio:%d ok\n",gpio_num);
}

int set_gpio_direction(int gpio_num, char direction)
{
	char path[100];
	int u8strlen;
	int ret = -1;
	FILE *fp;

	memset(path, 0x00, sizeof(path));
	sprintf(path, "/sys/class/gpio/gpio%d/direction", gpio_num);
	if ((fp = fopen(path, "rb+")) == NULL)
	{
		printf("Cannot open direction file.\n");
		return ret;
	}

	rewind(fp);
	memset(path, 0x00, sizeof(path));

	if(direction)
	{
		memcpy(path, "in", 2);
	}
	else
	{
		memcpy(path, "out", 3);
	}

	u8strlen = strlen(path);
	fwrite(path, sizeof(char), u8strlen, fp);

	fclose(fp);
	printf("set gpio:%d direction %s ok\n",gpio_num, direction?"in":"out");
}

int get_gpio_value(int gpio_num, int *value)
{
	char get_value = 0; 
	char path[128];
	int ret = -1;
	FILE *fp;

	memset(path, 0x00, sizeof(path));
	sprintf(path, "/sys/class/gpio/gpio%d/value", gpio_num);
	if ((fp = fopen(path, "rb+")) == NULL)
	{
		printf("Cannot open value file(read).\n");
		return ret;
	}

	rewind(fp);
	ret = fread(&get_value, sizeof(char), 1, fp);
	if(-1 !=ret) 
	{
		*value = get_value;
	}

	fclose(fp);

	return ret;
}

void *thread_gpio(void *arg)
{
	int gpio_value= 0;

	// printf("gpio_num_register=%d\n", gpio_num_register);

	if (-1 == gpio_num_register)
		return NULL;

	export_gpio(gpio_num_register);
	set_gpio_direction(gpio_num_register, 1);
	
	while (!g_exit)
	{
		usleep(200*1000);

		if (-1 != get_gpio_value(gpio_num_register, &gpio_value))
		{
			// printf("gpio_value=%d\n", gpio_value);
			if (gpio_value != '0')
			{
				gpio_do_register = 1;
				for (int i=0; !g_exit && i < 10; i++)
					usleep(200*1000);
			}
		}
	}

	return NULL;
}

int read_char()
{
	int in;
	struct termios new_settings;
	struct termios stored_settings;
	tcgetattr(0,&stored_settings);
	new_settings = stored_settings;
	new_settings.c_lflag &= (~ICANON);
	new_settings.c_cc[VTIME] = 0;
	tcgetattr(0,&stored_settings);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0,TCSANOW,&new_settings);
	 
	in = getchar();
	 
	tcsetattr(0,TCSANOW,&stored_settings);
	return in;
}

int wait_key_q_or_break()
{
	int key = -1;
	int ret = -1;
	pthread_t tid_gpio;

	pthread_create(&tid_gpio, NULL, thread_gpio, NULL);

	if (!g_signal_registered)
	{
		g_signal_registered = true;
		struct sigaction sigAction;
		sigAction.sa_handler = handle_signal;
		sigemptyset(&sigAction.sa_mask);
		sigAction.sa_flags = 0;
		sigaction(SIGINT, &sigAction, NULL);
	}

	while (!g_exit)
	{
		key = read_char();
		if (key == 'q' || key == 'Q')
			break;

		if (key == 'c')
		{
			extern int save_frame;
			save_frame = 1;
		}
		
		usleep(100*1000);
	}

	g_exit = true;
	pthread_join(tid_gpio,  NULL);

	return key;
}

void SYS_CtrlGetMemInfo(const char *func, int line)
{
    int totalKB = 0;
    int freeKB = 0;
    struct sysinfo info;

    if (-1 == sysinfo(&info))
    {
        return;
    }
    totalKB = info.totalram >> 10;
    freeKB = info.freeram >> 10;

    printf("%s %d total:%dKB, free:%dKB\n", func, line, totalKB, freeKB);

    return;
}


