#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#include "m_pd.h"

static t_class *wavelist_class;

typedef struct {
    size_t count;
    char **filenames;
    int next_num;  // the highest numbered file (e.g. 124.wav) +1, used to name next file
} Filenames;

typedef struct _wavelist {
  t_object  x_obj;
  t_outlet *outlet;
  Filenames filenames;
} t_wavelist;

// helpers //

static void append(Filenames *p, const char *path) {
    p->filenames = realloc(p->filenames, sizeof(char*) * (p->count + 1));
    p->filenames[p->count] = strdup(path);
    p->count++;
}

static int ends_with_wav(const char *str) {
    if(str == NULL)
        return 0;

    size_t len = strlen(str);
    if(len < 4)
        return 0;

    const char *extension = str + len - 4;

    return strcmp(extension, ".wav") == 0 || strcmp(extension, ".WAV") == 0;
}

static int compare(const void *a, const void *b)
{
    //return strverscmp(*(const char **)a, *(const char **)b);
    return strcmp(*(const char **)a, *(const char **)b);
}

static void clear_filenames(t_wavelist *x) {
    for (size_t i = 0; i < x->filenames.count; i++) {
        free(x->filenames.filenames[i]);
    }
    free(x->filenames.filenames);
    x->filenames.count = 0;
    x->filenames.filenames = NULL;
    x->filenames.next_num = 0;
}

static int scan_for_wavs(t_wavelist *x, const char *path) {
    DIR *d;
    struct dirent *dir;
    struct stat path_stat;
    char fullpath[1024];

    clear_filenames(x);

    d = opendir(path);

    if(d) {
        // loop over directory
        while ((dir = readdir(d)) != NULL && x->filenames.count < 10000) {
            
            // ignore anything starting with '.'
            if (dir->d_name[0] == '.')
                continue;
            
            // check if it is a file and ends with '.wav' or '.WAV' and append if so
            snprintf(fullpath, sizeof(fullpath), "%s/%s", path, dir->d_name);
            stat(fullpath, &path_stat);
            if(S_ISREG(path_stat.st_mode) && ends_with_wav(dir->d_name)) {
                append(&(x->filenames), dir->d_name);

                // if the filename is a number, we want to know the biggest one
                char name_no_ext[strlen(dir->d_name) - 4 + 1];  // +1 for the null-terminator
                memcpy(name_no_ext, dir->d_name, strlen(dir->d_name) - 4);
                name_no_ext[strlen(dir->d_name) - 4] = '\0';
                char *end;
                long num = strtol(name_no_ext, &end, 10);
                if (end != name_no_ext && *end == '\0' && num <= 9999 && num >= 0) {
                    if (num > x->filenames.next_num) {
                        x->filenames.next_num = num;
                    }
                }
            }
        }
        closedir(d);
    }

    qsort(x->filenames.filenames, x->filenames.count, sizeof(char *), compare);


    for (size_t i = 0; i < x->filenames.count; i++) {
        post("%s", x->filenames.filenames[i]);
    }
    x->filenames.next_num += 1;
    post("max: %d", x->filenames.next_num);
    post("number files: %d", x->filenames.count);

    return 0;
}

// pd class //

static void wavelist_scan(t_wavelist *x, t_symbol *s){
    t_atom atom[1];
    
    //scan_for_wavs(x, "/Users/owen1/repos/randomstuff/filelist/testfiles-names/");
    scan_for_wavs(x, s->s_name);


    SETFLOAT(atom, 10);
    outlet_anything(x->outlet, gensym("total_files"), 1, atom);
    
    SETFLOAT(atom, 100);
    outlet_anything(x->outlet, gensym("next_number"), 1, atom);
}

static void wavelist_float(t_wavelist *x, t_floatarg f){
    t_atom atom[1];
    
    //char* myString = "1.wav";
    t_symbol* mySymbol = gensym("1.wav");
    
    SETSYMBOL(atom, mySymbol);
    outlet_anything(x->outlet, gensym("filename"), 1, atom);
}

static void wavelist_add(t_wavelist *x, t_symbol *s){}

static void wavelist_next(t_wavelist *x){}

static void wavelist_previous(t_wavelist *x){}

static void wavelist_clear(t_wavelist *x){}

static void *wavelist_new(){
    t_wavelist *x = (t_wavelist *)pd_new(wavelist_class);
    x->outlet = outlet_new(&x->x_obj, &s_list);

    Filenames filenames = {0, NULL, 0};
    return (void *)x;
}

static void wavelist_free(t_wavelist *x) {
    //post("freeing...");
 
    for (size_t i = 0; i < x->filenames.count; i++) {
        free(x->filenames.filenames[i]);
    }
    free(x->filenames.filenames);

    outlet_free(x->outlet);
}

void wavelist_setup(void) {
    wavelist_class = class_new(gensym("wavelist"), (t_newmethod)wavelist_new, (t_method)wavelist_free, sizeof(t_wavelist), 0, 0);
    class_addmethod(wavelist_class, (t_method)wavelist_scan, gensym("scan"), A_SYMBOL, 0);
    class_addmethod(wavelist_class, (t_method)wavelist_float, gensym("float"), A_FLOAT, 0);
    class_addmethod(wavelist_class, (t_method)wavelist_add, gensym("add"), A_SYMBOL, 0);
    class_addmethod(wavelist_class, (t_method)wavelist_next, gensym("next"), 0);
    class_addmethod(wavelist_class, (t_method)wavelist_previous, gensym("previous"), 0);
    class_addmethod(wavelist_class, (t_method)wavelist_clear, gensym("clear"), 0);
}

