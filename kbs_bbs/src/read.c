/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@rock.b11.ingr.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/  
    
#include "bbs.h"
    
#define PUTCURS   move(3+locmem->crs_line-locmem->top_line,0);prints(">");
#define RMVCURS   move(3+locmem->crs_line-locmem->top_line,0);prints(" ");

struct fileheader SR_fptr;

int SR_BMDELFLAG = false;

int B_to_b = false;


/*---	Modified by period	2000-11-12	---*
char *pnt;
 *---	current code memory leak ---*/ 

extern char MsgDesUid[14];


struct keeploc {
    
char *key;
    
int top_line;
    
int crs_line;
    
struct keeploc *next;

};


static int search_articles(struct keeploc *locmem, char *query, int offset, int aflag);

static int search_author(struct keeploc *locmem, int offset, char *powner);

static int search_post(struct keeploc *locmem, int offset);

static int search_title(struct keeploc *locmem, int offset);

static int i_read_key(struct one_key *rcmdlist, struct keeploc *locmem, int ch, int ssize, char *pnt);

static int cursor_pos(struct keeploc *locmem, int val, int from_top);

static int search_thread(struct keeploc *locmem, int offset, char *title);


/*struct fileheader *files = NULL;*/ 
char currdirect[255];		/*COMMAN: increased directory length to MAX_PATH */

int screen_len;

int last_line;



/* COMMAN : use mmap to speed up searching */ 
#include <unistd.h>
#include <sys/mman.h>
int 
search_file(char *filename) /* Leeward 98.10.02 */ 
{
    
char p_name[256];
    
int i = 0;
    
int size;
    
struct fileheader *rptr, *rptr1;

    

if (uinfo.mode != RMAIL)
	setbdir(digestmode, p_name, currboard);
    
    else
	setmailfile(p_name, currentuser->userid, DOT_DIR);
    

switch (safe_mmapfile(p_name, O_RDONLY, PROT_READ, MAP_SHARED, (void *) &rptr, &size, NULL)) {
    
case 0:
	
return -1;
    
case 1:
	
for (i = 0, rptr1 = rptr; i < (int) (size / sizeof(struct fileheader)); i++, rptr1++)
	    
if (!strcmp(filename, rptr1->filename)) {
		
end_mmapfile((void *) rptr, size, -1);
		
return i;
	    
}
    
case 2:
	
break;
    
}
    
end_mmapfile((void *) rptr, size, -1);
    
return -1;

}


struct keeploc *getkeep(char *s, int def_topline, int def_cursline) 
{
    
static struct keeploc *keeplist = NULL;
    
struct keeploc *p;

    

for (p = keeplist; p != NULL; p = p->next) {
	
if (!strcmp(s, p->key)) {
	    
if (p->crs_line < 1)
		p->crs_line = 1;	/* DAMMIT! - rrr */
	    
return p;
	
}
    
}
    
p = (struct keeploc *) malloc(sizeof(*p));
    
p->key = (char *) malloc(strlen(s) + 1);
    
strcpy(p->key, s);
    
p->top_line = def_topline;
    
p->crs_line = def_cursline;	/* this should be safe */
    
p->next = keeplist;
    
keeplist = p;
    
return p;

}
	

void 
fixkeep(s, first, last) 
 char *s;
	
int first, last;


{
    
struct keeploc *k;

    
k = getkeep(s, 1, 1);
    
if (k->crs_line >= first) {
	
k->crs_line = (first == 1 ? 1 : first - 1);
	
k->top_line = (first < 11 ? 1 : first - 10);
    
}

}
	

void 
modify_locmem(locmem, total) 
 struct keeploc *locmem;
	
int total;


{
    
if (locmem->top_line > total) {
	
locmem->crs_line = total;
	
locmem->top_line = total - t_lines / 2;
	
if (locmem->top_line < 1)
	    locmem->top_line = 1;
    
} else if (locmem->crs_line > total) {
	
locmem->crs_line = total;
    
}

}
	

int 
move_cursor_line(locmem, mode) 
 struct keeploc *locmem;
	
int mode;


{
    
int top, crs;
    
int reload = 0;

    

top = locmem->top_line;
    
crs = locmem->crs_line;
    
if (mode == READ_PREV) {
	
if (crs <= top) {
	    
top -= screen_len - 1;
	    
if (top < 1)
		top = 1;
	    
reload = 1;
	
}
	
crs--;
	
if (crs < 1) {
	    
crs = 1;
	    
reload = -1;
	
}
    
} else if (mode == READ_NEXT) {
	
if (crs + 1 >= top + screen_len) {
	    
top += screen_len - 1;
	    
reload = 1;
	
}
	
crs++;
	
if (crs > last_line) {
	    
crs = last_line;
	    
reload = -1;
	
}
    
}
    
locmem->top_line = top;
    
locmem->crs_line = crs;
    
return reload;

}


static void draw_title(void (*dotitle) ()) 
{
    
move(0, 0);
    
clear();
    
(*dotitle) ();
    
clrtobot();

} 


/*---	Modified by period	2000-11-12	---*
void
draw_entry( doentry, locmem, num ,ssize)
char            *(*doentry)();
struct keeploc  *locmem;
int             num,ssize;
---*/ 
void draw_entry(READ_FUNC doentry, struct keeploc *locmem, 
int num, int ssize, char *pnt) 
{
    
char *str;
    
int base, i;
    
char foroutbuf[512];

    


base = locmem->top_line;
    
move(3, 0);
    
clrtobot();
    
for (i = 0; i < num; i++) {
	
str = (*doentry) (foroutbuf, base + i, &pnt[i * ssize]);
	
if (!check_stuffmode())
	    
prints("%s", str);
	
	else
	    
showstuff(str);
	
prints("\n");
    
}
    
move(t_lines - 1, 0);
    
clrtoeol();
    
update_endline();

}
	

static int 
search_author(locmem, offset, powner) 
 struct keeploc *locmem;
	
int offset;
	
char *powner;


