/********************************************************************
 * lst2cmt.c - Convert lwasm list files to MAME comment files
 *
 * Based on lst2cmt.pb from Eric Canales, playpi.net
 *
 * $Id$
 ********************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <cocotypes.h>

static void show_lst2cmt_help(char const *const *helpMessage);
error_code do_command(int nocrc_flag, char *system_name, char *cpu_name, char *srcfilename, char *destfilename, int nolinenumbers, unsigned int offset);
unsigned int crc32b(unsigned char *message, unsigned int length);
unsigned int Hex2Dec(char *string);
char *LTrim(char *s);
char *RTrim(char *s);
char *Trim(char *s);
char *Mid(char *s, int start, int length);
void HexString2Buffer(char *s, unsigned char *buf);
char *EscapeXML(char *s);

/* Help message */
static char const *const helpMessage[] = {
	"lst2cmt from Toolshed " TOOLSHED_VERSION "\n",
	"Based on lst2cmt.pb by Eric Canales\n",
	"Syntax: lst2cmt {[<opts>]} <srcfile> <destfile> {[<opts>]}\n",
	"Options:\n",
	"   -nocrc          Writes comment lines with no CRC field\n",
	"   -s<system>      Sets the system MAME should apply the comments file to.\n",
    "                   Default is blank, but this is required be specified.\n",
	"   -c<cpu>         Sets the CPU MAME should apply the comments file to.\n"
    "                   Default is \":maincpu\".\n",
	"   -nolinenumbers  Remove line numbers. Useful if your debugger has limited\n"
    "                   space.\n",
	"   -o<offset>      Offset the memory locations to place the comments.\n",
	NULL
};

// The string manipulation functions use temporary memory allocation.
// The mc array gives you the chance to collect and free these allocations at
// the appropraite time.
#define max_mc 256
char *mc[max_mc];
int mc_count = 0;

int main(int argc, char *argv[])
{
	error_code ec = 0;
	int i;
	char *p;

    int nocrc_flag = 0;
    char *system_name = "coco";
    char *cpu_name = ":maincpu";
    char *srcfilename = NULL;
    char *destfilename = NULL;
    int nolinenumbers = 0;
    unsigned int offset = 0;

	/* walk command line for options */
	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			for (p = &argv[i][1]; *p != '\0'; p++)
			{
				switch (*p)
				{
				case 'o':
					offset = atoi(p + 1);
					while (*(p + 1) != '\0') p++;
					break;
				case 'n':
                    if (strcmp(p, "nocrc")==0)
                    {
                        nocrc_flag = 1;
                        while (*(p + 1) != '\0') p++;
                    }
                    else if (strcmp(p, "nolinenumbers")==0)
                    {
                        nolinenumbers = 1;
                        while (*(p + 1) != '\0') p++;
                    }
                    break;
                case 'c':
                    cpu_name = p+1;
					while (*(p + 1) != '\0') p++;
                    break;
                case 's':
                    system_name = p+1;
					while (*(p + 1) != '\0') p++;
                    break;
				case 'h':
				case '?':
					show_lst2cmt_help(helpMessage);
					return (0);
				}
			}
		}
		else
		{
            if (srcfilename == NULL )
                srcfilename = argv[i];
            else
                destfilename = argv[i];
		}
	}

	if (srcfilename == NULL )
	{
		show_lst2cmt_help(helpMessage);
	}
	else
	{
        ec = do_command(nocrc_flag, system_name, cpu_name, srcfilename, destfilename, nolinenumbers, offset);
	}

	return (ec);
}

