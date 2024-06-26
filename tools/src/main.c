#include "hpck.h"
#include <string.h>

char * hpck_kernel_name __attribute__((weak)) = "<not defined>";

extern char  hpck_kernel_args_count;
extern char* hpck_kernel_args_key[];
extern char* hpck_kernel_args_desc[];
extern char  hpck_kernel_args_needed[];
extern char* hpck_kernel_args_default[];

char ** __hpck_kernel_args_values = NULL;

char * __hpck_compiler  __attribute__((weak)) = "n/a";
char * __hpck_cflags    __attribute__((weak)) = "n/a";
char * __hpck_ldflags   __attribute__((weak)) = "n/a";
char * __hpck_includes  __attribute__((weak)) = "n/a";
char * __hpck_libraries __attribute__((weak)) = "n/a";

int    __hpck_argc = 0;
char **__hpck_argv = NULL;

long __hpck_flops = 0;
long __hpck_iops= 0;

#define __HPCK_STR_SIZE 64
#define __HPCK_STR_NAME "%-30s: "

#define __HPCK_USAGE_OPT      "  %-12s %s\n"
#define __HPCK_USAGE_ARG_ID   "     <arg-%d>   %s (default=%s)\n"

#define __HPCK_OPS_IS_UNIT 1E+9

void __hpck_print(const char* fmt, va_list args)
{
   vprintf(fmt, args);
}

void __hpck_error(const char* fmt, va_list args)
{
   vfprintf(stderr, fmt, args);
}

void __hpck_print_rule(void)
{
   for(int i=0; i<__HPCK_STR_SIZE;i++) printf("=");
   printf("\n");
}

void __hpck_print_name(void)
{
   int i = strlen(hpck_kernel_name)+4;

   printf("= %s", hpck_kernel_name);
   while(i < __HPCK_STR_SIZE) { printf(" "); i++;}
   printf(" =\n");
}

void __hpck_print_line(const char* name, const char *format,...)
{
   va_list args;
   va_start(args,format);
   printf(__HPCK_STR_NAME, name);
   __hpck_print(format, args);
   printf("\n");
   va_end(args);
}

void __hpck_print_header ()
{
   __hpck_print_rule();
   __hpck_print_name();

   __hpck_print_rule(); // Build info
   __hpck_print_line("Compiler", "%s", __hpck_compiler);
   __hpck_print_line("Compiler flags", "%s", __hpck_cflags);
   __hpck_print_line("Linker flags", "%s", __hpck_ldflags);
   __hpck_print_line("Includes", "%s", __hpck_includes);
   __hpck_print_line("Libraries", "%s", __hpck_libraries);
}

void __hpck_print_version(void)
{
    fprintf(stderr, "Kernel %s \"%s\"; from the HPC-K suite v%d.%d%s\n",
             __hpck_argv[0], hpck_kernel_name,
            HPCK_VERSION_MAJOR, HPCK_VERSION_MINOR, HPCK_VERSION_ALPHA?"a":"");
    fprintf(stderr, "This is free software; see the source code for copying conditions (LICENSE file).\n");
    fprintf(stderr, "There is NO WARRANTY; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");

}

void __hpck_print_usage(void)
{
   fprintf(stderr,"Usage: %s [options]\n", __hpck_argv[0]);
   fprintf(stderr,"Where [options] are:\n");
   fprintf(stderr, __HPCK_USAGE_OPT, "-h", "Display this information (and exit)");
   fprintf(stderr, __HPCK_USAGE_OPT, "-i <args>","Set kernel input arguments, where:");
   if (!hpck_kernel_args_key[0]) {
      for (int i=0; i < hpck_kernel_args_count; i++) {
         fprintf(stderr, __HPCK_USAGE_ARG_ID,
               i+1, hpck_kernel_args_desc[i],
               hpck_kernel_args_needed[i]?"none":hpck_kernel_args_default[i]);
      }
   fprintf(stderr, __HPCK_USAGE_OPT, "-v", "Display the program version (and exit)");
   } else {
      // TODO: named arguments
   }
}