{
    
static char author[IDLEN + 1];
    
char ans[IDLEN + 1], pmt[STRLEN];
    
char currauth[STRLEN];

    

strncpy(currauth, powner, STRLEN);
    

snprintf(pmt, STRLEN, "%s的文章搜寻作者 [%s]: ", offset > 0 ? "往後来" : "往先前", currauth);
    
move(t_lines - 1, 0);
    
clrtoeol();
    
getdata(t_lines - 1, 0, pmt, ans, IDLEN + 1, DOECHO, NULL, true);	/*Haohmaru.98.09.29.修正作者查找只能11位ID的错误 */
    
if (ans[0] != '\0')
	strncpy(author, ans, IDLEN);
    
    else
	strcpy(author, currauth);
    

return search_articles(locmem, author, offset, 1);

}



void i_read(int cmdmode, char *direct, void (*dotitle) (), 
READ_FUNC doentry, struct one_key *rcmdlist, int ssize) 
{
    
extern int talkrequest;
    
struct keeploc *locmem;
    
char lbuf[11];
    
int lbc, recbase, mode, ch;
    
int num, entries;

    
    /*---	Moved from top of file	period	2000-11-12	---*/ 
    char *pnt;

    

strncpy(currdirect, direct, 255);	/* COMMAN: strncpy */
    
    /*---	HERE:	---*/ 
	screen_len = t_lines - 4;
    
modify_user_mode(cmdmode);
    
pnt = calloc(screen_len, ssize);
    
draw_title(dotitle);
    
last_line = get_num_records(currdirect, ssize);
    
if (last_line == 0) {
	
if (cmdmode == RMAIL)
	    
 {
	    
prints("没有任何新信件...");
	    
pressreturn();
	    
clear();
	    
}
	
	else if (cmdmode == GMENU)
	    
 {
	    
getdata(t_lines - 1, 0, "没有任何好友 (A)新增好友 (Q)离开？[Q] ", 
genbuf, 4, DOECHO, NULL, true);
	    
if (genbuf[0] == 'a' || genbuf[0] == 'A')
		
friend_add(0, NULL, 0);
	    
}
	
	else
	    
 {
	    
getdata(t_lines - 1, 0, "看板新成立 (P)发表文章 (Q)离开？[Q] ", 
genbuf, 4, DOECHO, NULL, true);
	    
if (genbuf[0] == 'p' || genbuf[0] == 'P')
		
do_post();
	    
}
	
free(pnt);
	
pnt = NULL;
	
return;
    
}
    
num = last_line - screen_len + 2;
    
locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
    
modify_locmem(locmem, last_line);
    
recbase = locmem->top_line;
    
entries = get_records(currdirect, pnt, ssize, recbase, screen_len);
    
    /*---	Modofied by period	2000-11-12	---*
    draw_entry( doentry, locmem, entries ,ssize);
     *---			---*/ 
	draw_entry(doentry, locmem, entries, ssize, pnt);
    
PUTCURS;
    
lbc = 0;
    
mode = DONOTHING;
    
while ((ch = egetch()) != EOF) {
	
if (talkrequest) {
	    
talkreply();
	    
mode = FULLUPDATE;
	    
		/*        } else if( ch >= '0' && ch <= '9' ) {
		   if( lbc < 9 )
		   lbuf[ lbc++ ] = ch;
		    *//*---	Modified by period	2000-09-11	---*/ 
	} else if ((ch >= '0' && ch <= '9') || 
((Ctrl('H') == ch || '\177' == ch) && (lbc > 0))) {
	    
if (Ctrl('H') == ch || '\177' == ch)
		
lbuf[lbc--] = 0;
	    
	    else if (lbc < 9)
		
lbuf[lbc++] = ch;
	    
lbuf[lbc] = 0;
	    
if (!lbc)
		update_endline();
	    
	    else if (DEFINE(currentuser, DEF_ENDLINE)) {
		
extern time_t login_start_time;
		
int allstay;
		
char pntbuf[256], nullbuf[2] = " ";

		
allstay = (time(0) - login_start_time) / 60;
		
snprintf(pntbuf, 256, "\033[33;44m转到∶[\033[36m%9.9s\033[33m]" 
 "  呼叫器[好友:%3s∶一般:%3s] 使用者[\033[36m%.12s\033[33m]%s停留[%3d:%2d]\033[m", 
lbuf, (!(uinfo.pager & FRIEND_PAGER)) ? "NO " : "YES", 
(uinfo.pager & ALL_PAGER) ? "YES" : "NO ", 
currentuser->userid,	/*13-strlen(currentuser->userid)
																																					   TODO:这个地方有问题，他想对齐，但是代码不对
																																					   , */ nullbuf, 
			  (allstay / 60) % 1000, allstay % 60);
		
move(t_lines - 1, 0);
		
clrtoeol();
		
prints(pntbuf);
	    
}
	    
	    /*---		---*/ 
	} else if (lbc > 0 && (ch == '\n' || ch == '\r')) {
	    
	    /*---	2000-09-11	---*/ 
		update_endline();
	    
	    /*---	---*/ 
		lbuf[lbc] = '\0';
	    
lbc = atoi(lbuf);
	    
if (cursor_pos(locmem, lbc, 10))
		
mode = PARTUPDATE;
	    
lbc = 0;
	
} else {
	    
	    /*---	2000-09-11	---*/ 
		if (lbc)
		update_endline();
	    
	    /*---	---*/ 
		lbc = 0;
	    
	    /*---	Modified by period	2000-11-12	---*
                   mode = i_read_key( rcmdlist, locmem, ch ,ssize);
             *---		---*/ 
		mode = i_read_key(rcmdlist, locmem, ch, ssize, pnt);
	    

while (mode == READ_NEXT || mode == READ_PREV) {
		
int reload;

		

reload = move_cursor_line(locmem, mode);
		
if (reload == -1) {
		    
mode = FULLUPDATE;
		    
break;
		
} else if (reload) {
		    
recbase = locmem->top_line;
		    
entries = get_records(currdirect, pnt, ssize, 
recbase, screen_len);
		    
if (entries <= 0) {
			
last_line = -1;
			
break;
		    
}
		
}
		
num = locmem->crs_line - locmem->top_line;
		
		/*---	Modified by period	2000-11-12	---*
                              mode = i_read_key( rcmdlist, locmem, ch ,ssize);
                 *---		---*/ 
		    mode = i_read_key(rcmdlist, locmem, ch, ssize, pnt);
	    
}
	    
modify_user_mode(cmdmode);
	
}
	
if (mode == DOQUIT)
	    
break;
	
if (mode == GOTO_NEXT) {
	    
cursor_pos(locmem, locmem->crs_line + 1, 1);
	    
mode = PARTUPDATE;
	
}
	
switch (mode) {
	
case NEWDIRECT:
	
case DIRCHANGED:
	    
recbase = -1;
	    
last_line = get_num_records(currdirect, ssize);
	    
if (last_line == 0 && digestmode > 0)
		
 {
		
if (digestmode == true)
		    
digest_mode();
		
		else if (digestmode == 2)
		    
thread_mode();
		
		else if (digestmode == 4)
		    
deleted_mode();
		
		else if (digestmode == 5)
		    
junk_mode();
		
}
	    
if (mode == NEWDIRECT) {
		
num = last_line - screen_len + 1;
		
locmem = getkeep(currdirect, num < 1 ? 1 : num, last_line);
	    
}
	
case FULLUPDATE:
	    
draw_title(dotitle);
	
case PARTUPDATE:
	    
if (last_line < locmem->top_line + screen_len) {
		
num = get_num_records(currdirect, ssize);
		
if (last_line != num) {
		    
last_line = num;
		    
recbase = -1;
		
}
	    
}
	    
if (last_line == 0) {
		
prints("No Messages\n");
		
entries = 0;
	    
} else if (recbase != locmem->top_line) {
		
recbase = locmem->top_line;
		
if (recbase > last_line) {
		    
recbase = last_line - screen_len / 2;
		    
if (recbase < 1)
			recbase = 1;
		    
locmem->top_line = recbase;
		
}
		
entries = get_records(currdirect, pnt, ssize, 
recbase, screen_len);
	    
}
	    
if (locmem->crs_line > last_line)
		
locmem->crs_line = last_line;
	    
	    /*---	Modified by period	2000-11-12	---*
                          draw_entry( doentry, locmem, entries,ssize);
             *---		---*/ 
		draw_entry(doentry, locmem, entries, ssize, pnt);
	    
PUTCURS;
	    
break;
	    
	    /*---	Commented by period	2000-11-12	coz it memory leak	---*/ 
#if 0
	case 100:		/*Haohmaru.2000.04.26 */
	    
clear();
	    
free(pnt);
	    
pnt = NULL;
	    
goto HERE;
	    
#endif	/*0*/
	default:
	    
break;
	
}
	
mode = DONOTHING;
	
if (entries == 0)
	    
break;
    
}
    
clear();
    
free(pnt);
    
pnt = NULL;

}