error_code do_command(int nocrc_flag, char *system_name, char *cpu_name, char *srcfilename, char *destfilename, int nolinenumbers, unsigned int offset)
{
    error_code ec = 0;
    FILE *input_file, *output_file;
    
    input_file = fopen(srcfilename, "rb");
    
    if (input_file == NULL)
    {
        fprintf(stderr, "Can not open input file: %s\n", srcfilename );
        return -1;
    }
    
    output_file = fopen(destfilename, "wb");
    
    if (output_file == NULL)
    {
        fprintf(stderr, "Can not open output file: %s\n", destfilename );
        return -1;
    }
    
    // write out xml header
    fprintf(output_file, "<?xml version=\"1.0\"?>\n");
    fprintf(output_file, "<!-- This file is autogenerated; comments and unknown tags will be stripped -->\n");
    fprintf(output_file, "<mamecommentfile version=\"1\">\n");
    fprintf(output_file, "    <system name=\"%s\">\n", system_name);
    fprintf(output_file, "        <cpu tag=\"%s\">\n", cpu_name);

    // read in entire file
    fseek(input_file, 0, SEEK_END);
    long fsize = ftell(input_file);
    fseek(input_file, 0, SEEK_SET);
    char *input_string = malloc(fsize + 1);
    fread(input_string, fsize, 1, input_file);
    fclose(input_file);
    
    // count lines
    int input_line_count = 0;
    char *p = input_string;
    while (*p!=0)
    {
        if (*p == 0x0a)
        {
            input_line_count++;
            if (*(p+1) == 0x0d) p++;
        }
        else if (*p == 0x0d)
        {
            input_line_count++;
            if (*(p+1) == 0x0a) p++;
        }
        
        p++;
    }
       
    // create line array
    char **input_line_array;
    
    input_line_array = malloc(sizeof(char *) * input_line_count);
    
    if (input_line_array==NULL)
    {
        fprintf(stderr,"No memory to allocate line array.\n");
        return -1;
    }
    
    p = input_string;
    int i=0;
    input_line_array[i++] = p;
    while (*p!=0)
    {
        if (*p == 0x0a)
        {
            *p = 0x00;
            p++;
            if (*p == 0x0d) p++;
            input_line_array[i++] = p;
        }
        else if (*p == 0x0d)
        {
            *p = 0x00;
            p++;
            if (*p == 0x0a) p++;
            input_line_array[i++] = p;
        }
        else
            p++;
    }

    // run line array and turn them into comments
    for(i=0; i<input_line_count; i++)
    {
        char *line = input_line_array[i];
        
        if (*line == '\0' ) continue;
        
        unsigned int memory_location = Hex2Dec(Trim(Mid(line, 1, 4)));
        
        if (memory_location == 0) continue;
        
        char *memory_contents_readable = Trim(Mid(line, 5, 17));
        
        if (memory_contents_readable[0] == '\0') continue;
        
        char line_number[256];
        
        if (nolinenumbers)
        {
            line_number[0] = '\0';
        }
        else
        {
            sprintf(line_number, "%s: ", Trim(Mid(line, 43, 14)));
        }

        char listing_line[512];
        sprintf(listing_line, "%s%s", line_number, RTrim(Mid(line, 57, 256)));

        unsigned char buffer[17/2];
        HexString2Buffer(memory_contents_readable, buffer);
        int memory_contents_crc = crc32b(buffer, strlen(memory_contents_readable)/2);

        if (nocrc_flag)
        {
            fprintf(output_file, "            <comment address=\"%u\" color=\"16711680\">\n", memory_location + offset);
        }
        else
        {
            fprintf(output_file, "            <comment address=\"%u\" color=\"16711680\" crc=\"%X\">\n", memory_location + offset, memory_contents_crc);
        }
    
        fprintf(output_file, "                %s\n", EscapeXML(listing_line));
        fprintf(output_file, "            </comment>\n");
        
        // Flush mallocs
        for( int i=0; i<mc_count; i++ )
        {
            free(mc[i]);
        }
        
        mc_count = 0;
    }
 
    fprintf(output_file, "        </cpu>\n");
    fprintf(output_file, "    </system>\n");
    fprintf(output_file, "</mamecommentfile>\n");
    fprintf(output_file, "\n");

    free(input_line_array);
    free(input_string);
    fclose(output_file);
    
    return ec;
}

