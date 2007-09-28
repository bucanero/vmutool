/********************************************************************
 * VMU Backup Tool
 * Version 0.5.beta (08/Jan/2004)
 * coded by El Bucanero
 *
 * Copyright (C) 2004 Damián Parrino (bucanero@elitez.com.ar)
 * http://www.bucanero.com.ar/
 *
 * Greetz to:
 * Dan Potter for KOS and other great libs like Tsunami
 * Andrew K. for the lovely DC ip-load/tool
 * Lawrence Sebald for the MinGW/msys cross compiler tutorial
 *
 * and last, but not least, thanks to SEGA for the Dreamcast! =)
 *
 ********************************************************************/

#include <kos.h>
#include <tsu/texture.h>
#include <tsu/drawables/scene.h>
#include <tsu/drawables/banner.h>
#include <tsu/drawables/label.h>

// GUI constants
#define VMU_X 236
#define VMU_Y 148
#define VMU_W 84
#define VMU_H 112
#define MAX_FILES_LIST 14
#define FILES_X 240
#define FILES_Y 80
#define FILES_W 172

// Default paths
#define PC_DIR "/pc/vmutool"
#define CD_DIR "/cd/vmutool"
#define DEFAULT_VMU "/vmu/a1"

// Defined actions
#define VMU_TO_PC 1
#define PC_TO_VMU 2
#define CD_TO_VMU 3
#define VMU_TO_VMU 4

void scan_vmu_dir(int vmuport, int vmuslot, RefPtr<Label> label);
void copy_vmu_file(char *srcdir, char *dstdir, char *vmufile, ssize_t size);
void copy_directory(char *srcdir, char *dstdir, int selection);
void backup_action(int vmuport, int vmuslot, int action);
int list_directory(char *srcdir, int begin);
int file_select(char *srcdir);

extern uint8 romdisk[];

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

void backup_action(int vmuport, int vmuslot, int action) {
	int		selection;
	char	*vmudir;

	vmudir=(char *)malloc(10);
	sprintf(vmudir, "/vmu/%c%d", vmuport+97, vmuslot+1);
	switch (action) {
		case VMU_TO_PC:
			selection=file_select(vmudir);
			if (selection != -1) {
				copy_directory(vmudir, PC_DIR, selection);
			}
			break;
		case PC_TO_VMU:
			selection=file_select(PC_DIR);
			if (selection != -1) {
				copy_directory(PC_DIR, vmudir, selection);
			}
			break;
		case CD_TO_VMU:
			selection=file_select(CD_DIR);
			if (selection != -1) {
				copy_directory(CD_DIR, vmudir, selection);
			}
			break;
		case VMU_TO_VMU:
			selection=file_select(vmudir);
			if (selection != -1) {
				copy_directory(vmudir, DEFAULT_VMU, selection);
			}
			break;
	}
	free(vmudir);
}

int file_select(char *srcdir) {
	int		pos=-1, done=0, top=-1, total, o;
	uint64	timer = timer_ms_gettime64();

	total=list_directory(srcdir, top);
	o = FILES_Y * 640 + FILES_X - 20;
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	bfont_draw_str(vram_s + o, 640, 1, ">");
	bfont_draw_str(vram_s + o + FILES_W, 640, 1, "<");
	while (!done) {
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)
			if ((t->buttons & CONT_DPAD_UP) && (pos >= top) && (timer + 200 < timer_ms_gettime64())) {
				if (pos > top) {
					pos--;
					bfont_draw_str(vram_s + o, 640, 1, " ");
					bfont_draw_str(vram_s + o + FILES_W, 640, 1, " ");
					o -= 640*24;
					bfont_draw_str(vram_s + o, 640, 1, ">");
					bfont_draw_str(vram_s + o + FILES_W, 640, 1, "<");
				} else {
					if ((pos == top) && (top != -1)) {
						top=top - MAX_FILES_LIST;
						pos--;
						bfont_draw_str(vram_s + o, 640, 1, " ");
						bfont_draw_str(vram_s + o + FILES_W, 640, 1, " ");
						list_directory(srcdir, top);
						o = FILES_Y * 640 + FILES_X - 20 + 640 * 24 * (MAX_FILES_LIST - 1);
						bfont_draw_str(vram_s + o, 640, 1, ">");
						bfont_draw_str(vram_s + o + FILES_W, 640, 1, "<");
					}
				}
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_DOWN) && (pos <= top + MAX_FILES_LIST) && (timer + 200 < timer_ms_gettime64())) {
				if ((pos < top + MAX_FILES_LIST - 1) && (pos < total)) {
					pos++;
					bfont_draw_str(vram_s + o, 640, 1, " ");
					bfont_draw_str(vram_s + o + FILES_W, 640, 1, " ");
					o += 640*24;
					bfont_draw_str(vram_s + o, 640, 1, ">");
					bfont_draw_str(vram_s + o + FILES_W, 640, 1, "<");
				} else {
					if ((pos == top + MAX_FILES_LIST - 1) && (top + MAX_FILES_LIST - 1 < total)) {
						top=top + MAX_FILES_LIST;
						pos++;
						bfont_draw_str(vram_s + o, 640, 1, " ");
						bfont_draw_str(vram_s + o + FILES_W, 640, 1, " ");
						list_directory(srcdir, top);
						o = FILES_Y * 640 + FILES_X - 20;
						bfont_draw_str(vram_s + o, 640, 1, ">");
						bfont_draw_str(vram_s + o + FILES_W, 640, 1, "<");
					}
				}
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_A) && (timer + 200 < timer_ms_gettime64())) {
				done=1;
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_B) && (timer + 200 < timer_ms_gettime64())) {
				done=1;
				pos=-1;
				timer = timer_ms_gettime64();
			}
		MAPLE_FOREACH_END()
	}
	return(pos);
}