/*---	Modified by period	2000-11-12	---*
int
i_read_key( rcmdlist, locmem, ch,ssize)
struct one_key  *rcmdlist ;
struct keeploc  *locmem;
int     ch,ssize;
 *---		---*/ 
static int i_read_key(struct one_key *rcmdlist, struct keeploc *locmem, int ch, int ssize, char *pnt) 
{
    
int i, mode = DONOTHING;

    

switch (ch) {
    
case Ctrl('Z'):
	r_lastmsg();		/* Leeward 98.07.30 support msgX */
	
break;
	
	/*---	Commented by period	2000-11-12	coz it memory leak	---*/ 
#if 0 /*1 0   revised by stephen 2000-12-03 for a mailbox-board bug */
    case 'v':			/*Haohmaru.2000.04.26 */
	
if (uinfo.mode == RMAIL || digestmode != false)
	    
break;
	
strcpy(buf, currdirect);
	
m_read();
	
strcpy(currdirect, buf);
	
return 100;
	
#endif	/*0*/
    case 'q':
    case 'e':
    case KEY_LEFT:
	
if (digestmode == true)
	    
return digest_mode();
	
	else if (digestmode == 2)
	    
return thread_mode();
	
	else if (digestmode == 4)
	    
return deleted_mode();
	
	else if (digestmode == 5)
	    
return junk_mode();
	
	else
	    
return DOQUIT;
    
case Ctrl('L'):
	
redoscr();
	
break;
    
case 'k':
    case KEY_UP:
	
if (cursor_pos(locmem, locmem->crs_line - 1, screen_len - 2))
	    
return PARTUPDATE;
	
break;
    
case 'j':
    case KEY_DOWN:
	
if (cursor_pos(locmem, locmem->crs_line + 1, 0))
	    
return PARTUPDATE;
	
break;
    
case 'N':
    case Ctrl('F'):
    case KEY_PGDN:
    case ' ':
	
if (last_line >= locmem->top_line + screen_len) {
	    
locmem->top_line += screen_len - 1;
	    
locmem->crs_line = locmem->top_line;
	    
return PARTUPDATE;
	
}
	
RMVCURS;
	
locmem->crs_line = last_line;
	
PUTCURS;
	
break;
    
case 'P':
    case Ctrl('B'):
    case KEY_PGUP:
	
if (locmem->top_line > 1) {
	    
locmem->top_line -= screen_len - 1;
	    
if (locmem->top_line <= 0)
		
locmem->top_line = 1;
	    
locmem->crs_line = locmem->top_line;
	    
return PARTUPDATE;
	
} else
	    
 {
	    
RMVCURS;
	    
locmem->crs_line = locmem->top_line;
	    
PUTCURS;
	    
}
	
break;
    
case KEY_HOME:
	
locmem->top_line = 1;
	
locmem->crs_line = 1;
	
return PARTUPDATE;
    
case '$':
    
case KEY_END:
	
if (last_line >= locmem->top_line + screen_len) {
	    
locmem->top_line = last_line - screen_len + 1;
	    
if (locmem->top_line <= 0)
		
locmem->top_line = 1;
	    
locmem->crs_line = last_line;
	    
return PARTUPDATE;
	
}
	
RMVCURS;
	
locmem->crs_line = last_line;
	
PUTCURS;
	
break;
    
case 'L':
    case 'l':			/* Luzi 1997.10.31 */
	
if (uinfo.mode != LOOKMSGS)
	    
 {
	    
show_allmsgs();
	    
return FULLUPDATE;
	    
break;
	    
}
	
	else
	    
return DONOTHING;
    
case 'H':			/* Luzi 1997.10.31 */
	
r_lastmsg();
	
break;
    
case 'w':			/* Luzi 1997.10.31 */
	
if (!HAS_PERM(currentuser, PERM_PAGE))
	    break;
	
s_msg();
	
return FULLUPDATE;
	
break;
    
case 'u':			/* Haohmaru. 99.11.29 */
	
clear();
	
modify_user_mode(QUERY);
	
t_query(NULL);
	
return FULLUPDATE;
	
break;
    
case 'O':
    case 'o':			/* Luzi 1997.10.31 */
	
 {			/* Leeward 98.10.26 fix a bug by saving old mode */
	    
int savemode = uinfo.mode;

	    
if (!HAS_PERM(currentuser, PERM_BASIC))
		break;
	    
t_friends();
	    
modify_user_mode(savemode);
	    
return FULLUPDATE;
	    
break;
	
}
    
case '!':			/*Haohmaru 1998.09.24 */
	
Goodbye();
	
return FULLUPDATE;
	
break;
    
case '\n':
    case '\r':
    case KEY_RIGHT:
	
ch = 'r';
	
	    /* lookup command table */ 
    default:
	
for (i = 0; rcmdlist[i].fptr != NULL; i++) {
	    
if (rcmdlist[i].key == ch) {
		
mode = (*(rcmdlist[i].fptr)) (locmem->crs_line, 
&pnt[(locmem->crs_line - locmem->top_line) * ssize], 
currdirect);
		
break;
	    
}
	
}
    
}
    
return mode;

}
	

