#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define MAX_SECTION 128

#define MAX_LINE 2048
static char linebuf[MAX_LINE];

int read_line(FILE *fr)
{
	int i = 0;
	int c;
	for(;;)
	{
		c = fgetc(fr);
		switch(c)
		{
			case '\r':
				break;
			case '\n':
			case EOF:
				linebuf[i] = '\0';
				break;
			default:
				if(i < MAX_LINE-1)
				{
					linebuf[i] = c;
					i++;
				}
				break;
		}
		
		if(c == '\n') break;
		if(c == EOF)
		{
			if(i == 0) return 0;
		}
	}
	
	return 1;
}

void put_line(FILE *fw)
{
	size_t s = strlen(linebuf);
	fwrite(linebuf, 1, s, fw);
	fputc('\r', fw);
	fputc('\n', fw);
}

void help(const char *progname)
{
	printf("%s [options] <source inf> <dest. inf> [enabled-section-1, [enable-section-2, [...]]]\n"
		"options:\n"
		"\t--softgpu\n"
		"\t--build <num>\n"
		"\t--day <num>\n"
		"\t--month <num>\n"
		"\t--year <num>\n\n",
		progname);
}

int main(int argc, char **argv)
{
	int i = 0;
	char **sections = calloc(argc, sizeof(char**));
	int sections_count = 0;
	char *src = NULL;
	char *dst = NULL;
	int softgpu = 0;
	int build = 0;
	int day;
	int month;
	int year;
	int print_help = 0;
	char section_test[MAX_SECTION+3];
	
	time_t atime;
  struct tm *gmt;

  time(&atime);
  gmt = gmtime(&atime);
  
  day   = gmt->tm_mday;
  month = gmt->tm_mon + 1;
  year  = gmt->tm_year + 1900;
	
	for(i = 1; i < argc; i++)
	{
		if(strcmp(argv[i], "--help") == 0)
		{
			print_help = 1;
		}
		else if(strcmp(argv[i], "--softgpu") == 0)
		{
			softgpu = 1;
		}
		else if(strcmp(argv[i], "--build") == 0)
		{
			if(i+1 < argc)
			{
				build = atoi(argv[i+1]);
				i++;
			}
		}
		else if(strcmp(argv[i], "--day") == 0)
		{
			if(i+1 < argc)
			{
				day = atoi(argv[i+1]);
				i++;
			}
		}
		else if(strcmp(argv[i], "--month") == 0)
		{
			if(i+1 < argc)
			{
				month = atoi(argv[i+1]);
				i++;
			}
		}
		else if(strcmp(argv[i], "--year") == 0)
		{
			if(i+1 < argc)
			{
				year = atoi(argv[i+1]);
				i++;
			}
		}
		else
		{
			if(src == NULL)
			{
				src = argv[i];
			}
			else if(dst == NULL)
			{
				dst = argv[i];
			}
			else
			{
				sections[sections_count] = argv[i];
				sections_count++;
			}
		}
	}
	
	if(print_help)
	{
		help(argv[0]);
		return 0;
	}
	
	if(dst == NULL)
	{
		printf("source and destination are required!\n\n");
		help(argv[0]);
		return 1;
	}
	
	FILE *out = NULL;
	FILE *in  = fopen(src, "rb");
	if(in)
	{
		out = fopen(dst, "wb");
		if(out)
		{
			while(read_line(in))
			{
				if(strstr(linebuf, "DriverVer=") == linebuf)
				{
					sprintf(linebuf, "DriverVer=%02d/%02d/%04d,%d.%d.%d.%d", month, day, year, 4, year, softgpu, build);
				}
				else
				{
					for(i = 0; i < sections_count; i++)
					{
						sprintf(section_test, ";%s:", sections[i]);
						
						if(strstr(linebuf, section_test) == linebuf)
						{
							size_t lb_len = strlen(linebuf);
							size_t s_len  = strlen(section_test);
							
							memmove(linebuf, linebuf+s_len, lb_len-s_len+1);
						}
					}
				}
				
				put_line(out);
			}
		}
		else
		{
			fprintf(stderr, "Failed to open output: %s\n", dst);
		}
		
		fclose(in);
	}
	else
	{
		fprintf(stderr, "Failed to open input: %s\n", src);
	}
	
	return 0;
}