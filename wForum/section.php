<?php

$setboard=1;

require("inc/funcs.php");
require("inc/user.inc.php");

setStat("分区版面列表");

preprocess();

show_nav();

showUserMailBoxOrBR();
head_var($section_names[$secNum][0],'section.php?sec='.$secNum, 0);
?>
<script src="inc/loadThread.js"></script>
<table cellSpacing=0 cellPadding=0 width=97% border=0 align=center>
 <?php
	showAnnounce(); 
?>
</table>
<?php
showSecs($secNum,0,true);
showUserInfo();
showSample();

show_footer();

/*--------------- function defines ------------------*/

function preprocess(){
	GLOBAL $sectionCount;
	global $secNum;

	$path='';
	$secNum=intval($_GET['sec']);
	if ( ($secNum<0)  || ($secNum>=$sectionCount)) {
		foundErr("您指定的分区不存在！");
	}
}

?>