int 
auth_search_down(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_author(locmem, 1, fileinfo->owner))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}
	

int 
auth_search_up(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_author(locmem, -1, fileinfo->owner))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}
	

int 
post_search_down(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_post(locmem, 1))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}
	

int 
post_search_up(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_post(locmem, -1))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}



/*Haohmaru.98.12.05.系统管理员可以直接查作者资料*/ 
int show_authorinfo(int ent, struct fileheader *fileinfo, char *direct) 
{
    
struct userec uinfo;
    
struct userec *lookupuser;
    
int id;

    
if (!HAS_PERM(currentuser, PERM_ACCOUNTS) || !strcmp(fileinfo->owner, "Anonymous") || !strcmp(fileinfo->owner, "deliver"))
	
return DONOTHING;
    
    else
	
 {
	
if (0 == (id = getuser(fileinfo->owner, &lookupuser))) {
	    
move(2, 0);
	    
prints("不正确的使用者代号");
	    
clrtoeol();
	    
return PARTUPDATE;
	
}
	
uinfo = *lookupuser;
	
move(1, 0);
	
clrtobot();
	
disply_userinfo(&uinfo, 1);
	
uinfo_query(&uinfo, 1, id);
	
}
    
return FULLUPDATE;

}
	

int 
sendmsgtoauthor(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct user_info *uin;

    
if (!HAS_PERM(currentuser, PERM_PAGE))
	
return DONOTHING;
    
clear();
    
uin = (struct user_info *) t_search(fileinfo->owner, false);
    
if (!uin || !canmsg(currentuser, uin))
	
do_sendmsg(NULL, NULL, 0);
    
    else {
	
strncpy(MsgDesUid, uin->userid, 20);
	
do_sendmsg(uin, NULL, 0);
    
}
    
return FULLUPDATE;

}
	

int 
show_author(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
if ( /*strchr(fileinfo->owner,'.')|| */ !strcmp(fileinfo->owner, "Anonymous") || !strcmp(fileinfo->owner, "deliver"))	/* Leeward 98.04.14 */
	
return DONOTHING;
    
    else
	
t_query(fileinfo->owner);
    
return FULLUPDATE;

}
	

int 
SR_BMfunc(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
int i;
    
char buf[256], ch[4], BMch;
    
char *SR_BMitems[] = { "删除", "保留", "文摘", "放入精华区", "放入暂存档", "标记删除", "设为不可回复" };
    
char linebuffer[256];

    

if (!chk_currBM(currBM, currentuser))
	
 {
	
return DONOTHING;
	
}
    
if (digestmode == 4 || digestmode == 5)	/* KCN:暂不允许 */
	
return DONOTHING;
    
saveline(t_lines - 3, 0, linebuffer);
    
saveline(t_lines - 2, 0, NULL);
    
move(t_lines - 3, 0);
    
clrtoeol();
    
strcpy(buf, "相同主题 (0)取消  ");
    
for (i = 0; i < 7; i++) {
	
char t[40];

	
sprintf(t, "(%d)%s", i + 1, SR_BMitems[i]);
	
strcat(buf, t);
	
/*        snprintf(buf,256,"%s(%d)%s  ",buf,i+1,SR_BMitems[i]);*/ 
    };
    
strcat(buf, "? [0]: ");
    
if (strlen(buf) > 76) {
	
char savech = buf[76];

	
buf[76] = 0;
	
prints("%s", buf);
	
buf[76] = savech;
	
/*        strcpy(buf,buf+76);*/ 
	    getdata(t_lines - 2, 0, buf + 76, ch, 3, DOECHO, NULL, true);
    
} else
	
getdata(t_lines - 3, 0, buf, ch, 3, DOECHO, NULL, true);
    
BMch = atoi(ch);
    
if (BMch <= 0 || BMch > 7)
	
 {
	
saveline(t_lines - 2, 1, NULL);
	
saveline(t_lines - 3, 1, linebuffer);
	
return DONOTHING;
	
}
    
if (digestmode == 2 && BMch <= 3) {
	
saveline(t_lines - 2, 1, NULL);
	
saveline(t_lines - 3, 1, linebuffer);
	
return DONOTHING;
    
}
    
move(t_lines - 2, 0);
    
clrtoeol();
    
move(t_lines - 3, 0);
    
clrtoeol();
    
	/* Leeward 98.04.16 */ 
	snprintf(buf, 256, "是否从此主题第一篇开始%s (Y)第一篇 (N)目前这篇 (C)取消 (Y/N/C)? [Y]: ", SR_BMitems[BMch - 1]);
    
getdata(t_lines - 3, 0, buf, ch, 3, DOECHO, NULL, true);
    
switch (ch[0])
	
 {
    
default:
    
case 'y':
    case 'Y':
	
ent = sread(2, 0, ent, 0, fileinfo);	/* 增加返回值,修改同主题 Bigman: 2000.8.20 */
	
fileinfo = &SR_fptr;
	
break;
    
case 'n':
    case 'N':
	
break;
    
case 'c':
    case 'C':
	
saveline(t_lines - 2, 1, NULL);
	
saveline(t_lines - 3, 1, linebuffer);
	
return DONOTHING;
	
}
    
sread(BMch + SR_BMBASE, 0, ent, 0, fileinfo);
    
return DIRCHANGED;

}