int list_directory(char *srcdir, int begin) {
	file_t		d;
	dirent_t	*de;
	int			i=0, j, o;

	o = FILES_Y * 640 + FILES_X;
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	d = fs_open(srcdir, O_RDONLY | O_DIR);
	if (!d) {
		printf("Can't open source directory (%s)\n", srcdir);
	} else {
		if (begin == -1) {
			bfont_draw_str(vram_s + o, 640, 1, "CANCEL      ");
			o += 640*24;
			bfont_draw_str(vram_s + o, 640, 1, "ALL FILES   ");
			o += 640*24;
		}
		while ( (de = fs_readdir(d)) ) {
			i++;
			if ((i >= begin) && (i < begin + MAX_FILES_LIST)) {
				bfont_draw_str(vram_s + o, 640, 1, de->name);
				o += 640*24;
			}
		}
		if (i - begin + 1 < MAX_FILES_LIST) {
			for (j=i - begin + 1; j < MAX_FILES_LIST; j++) {
				bfont_draw_str(vram_s + o, 640, 1, "            ");
				o += 640*24;
			}
		}
	}
	fs_close(d);
	return(i);
}

void copy_vmu_file(char *srcdir, char *dstdir, char *vmufile, ssize_t size) {
	char	*tmpsrc;
	char	*tmpdst;

	tmpsrc=(char *)malloc(64);
	tmpdst=(char *)malloc(64);
	sprintf(tmpsrc, "%s/%s", srcdir, vmufile);
	sprintf(tmpdst, "%s/%s", dstdir, vmufile);
	printf("Copying (%s -> %s) Name: %s ... ", srcdir, dstdir, vmufile);
	if (fs_copy(tmpsrc, tmpdst) == size) {
		printf("%d bytes copied.\n", size);
	} else {
		printf("Error!\n");
	}
	free(tmpsrc);
	free(tmpdst);
}

void copy_directory(char *srcdir, char *dstdir, int selection) {
	file_t		d;
	dirent_t	*de;
	int			i=0;

	d = fs_open(srcdir, O_RDONLY | O_DIR);
	if (!d) {
		printf("Can't open source directory (%s)\n", srcdir);
	} else {
		while ( (de = fs_readdir(d)) ) {
			if (selection == 0) {
				copy_vmu_file(srcdir, dstdir, de->name, de->size);
			} else {
				i++;
				if (selection == i) {
					copy_vmu_file(srcdir, dstdir, de->name, de->size);
				}
			}
		}
	}
	fs_close(d);
}

void scan_vmu_dir(int vmuport, int vmuslot, RefPtr<Label> label) {
	file_t		d;
	dirent_t	*de;
	char		*vmudir, *tmp;
	int			files=0, blocks=0;
	
	tmp=(char *)malloc(128);
	vmudir=(char *)malloc(10);
	sprintf(vmudir, "/vmu/%c%d", vmuport+97, vmuslot+1);
	d = fs_open(vmudir, O_RDONLY | O_DIR);
	if (!d) {
		sprintf(tmp, "Unable to open VMU %c%d", vmuport+65, vmuslot+1);
		label->setText(tmp);
	} else {
		while ( (de = fs_readdir(d)) ) {
			files++;
			blocks=blocks + (de->size / 512);
		}
		sprintf(tmp, "VMU %c%d: %d Files - %d Blocks Used", vmuport+65, vmuslot+1, files, blocks);
		label->setText(tmp);
	}
	fs_close(d);
	free(vmudir);
	free(tmp);
}