static void show_lst2cmt_help(char const *const *helpMessage)
{
	char const *const *p = helpMessage;

	while (*p)
	{
		fputs(*(p++), stderr);
	}

	return;
}

void HexString2Buffer(char *s, unsigned char *buf)
{
    char temp[3];
    int length = strlen(s);
   
    for(int i=0; i<length; i+=2)
    {
        temp[0] = s[i];
        temp[1] = s[i+1];
        temp[2] = '\0';
        buf[i/2] = Hex2Dec(temp);
    }
}

char *Mid(char *s, int start, int length)
{
    int l = strlen(s);
    char *r = malloc(length+1);
    mc[mc_count++] = r;
    int i = start-1, j=0;
    
    while (i<l)
    {
        r[j] = s[i];
        j++;
        i++;
        r[j] = '\0';
        
        if (j==length) break;
    }
    
    return r;
}

char *Trim(char *s)
{
    return LTrim(RTrim(s));
}

char *RTrim(char *s)
{
    int l = strlen(s);
    unsigned char *r = malloc(l+1);
    mc[mc_count++] = (char *)r;
    int i = l-1;
   
    stpcpy((char *)r, s);
    
    while (i >= 0)
    {
        if (isspace(r[i]))
        {
            r[i] = '\0';
        }
        else
        {
            break;
        }
        
        i--;
    }
    
    return (char *)r;
}

char *LTrim(char *s)
{
    int l = strlen(s);
    char *r = malloc(l+1);
    mc[mc_count++] = r;
    int i = 0, j = 0;
    int done_trim = 0;
    
    r[0] = '\0';
    
    while (i<l)
    {
        unsigned char z = s[i];
        if (isspace(z) && done_trim == 0)
        {
        }
        else
        {
            done_trim = 1;
            r[j] = s[i];
            j++;
            r[j] = '\0';
        }

        i++;
    }
    
    return r;
}

unsigned int Hex2Dec(char *s)
{
    int length = strlen(s);
    int i = 0;
    unsigned int result = 0;
    
    while (i < length)
    {
        if ((*s >= '0') && (*s <= '9'))
        {
            result <<= 4;
            result |= *s - '0';
        }
        else if ((*s >= 'a') && (*s <= 'f'))
        {
            result <<= 4;
            result |= *s - 'a' + 0x0a;
        }
        else if ((*s >= 'A') && (*s <= 'F'))
        {
            result <<= 4;
            result |= *s - 'A' + 0x0a;
        }
        
        i++;
        s++;
    }
    
    return result;
}

char *EscapeXML(char *s)
{
    int size;
    char *t;
    
    t = s;
    size = 0;
    
    while (*t)
    {
        switch (*t)
        {
            case '"':
            case '\'':
               size += 6;
                break;
            case '<':
            case '>':
                size += 4;
                break;
            case '&':
                size += 5;
                break;
            default:
                size++;
                break;
        }
        
        t++;
    }
    
    char *r = malloc(size+1);
    mc[mc_count++] = r;
    t = r;
    r[0] = '\0';
    
    while (*s)
    {
        switch (*s)
        {
            case '"':
                t=strcpy(t,"&quot;")+6;
                break;
            case '\'':
                t=strcpy(t,"&apos;")+6;
                break;
            case '<':
                t=strcpy(t,"&lt;")+4;
                break;
            case '>':
                t=strcpy(t,"&gt;")+4;
                break;
            case '&':
                t=strcpy(t,"&amp;")+5;
                break;
            default:
                *t = *s;
                t++;
                *t = '\0';
                break;
        }
        
        s++;
    }
    
    return r;
}

unsigned int crc32b(unsigned char *message, unsigned int length)
{
   int i, j;
   unsigned int byte, crc, mask;

   i = 0;
   crc = 0xFFFFFFFF;
   
   while (i < length)
   {
      byte = message[i];            // Get next byte.
      crc = crc ^ byte;
      for (j = 7; j >= 0; j--)     // Do eight times.
      {
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
      i = i + 1;
   }
   
   return ~crc;
}