int 
SR_BMfuncX(ent, fileinfo, direct) /* Leeward 98.04.16 */ 
	int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
int i;
    
char buf[256], ch[4], BMch;
    
char *SR_BMitems[] = { "删除", "保留", "文摘", "放入精华区", "放入暂存档", "标记删除", "设为不可回复" };
    
char linebuffer[256];

    


if (!chk_currBM(currBM, currentuser))
	
 {
	
return DONOTHING;
	
}
    
if (digestmode == 4 || digestmode == 5)	/* KCN:不允许 */
	
return DONOTHING;
    

saveline(t_lines - 3, 0, linebuffer);
    
saveline(t_lines - 2, 0, NULL);
    
move(t_lines - 3, 0);
    
clrtoeol();
    
strcpy(buf, "相同主题 (0)取消  ");
    
for (i = 0; i < 7; i++) {
	
char t[40];

	
sprintf(t, "(%d)%s", i + 1, SR_BMitems[i]);
	
strcat(buf, t);
    
} 
strcat(buf, "? [0]: ");
    
if (strlen(buf) > 76) {
	
char savech = buf[76];

	
buf[76] = 0;
	
prints("%s", buf);
	
buf[76] = savech;
	
/*        strcpy(buf,buf+76);*/ 
	    getdata(t_lines - 2, 0, buf + 76, ch, 3, DOECHO, NULL, true);
    
} else
	
getdata(t_lines - 3, 0, buf, ch, 3, DOECHO, NULL, true);
    

BMch = atoi(ch);
    
if (BMch <= 0 || BMch > 7)
	
 {
	
saveline(t_lines - 2, 1, NULL);
	
saveline(t_lines - 2, 1, linebuffer);
	
return DONOTHING;
	
}
    
if (digestmode == 2 && BMch <= 3)
	
return DONOTHING;
    
move(t_lines - 3, 0);
    
clrtoeol();
    
	/* Leeward 98.04.16 */ 
	snprintf(buf, 256, "是否从此主题第一篇开始%s (Y)第一篇 (N)目前这篇 (C)取消 (Y/N/C)? [Y]: ", SR_BMitems[BMch - 1]);
    
getdata(t_lines - 2, 0, buf, ch, 3, DOECHO, NULL, true);
    
B_to_b = true;
    
switch (ch[0])
	
 {
    
default:
    
case 'y':
    case 'Y':
	
ent = sread(2, 0, ent, 0, fileinfo);	/* 增加返回值,修改同主题 Bigman: 2000.8.20 */
	
fileinfo = &SR_fptr;
	
break;
    
case 'n':
    case 'N':
	
break;
    
case 'c':
    case 'C':
	
saveline(t_lines - 2, 1, NULL);
	
return DONOTHING;
	
}
    
if (SR_BMTMP == BMch + SR_BMBASE)	/* Leeward 98.04.16 */
	
sread(-(BMch + SR_BMBASE), 0, ent, 0, fileinfo);
    
    else
	
sread(BMch + SR_BMBASE, 0, ent, 0, fileinfo);
    
B_to_b = false;
    
return DIRCHANGED;

}
	

int 
SR_first_new(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
sread(2, 0, ent, 0, fileinfo);
    
if (sread(3, 0, NULL, 0, &SR_fptr) == -1)	/*Found The First One */
	
 {
	
sread(0, 1, NULL, 0, &SR_fptr);
	
return FULLUPDATE;
	
}
    
return PARTUPDATE;

}
	

int 
SR_last(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
sread(1, 0, ent, 0, fileinfo);
    
return PARTUPDATE;

}
	

int 
SR_first(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
sread(2, 0, ent, 0, fileinfo);
    
return PARTUPDATE;

}
	

int 
SR_read(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
sread(0, 1, NULL, 0, fileinfo);
    
return FULLUPDATE;

}



/* Leeward 98.10.03 同主题阅读退出时文章位置不变 */ 
	int 
SR_readX(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
sread(-1003, 1, NULL, 0, fileinfo);
    
return FULLUPDATE;

}
	

int 
SR_author(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
sread(0, 1, NULL, 1, fileinfo);
    
return FULLUPDATE;

}



/* Leeward 98.10.03 同作者阅读退出时文章位置不变 */ 
	int 
SR_authorX(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
sread(-1003, 1, NULL, 1, fileinfo);
    
return FULLUPDATE;

}
	

int 
auth_post_down(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_author(locmem, 1, fileinfo->owner))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}
	

int 
auth_post_up(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_author(locmem, -1, fileinfo->owner))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}
	

int 
t_search_down(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_title(locmem, 1))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}
	

int 
t_search_up(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_title(locmem, -1))
	
return PARTUPDATE;
    
    else
	
