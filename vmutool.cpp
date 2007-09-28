/********************************************************************
 *	VMU Backup Tool
 *	Version 0.5.3 (24/Jan/2004)
 *	coded by El Bucanero
 *
 *	Copyright (C) 2004 Damián Parrino (bucanero@elitez.com.ar)
 *	http://www.bucanero.com.ar/
 *
 *	Greetz to:
 *		Dan Potter for KOS and other great libs like Tsunami
 *		Andrew Kieschnick for the lovely DC load-ip/tool
 *		Lawrence Sebald for the MinGW/msys cross compiler tutorial
 *
 *	and last, but not least, thanks to SEGA for the Dreamcast! =)
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
#define VMU_FILE_ERASE 5

typedef struct f_list {
	char name[13];
	ssize_t size;
	struct f_list *next;
} f_node;

extern uint8 romdisk[];

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_MALLOCSTATS);
KOS_INIT_ROMDISK(romdisk);

class ListLoader {
private:
	char	*srcdir;
	f_node	*ptr;
	int		total;

public:
	ListLoader(char *s) {
		srcdir=s;
		ptr=(f_node *)malloc(sizeof(f_node));
		total=0;
	}
	~ListLoader() {
		f_node	*aux;

		while (ptr != NULL) {
			aux=ptr->next;
			free(ptr);
			ptr=aux;
		}
	}
	int getTotal() {
		return(total);
	}
	f_node *getListPtr() {
		return(ptr);
	}
	f_node *getListItem(int j) {
		int i=0;
		f_node *aux;

		aux=ptr;
		while ((aux != NULL) && (i < j)) {
			i++;
			aux=aux->next;
		}
		return(aux);
	}
	void loadList() {
		file_t		d;
		dirent_t	*de;
		f_node		*aux;

		total=0;
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
					total++;
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
	}
};

class ListSelector {
private:
	ListLoader	*list;
	int			pos, top;

	void list_directory() {
		int		i=0, j, o=FILES_Y * 640 + FILES_X;
		f_node	*aux;

		bfont_set_encoding(BFONT_CODE_ISO8859_1);
		aux=list->getListPtr();
		while ((aux != NULL) && (i < top + MAX_FILES_LIST)) {
			if (i >= top) {
				bfont_draw_str(vram_s + o, 640, 1, aux->name);
				o += 640*24;
			}
			i++;
			aux=aux->next;
		}
		if (i - top < MAX_FILES_LIST) {
			for (j=i - top; j < MAX_FILES_LIST; j++) {
				bfont_draw_str(vram_s + o, 640, 1, "            ");
				o += 640*24;
			}
		}
	}
	void move_cursor(int *pos, int newpos) {
		bfont_draw_str(vram_s + *pos, 640, 1, " ");
		bfont_draw_str(vram_s + *pos + FILES_W, 640, 1, " ");
		*pos=newpos;
		bfont_draw_str(vram_s + *pos, 640, 1, ">");
		bfont_draw_str(vram_s + *pos + FILES_W, 640, 1, "<");
	}

public:
	ListSelector(char *s) {
		pos=0;
		top=0;
		list=new ListLoader(s);
		list->loadList();
	}
	~ListSelector() {
		delete(list);
	}
	int getSelection() {
		return(pos);
	}
	ListLoader *getListLoader() {
		return(list);
	}
	int doSelection() {
		int		done=0, o=FILES_Y * 640 + FILES_X - 20;
		uint64	timer = timer_ms_gettime64();

		list_directory();
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
							list_directory();
							move_cursor(&o, FILES_Y * 640 + FILES_X - 20 + 640 * 24 * (MAX_FILES_LIST - 1));
						}
					}
					timer = timer_ms_gettime64();
				}
				if ((t->buttons & CONT_DPAD_DOWN) && (pos <= top + MAX_FILES_LIST) && (timer + 200 < timer_ms_gettime64())) {
					if ((pos < top + MAX_FILES_LIST - 1) && (pos < list->getTotal())) {
						pos++;
						move_cursor(&o, o + 640*24);
					} else {
						if ((pos == top + MAX_FILES_LIST - 1) && (top + MAX_FILES_LIST - 1 < list->getTotal())) {
							top=top + MAX_FILES_LIST;
							pos++;
							list_directory();
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
};

class VmuScaner {
private:
	int vmuport, vmuslot, files, blocks;
	char *text;

public:
	VmuScaner(int p, int s) {
		text=(char *)malloc(64);
		setVmu(p, s);
	}
	~VmuScaner() {
		free(text);
	}
	void setVmu(int p, int s) {
		vmuport=p;
		vmuslot=s;
		files=0;
		blocks=0;
		scan();
	}
	int getVmuPort() {
		return(vmuport);
	}
	int getVmuSlot() {
		return(vmuport);
	}
	char *getText() {
		return(text);
	}
	int getFiles() {
		return(files);
	}
	int getBlocks() {
		return(blocks);
	}
	int getFreeBlocks() {
		return(200-blocks);
	}
	void scan() {
		file_t		d;
		dirent_t	*de;
		char		*vmudir;

		vmudir=(char *)malloc(10);
		sprintf(vmudir, "/vmu/%c%d", vmuport+97, vmuslot+1);
		d = fs_open(vmudir, O_RDONLY | O_DIR);
		if (!d) {
			sprintf(text, "Unable to open VMU %c%d", vmuport+65, vmuslot+1);
			files=-1;
			blocks=201;
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
};

class Backup {
private:
	VmuScaner		*vmuscan;
	ListSelector	*select;

	void copy_file(char *srcdir, char *dstdir, f_node *ptr) {
		file_t	f;
		char	*tmpsrc;
		char	*tmpdst;

		if ((ptr->size > 0) && ((strstr(dstdir, "/vmu/") == NULL) || ((strstr(dstdir, "/vmu/") != NULL) && (vmuscan->getFreeBlocks() >= (int)(ptr->size / 512))))) {
			tmpsrc=(char *)malloc(64);
			tmpdst=(char *)malloc(64);
			if (strstr(ptr->name, ".VMS") == NULL) {
				sprintf(tmpdst, "%s/%s", dstdir, ptr->name);
			} else {
				sprintf(tmpdst, "%s/%.*sVMI", srcdir, strlen(ptr->name)-3, ptr->name);
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
					printf("Reading %.*sVMI for original file name: %s\n", strlen(ptr->name)-3, ptr->name, tmpsrc);
					sprintf(tmpdst, "%s/%s", dstdir, tmpsrc);
				}
			}
			sprintf(tmpsrc, "%s/%s", srcdir, ptr->name);
			printf("Copying (%s -> %s) Name: %s ... ", srcdir, dstdir, ptr->name);
			if (fs_copy(tmpsrc, tmpdst) == ptr->size) {
				printf("%d bytes copied.\n", ptr->size);
			} else {
				printf("Error!\n");
			}
			free(tmpsrc);
			free(tmpdst);
		}
	}
	void copy_directory(char *srcdir, char *dstdir) {
		f_node	*aux;
		int		i=0;

		aux=select->getListLoader()->getListPtr();
		while ((aux != NULL) && (i <= select->getSelection())) {
			if (select->getSelection() == 0) {
				copy_file(srcdir, dstdir, aux);
			} else {
				if (select->getSelection() == i) {
					copy_file(srcdir, dstdir, aux);
				}
				i++;
			}
			aux=aux->next;
		}
	}
	void erase_file(char *srcdir) {
		char *tmpsrc;

		tmpsrc=(char *)malloc(64);
		sprintf(tmpsrc, "%s/%s", srcdir, select->getListLoader()->getListItem(select->getSelection())->name);
		printf("Erasing file %s ... ", tmpsrc);
		if (fs_unlink(tmpsrc) == 0) {
			printf("Done.\n");
		} else {
			printf("Error!\n");
		}
		free(tmpsrc);
	}

public:
	Backup(VmuScaner *v) {
		vmuscan=v;
		select=NULL;
	}
	void doAction(int action) {
		char	*vmudir;

		vmudir=(char *)malloc(32);
		sprintf(vmudir, "/vmu/%c%d", vmuscan->getVmuPort()+97, vmuscan->getVmuSlot()+1);
		switch (action) {
			case VMU_TO_PC:
				select=new ListSelector(vmudir);
				if ((select->getListLoader()->getTotal() > 0) && (select->doSelection() != -1)) {
					copy_directory(vmudir, PC_DIR);
				}
				delete(select);
				break;
			case PC_TO_VMU:
				select=new ListSelector(PC_DIR);
				if ((select->getListLoader()->getTotal() > 0) && (select->doSelection() != -1)) {
					copy_directory(PC_DIR, vmudir);
				}
				delete(select);
				break;
			case CD_TO_VMU:
				select=new ListSelector(CD_DIR);
				if ((select->getListLoader()->getTotal() > 0) && (select->doSelection() != -1)) {
					copy_directory(CD_DIR, vmudir);
				}
				delete(select);
				break;
			case VMU_TO_VMU:
				select=new ListSelector(vmudir);
				if ((select->getListLoader()->getTotal() > 0) && (select->doSelection() != -1)) {
					copy_directory(vmudir, DEFAULT_VMU);
				}
				delete(select);
				break;
			case VMU_FILE_ERASE:
				select=new ListSelector(vmudir);
				if ((select->getListLoader()->getTotal() > 0) && (select->doSelection() > 0)) {
						erase_file(vmudir);
				}
				delete(select);
				break;
		}
		free(vmudir);
	}
};

int main(int argc, char **argv) {
	int			done=0, x=0, y=0;
	uint64		timer = timer_ms_gettime64();
	Backup		*backup;
	VmuScaner	*vmuscan;

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

	vmuscan=new VmuScaner(x, y);
	l_info->setText(vmuscan->getText());
	backup=new Backup(vmuscan);

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
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_DOWN) && (y<1) && (timer + 200 < timer_ms_gettime64())) {
				y++;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_LEFT) && (x>0) && (timer + 200 < timer_ms_gettime64())) {
				x--;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_DPAD_RIGHT) && (x<3) && (timer + 200 < timer_ms_gettime64())) {
				x++;
				b_vmu->setTranslate(Vector(VMU_X + x*VMU_W, VMU_Y + y*VMU_H, 7));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_A) && (timer + 200 < timer_ms_gettime64()) && (vmuscan->getFiles() != -1)) {
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
				backup->doAction(VMU_TO_PC);
				b_file->setTranslate(Vector(800, 600, 11));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_B) && (timer + 200 < timer_ms_gettime64()) && (vmuscan->getFreeBlocks() > 0)) {
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
				backup->doAction(PC_TO_VMU);
				b_file->setTranslate(Vector(800, 600, 11));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_X) && (timer + 200 < timer_ms_gettime64()) && (vmuscan->getFreeBlocks() > 0)) {
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
				backup->doAction(CD_TO_VMU);
				b_file->setTranslate(Vector(800, 600, 11));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_Y) && (t->ltrig == 0) && (timer + 200 < timer_ms_gettime64()) && (vmuscan->getFiles() != -1)) {
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
				backup->doAction(VMU_TO_VMU);
				b_file->setTranslate(Vector(800, 600, 11));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
			if ((t->buttons & CONT_Y) && (t->ltrig > 0) && (timer + 200 < timer_ms_gettime64()) && (vmuscan->getFiles() != -1)) {
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
				backup->doAction(VMU_FILE_ERASE);
				b_file->setTranslate(Vector(800, 600, 11));
				vmuscan->setVmu(x, y);
				timer = timer_ms_gettime64();
			}
		MAPLE_FOREACH_END()
	}

	delete(vmuscan);
	delete(backup);
	return(0);
}
