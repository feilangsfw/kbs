#! /bin/sh

BBS_HOME=/home/bbs
INSTALL="//bin/install -c"
TARGET=/home/bbs/bin

echo "This script will install the whole BBS to ${BBS_HOME}..."
echo -n "Press <Enter> to continue ..."
read ans

if [ -d ${BBS_HOME} ] ; then
        echo -n "Warning: ${BBS_HOME} already exists, overwrite whole bbs [N]?"
        read ans
        ans=${ans:-N}
        case $ans in
            [Yy]) echo "Installing new bbs to ${BBS_HOME}" ;;
            *) echo "Abort ..." ; exit ;;
        esac
else
        echo "Making dir ${BBS_HOME}"
        mkdir ${BBS_HOME}
        chown -R bbs ${BBS_HOME}
        chgrp -R bbs ${BBS_HOME}
fi

echo "Setup bbs directory tree ....."
( cd bbshome ; tar cf - * ) | ( cd ${BBS_HOME} ; tar xf - )

chown -R bbs ${BBS_HOME}
chgrp -R bbs ${BBS_HOME}

${INSTALL} -m 770  -s -g bbs -o bbs    bbs        ${TARGET}
${INSTALL} -m 770  -s -g bbs -o bbs    bbs.chatd  ${TARGET}
${INSTALL} -m 4755 -s -g bin -o root   bbsrf      ${TARGET}

cat > ${BBS_HOME}/etc/sysconf.ini << EOF
# comment

BBSHOME         = "/home/bbs"
BBSID           = "NoName"
BBSNAME         = "Never Land BBS"
BBSDOMAIN       = "some.where.on.earth"

#SHOW_IDLE_TIME         = 1
KEEP_DELETED_HEADER     = 0

#BBSNTALKD      = 1
#NTALK          = "/bin/ctalk.sh"
#REJECTCALL     = "/bin/rejectcall.sh"
#GETPAGER       = "getpager"

BCACHE_SHMKEY   = 7813
UCACHE_SHMKEY   = 7912
UTMP_SHMKEY     = 3785
ACBOARD_SHMKEY  = 9013
ISSUE_SHMKEY    = 5002
GOODBYE_SHMKEY  = 5003

IDENTFILE       = "etc/preach"
EMAILFILE       = "etc/mailcheck"
#NEWREGFILE     = "etc/newregister"

PERM_BASIC      = 0x00001
PERM_CHAT       = 0x00002
PERM_PAGE       = 0x00004
PERM_POST       = 0x00008
PERM_LOGINOK    = 0x00010
PERM_DENYPOST   = 0x00020
PERM_CLOAK      = 0x00040
PERM_SEECLOAK   = 0x00080
PERM_XEMPT      = 0x00100
PERM_WELCOME    = 0x00200
PERM_BOARDS     = 0x00400
PERM_ACCOUNTS   = 0x00800
PERM_CHATCLOAK  = 0x01000
PERM_OVOTE      = 0x02000
PERM_SYSOP      = 0x04000
PERM_POSTMASK   = 0x08000
PERM_ANNOUNCE   = 0x10000
PERM_OBOARDS    = 0x20000
PERM_ACBOARD    = 0x40000
UNUSE1          = 0x80000
UNUSE2          = 0x100000
UNUSE3          = 0x200000
UNUSE4          = 0x400000
UNUSE5          = 0x800000
UNUSE6          = 0x1000000
UNUSE7          = 0x2000000
UNUSE8          = 0x4000000
UNUSE9          = 0x8000000
UNUSE10         = 0x10000000
UNUSE11         = 0x20000000

PERM_ADMENU    = PERM_ACCOUNTS , PERM_OVOTE , PERM_SYSOP,PERM_OBOARDS,PERM_WELCOME,PERM_ANNOUNCE
AUTOSET_PERM    = PERM_CHAT, PERM_PAGE, PERM_POST, PERM_LOGINOK

#include "etc/menu.ini"
EOF

#------------------------------------------------------------------
echo "Install is over...."