update_endline();
    
return DONOTHING;

}
	
int 
thread_up(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_thread(locmem, -1, fileinfo->title))
	
 {
	
update_endline();
	
return PARTUPDATE;
	
}
    
update_endline();
    
return DONOTHING;

}
	

int 
thread_down(ent, fileinfo, direct) 
 int ent;
	
struct fileheader *fileinfo;
	
char *direct;


{
    
struct keeploc *locmem;

    

locmem = getkeep(direct, 1, 1);
    
if (search_thread(locmem, 1, fileinfo->title))
	
 {
	
update_endline();
	
return PARTUPDATE;
	
}
    
update_endline();
    
return DONOTHING;

}
	


static int 
search_post(locmem, offset) 
 struct keeploc *locmem;
	
int offset;


{
    
static char query[STRLEN];
    
char ans[STRLEN], pmt[STRLEN];

    

strncpy(ans, query, STRLEN);
    
snprintf(pmt, STRLEN, "搜寻%s的文章 [%s]: ", offset > 0 ? "往後来" : "往先前", ans);
    
move(t_lines - 1, 0);
    
clrtoeol();
    
getdata(t_lines - 1, 0, pmt, ans, 50, DOECHO, NULL, true);
    
if (ans[0] != '\0')
	strcpy(query, ans);
    

return search_articles(locmem, query, offset, -1);

}
	

static int 
search_title(locmem, offset) 
 struct keeploc *locmem;
	
int offset;


{
    
static char title[STRLEN];
    
char ans[STRLEN], pmt[STRLEN];

    

strncpy(ans, title, STRLEN);
    
snprintf(pmt, STRLEN, "%s搜寻标题 [%s]: ", offset > 0 ? "往後" : "往前", ans);
    
move(t_lines - 1, 0);
    
clrtoeol();
    
getdata(t_lines - 1, 0, pmt, ans, STRLEN - 1, DOECHO, NULL, true);
    
if (*ans != '\0')
	strcpy(title, ans);
    
return search_articles(locmem, title, offset, 0);

}


static int search_thread(struct keeploc *locmem, int offset, char *title) 
{
    

if (title[0] == 'R' && (title[1] == 'e' || title[1] == 'E') && title[2] == ':')
	
title += 4;
    
setqtitle(title);
    
return search_articles(locmem, title, offset, 2);

}



/*Add by SmallPig*/ 
	int 
sread(passonly, readfirst, pnum, auser, ptitle) 
 int passonly, readfirst, auser, pnum;
	
struct fileheader *ptitle;


{
    
struct keeploc *locmem;
    
int istest = 0, isstart = 0, isnext = 1;
    
int previous;
    
char genbuf[STRLEN], title[STRLEN];
    
int B;			/* Leeward: 表示按的是 B(-1) 还是 b(+1) */
    
int ori_top, ori_crs;	/* Leeward 98.10.03 add these 3 ori_...  and Xflag */
    
char ori_file[STRLEN];

    
/*    int Xflag = (-1003 != passonly )? 0 : !(passonly = 0);奇怪啊KCN */ 
    int Xflag = (-1003 != passonly) ? 0 : (passonly = 0, 1);

    

strncpy(ori_file, ptitle->filename, STRLEN);
    

B = (passonly < 0 ? -1 : 1);	/* Leeward 98.04.16 */
    
passonly *= B;
    

RemoveAppendedSpace(ptitle->title);	/* Leeward 98.02.13 */
    

previous = pnum;
    
if (passonly == 0)
	
 {
	
if (readfirst)
	    
isstart = 1;
	
	else {
	    
isstart = 0;
	    
move(t_lines - 1, 0);
	    
clrtoeol();
	    
prints("[1m[44m[31m[%8s] [33m下一封 ' ',<Enter>,↓│上一封 ↑,U                                  [m", auser ? "相同作者" : "主题阅读");
	    
switch (egetch()) {
	    
case ' ':
	    case '\n':
	    
case KEY_DOWN:
		
isnext = 1;
		
break;
	    
case KEY_UP:
	    case 'u':
	    case 'U':
		
isnext = -1;
		
break;
	    
default:
		break;
	    }
	
}
    
} else if (passonly == 1 || passonly >= 3)
	
isnext = 1;
    
    else
	
isnext = -1;
    
locmem = getkeep(currdirect, 1, 1);
    
ori_top = locmem->top_line;	/* Leeward 98.10.02 */
    
ori_crs = locmem->crs_line;
    
if (auser == 0)
	
 {
	
strncpy(title, ptitle->title, STRLEN);
	
setqtitle(title);
    
} else
	
 {
	
strncpy(title, ptitle->owner, STRLEN);
	
setqtitle(ptitle->title);
	
}
    
if (!strncmp(title, "Re: ", 4) | !strncmp(title, "RE: ", 4))
	
