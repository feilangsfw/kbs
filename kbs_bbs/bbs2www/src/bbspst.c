/*
 * $Id$
 */
#include "bbslib.h"

int main()
{
   	FILE *fp;
   	int i;
	char userid[80], buf[512], path[512], file[512], board[512], title[80]="";
   	struct boardheader  brdhdr;
   	struct fileheader  DirInfo;

   	init_all();
	if(!loginok)
		http_fatal("匆匆过客不能发表文章，请先登录");
	strsncpy(board, getparm("board"), 20);
	strsncpy(file, getparm("file"), 20);
	strsncpy(title, getparm("title"), 50);
	if(title[0] && strncmp(title, "Re: ", 4))
		sprintf(title, "Re: %s", getparm("title"));
	strsncpy(userid, getparm("userid"), 40);
	if(file[0]!='M' && file[0])
		http_fatal("错误的文件名");
	if(!haspostperm(currentuser, board))
		http_fatal("错误的讨论区或者您无权在此讨论区发表文章");
   	printf("<center>\n");
	printf("%s -- 发表文章 [使用者: %s]<hr color=\"green\">\n", BBSNAME, currentuser->userid);
   	printf("<form method=\"post\" action=\"bbssnd?board=%s\">\n<table border=\"1\">\n",board);
	printf("<tr><td>");
	printf("<font color=\"green\">发文注意事项: <br>\n");
	printf("发文时应慎重考虑文章内容是否适合公开场合发表，请勿肆意灌水。谢谢您的合作。<br></font></td></tr>\n");
	printf("<tr><td>\n");
   	printf("作者: %s<br>\n", currentuser->userid);
   	printf("使用标题: <input type=\"text\" name=\"title\" size=\"40\" maxlength=\"100\" value=\"%s\">\n", 
		void1(title));
	printf("讨论区: [%s]<br>\n",board);
   	printf("使用签名档 <select name=\"signature\">\n");
	if (currentuser->signature == 0)
		printf("<option value=\"0\" selected>不使用签名档</option>\n");
	else
		printf("<option value=\"0\">不使用签名档</option>\n");
	for (i = 1; i < 6; i++)
	{
		if (currentuser->signature == i)
   			printf("<option value=\"%d\" selected>第 %d 个</option>\n", i, i);
		else
   			printf("<option value=\"%d\">第 %d 个</option>\n", i, i);
	}
	printf("</select>\n");
   	printf(" [<a target=\"_balnk\" href=\"bbssig\">查看签名档</a>] \n");
	/* 加上转信支持*/
	printf("<input type=\"checkbox\" name=\"outgo\" value=\"1\">转信\n");
	/* 加上匿名讨论区支持*/
	if (seek_in_file("etc/anonymous",board))
		printf("<input type=\"checkbox\" name=\"anony\" value=\"1\">使用匿名\n");
   	printf("<br>\n<textarea name=\"text\" rows=\"20\" cols=\"80\" wrap=\"physicle\">\n\n");
	if(file[0])
	{
		int lines=0;
		printf("【 在 %s 的大作中提到: 】\n", userid);
		sprintf(path, "boards/%s/%s", board, file);
		fp=fopen(path, "r");
		if(fp)
		{
			for(i=0; i<3; i++)
			{
				if(fgets(buf, 500, fp)==0)
					break;
			}
			while(1)
			{
				if(fgets(buf, 500, fp)==0) break;
				if(!strncmp(buf, ": 【", 4)) continue;
				if(!strncmp(buf, ": : ", 4)) continue;
				if(!strncmp(buf, "--\n", 3)) break;
				if(buf[0]=='\n') continue;
				if(++lines>10)
				{
					printf(": ...................\n");
					break;
				}
				/* faint, why check </textarea> */
				if(!strcasestr(buf, "</textarea>"))
					printf(": %s", buf);
			}
			fclose(fp);
		}
	}
   	printf("</textarea></td></tr>\n");
   	printf("<tr><td class=\"post\" align=\"center\">\n");
	printf("<input type=\"submit\" value=\"发表\"> \n");
   	printf("<input type=\"reset\" value=\"清除\"></td></tr>\n");
	printf("</table></form>\n");
	http_quit();
}
