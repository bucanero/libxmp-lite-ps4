/* A simple frontend for libxmp */
/* This file is in public domain */

#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xmp.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <audsrv.h>

#define BUFFER_SIZE 2048

//#define NORETURN
#define SCREEN_STDOUT


#ifdef NORETURN
#define PGM_END(x) waste_time()
static void waste_time()
{
    while(1)
        sleep(1);
}
#else
#define PGM_END(x) return (x)

#endif

#ifdef SCREEN_STDOUT
#include <debug.h>
#define printf scr_printf
#define puts(x) scr_printf("%s\n", (x))
#endif

int fillbuffer_sema;

static inline void sound_play(void* a, int b)
{
    WaitSema(fillbuffer_sema);
    audsrv_play_audio(a, b);
}

static void display_data(struct xmp_module_info *mi, struct xmp_frame_info *fi, int scr_x, int scr_y)
{
#ifdef SCREEN_STDOUT
    scr_printf("%3d/%3d %3d/%3d",
	       fi->pos, mi->mod->len, fi->row, fi->num_rows);
    scr_setXY(scr_x, scr_y);
#else
	printf("%3d/%3d %3d/%3d\r",
	       fi->pos, mi->mod->len, fi->row, fi->num_rows);

	fflush(stdout);
#endif
}

static int fillbuffer(void *arg)
{
	iSignalSema((int)arg);
	return 0;
}

int main(int argc, char **argv)
{
#ifdef SCREEN_STDOUT
    init_scr();
#endif
    char buf[BUFFER_SIZE];
    puts("Init RPC");
    SifInitRpc(0);
    
    puts("Create sema");
    ee_sema_t sema;
    sema.init_count = 0;
    sema.max_count = 1;
    sema.option = 0;
    fillbuffer_sema = CreateSema(&sema);

    puts("Audsrv init");
    struct audsrv_fmt_t format;
    format.bits = 16;
    format.freq = 44100;
    format.channels = 2;

    int ret = SifLoadModule("rom0:LIBSD", 0, NULL);
    printf("libsd loadmodule %d\n", ret);
    ret = SifLoadModule("host:audsrv.irx", 0, NULL);
    printf("audsrv loadmodule %d\n", ret);

    ret = audsrv_init();
    if (ret != 0)
    {
        printf("sample: failed to initialize audsrv\n");
        printf("audsrv returned error string: %s\n", audsrv_get_error_string());
        PGM_END(1);
    }
    
    ret = audsrv_on_fillbuf(BUFFER_SIZE, fillbuffer, (void *)fillbuffer_sema);
    if (ret != 0)
    {
        printf("audsrv on fillbuff %d\n", ret);
        PGM_END(1);
    }

    ret = audsrv_set_format(&format);
    printf("set format returned %d\n", ret);

    audsrv_set_volume(MAX_VOLUME);
    
    puts("Audsrv init end");

    printf("Xmp Init\n");
	xmp_context ctx;
	struct xmp_module_info mi;
	struct xmp_frame_info fi;
	int row;
    //char* modfile = "host:module.mod";
    const char* modfile = "host:module.mod";

    puts("Past init");
	ctx = xmp_create_context();
    
    printf("Loading %s into memory\n", modfile);
    FILE* f = fopen(modfile, "rb");
    fseek(f, 0, SEEK_END);
    size_t modsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    printf("Size %u\n", modsize);
    void* data = malloc(modsize);
    printf("Malloc returned %p\n", data);
    fread(data, modsize, 1, f);
    fclose(f);

    /*if ((err = xmp_load_module_from_file(ctx, f, modsize)) < 0) {
        printf("error loading %s: %d\n", modfile, err);
        PGM_END();
    }*/

    if ((ret = xmp_load_module_from_memory(ctx, data, modsize)) < 0) {
        printf("error loading %s: %d\n", modfile + 5, ret);
        PGM_END(1);
    }
    
    /*if ((ret = xmp_load_module(ctx, modfile)) < 0) {
        printf("error loading %s: %d\n", modfile + 5, ret);
        PGM_END();
    }*/

    puts("Past load module");
    if (xmp_start_player(ctx, 44100, 0) == 0) {
        xmp_set_position(ctx, 15);
        
        // Fill buffer initially
        xmp_play_buffer(ctx, buf, BUFFER_SIZE, 0);

        /* Show module data */
        puts("Inside start player");
        xmp_get_module_info(ctx, &mi);
        printf("%s (%s)\n", mi.mod->name, mi.mod->type);

        /* Play module */
        puts("Playback start");
        row = -1;
#ifdef SCREEN_STDOUT
        int scr_y = scr_getY();
        int scr_x = scr_getX();
#endif
        while (xmp_play_buffer(ctx, buf, BUFFER_SIZE, 0) == 0) {
            xmp_get_frame_info(ctx, &fi);
            if (fi.loop_count > 0)
                break;

            //sound_play(fi.buffer, fi.buffer_size);
            sound_play(buf, BUFFER_SIZE);
        
            if (fi.row != row) {
                display_data(&mi, &fi, scr_x, scr_y);
                row = fi.row;
            }
        }
        xmp_end_player(ctx);
    }
    
    //fclose(f);

    xmp_release_module(ctx);
    printf("\n");

	xmp_free_context(ctx);
    
    audsrv_quit();
    
    puts("End of playback! Returning in 5 seconds...");
    sleep(5);
    
    PGM_END(0);

	return 0;
}