 {
	strcpy(title, title + 4);
	}
    
memcpy(&SR_fptr, ptitle, sizeof(SR_fptr));
    
while (!istest) {
	
switch (passonly)
	    
 {
	
case 0:
	case 1:
	case 2:
	    
break;
	
case 3:
	    
if (brc_unread(FILENAME2POSTTIME(SR_fptr.filename)))
		
return -1;
	    
	    else
		
break;
	
case SR_BMDEL:
	    
if (digestmode)
		
return -1;
	    
		/* Leeward 97.11.18: fix bugs: add "if" block */ 
		/* if (!( ptitle->accessed[0] & FILE_MARKED )) */ 
		if (!(SR_fptr.accessed[0] & FILE_MARKED))
		
		    /* Bigman 2000.8.20: 修改同主题删除错误.... Leeward这个增加的不对呀,以后的内容没有读呀 */ 
	    {
		
SR_BMDELFLAG = true;
		
del_post(locmem->crs_line, &SR_fptr, currdirect);
		
SR_BMDELFLAG = false;
		
if (sysconf_eval("KEEP_DELETED_HEADER") <= 0)
		    
 {
		    
last_line--;
		    
locmem->crs_line--;
		    
previous = locmem->crs_line;
		    
}
	    
}
	    
break;
	
case SR_BMMARKDEL:
	    
if (digestmode)
		
return -1;
	    
		/* Leeward 97.11.18: fix bugs: add "if" block */ 
		/* if (!( ptitle->accessed[0] & FILE_MARKED )) */ 
		if (!(SR_fptr.accessed[0] & FILE_MARKED))
		
		    /* Bigman 2000.8.20: 修改同主题删除错误.... Leeward这个增加的不对呀,以后的内容没有读呀 */ 
	    {
		
set_delete_mark(locmem->crs_line, &SR_fptr, currdirect);
	    
}
	    
break;
	
case SR_BMNOREPLY:
	    
if (digestmode)
		
return -1;
	    
if (!(SR_fptr.accessed[1] & FILE_SIGN))
		
		    /* Bigman 2000.8.20: 修改同主题删除错误.... Leeward这个增加的不对呀,以后的内容没有读呀 */ 
	    {
		
noreply_post_noprompt(locmem->crs_line, &SR_fptr, currdirect);
	    
}
	    
break;
	
case SR_BMMARK:
	    
if (digestmode == 2)
		
return -1;
	    
mark_post(locmem->crs_line, &SR_fptr, currdirect);
	    
break;
	
case SR_BMDIGEST:
	    
if (digestmode == true || digestmode == 4 || digestmode == 5)
		
return -1;
	    
digest_post(locmem->crs_line, &SR_fptr, currdirect);
	    
break;
	
case SR_BMIMPORT:
	    
a_Import("0Announce", currboard, &SR_fptr, true, currdirect, locmem->crs_line);	/* Leeward 98.04.15 */
	    
break;
	
case SR_BMTMP:	/* Leeward 98.04.16 */
	    
if (-1 == B)
		a_SeSave("0Announce", currboard, &SR_fptr, true);
	    
	    else
		a_Save("0Announce", currboard, &SR_fptr, true, currdirect, locmem->crs_line);
	    
break;
	    
}
	
if (!isstart)
	    
 {
	    
search_articles(locmem, title, isnext, auser + 2);
	    
}
	
if (previous == locmem->crs_line)
	    
 {
	    
break;
	    
}
	
if (uinfo.mode != RMAIL)
	    
setbfile(genbuf, currboard, SR_fptr.filename);
	
	else
	    
setmailfile(genbuf, currentuser->userid, SR_fptr.filename);
	
previous = locmem->crs_line;
	
setquotefile(genbuf);
	
if (passonly == 0)
	    
 {
	    
int lch;		/* period 2000-09-11    方案1:                          *
				   * 解决:同主题向上查找,文章大于一屏时按一次UP键屏幕无内容 *
				   * 方案2: rawmore()函数中, 判断KEY_UP==ch处不应该清屏 */
	    
lch = ansimore(genbuf, false);
	    
		/*    ansimore(genbuf,false) ;  */ 
		brc_add_read(SR_fptr.filename);
	    
isstart = 0;
	    
move(t_lines - 1, 0);
	    
clrtoeol();
	    
prints("[1m[44m[31m[%8s] [33m回信 R │ 结束 Q,← │下一封 ↓,Enter│上一封 ↑,U │ ^R 回给作者   \033[m", auser ? "相同作者" : "主题阅读");
	    
		/*  period 2000-09-11       原因同上 */ 
		if (KEY_UP != lch && 'U' != lch)
		lch = egetch();
	    
switch (lch) {
		
		    /*    switch( egetch() ) { */ 
	    case Ctrl('Z'):
		r_lastmsg();	/* Leeward 98.07.30 support msgX */
		
break;
	    
case 'N':
	    case 'Q':
	    
case 'n':
	    case 'q':
	    
case KEY_LEFT:
		
istest = 1;
		
break;
	    
case 'Y':
	    case 'R':
	    
case 'y':
	    case 'r':
		
do_reply(&SR_fptr);
	    
case ' ':
	    case '\n':
	    
case KEY_DOWN:
		
isnext = 1;
		
break;
	    
case Ctrl('A'):
		
clear();
		
show_author(0, &SR_fptr, currdirect);
		
isnext = 1;
		
break;
	    
case Ctrl('Q'):	/*Haohmaru.98.12.05.系统管理员直接查作者资料 */
		
clear();
		
show_authorinfo(0, &SR_fptr, currdirect);
		
isnext = 1;
		
break;
	    
case Ctrl('W'):	/* cityhunter 00.10.18 版主管理直接察看版主信息 */
		
clear();
		
show_authorBM(0, &SR_fptr, currdirect);
		
isnext = 1;
		
break;
	    
case 'z':
	    case 'Z':		/*Haohmaru.2000.5.19,直接给作者发msg */
		
if (!HAS_PERM(currentuser, PERM_PAGE))
		    break;
		
sendmsgtoauthor(0, &SR_fptr, currdirect);
		
isnext = 1;
		
break;
	    
case KEY_UP:
	    case 'u':
	    case 'U':
		
isnext = -1;
		
break;
	    
case Ctrl('R'):
		
post_reply(0, &SR_fptr, (char *) NULL);
		
break;
	    
case 'g':
		
digest_post(0, &SR_fptr, currdirect);
		
break;
	    
case 'L':
	    case 'l':		/* Luzi 1997.11.1 */
		
if (uinfo.mode == LOOKMSGS)
		    
break;
		
show_allmsgs();
		
break;
	    
case 'H':		/* Luzi 1997.11.1 */
		
r_lastmsg();
		
break;
	    
case 'w':		/* Luzi 1997.11.1 */
		
if (!HAS_PERM(currentuser, PERM_PAGE))
		    break;
		
s_msg();
		
break;
	    
case '!':		/*Haohmaru 1998.09.24 */
		
Goodbye();
		
break;
	    
case 'O':
	    case 'o':		/* Luzi 1997.11.1 */
		
if (!HAS_PERM(currentuser, PERM_BASIC))
		    break;
		
t_friends();
		
break;
	    
default:
		break;
	    
}
	    
}
    
}
    