int main(int argc, char **argv) {
	int done = 0;
	int x = 0;
	int y = 0;
	uint64 timer = timer_ms_gettime64();

	cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
		(void (*)(unsigned char, long  unsigned int))arch_exit);

	pvr_init_defaults();

	RefPtr<Scene> sc = new Scene();
	RefPtr<Banner> b_menu = new Banner(PVR_LIST_TR_POLY, new Texture("/rd/vmumenu.png", true));
	RefPtr<Banner> b_vmu = new Banner(PVR_LIST_TR_POLY, new Texture("/rd/vmu.png", true));
	RefPtr<Banner> b_file = new Banner(PVR_LIST_TR_POLY, new Texture("/rd/fileselect.png", true));

	sc->subAdd(b_menu);
	sc->subAdd(b_vmu);
	sc->subAdd(b_file);
	b_menu->setTranslate(Vector(320, 240, 5));
	b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
	b_file->setTranslate(Vector(800, 600, 11));

	RefPtr<Font> fnt = new Font("/rd/helvetica.txf");
	RefPtr<Label> lbl1 = new Label(fnt, "VMU Backup Tool", 20, true, true);
	lbl1->setTranslate(Vector(320, 420, 9));
	sc->subAdd(lbl1);

	pvr_set_bg_color(1.0f, 1.0f, 1.0f);

	scan_vmu_dir(x, y, lbl1);

	while (!done) {
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_OP_POLY);
		sc->draw(PVR_LIST_OP_POLY);
		pvr_list_begin(PVR_LIST_TR_POLY);
		sc->draw(PVR_LIST_TR_POLY);
		pvr_scene_finish();
		sc->nextFrame();

		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)
			if (t->buttons & CONT_START) {
				done = 1;
			}
			if ((t->buttons & CONT_DPAD_UP) && (y>0) && (timer + 200 < timer_ms_gettime64())) {
				y--;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_DOWN) && (y<1) && (timer + 200 < timer_ms_gettime64())) {
				y++;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_LEFT) && (x>0) && (timer + 200 < timer_ms_gettime64())) {
				x--;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_RIGHT) && (x<3) && (timer + 200 < timer_ms_gettime64())) {
				x++;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_A) && (timer + 200 < timer_ms_gettime64())) {
				b_file->setTranslate(Vector(320, 240, 11));
				timer = timer_ms_gettime64();
				while (timer + 50 > timer_ms_gettime64()) {
					pvr_wait_ready();
					pvr_scene_begin();
					pvr_list_begin(PVR_LIST_OP_POLY);
					sc->draw(PVR_LIST_OP_POLY);
					pvr_list_begin(PVR_LIST_TR_POLY);
					sc->draw(PVR_LIST_TR_POLY);
					pvr_scene_finish();
					sc->nextFrame();
				}
				backup_action(x, y, VMU_TO_PC);
				b_file->setTranslate(Vector(800, 600, 11));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_B) && (timer + 200 < timer_ms_gettime64())) {
				b_file->setTranslate(Vector(320, 240, 11));
				timer = timer_ms_gettime64();
				while (timer + 50 > timer_ms_gettime64()) {
					pvr_wait_ready();
					pvr_scene_begin();
					pvr_list_begin(PVR_LIST_OP_POLY);
					sc->draw(PVR_LIST_OP_POLY);
					pvr_list_begin(PVR_LIST_TR_POLY);
					sc->draw(PVR_LIST_TR_POLY);
					pvr_scene_finish();
					sc->nextFrame();
				}
				backup_action(x, y, PC_TO_VMU);
				b_file->setTranslate(Vector(800, 600, 11));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_X) && (timer + 200 < timer_ms_gettime64())) {
				b_file->setTranslate(Vector(320, 240, 11));
				timer = timer_ms_gettime64();
				while (timer + 50 > timer_ms_gettime64()) {
					pvr_wait_ready();
					pvr_scene_begin();
					pvr_list_begin(PVR_LIST_OP_POLY);
					sc->draw(PVR_LIST_OP_POLY);
					pvr_list_begin(PVR_LIST_TR_POLY);
					sc->draw(PVR_LIST_TR_POLY);
					pvr_scene_finish();
					sc->nextFrame();
				}
				backup_action(x, y, CD_TO_VMU);
				b_file->setTranslate(Vector(800, 600, 11));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_Y) && (timer + 200 < timer_ms_gettime64())) {
				b_file->setTranslate(Vector(320, 240, 11));
				timer = timer_ms_gettime64();
				while (timer + 50 > timer_ms_gettime64()) {
					pvr_wait_ready();
					pvr_scene_begin();
					pvr_list_begin(PVR_LIST_OP_POLY);
					sc->draw(PVR_LIST_OP_POLY);
					pvr_list_begin(PVR_LIST_TR_POLY);
					sc->draw(PVR_LIST_TR_POLY);
					pvr_scene_finish();
					sc->nextFrame();
				}
				backup_action(x, y, VMU_TO_VMU);
				b_file->setTranslate(Vector(800, 600, 11));
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
		MAPLE_FOREACH_END()
	}
	return 0;
}
