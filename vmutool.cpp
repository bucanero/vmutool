/********************************************************************
 * VMU Backup Tool
 * Version 0.5.1 (12/Jan/2004)
 * coded by El Bucanero
 *
 * Copyright (C) 2004 Damián Parrino (bucanero@elitez.com.ar)
 * http://www.bucanero.com.ar/
 *
 * Greetz to:
 * Dan Potter for KOS and other great libs like Tsunami
 * Andrew K. for the lovely DC load-ip/tool
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

typedef struct f_list {
	char name[13];
	ssize_t size;
	struct f_list *next;
} f_node;

void scan_vmu_dir(int vmuport, int vmuslot, char *text);
void copy_vmu_file(char *srcdir, char *dstdir, char *vmufile, ssize_t size);
void copy_directory(char *srcdir, char *dstdir, f_node *ptr, int selection);
void backup_action(int vmuport, int vmuslot, int action);
int load_file_list(char *srcdir, f_node *ptr);
int file_select(f_node *ptr, int total);
void list_directory(f_node *ptr, int begin);
void move_cursor(int *pos, int newpos);

extern uint8 romdisk[];

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

void backup_action(int vmuport, int vmuslot, int action) {
	int		selection, f_count;
	char	*vmudir;
	f_node	*f_top, *aux;

	vmudir=(char *)malloc(10);
	f_top=(f_node *)malloc(sizeof(f_node));
	sprintf(vmudir, "/vmu/%c%d", vmuport+97, vmuslot+1);
	switch (action) {
		case VMU_TO_PC:
			f_count=load_file_list(vmudir, f_top);
			if (f_count > 0) {
				selection=file_select(f_top, f_count);
				if (selection != -1) {
					copy_directory(vmudir, PC_DIR, f_top, selection);
				}
			}
			break;
		case PC_TO_VMU:
			f_count=load_file_list(PC_DIR, f_top);
			if (f_count > 0) {
				selection=file_select(f_top, f_count);
				if (selection != -1) {
					copy_directory(PC_DIR, vmudir, f_top, selection);
				}
			}
			break;
		case CD_TO_VMU:
			f_count=load_file_list(CD_DIR, f_top);
			if (f_count > 0) {
				selection=file_select(f_top, f_count);
				if (selection != -1) {
					copy_directory(CD_DIR, vmudir, f_top, selection);
				}
			}
			break;
		case VMU_TO_VMU:
			f_count=load_file_list(vmudir, f_top);
			if (f_count > 0) {
				selection=file_select(f_top, f_count);
				if (selection != -1) {
					copy_directory(vmudir, DEFAULT_VMU, f_top, selection);
				}
			}
			break;
	}
	while (f_top != NULL) {
		aux=f_top->next;
		free(f_top);
		f_top=aux;
	}
	free(vmudir);
}

int load_file_list(char *srcdir, f_node *ptr) {
	file_t		d;
	dirent_t	*de;
	f_node		*aux;
	int			i=0;

	d = fs_open(srcdir, O_RDONLY | O_DIR);
	if (!d) {
		printf("Can't open source directory (%s)\n", srcdir);
		ptr->next=NULL;
	} else {
		aux=ptr;
		strcpy(aux->name, "ALL FILES   ");
		aux->size=0;
		aux->next=(f_node *)malloc(sizeof(f_node));
		while ( (de = fs_readdir(d)) ) {
			if (de->size > 108) {
				i++;
				aux=aux->next;
				strcpy(aux->name, de->name);
				aux->size=de->size;
				aux->next=(f_node *)malloc(sizeof(f_node));
			}
		}
		free(aux->next);
		aux->next=NULL;
	}
	fs_close(d);
	return(i);
}

void move_cursor(int *pos, int newpos) {
	bfont_draw_str(vram_s + *pos, 640, 1, " ");
	bfont_draw_str(vram_s + *pos + FILES_W, 640, 1, " ");
	*pos=newpos;
	bfont_draw_str(vram_s + *pos, 640, 1, ">");
	bfont_draw_str(vram_s + *pos + FILES_W, 640, 1, "<");
}

int file_select(f_node *ptr, int total) {
	int		pos=0, done=0, top=0, o=FILES_Y * 640 + FILES_X - 20;
	uint64	timer = timer_ms_gettime64();

	list_directory(ptr, top);
	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	move_cursor(&o, o);
	while (!done) {
		MAPLE_FOREACH_BEGIN(MAPLE_FUNC_CONTROLLER, cont_state_t, t)
			if ((t->buttons & CONT_DPAD_UP) && (pos >= top) && (timer + 200 < timer_ms_gettime64())) {
				if (pos > top) {
					pos--;
					move_cursor(&o, o - 640*24);
				} else {
					if ((pos == top) && (top != 0)) {
						top=top - MAX_FILES_LIST;
						pos--;
						list_directory(ptr, top);
						move_cursor(&o, FILES_Y * 640 + FILES_X - 20 + 640 * 24 * (MAX_FILES_LIST - 1));
					}
				}
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_DOWN) && (pos <= top + MAX_FILES_LIST) && (timer + 200 < timer_ms_gettime64())) {
				if ((pos < top + MAX_FILES_LIST - 1) && (pos < total)) {
					pos++;
					move_cursor(&o, o + 640*24);
				} else {
					if ((pos == top + MAX_FILES_LIST - 1) && (top + MAX_FILES_LIST - 1 < total)) {
						top=top + MAX_FILES_LIST;
						pos++;
						list_directory(ptr, top);
						move_cursor(&o, FILES_Y * 640 + FILES_X - 20);
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

void list_directory(f_node *ptr, int begin) {
	int		i=0, j, o=FILES_Y * 640 + FILES_X;
	f_node	*aux;

	bfont_set_encoding(BFONT_CODE_ISO8859_1);
	aux=ptr;
	while ((aux != NULL) && (i < begin + MAX_FILES_LIST)) {
		if (i >= begin) {
			bfont_draw_str(vram_s + o, 640, 1, aux->name);
			o += 640*24;
		}
		i++;
		aux=aux->next;
	}
	if (i - begin < MAX_FILES_LIST) {
		for (j=i - begin; j < MAX_FILES_LIST; j++) {
			bfont_draw_str(vram_s + o, 640, 1, "            ");
			o += 640*24;
		}
	}
}

void copy_vmu_file(char *srcdir, char *dstdir, char *vmufile, ssize_t size) {
	file_t	f;
	char	*tmpsrc;
	char	*tmpdst;

	if (size > 0) {
		tmpsrc=(char *)malloc(64);
		tmpdst=(char *)malloc(64);
		if (strstr(vmufile, ".VMS") == NULL) {
			sprintf(tmpdst, "%s/%s", dstdir, vmufile);
		} else {
			sprintf(tmpdst, "%s/%.*sVMI", srcdir, strlen(vmufile)-3, vmufile);
			f = fs_open(tmpdst, O_RDONLY);
			if (!f) {
				printf("Unable to open %s\n", tmpdst);
				free(tmpsrc);
				free(tmpdst);
				return;
			} else {
				fs_seek(f, 0x58, SEEK_SET);
				fs_read(f, tmpsrc, 13);
				fs_close(f);
				printf("Reading %.*sVMI for original file name: %s\n", strlen(vmufile)-3, vmufile, tmpsrc);
				sprintf(tmpdst, "%s/%s", dstdir, tmpsrc);
			}
		}
		sprintf(tmpsrc, "%s/%s", srcdir, vmufile);
		printf("Copying (%s -> %s) Name: %s ... ", srcdir, dstdir, vmufile);
		if (fs_copy(tmpsrc, tmpdst) == size) {
			printf("%d bytes copied.\n", size);
		} else {
			printf("Error!\n");
		}
		free(tmpsrc);
		free(tmpdst);
	}
}

void copy_directory(char *srcdir, char *dstdir, f_node *ptr, int selection) {
	f_node	*aux;
	int		i=0;

	aux=ptr;
	while ((aux != NULL) && (i <= selection)) {
		if (selection == 0) {
			copy_vmu_file(srcdir, dstdir, aux->name, aux->size);
		} else {
			if (selection == i) {
				copy_vmu_file(srcdir, dstdir, aux->name, aux->size);
			}
			i++;
		}
		aux=aux->next;
	}
}

void scan_vmu_dir(int vmuport, int vmuslot, char *text) {
	file_t		d;
	dirent_t	*de;
	char		*vmudir;
	int			files=0, blocks=0;
	
	vmudir=(char *)malloc(10);
	sprintf(vmudir, "/vmu/%c%d", vmuport+97, vmuslot+1);
	d = fs_open(vmudir, O_RDONLY | O_DIR);
	if (!d) {
		sprintf(text, "Unable to open VMU %c%d", vmuport+65, vmuslot+1);
	} else {
		while ( (de = fs_readdir(d)) ) {
			files++;
			blocks=blocks + (de->size / 512);
		}
		sprintf(text, "VMU %c%d: %d Files - %d Blocks Used", vmuport+65, vmuslot+1, files, blocks);
	}
	fs_close(d);
	free(vmudir);
}

int main(int argc, char **argv) {
	int done=0, x=0, y=0;
	char *vmuinfo=(char *)malloc(64);
	uint64 timer = timer_ms_gettime64();

	cont_btn_callback(0, CONT_START | CONT_A | CONT_B | CONT_X | CONT_Y,
		(void (*)(unsigned char, long  unsigned int))arch_exit);

	pvr_init_defaults();
	pvr_set_bg_color(1.0f, 1.0f, 1.0f);

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
	RefPtr<Label> l_info = new Label(fnt, "VMU Backup Tool", 20, true, true);
	l_info->setTranslate(Vector(320, 420, 9));
	sc->subAdd(l_info);

	scan_vmu_dir(x, y, vmuinfo);
	l_info->setText(vmuinfo);

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
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_DOWN) && (y<1) && (timer + 200 < timer_ms_gettime64())) {
				y++;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_LEFT) && (x>0) && (timer + 200 < timer_ms_gettime64())) {
				x--;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_RIGHT) && (x<3) && (timer + 200 < timer_ms_gettime64())) {
				x++;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
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
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
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
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
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
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
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
				scan_vmu_dir(x, y, vmuinfo);
				l_info->setText(vmuinfo);
				timer = timer_ms_gettime64();
			}
		MAPLE_FOREACH_END()
	}

	free(vmuinfo);
	return(0);
}