	/* Leeward 98.10.02 add all below except last "return 1" */ 
	if (Xflag) {
	
if (search_file(ori_file) != ori_crs)
	    bell();
	

RMVCURS;
	
locmem->top_line = ori_top;
	
locmem->crs_line = ori_crs;
	
PUTCURS;
    
}
    

if ((passonly == 2) && (readfirst == 0) && (auser == 0))	/*在同主题删除时,能够返回第一篇文章 Bigman:2000.8.20 */
	
return previous;
    
    else
	
return 1;

}
	

void 
get_upper_str(ptr2, ptr1) 
 char *ptr1, *ptr2;


{
    
int ln, i;

    

for (ln = 0; (ln < STRLEN) && (ptr1[ln] != 0); ln++);
    
for (i = 0; i < ln; i++)
	
 {
	
ptr2[i] = toupper(ptr1[i]);
	
	/******** 下面为Luzi添加 ************/ 
	    if (ptr2[i] == '\0')
	    
ptr2[i] = '\1';
	
	/******** 以上为Luzi添加 ************/ 
	}
    
ptr2[ln] = '\0';

}
	

int 
searchpattern(filename, query) 
 char *filename;
	
char *query;


{
    
FILE * fp;
    
char buf[256];

    

if ((fp = fopen(filename, "r")) == NULL)
	
return 0;
    
while (fgets(buf, 256, fp) != NULL)
	
 {
	
if (strstr(buf, query))
	    
 {
	    
fclose(fp);
	    
return true;
	    
}
	
}
    
fclose(fp);
    
return false;

}



/* COMMAN : use mmap to speed up searching */ 
static int search_articles(struct keeploc *locmem, char *query, int offset, int aflag) 
{
    
char ptr[STRLEN];
    
int now, match = 0;
    
int complete_search;
    
char upper_ptr[STRLEN], upper_query[STRLEN];

    
/*	int mmap_offset,mmap_length; */ 
    struct fileheader *pFh, *pFh1;
    
int size;

    

get_upper_str(upper_query, query);
    

if (aflag >= 2)
	
 {
	
complete_search = 1;
	
aflag -= 2;
    
} else
	
 {
	
complete_search = 0;
	
}
    

if (*query == '\0') {
	
return 0;
    
}
    
	/*
	   move(t_lines-1,0);
	   clrtoeol();
	   prints("[44m[33m搜寻中，请稍候....                                                             [m");
	 */ 
	now = locmem->crs_line;
    
/*    refresh();*/ 
	
switch (safe_mmapfile(currdirect, O_RDONLY, PROT_READ, MAP_SHARED, (void **) &pFh, &size, NULL)) {
    
case 0:
	return 0;
    
case 1:
	
pFh1 = pFh + now - 1;
	
while (1) {
	    
if (offset > 0) {
		
if (++now > last_line)
		    break;
		
pFh1++;
	    
} else {
		
if (--now < 1)
		    break;
		
pFh1--;
	    
}
	    
if (now == locmem->crs_line)
		
break;
	    
if (aflag == -1)
		
 {
		
char p_name[256];

		
if (uinfo.mode != RMAIL)
		    
setbfile(p_name, currboard, pFh1->filename);
		
		else
		    
setmailfile(p_name, currentuser->userid, pFh1->filename);
		
if (searchpattern(p_name, query))
		    
 {
		    
match = cursor_pos(locmem, now, 10);
		    
break;
		
} else
		    
continue;
		
}
	    
strncpy(ptr, aflag ? pFh1->owner : pFh1->title, STRLEN - 1);
	    
ptr[STRLEN - 1] = 0;
	    
if (complete_search == 1)
		
 {
		
char *ptr2 = ptr;

		
if ((*ptr == 'R' || *ptr == 'r') && (*(ptr + 1) == 'E' || *(ptr + 1) == 'e') 
 &&(*(ptr + 2) == ':') && (*(ptr + 3) == ' '))
		    
 {
		    
ptr2 = ptr + 4;
		    
}
		
if (!strcmp(ptr2, query)) {
		    
match = cursor_pos(locmem, now, 10);
		    
break;
		
}
	    
} else
		
 {
		
		    /*  COMMAN : move upper_query out of loop  */ 
		    get_upper_str(upper_ptr, ptr);
		
		    /* 同作者查询改成完全匹配 by dong, 1998.9.12 */ 
		    if (aflag == 1)	/* 进行同作者查询 */
		    
 {
		    
if (!strcasecmp(upper_ptr, upper_query)) {
			
match = cursor_pos(locmem, now, 10);
			
break;
		    
}
		    
}
		
		else if (strstr(upper_ptr, upper_query) != NULL) {
		    
match = cursor_pos(locmem, now, 10);
		    
break;
		
}
		
}
	
}
	
memcpy(&SR_fptr, pFh + locmem->crs_line - 1, sizeof(struct fileheader));
	
break;
    
case 2:
	
memset(&SR_fptr, 0, sizeof(struct fileheader));
	
match = 0;
    
} 
end_mmapfile((void *) pFh, size, -1);
    
move(t_lines - 1, 0);
    
clrtoeol();
    
return match;

}



/* calc cursor pos and show cursor correctly -cuteyu */ 
	static int cursor_pos(locmem, val, from_top) 
 struct keeploc *locmem;
	
int val;
	
int from_top;


{
    
if (val > last_line) {
	
val = DEFINE(currentuser, DEF_CIRCLE) ? 1 : last_line;
    
}
    
if (val <= 0) {
	
val = DEFINE(currentuser, DEF_CIRCLE) ? last_line : 1;
    
}
    
if (val >= locmem->top_line && val < locmem->top_line + screen_len - 1) {
	
RMVCURS;
	
locmem->crs_line = val;
	
PUTCURS;
	
return 0;
    
}
    
locmem->top_line = val - from_top;
    
if (locmem->top_line <= 0)
	
locmem->top_line = 1;
    
locmem->crs_line = val;
    
return 1;

}



