/********************************************************************
 * VMU Backup Tool
 * Version 0.0.2 (05/Jan/2004)
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

#define VMU_X 236
#define VMU_Y 148
#define VMU_W 84
#define VMU_H 112
#define PC_DIR "/pc/vmutool"

void scan_vmu_dir(int vmuport, int vmuslot, RefPtr<Label> label);
void copy_vmu_file(char *srcdir, char *dstdir, char *vmufile, ssize_t size);
void copy_vmu_dir(int vmuport, int vmuslot);
void copy_pc_dir(int vmuport, int vmuslot);

extern uint8 romdisk[];

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

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

void copy_vmu_dir(int vmuport, int vmuslot) {
	file_t		d;
	dirent_t	*de;
	char		*vmudir;
	
	vmudir=(char *)malloc(10);
	sprintf(vmudir, "/vmu/%c%d", vmuport+97, vmuslot+1);
	d = fs_open(vmudir, O_RDONLY | O_DIR);
	if (!d) {
		printf("Can't open VMU (%s)\n", vmudir);
	} else {
		while ( (de = fs_readdir(d)) ) {
			copy_vmu_file(vmudir, PC_DIR, de->name, de->size);
		}
	}
	fs_close(d);
	free(vmudir);
}

void copy_pc_dir(int vmuport, int vmuslot) {
	file_t		d;
	dirent_t	*de;
	char		*vmudir;
	
	vmudir=(char *)malloc(10);
	sprintf(vmudir, "/vmu/%c%d", vmuport+97, vmuslot+1);
	d = fs_open(PC_DIR, O_RDONLY | O_DIR);
	if (!d) {
		printf("Can't open PC (%s)\n", PC_DIR);
	} else {
		while ( (de = fs_readdir(d)) ) {
			copy_vmu_file(PC_DIR, vmudir, de->name, de->size);
		}
	}
	fs_close(d);
	free(vmudir);
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

	// Setup a scene and place a banner in it
	RefPtr<Scene> sc = new Scene();
	RefPtr<Banner> b_menu = new Banner(PVR_LIST_TR_POLY, new Texture("/rd/vmumenu.png", true));
	RefPtr<Banner> b_vmu = new Banner(PVR_LIST_TR_POLY, new Texture("/rd/vmu.png", true));

	sc->subAdd(b_menu);
	sc->subAdd(b_vmu);
	b_menu->setTranslate(Vector(320, 240, 5));
	b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));

	RefPtr<Font> fnt = new Font("/rd/helvetica.txf");
	RefPtr<Label> lbl1 = new Label(fnt, "VMU Backup Tool v0.0.1", 20, true, true);
	lbl1->setTranslate(Vector(320, 390, 20));
	sc->subAdd(lbl1);

	RefPtr<Label> lbl2 = new Label(fnt, "Select a VMU to backup and press A", 20, true, true);
	lbl2->setTranslate(Vector(320, 420, 20));
	sc->subAdd(lbl2);

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
				copy_vmu_dir(x, y);
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_B) && (timer + 200 < timer_ms_gettime64())) {
				copy_pc_dir(x, y);
				scan_vmu_dir(x, y, lbl1);
				timer = timer_ms_gettime64();
			}
		MAPLE_FOREACH_END()
	}
	return 0;
}