void __hpck_parse_arguments(void)
{
   char kernel_args = FALSE;
   __hpck_kernel_args_values = (char **) malloc (hpck_kernel_args_count*sizeof(char *));

   for(int i=1; i<__hpck_argc; ) {
      if (__hpck_argv[i][0] == '-') {
         switch (__hpck_argv[i][1]) {
            case 'h': i++;
                      __hpck_print_usage();
                      exit(EXIT_SUCCESS);
            case 'i': i++;
                      int j=i,k=0;
                      while (j < __hpck_argc && __hpck_argv[j][0] != '-' && (j-i) < hpck_kernel_args_count) j++;
                      if (!hpck_kernel_args_key[0]) {
                         while (i<j) {
                            __hpck_kernel_args_values[k] = __hpck_argv[i];
                            i++;k++;
                         }
                         while (k<hpck_kernel_args_count) {
                            if (hpck_kernel_args_needed[k]) {
                               __hpck_print_usage();
                               hpck_error("In kernel arguments (-i); <arg-%d> needed and not provided.",k+1);
                            } else {
                               __hpck_kernel_args_values[k] = hpck_kernel_args_default[k];
                            }
                            k++;
                         }
                      } else {
                         // TODO: named arguments
                      }
                      kernel_args = TRUE;
                      break;
            case 'v': i++;
                      __hpck_print_version();
                      exit(EXIT_SUCCESS);
            default:
                      __hpck_print_usage();
                      hpck_error("Option '-%c' not recognized", __hpck_argv[i][1]);
         }
      } else {
         __hpck_print_usage();
         hpck_error("Option '%s' not recognized", __hpck_argv[i]);
      }
      
   }

   if (!kernel_args) {
      int k=0;
      while (k<hpck_kernel_args_count) {
         if (hpck_kernel_args_needed[k]) {
            __hpck_print_usage();
            hpck_error("In kernel arguments (-i); <arg-%d> needed and not provided.",k+1);
         } else {
            __hpck_kernel_args_values[k] = hpck_kernel_args_default[k];
         }
         k++;
      }
   }

}

char*hpck_get_arg_idx(int idx)
{
   return __hpck_kernel_args_values[idx];
}

void hpck_error(const char *msg,...)
{
   va_list args;
   va_start(args,msg);
   fprintf(stderr,"Error: ");
   __hpck_error(msg,args);
   fprintf(stderr,"\n");
   va_end(args);
   exit(EXIT_FAILURE);
}

void hpck_warning(const char *msg,...)
{
   printf("Warning: %s\n", msg);
}

void hpck_print_settings(const char* name, const char *format, ...)
{
   static char called=FALSE;
   if (!called) {
      __hpck_print_rule();
      called=TRUE;
   }
   
   va_list args;
   va_start(args,format);
   printf(__HPCK_STR_NAME, name);
   __hpck_print(format, args);
   printf("\n");
   va_end(args);
}

void hpck_print_results(const char* name, const char *format, ...)
{
   static char called=FALSE;
   if (!called) {
      __hpck_print_rule();
      called=TRUE;
   }

   va_list args;
   va_start(args,format);
   printf(__HPCK_STR_NAME, name);
   __hpck_print(format, args);
   printf("\n");
   va_end(args);
}

void hpck_set_flops(long flops)
{
   __hpck_flops = flops;
}

void hpck_set_iops(long iops)
{
   __hpck_iops = iops;
}

int main(int argc, char *argv[])
{
   __hpck_argc = argc;
   __hpck_argv = (char **) argv;

   char *str_result[3] = {"fail","n/a","pass"};
   struct timeval tv1, tv2;
   struct timezone tz;
   double kernel_time, initialize_time, finalize_time;

   __hpck_parse_arguments();

   __hpck_print_header();

   gettimeofday(&tv1, &tz);
   void * args = hpck_initialize();
   gettimeofday(&tv2, &tz);
   initialize_time = (double) (tv2.tv_sec-tv1.tv_sec) + (double) (tv2.tv_usec-tv1.tv_usec) * 1.e-6;

   gettimeofday(&tv1, &tz);
   hpck_kernel(args);
   gettimeofday(&tv2, &tz);
   kernel_time = (double) (tv2.tv_sec-tv1.tv_sec) + (double) (tv2.tv_usec-tv1.tv_usec) * 1.e-6;

   gettimeofday(&tv1, &tz);
   int result = hpck_finalize(args);
   gettimeofday(&tv2, &tz);
   finalize_time = (double) (tv2.tv_sec-tv1.tv_sec) + (double) (tv2.tv_usec-tv1.tv_usec) * 1.e-6;

   __hpck_print_rule();
   __hpck_print_line("Initialization time (seconds)", "%.3lf", initialize_time);
   __hpck_print_line("Kernel compute time (seconds)", "%.3lf", kernel_time);
   if (__hpck_flops)
      __hpck_print_line("Kernel throughput (GFLOPs/s)", "%.3lf", ((__hpck_flops)/(kernel_time)) / __HPCK_OPS_IS_UNIT);
   if (__hpck_iops)
      __hpck_print_line("Kernel throughput (GIOPs/s)", "%.3lf", ((__hpck_iops)/(kernel_time)) / __HPCK_OPS_IS_UNIT);
   __hpck_print_line("Finalization time (seconds)", "%.3lf", finalize_time);
   __hpck_print_line("Result verification", "%s", str_result[result+1] );
   __hpck_print_rule();

   return EXIT_SUCCESS;
}
